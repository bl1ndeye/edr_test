#pragma once

#include <cassert>
#include <cstdint>
#include <ios>
#include <iosfwd>
#include <iostream>
#include <fstream>
#include <memory>
#include <ostream>
#include <string>
#include <thread>
#include <vector>
#include "ring_buffer.hpp"
class EventEnumerator
{
public:
    EventEnumerator() = default;
    EventEnumerator(uint8_t threads_count):m_threads_count{threads_count}{};
    EventEnumerator(uint8_t threads_count, std::string file_name):EventEnumerator(threads_count)
    {
        m_file_name= std::move(file_name);
    };
    EventEnumerator(const EventEnumerator&) = delete;
    EventEnumerator& operator=(const EventEnumerator&) = delete;
    EventEnumerator(EventEnumerator&&) = default;
    EventEnumerator& operator=(EventEnumerator&&) = default;
    void startEnumeration()
    {
        assert(m_buffer_ptr!= nullptr && "Create and provide buffer PTR");
        //assert(std::is_same_v<decltype(m_buffer_ptr), std::nullptr_t>, 
        //      "Create and provide buffer PTR");
        calculateChunksPos();
        std::vector<std::jthread> vec_threads;
        for (const auto& chunk: m_chunks)
        {
            vec_threads.emplace_back(&EventEnumerator::read_file_chunk,this,chunk);
        }
    }

    void setBuffer(std::shared_ptr<BufferRingThreadSafe<std::string>> buf){
        m_buffer_ptr = buf;
    };
private:
    struct ThreadChunkPos
    {
        uint8_t m_thread_index;
        std::streampos m_chunk_begin;
        std::streampos m_chunk_end;
    };
    void read_file_chunk(ThreadChunkPos chunk_info)
    {
        std::ifstream file(m_file_name);
        if (!file)
        {
            std::cerr<<"Cant open file, thread-index="<<chunk_info.m_thread_index<<"\tthread-id="<<std::this_thread::get_id()<<std::endl;
            //TODO: stop all threads from executing, terminate signal
            return;
        }
        file.seekg(chunk_info.m_chunk_begin);
        std::string event_line;
        while (std::getline(file,event_line) && file.tellg()!=-1 && (file.tellg()<=chunk_info.m_chunk_end)  )
        {
            if (!m_buffer_ptr->push(event_line))
            {
                break;
            }
        }
    }
    // by default , read whole file
    // also if section_start passed, allows to calc for section from given start till the eof
    void calculateChunksPos(std::streampos section_start = 0)
    {
        bool wholeFile = section_start==0;
        std::ifstream file{m_file_name, std::ios::binary | ((wholeFile) ? std::ios::ate : static_cast<std::ios::openmode>(0))};
        if (!file.is_open()) 
        {
            std::cerr << "Error opening file, check filename or access rights.\n";
            return;
        }
        std::streampos section_size;
        if (wholeFile)
        {
            section_size = file.tellg();
        }
        else {
            file.seekg(section_start, std::ios::end);
            std::streamoff end = file.tellg();
            section_size = end-section_start;
        }
        //std::streampos section_size = wholeFile ? static_cast<std::streampos>(file.tellg()): static_cast<std::streampos>(file.tellg()-section_start,(file.seekg(section_start, std::ios::end)));
        std::streampos chunk_size = section_size / m_threads_count;
        std::streampos calc_valid_chunk_begin = wholeFile? static_cast<std::streampos>(0): section_start; // every chunk should begin from "newline"
        m_chunks.clear();
        for (int i =0;i<m_threads_count;++i)
        {
            uint8_t thread_index=i;
            std::streampos chunk_begin = (calc_valid_chunk_begin==0) ? static_cast<std::streampos>(thread_index*chunk_size): calc_valid_chunk_begin;
            std::streampos chunk_end = chunk_begin+chunk_size;
            std::streamoff delta;
            auto validate_chunk_end = [&chunk_end,&delta,&file]()
            {
                delta=0;
                file.seekg(chunk_end);
                char end_line;
                while (file.get(end_line))
                {
                    ++delta;
                    if (end_line=='\n')
                    {
                        break;
                        // eof check is not needed , while loop will be terminated 
                    }
                }
            };
            validate_chunk_end();
            chunk_end = calc_valid_chunk_begin = chunk_end+delta;
            //chunk_end+= delta;
            m_chunks.emplace_back(ThreadChunkPos{static_cast<uint8_t>(i), chunk_begin,chunk_end});
        }
        file.close();
    }
    std::vector<ThreadChunkPos> m_chunks;
    std::shared_ptr<BufferRingThreadSafe<std::string>> m_buffer_ptr;
    uint8_t m_threads_count=2;
    std::string m_file_name;
};