#ifndef FACEMEMORY_H
#define FACEMEMORY_H

#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>
#include <map>
#include <string>
#include <vector>

class FaceMemory {
public:
    FaceMemory(const std::string& modelPath, const std::string& dbPath, float threshold = 0.40);

    bool registerFace(const cv::Mat& frame, const cv::Rect& bbox, const std::string& name);
    std::pair<std::string, float> verify(const cv::Mat& frame, const cv::Rect& bbox);
    bool deleteFace(const std::string& name);

    std::vector<std::string> getUserList() const;

private:
    std::vector<float> getEmbedding(const cv::Mat& faceROI);
    void loadDatabase();
    void saveDatabase();

    std::string dbPath;
    float threshold;
    std::map<std::string, std::vector<float>> database;

    Ort::Env env;
    Ort::Session session;
    Ort::MemoryInfo memory_info;
    std::vector<const char*> input_names;
    std::vector<const char*> output_names;
};

#endif