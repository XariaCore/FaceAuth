#ifndef CONFIG_H
#define CONFIG_H

#include <string>

namespace Config {
    // --- Constant Paths (/opt structure) ---
    const std::string BASE_DIR      = "/opt/FaceAuth/";

    // Model file paths
    const std::string DETECTOR_PATH = BASE_DIR + "models/det_500m.onnx";
    const std::string SPOOF_PATH    = BASE_DIR + "models/2.7_80x80_MiniFASNetV2.onnx";
    const std::string RECOG_PATH    = BASE_DIR + "models/w600k_mbf.onnx";

    // User vector database file
    const std::string DB_PATH       = BASE_DIR + "data/database.bin";

    // IPC Socket path for client communication
    const std::string SOCKET_PATH   = "/tmp/faceauth.sock";

    // --- Configuration Parameters ---
    const int REQUIRED_FRAMES      = 3;      // Consecutive frames required for liveness
    const float SPOOF_THRESHOLD    = 0.80f;  // Liveness detection threshold
    const float RECOG_THRESHOLD    = 0.50f;  // Recognition similarity threshold
    const float CONF_THRESHOLD     = 0.50f;  // Face detection confidence threshold
    const int SCAN_TIMEOUT_MS      = 4096;   // Scanning timeout duration
}

#endif