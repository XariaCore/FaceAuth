#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include "Config.h"
#include "Detector.h"
#include "SpoofDetector.h"
#include "FaceMemory.h"
#include "SocketServer.h"

std::atomic<DaemonState> currentState(STATE_IDLE);
std::string targetName = "";
std::string lastResult = "";

int main() {
    FaceDetector detector(Config::DETECTOR_PATH, Config::CONF_THRESHOLD);
    SpoofDetector spoof(Config::SPOOF_PATH, Config::SPOOF_THRESHOLD);
    FaceMemory memory(Config::RECOG_PATH, Config::DB_PATH, Config::RECOG_THRESHOLD);

    SocketServer server(Config::SOCKET_PATH);
    if (!server.start()) {
        std::cerr << "!!! [ERROR] Failed to start socket." << std::endl;
        return -1;
    }

    std::thread s_thread(&SocketServer::listenAndAccept, &server, std::ref(memory), std::ref(currentState), std::ref(targetName), std::ref(lastResult));
    s_thread.detach();

    cv::VideoCapture cap;
    cv::Mat frame;
    int consecutiveReal = 0;

    auto start_time = std::chrono::steady_clock::now();
    bool timer_started = false;

    std::cout << ">>> [DAEMON] Started. Waiting for commands..." << std::endl;

    while (true) {
        if (currentState != STATE_IDLE) {
            if (!cap.isOpened()) {
                cap.open(0);
                timer_started = false;
                if (!cap.isOpened()) {
                    lastResult = "{\"status\": \"ERROR\", \"msg\": \"Camera busy\"}";
                    currentState = STATE_IDLE;
                    continue;
                }
            }

            cap >> frame;
            if (frame.empty()) continue;

            FaceData face = detector.detect(frame);

            if (face.score > Config::CONF_THRESHOLD) {

                if (!timer_started) {
                    std::cout << ">>> [TIMER] Face detected, starting timeout sequence..." << std::endl;
                    start_time = std::chrono::steady_clock::now();
                    timer_started = true;
                }

                if (spoof.isReal(frame, face.bbox)) {
                    consecutiveReal++;
                    if (consecutiveReal >= Config::REQUIRED_FRAMES) {
                        if (currentState == STATE_VERIFYING) {
                            auto res = memory.verify(frame, face.bbox);
                            if (res.first != "Unknown")
                                lastResult = "{\"status\": \"SUCCESS\", \"user\": \"" + res.first + "\", \"score\": " + std::to_string(res.second) + "}";
                            else
                                lastResult = "{\"status\": \"DENIED\", \"msg\": \"Unauthorized\"}";
                        } else if (currentState == STATE_ENROLLING) {
                            memory.registerFace(frame, face.bbox, targetName);
                            lastResult = "{\"status\": \"SUCCESS\", \"msg\": \"Enrollment Complete\"}";
                        }
                        currentState = STATE_IDLE;
                    }
                } else {
                    consecutiveReal = 0;
                }
            } else {
                consecutiveReal = 0;
            }

            if (timer_started) {
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count();
                if (elapsed > Config::SCAN_TIMEOUT_MS) {
                    std::cout << ">>> [TIMEOUT] Operation expired." << std::endl;
                    lastResult = "{\"status\": \"TIMEOUT\", \"msg\": \"Operation timed out\"}";
                    currentState = STATE_IDLE;
                }
            }

        } else {
            if (cap.isOpened()) {
                cap.release();
            }
            timer_started = false;
            consecutiveReal = 0;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    return 0;
}