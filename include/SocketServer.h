#ifndef SOCKETSERVER_H
#define SOCKETSERVER_H

#include <string>
#include <atomic>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

class FaceMemory;

enum DaemonState { STATE_IDLE, STATE_VERIFYING, STATE_ENROLLING };

class SocketServer {
public:
    SocketServer(const std::string& socketPath);
    ~SocketServer();
    bool start();
    void listenAndAccept(FaceMemory& memory, std::atomic<DaemonState>& currentState, std::string& targetName, std::string& lastResult);

private:
    std::string path;
    int server_fd;
    void handleClient(int client_fd, FaceMemory& memory, std::atomic<DaemonState>& currentState, std::string& targetName, std::string& lastResult);
};

#endif