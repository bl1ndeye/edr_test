#pragma once

#include "alerts.hpp"
#include "ring_buffer.hpp"
#include <iostream>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <picosha2.h>

namespace  fs =std::filesystem;
static constexpr auto iteratorOptionsIgnorePermissions = fs::directory_options::skip_permission_denied;


class FileEstimator
{
    public:
    FileEstimator() = default;
    FileEstimator(std::string filename):m_file_name(filename){};
    FileEstimator(std::string filename, std::string directory_path):FileEstimator(filename)
    {
        m_directory_path = directory_path;
    };


    FileEstimator(const FileEstimator&) = delete;
    FileEstimator& operator=(const FileEstimator&) = delete;
    FileEstimator(FileEstimator&&) = default;
    FileEstimator& operator=(FileEstimator&&) = default;
    void createNewManifest(std::string directory_to_scan, std::string manifest_file_name)
    {
        if (fs::exists( directory_to_scan))
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
                                auto file_name = entry.path().string();
                                // TODO change to hex of file
                                std::string hex_str = get_file_hash(file_name);
                                new_manifest.emplace(file_name, hex_str);
                            }
                        }
                    //std::cout<< new_manifest.dump(4)<<std::endl;
                    std::ofstream file(manifest_file_name, std::ios::out | std::ios::trunc);
                    if (file.is_open())
                    {
                        file<<new_manifest.dump(4);
                        file.close();
                    }
                    else
                    {
                        std::cerr<<"Could not open file "<<manifest_file_name<<" to create manifest\n";
                    }
                }
                else 
                {
                    std::cerr<<"Path to directory is not really a directory"<<std::endl;
                    return;
                }
            }
            else 
            {
                std::cerr<<"Path to manifest file is not valid"<<std::endl;
                return;
            }
        }
        else {
        std::cerr<<"Directory to scan file to create manifest are not exist"<<std::endl;
        return;
        }
    }
    void parseManifestFile()
    {
        std::ifstream file(m_file_name);
        if (!file.is_open())
        {
            std::cerr<<"Failed to open\t"<<m_file_name<<" manifest file\n";
            return;
        }
        try
        {
            file>>m_baseline_manifest;
        }
        catch(const nlohmann::json::parse_error& error)
        {
            std::cerr<<"Error parsing JSON\n";
            std::cerr<<"Error:"<<error.what()<<'\n';
            std::cerr<<"Near byte:"<<error.byte<<'\n';
        }
        file.close();
    }
    // evaluate  map/unordered map effectiveness
    void estimateFilesWithManifest()
    {
        populateMapFromManifestJSON(m_baseline_manifest, m_parsed_manifest);
        // i dunno, probably could be tested on large datasets
        // what if remove and rebalance of map will affect 
        // posivitely due to faster find call
        // for smaller datasets / file count for sure
        // std::unordered_map<std::string, std::string> tmp_map_manifest;
        // for (const auto& pair: m_parsed_manifest)
        // {
        //     tmp_map_manifest.insert(pair);
        // }
        auto tmp_map_manifest {m_parsed_manifest};
        // std::vector<std::string> estimation_result;
        // enum class ESTIMATION_TYPE
        // {
        //     FILE_ADDED      = 5,
        //     FILE_CHANGED    = 7,
        //     FILE_DELETED    = 13
        // };
        auto add_estimation_item = [&](const std::string& filename, ALERT_TYPE type )
        {
            // std::string item_text;
            // switch (type) 
            // {
            //     case ESTIMATION_TYPE::FILE_ADDED:
            //     item_text = "File Added: ";
            //     break;
            //     case ESTIMATION_TYPE::FILE_CHANGED:
            //     item_text = "File Changed: ";
            //     break;
            //     case ESTIMATION_TYPE::FILE_DELETED:
            //     item_text = "File Deleted: ";
            //     break;
            // }
            // item_text+=filename;
            // estimation_result.emplace_back(item_text);

            std::unique_ptr<FileAlert> p_alert =std::make_unique<FileAlert>() ;
            p_alert->m_type = type;
            p_alert->m_file_name= filename;
            m_buffer_alert->push(std::move(p_alert));
        };
        for (const auto& entry : fs::recursive_directory_iterator(m_directory_path)) 
            {
                if (fs::is_regular_file(entry.status())) 
                {
                    auto cur_file_name = entry.path().string();
                    std::string cur_hex_str = get_file_hash(cur_file_name);
                    auto find_iterator = tmp_map_manifest.find(cur_file_name);
                    if (find_iterator!=tmp_map_manifest.end())
                    {
                        if (find_iterator->second==cur_hex_str)
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
        for (const auto& item:tmp_map_manifest)
        {
            add_estimation_item(item.first, ALERT_TYPE::FileDeleted);
        }    
    }

    void setBufferAleft(std::shared_ptr<BufferRingThreadSafe<std::unique_ptr<EDR_AlertBase>>>  buf)
    {
        m_buffer_alert = buf;
    };
    private:
    void populateMapFromManifestJSON(const nlohmann::json& manifest_json, std::map<std::string, std::string>& manifest_map)
    {
        for (const auto& el : manifest_json.items()) 
        {
            m_parsed_manifest[el.key()] = el.value().get<std::string>();
        }
        //error proof?
        //manifest_map = manifest_json.get<std::map<std::string, std::string>>();
    }
    std::string get_file_hash(const std::string& filepath) 
    {
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) 
        {
            std::cerr<<"File:\t"<< filepath <<"couldnot be open to calc hash\n";
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
    bool is_path_syntax_valid(const fs::path& p) 
    {
        std::error_code ec;
        fs::weakly_canonical(p, ec);
        return !ec; 
    }
    // to analyse files
    std::string m_directory_path;
    // for manifest
    std::string m_file_name;
    // to store manifest as json
    nlohmann::json m_baseline_manifest;
    // filename:sha hash to speed up search
    std::map<std::string, std::string> m_parsed_manifest;
    // for alerts objects
    std::shared_ptr<BufferRingThreadSafe<std::unique_ptr<EDR_AlertBase>>> m_buffer_alert;

};