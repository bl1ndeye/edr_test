#include "file_estimator.hpp"

#include <fstream>
#include <iostream>
#include <iterator>
#include <utility>

#include <picosha2.h>

namespace fs = std::filesystem;

FileEstimator::FileEstimator(std::string filename)
    : m_file_name(std::move(filename))
{
}

FileEstimator::FileEstimator(std::string filename, std::string directory_path)
    : FileEstimator(std::move(filename))
{
    m_directory_path = std::move(directory_path);
}

void FileEstimator::createNewManifest(std::string directory_to_scan, std::string manifest_file_name)
{
    if (fs::exists(directory_to_scan))
    {
        if (is_path_syntax_valid(manifest_file_name))
        {
            if (fs::is_directory(directory_to_scan))
            {
                nlohmann::json new_manifest;
                for (const auto& entry : fs::recursive_directory_iterator(directory_to_scan))
                {
                    if (fs::is_regular_file(entry.status()))
                    {
                        std::string hex_str = get_file_hash(entry.path().string());
                        new_manifest.emplace(normalized_full_path(entry.path()), hex_str);
                    }
                }
                std::ofstream file(manifest_file_name, std::ios::out | std::ios::trunc);
                if (file.is_open())
                {
                    file << new_manifest.dump(4);
                    file.close();
                }
                else
                {
                    std::cerr << "Could not open file " << manifest_file_name << " to create manifest\n";
                }
            }
            else
            {
                std::cerr << "Path to directory is not really a directory" << std::endl;
                return;
            }
        }
        else
        {
            std::cerr << "Path to manifest file is not valid" << std::endl;
            return;
        }
    }
    else
    {
        std::cerr << "Directory to scan file to create manifest are not exist" << std::endl;
        return;
    }
}

void FileEstimator::parseManifestFile()
{
    std::ifstream file(m_file_name);
    if (!file.is_open())
    {
        std::cerr << "Failed to open\t" << m_file_name << " manifest file\n";
        return;
    }
    try
    {
        file >> m_baseline_manifest;
    }
    catch (const nlohmann::json::parse_error& error)
    {
        std::cerr << "Error parsing JSON\n";
        std::cerr << "Error:" << error.what() << '\n';
        std::cerr << "Near byte:" << error.byte << '\n';
    }
    file.close();
}

void FileEstimator::estimateFilesWithManifest()
{
    populateMapFromManifestJSON(m_baseline_manifest);
    auto tmp_map_manifest {m_parsed_manifest};

    auto add_estimation_item = [&](const std::string& filename, ALERT_TYPE type)
    {
        std::unique_ptr<FileAlert> p_alert = std::make_unique<FileAlert>();
        p_alert->m_type = type;
        p_alert->m_file_name = filename;
        m_buffer_alert->push(std::move(p_alert));
    };

    bool stopped = false;
    for (const auto& entry : fs::recursive_directory_iterator(m_directory_path))
    {
        if (m_stop_token.stop_requested())
        {
            stopped = true;
            break;
        }
        if (fs::is_regular_file(entry.status()))
        {
            auto cur_file_name = normalized_full_path(entry.path());
            std::string cur_hex_str = get_file_hash(entry.path().string());
            auto find_iterator = tmp_map_manifest.find(cur_file_name);
            if (find_iterator != tmp_map_manifest.end())
            {
                if (find_iterator->second == cur_hex_str)
                {
                    tmp_map_manifest.erase(find_iterator);
                }
                else
                {
                    add_estimation_item(cur_file_name, ALERT_TYPE::FileChanged);
                    tmp_map_manifest.erase(find_iterator);
                }
            }
            else
            {
                add_estimation_item(cur_file_name, ALERT_TYPE::FileAdded);
            }
        }
    }
    // On early stop we skip the deletion pass to avoid emitting false
    // "removed" alerts for files that were simply never scanned.
    if (!stopped)
    {
        for (const auto& item : tmp_map_manifest)
        {
            add_estimation_item(item.first, ALERT_TYPE::FileDeleted);
        }
    }
}

void FileEstimator::setBufferAleft(std::shared_ptr<BufferRingThreadSafe<std::unique_ptr<EDR_AlertBase>>> buf)
{
    m_buffer_alert = std::move(buf);
}

void FileEstimator::set_stop_token(std::stop_token token)
{
    m_stop_token = std::move(token);
}

void FileEstimator::populateMapFromManifestJSON(const nlohmann::json& manifest_json)
{
    for (const auto& el : manifest_json.items())
    {
        m_parsed_manifest[normalized_full_path(el.key())] = el.value().get<std::string>();
    }
}

// Canonical full path: forward slashes + upper-cased drive letter, so
// manifest keys and scanned paths compare equal regardless of separator/case.
std::string FileEstimator::normalized_full_path(const fs::path& p)
{
    std::string s = fs::absolute(p).lexically_normal().generic_string();
    if (s.size() >= 2 && s[1] == ':' && s[0] >= 'a' && s[0] <= 'z')
    {
        s[0] = static_cast<char>(s[0] - 'a' + 'A');
    }
    return s;
}

std::string FileEstimator::get_file_hash(const std::string& filepath)
{
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "File:\t" << filepath << "couldnot be open to calc hash\n";
        return "";
    }

    std::string hex_str;
    picosha2::hash256_hex_string(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>(),
        hex_str
    );
    return hex_str;
}

bool FileEstimator::is_path_syntax_valid(const fs::path& p)
{
    std::error_code ec;
    [[maybe_unused]] auto resolved = fs::weakly_canonical(p, ec);
    return !ec;
}
