#pragma once

#include <cstdint>
#include <ios>
#include <memory>
#include <stop_token>
#include <string>
#include <vector>

#include "ring_buffer.hpp"

// многопоточное чтение файла
// строки напихывает в буффер для последующей обработки детектором
class EventEnumerator
{
public:
    EventEnumerator() = default;
    EventEnumerator(std::uint8_t threads_count);
    EventEnumerator(std::uint8_t threads_count, std::string file_name);

    EventEnumerator(const EventEnumerator&) = delete;
    EventEnumerator& operator=(const EventEnumerator&) = delete;
    EventEnumerator(EventEnumerator&&) = default;
    EventEnumerator& operator=(EventEnumerator&&) = default;

    void startEnumeration();
    void setBuffer(std::shared_ptr<BufferRingThreadSafe<std::string>> buf);
    void set_stop_token(std::stop_token token);

private:
    struct ThreadChunkPos
    {
        std::uint8_t m_thread_index;
        std::streampos m_chunk_begin;
        std::streampos m_chunk_end;
    };

    void read_file_chunk(ThreadChunkPos chunk_info);
    // высчитывает позиции кусков
    // может быть вызван повторно
    // чтобы пересчитать куски для чтения для добавленных строк в файл
    // аля реальный файл журнала/ лога в который что-то пишется сторонним сервисом
    void calculateChunksPos(std::streampos section_start = 0);

    std::vector<ThreadChunkPos> m_chunks;
    std::shared_ptr<BufferRingThreadSafe<std::string>> m_buffer_ptr;
    std::uint8_t m_threads_count = 2;
    std::string m_file_name;
    std::stop_token m_stop_token;
};
