#include "FaceMemory.h"
#include <fstream>
#include <numeric>

FaceMemory::FaceMemory(const std::string& modelPath, const std::string& dbPath, float threshold)
    : dbPath(dbPath), threshold(threshold),
      env(ORT_LOGGING_LEVEL_WARNING, "FaceMemory"),
      session(env, modelPath.c_str(), Ort::SessionOptions{nullptr}),
      memory_info(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault))
{
    Ort::AllocatorWithDefaultOptions allocator;
    input_names = {session.GetInputNameAllocated(0, allocator).release()};
    output_names = {session.GetOutputNameAllocated(0, allocator).release()};
    loadDatabase();
}

std::vector<float> FaceMemory::getEmbedding(const cv::Mat& faceROI) {
    cv::Mat blob = cv::dnn::blobFromImage(
        faceROI,
        1.0 / 128.0,
        cv::Size(112, 112),
        cv::Scalar(127.5, 127.5, 127.5),
        true,
        false
    );

    std::vector<int64_t> input_shape = {1, 3, 112, 112};

    auto input_tensor = Ort::Value::CreateTensor<float>(
        memory_info,
        (float*)blob.data,
        3 * 112 * 112,
        input_shape.data(),
        4
    );

    auto outputs = session.Run(Ort::RunOptions{nullptr}, input_names.data(), &input_tensor, 1, output_names.data(), 1);
    float* out_ptr = outputs[0].GetTensorMutableData<float>();

    std::vector<float> emb(out_ptr, out_ptr + 512);

    float norm = std::sqrt(std::inner_product(emb.begin(), emb.end(), emb.begin(), 0.0f));

    if (norm > 1e-6) {
        for (float& f : emb) f /= norm;
    }

    return emb;
}

std::pair<std::string, float> FaceMemory::verify(const cv::Mat& frame, const cv::Rect& bbox) {
    cv::Rect safe_bbox = bbox & cv::Rect(0, 0, frame.cols, frame.rows);
    auto curr_emb = getEmbedding(frame(safe_bbox));

    std::string best_match = "Unknown";
    float max_score = 0.0f;

    for (auto const& [name, db_emb] : database) {
        float score = std::inner_product(curr_emb.begin(), curr_emb.end(), db_emb.begin(), 0.0f);
        if (score > max_score) {
            max_score = score;
            best_match = name;
        }
    }

    return (max_score > threshold) ? std::make_pair(best_match, max_score) : std::make_pair("Unknown", max_score);
}

bool FaceMemory::deleteFace(const std::string& name) {
    if (database.erase(name)) {
        saveDatabase();
        return true;
    }
    return false;
}

void FaceMemory::loadDatabase() {
    std::ifstream f(dbPath, std::ios::binary);
    if (!f.is_open()) {
        std::cout << ">>> [MEMORY] Database file not found. A new one will be created." << std::endl;
        return;
    }

    size_t userCount;
    f.read(reinterpret_cast<char*>(&userCount), sizeof(userCount));

    for (size_t i = 0; i < userCount; ++i) {
        size_t nameLen;
        f.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
        std::string name(nameLen, ' ');
        f.read(&name[0], nameLen);

        std::vector<float> emb(512);
        f.read(reinterpret_cast<char*>(emb.data()), 512 * sizeof(float));
        database[name] = emb;
    }
    f.close();
    std::cout << ">>> [MEMORY] Loaded " << database.size() << " users." << std::endl;
}

void FaceMemory::saveDatabase() {
    std::ofstream f(dbPath, std::ios::binary);
    if (!f.is_open()) {
        std::cerr << "!!! [ERROR] Failed to save database!" << std::endl;
        return;
    }

    size_t userCount = database.size();
    f.write(reinterpret_cast<const char*>(&userCount), sizeof(userCount));

    for (auto const& [name, emb] : database) {
        size_t nameLen = name.length();
        f.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));
        f.write(name.c_str(), nameLen);
        f.write(reinterpret_cast<const char*>(emb.data()), 512 * sizeof(float));
    }
    f.close();
    std::cout << ">>> [MEMORY] Database synced to disk." << std::endl;
}

bool FaceMemory::registerFace(const cv::Mat& frame, const cv::Rect& bbox, const std::string& name) {
    cv::Rect safe_bbox = bbox & cv::Rect(0, 0, frame.cols, frame.rows);
    if (safe_bbox.empty()) return false;

    database[name] = getEmbedding(frame(safe_bbox));
    saveDatabase();
    return true;
}

std::vector<std::string> FaceMemory::getUserList() const {
    std::vector<std::string> userNames;
    userNames.reserve(database.size());

    for (const auto& [name, emb] : database) {
        userNames.push_back(name);
    }
    return userNames;
}