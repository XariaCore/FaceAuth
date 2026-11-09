#include <security/pam_modules.h>
#include <security/pam_ext.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <vector>

#define SOCKET_PATH "/tmp/faceauth.sock"

void notifyUser(pam_handle_t *pamh, int msg_style, const std::string &message) {
    const struct pam_message msg = { .msg_style = msg_style, .msg = message.c_str() };
    const struct pam_message *msgp = &msg;
    struct pam_response *resp = nullptr;
    struct pam_conv *conv;

    int retval = pam_get_item(pamh, PAM_CONV, (const void **)&conv);
    if (retval == PAM_SUCCESS && conv && conv->conv) {
        conv->conv(1, &msgp, &resp, conv->appdata_ptr);
        if (resp) {
            if (resp->resp) free(resp->resp);
            free(resp);
        }
    }
}

std::string getJsonValue(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\":";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return "";
    pos += searchKey.length();
    while (json[pos] == ' ' || json[pos] == '\"') pos++;
    size_t endPos = json.find_first_of("\",}", pos);
    return json.substr(pos, endPos - pos);
}

PAM_EXTERN int pam_sm_authenticate(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    (void)flags; (void)argc; (void)argv;

    const char *username;
    if (pam_get_user(pamh, &username, NULL) != PAM_SUCCESS) return PAM_AUTH_ERR;

    notifyUser(pamh, PAM_TEXT_INFO, "Verifying biometrics...");

    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        return PAM_AUTHINFO_UNAVAIL;
    }

    std::string request = "{\"cmd\": \"VERIFY\"}";
    write(sock, request.c_str(), request.size());

    char buffer[1024] = {0};
    read(sock, buffer, 1024);
    close(sock);

    std::string response(buffer);
    std::string status = getJsonValue(response, "status");
    std::string detectedUser = getJsonValue(response, "user");

    if (status == "SUCCESS") {
        std::string msg = "Identity confirmed: " + detectedUser;
        notifyUser(pamh, PAM_TEXT_INFO, msg);
        return PAM_SUCCESS;
    } else {
        notifyUser(pamh, PAM_ERROR_MSG, "Face verification failed.");
        return PAM_AUTH_ERR;
    }
}

PAM_EXTERN int pam_sm_setcred(pam_handle_t *pamh, int flags, int argc, const char **argv) { (void)pamh; (void)flags; (void)argc; (void)argv; return PAM_SUCCESS; }
PAM_EXTERN int pam_sm_acct_mgmt(pam_handle_t *pamh, int flags, int argc, const char **argv) { (void)pamh; (void)flags; (void)argc; (void)argv; return PAM_SUCCESS; }
PAM_EXTERN int pam_sm_open_session(pam_handle_t *pamh, int flags, int argc, const char **argv) { (void)pamh; (void)flags; (void)argc; (void)argv; return PAM_SUCCESS; }
PAM_EXTERN int pam_sm_close_session(pam_handle_t *pamh, int flags, int argc, const char **argv) { (void)pamh; (void)flags; (void)argc; (void)argv; return PAM_SUCCESS; }
PAM_EXTERN int pam_sm_chauthtok(pam_handle_t *pamh, int flags, int argc, const char **argv) { (void)pamh; (void)flags; (void)argc; (void)argv; return PAM_SUCCESS; }