#ifndef SPOOFDETECTOR_H
#define SPOOFDETECTOR_H

#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>
#include <vector>
#include <string>

class SpoofDetector {
public:
    SpoofDetector(const std::string& modelPath, float threshold = 0.70);
    bool isReal(const cv::Mat& frame, const cv::Rect& bbox);

private:
    float threshold;
    Ort::Env env;
    Ort::Session session;
    Ort::MemoryInfo memory_info;
    std::vector<const char*> output_names = {"output"}; // Modelinize göre "output0" olabilir
    cv::Size inputSize = cv::Size(80, 80);
};

#endif