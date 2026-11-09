#ifndef CONFIG_H
#define CONFIG_H

#include <string>

namespace Config {
    const std::string BASE_DIR      = "/opt/FaceAuth/";

    const std::string DETECTOR_PATH = BASE_DIR + "models/det_500m.onnx";
    const std::string SPOOF_PATH    = BASE_DIR + "models/2.7_80x80_MiniFASNetV2.onnx";
    const std::string RECOG_PATH    = BASE_DIR + "models/w600k_mbf.onnx";

    const std::string DB_PATH       = BASE_DIR + "data/database.bin";

    const std::string SOCKET_PATH   = "/tmp/faceauth.sock";

    const int REQUIRED_FRAMES      = 3;
    const float SPOOF_THRESHOLD    = 0.80f;
    const float RECOG_THRESHOLD    = 0.50f;
    const float CONF_THRESHOLD     = 0.50f;
    const int SCAN_TIMEOUT_MS      = 4096;
}

#endif