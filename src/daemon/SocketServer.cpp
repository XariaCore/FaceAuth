#include "SocketServer.h"
#include "FaceMemory.h"
#include "Config.h"
#include <iostream>
#include <cstring>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <thread>
#include <chrono>
#include <vector>
#include <cstdlib>

SocketServer::SocketServer(const std::string& socketPath) : path(socketPath), server_fd(-1) {}
SocketServer::~SocketServer() { if (server_fd != -1) close(server_fd); unlink(path.c_str()); }

pid_t getClientPID(int client_fd) {
    struct ucred credentials;
    socklen_t len = sizeof(struct ucred);
    if (getsockopt(client_fd, SOL_SOCKET, SO_PEERCRED, &credentials, &len) == 0) {
        return credentials.pid;
    }
    return -1;
}

bool checkPolkit(pid_t pid, const std::string& action_id) {
    if (pid <= 0) return false;
    std::string cmd = "pkcheck --process " + std::to_string(pid) +
                      " --action-id " + action_id +
                      " --allow-user-interaction";
    int result = std::system(cmd.c_str());
    return (result == 0);
}

bool SocketServer::start() {
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);
    unlink(path.c_str());

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) return false;

    chmod(path.c_str(), 0666);
    return (listen(server_fd, 5) != -1);
}

void SocketServer::listenAndAccept(FaceMemory& memory, std::atomic<DaemonState>& currentState, std::string& targetName, std::string& lastResult) {
    while (true) {
        int c_fd = accept(server_fd, nullptr, nullptr);
        if (c_fd >= 0) handleClient(c_fd, memory, currentState, targetName, lastResult);
    }
}

void SocketServer::handleClient(int client_fd, FaceMemory& memory, std::atomic<DaemonState>& currentState, std::string& targetName, std::string& lastResult) {

    pid_t clientPID = getClientPID(client_fd);
    char buffer[1024] = {0};

    if (read(client_fd, buffer, 1024) > 0) {
        std::string incoming(buffer);
        std::string response = "{\"status\": \"ERROR\", \"msg\": \"Unknown Error\"}";

        if (incoming.find("ENROLL") != std::string::npos) {
            if (!checkPolkit(clientPID, "org.faceauth.enroll")) {
                response = "{\"status\": \"DENIED\", \"msg\": \"Permission Denied (Polkit)\"}";
            }
            else {
                size_t pos = incoming.find("\"name\": \"");
                if (pos != std::string::npos) {
                    pos += 9;
                    targetName = incoming.substr(pos, incoming.find("\"", pos) - pos);
                    currentState = STATE_ENROLLING;
                } else {
                     lastResult = "{\"status\": \"ERROR\", \"msg\": \"Invalid Username\"}";
                }

                int wait_limit = 5120;
                while(currentState != STATE_IDLE && wait_limit > 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    wait_limit -= 50;
                }
                response = lastResult.empty() ? "{\"status\": \"TIMEOUT\", \"msg\": \"Operation Timed Out\"}" : lastResult;
            }
        }

        else if (incoming.find("DELETE") != std::string::npos) {
            if (!checkPolkit(clientPID, "org.faceauth.delete")) {
                response = "{\"status\": \"DENIED\", \"msg\": \"Permission Denied (Polkit)\"}";
            }
            else {
                size_t pos = incoming.find("\"name\": \"");
                if (pos != std::string::npos) {
                    pos += 9;
                    std::string name = incoming.substr(pos, incoming.find("\"", pos) - pos);
                    if (memory.deleteFace(name)) response = "{\"status\": \"SUCCESS\", \"msg\": \"User Deleted\"}";
                    else response = "{\"status\": \"ERROR\", \"msg\": \"User Not Found\"}";
                }
            }
        }

        else if (incoming.find("LIST") != std::string::npos) {
             std::vector<std::string> users = memory.getUserList();
             std::string userJsonArray = "[";
             for (size_t i = 0; i < users.size(); ++i) {
                 userJsonArray += "\"" + users[i] + "\"";
                 if (i < users.size() - 1) userJsonArray += ", ";
             }
             userJsonArray += "]";
             response = "{\"status\": \"SUCCESS\", \"count\": " + std::to_string(users.size()) + ", \"users\": " + userJsonArray + "}";
        }

        else if (incoming.find("VERIFY") != std::string::npos) {
            lastResult = "";
            currentState = STATE_VERIFYING;
            int wait_limit = 5120;
            while(currentState != STATE_IDLE && wait_limit > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                wait_limit -= 50;
            }
            response = lastResult.empty() ? "{\"status\": \"TIMEOUT\", \"msg\": \"Operation Timed Out\"}" : lastResult;
        }

        send(client_fd, response.c_str(), response.length(), 0);
    }
    close(client_fd);
}