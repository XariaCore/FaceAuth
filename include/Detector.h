#ifndef DETECTOR_H
#define DETECTOR_H

#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>
#include <vector>
#include <string>

struct FaceData {
    cv::Rect bbox;
    float score;
};

class FaceDetector {
public:
    FaceDetector(const std::string& modelPath, float confThreshold = 0.55);
    FaceData detect(const cv::Mat& frame);

private:
    float confThreshold;
    cv::Size inputSize = cv::Size(640, 640);
    std::vector<int> feat_stride_fpn = {8, 16, 32};

    Ort::Env env;
    Ort::Session session;
    Ort::MemoryInfo memory_info;

    std::vector<std::string> input_names_str;
    std::vector<std::string> output_names_str;
    std::vector<const char*> input_names;
    std::vector<const char*> output_names;
};

#endif