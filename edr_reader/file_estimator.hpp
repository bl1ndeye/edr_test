#pragma once

#include <filesystem>
#include <map>
#include <memory>
#include <string>

#include "alerts.hpp"
#include "nlohmann/json.hpp"
#include "ring_buffer.hpp"

class FileEstimator
{
public:
    FileEstimator() = default;
    FileEstimator(std::string filename);
    FileEstimator(std::string filename, std::string directory_path);

    FileEstimator(const FileEstimator&) = delete;
    FileEstimator& operator=(const FileEstimator&) = delete;
    FileEstimator(FileEstimator&&) = default;
    FileEstimator& operator=(FileEstimator&&) = default;

    void createNewManifest(std::string directory_to_scan, std::string manifest_file_name);
    void parseManifestFile();
    void estimateFilesWithManifest();
    void setBufferAleft(std::shared_ptr<BufferRingThreadSafe<std::unique_ptr<EDR_AlertBase>>> buf);

private:
    void populateMapFromManifestJSON(const nlohmann::json& manifest_json);
    std::string normalized_full_path(const std::filesystem::path& p);
    std::string get_file_hash(const std::string& filepath);
    bool is_path_syntax_valid(const std::filesystem::path& p);

    std::string m_directory_path;
    std::string m_file_name;
    nlohmann::json m_baseline_manifest;
    std::map<std::string, std::string> m_parsed_manifest;
    std::shared_ptr<BufferRingThreadSafe<std::unique_ptr<EDR_AlertBase>>> m_buffer_alert;
};
