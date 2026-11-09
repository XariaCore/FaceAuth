#include "SpoofDetector.h"
#include <cmath>
#include <algorithm>
#include <vector>
#include <iostream>
#include <iomanip>

SpoofDetector::SpoofDetector(const std::string& modelPath, float threshold)
    : threshold(threshold),
      env(ORT_LOGGING_LEVEL_WARNING, "SpoofDetector"),
      session(env, modelPath.c_str(), Ort::SessionOptions{nullptr}),
      memory_info(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault)) {}

bool SpoofDetector::isReal(const cv::Mat& frame, const cv::Rect& bbox) {
    if (frame.empty() || bbox.width <= 0 || bbox.height <= 0) return false;

    int w_box = bbox.width;
    int h_box = bbox.height;
    int cx = bbox.x + w_box / 2;
    int cy = bbox.y + h_box / 2;

    int new_w = static_cast<int>(w_box * 2.7f);
    int new_h = static_cast<int>(h_box * 2.7f);

    int x1_new = std::max(0, cx - new_w / 2);
    int y1_new = std::max(0, cy - new_h / 2);
    int x2_new = std::min(frame.cols, cx + new_w / 2);
    int y2_new = std::min(frame.rows, cy + new_h / 2);

    cv::Mat crop = frame(cv::Range(y1_new, y2_new), cv::Range(x1_new, x2_new)).clone();
    if (crop.empty()) return false;

    cv::Mat resized;
    cv::resize(crop, resized, cv::Size(80, 80));
    cv::cvtColor(resized, resized, cv::COLOR_BGR2RGB);

    std::vector<float> input_tensor_values(1 * 3 * 80 * 80);
    for (int c = 0; c < 3; ++c) {
        for (int h = 0; h < 80; ++h) {
            for (int w = 0; w < 80; ++w) {
                input_tensor_values[c * 80 * 80 + h * 80 + w] = static_cast<float>(resized.at<cv::Vec3b>(h, w)[c]);
            }
        }
    }

    std::vector<int64_t> input_shape = {1, 3, 80, 80};
    Ort::AllocatorWithDefaultOptions allocator;
    auto in_name = session.GetInputNameAllocated(0, allocator);
    auto out_name = session.GetOutputNameAllocated(0, allocator);
    const char* in_names[] = {in_name.get()};
    const char* out_names[] = {out_name.get()};

    auto input_tensor = Ort::Value::CreateTensor<float>(
        memory_info, input_tensor_values.data(), input_tensor_values.size(),
        input_shape.data(), input_shape.size()
    );

    auto outputs = session.Run(Ort::RunOptions{nullptr}, in_names, &input_tensor, 1, out_names, 1);
    float* scores = outputs[0].GetTensorMutableData<float>();

    float s0 = scores[0];
    float s1 = scores[1];
    float s2 = scores[2];

    float max_logit = std::max({s0, s1, s2});
    float exp0 = std::exp(s0 - max_logit);
    float exp1 = std::exp(s1 - max_logit);
    float exp2 = std::exp(s2 - max_logit);
    float sum_exp = exp0 + exp1 + exp2;

    float real_score = exp1 / sum_exp;

    std::cout << "--------------------------------------" << std::endl;
    std::cout << "[DEBUG] Logits: [" << std::fixed << std::setprecision(6)
              << s0 << ", " << s1 << ", " << s2 << "]" << std::endl;
    std::cout << "[DEBUG] Real Probability: " << real_score << std::endl;
    std::cout << "--------------------------------------" << std::endl;

    return (real_score > threshold);
}