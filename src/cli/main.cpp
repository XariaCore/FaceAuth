#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include "../include/Config.h" 

std::string getJsonValue(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\":";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return "";
    pos += searchKey.length();
    while (json[pos] == ' ' || json[pos] == '\"') pos++;
    size_t endPos = json.find_first_of("\",}", pos);
    return json.substr(pos, endPos - pos);
}

std::string sendCommand(const std::string& cmd) {
    int sock = 0;
    struct sockaddr_un serv_addr;
    char buffer[1024] = {0};

    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        return "{\"status\": \"ERROR\", \"msg\": \"Socket creation error\"}";
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    
    // Using socket path defined in Config namespace
    strncpy(serv_addr.sun_path, Config::SOCKET_PATH.c_str(), sizeof(serv_addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        return "{\"status\": \"ERROR\", \"msg\": \"Daemon is not running! Please start faceauth.service.\"}";
    }

    send(sock, cmd.c_str(), cmd.length(), 0);
    
    // Explicitly ignoring return value of read
    (void)read(sock, buffer, 1024);
    
    close(sock);
    return std::string(buffer);
}

int main(int argc, char const *argv[]) {
    if (argc < 2) {
        std::cout << "Usage:\n";
        std::cout << "  faceauth-cli enroll <name>  : Enroll a new face\n";
        std::cout << "  faceauth-cli delete <name>  : Delete a face\n";
        std::cout << "  faceauth-cli list           : List all users\n";
        std::cout << "  faceauth-cli verify         : Test verification\n";
        return 1;
    }

    std::string command = argv[1];
    std::string response;

    if (command == "enroll") {
        if (argc < 3) { std::cout << "[ERROR] Please provide a username.\n"; return 1; }
        std::cout << ">>> Sending enrollment request for: " << argv[2] << "...\n";
        response = sendCommand("{\"cmd\": \"ENROLL\", \"name\": \"" + std::string(argv[2]) + "\"}");
    } 
    else if (command == "delete") {
        if (argc < 3) { std::cout << "[ERROR] Please provide a username.\n"; return 1; }
        std::cout << ">>> Deleting user: " << argv[2] << "...\n";
        response = sendCommand("{\"cmd\": \"DELETE\", \"name\": \"" + std::string(argv[2]) + "\"}");
    }
    else if (command == "list") {
        std::cout << ">>> Fetching user list...\n";
        response = sendCommand("{\"cmd\": \"LIST\"}");
    }
    else if (command == "verify") {
        std::cout << ">>> Starting verification (Look at the camera)...\n";
        response = sendCommand("{\"cmd\": \"VERIFY\"}");
    }
    else {
        std::cout << "[ERROR] Unknown command.\n";
        return 1;
    }

    std::string status = getJsonValue(response, "status");
    std::string msg = getJsonValue(response, "msg");

    if (status == "SUCCESS") {
        if (command == "list") {
             std::string users = response.substr(response.find("\"users\":") + 8);
             users = users.substr(0, users.find_last_of("]") + 1);
             std::cout << "USERS: " << users << "\n";
        } else if (command == "verify") {
             std::string user = getJsonValue(response, "user");
             std::string score = getJsonValue(response, "score");
             std::cout << "VERIFIED: " << user << " (Score: " << score << ")\n";
        } else {
             std::cout << "SUCCESS: " << msg << "\n";
        }
    } else if (status == "DENIED") {
        std::cout << "ACCESS DENIED: " << msg << "\n";
    } else {
        std::cout << "ERROR: " << msg << "\n";
    }

    return 0;
}