#include "Detector.h"
#include <iostream>

FaceDetector::FaceDetector(const std::string& modelPath, float confThreshold)
    : confThreshold(confThreshold),
      env(ORT_LOGGING_LEVEL_WARNING, "FaceDetector"),
      session(env, modelPath.c_str(), Ort::SessionOptions{nullptr}),
      memory_info(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault))
{
    Ort::AllocatorWithDefaultOptions allocator;

    auto input_name_ptr = session.GetInputNameAllocated(0, allocator);
    input_names_str.push_back(input_name_ptr.get());
    input_names = {input_names_str[0].c_str()};

    size_t num_outputs = session.GetOutputCount();
    for (size_t i = 0; i < num_outputs; i++) {
        auto out_name_ptr = session.GetOutputNameAllocated(i, allocator);
        output_names_str.push_back(out_name_ptr.get());
    }
    for (const auto& name : output_names_str) output_names.push_back(name.c_str());
}

FaceData FaceDetector::detect(const cv::Mat& frame) {
    if (frame.empty()) return {cv::Rect(), 0.0f};

    cv::Mat det_image = cv::Mat::zeros(inputSize, CV_8UC3);
    float ratio = std::min((float)inputSize.width / frame.cols, (float)inputSize.height / frame.rows);
    int new_w = frame.cols * ratio;
    int new_h = frame.rows * ratio;

    cv::Mat resized;
    cv::resize(frame, resized, cv::Size(new_w, new_h));
    resized.copyTo(det_image(cv::Rect(0, 0, new_w, new_h)));

    cv::Mat blob = cv::dnn::blobFromImage(det_image, 1.0/128.0, inputSize, cv::Scalar(127.5, 127.5, 127.5), true);
    std::vector<int64_t> input_shape = {1, 3, 640, 640};
    auto input_tensor = Ort::Value::CreateTensor<float>(memory_info, (float*)blob.data, blob.total(), input_shape.data(), input_shape.size());
    auto outputs = session.Run(Ort::RunOptions{nullptr}, input_names.data(), &input_tensor, 1, output_names.data(), output_names.size());

    FaceData best_face = {cv::Rect(), 0.0f};

    for (int i = 0; i < 3; i++) {
        float* scores = outputs[i].GetTensorMutableData<float>();
        float* bboxes = outputs[i+3].GetTensorMutableData<float>();
        int stride = feat_stride_fpn[i];
        int cols = inputSize.width / stride;
        int rows = inputSize.height / stride;

        for (int idx = 0; idx < rows * cols * 2; idx++) {
            float score = scores[idx];
            if (score > best_face.score && score > confThreshold) {
                best_face.score = score;

                int anchor_idx = idx / 2;
                float anchor_x = (anchor_idx % cols) * stride;
                float anchor_y = (anchor_idx / cols) * stride;

                float x1 = (anchor_x - bboxes[idx*4] * stride) / ratio;
                float y1 = (anchor_y - bboxes[idx*4+1] * stride) / ratio;
                float x2 = (anchor_x + bboxes[idx*4+2] * stride) / ratio;
                float y2 = (anchor_y + bboxes[idx*4+3] * stride) / ratio;

                best_face.bbox = cv::Rect(cv::Point(x1, y1), cv::Point(x2, y2));
            }
        }
    }
    return best_face;
}