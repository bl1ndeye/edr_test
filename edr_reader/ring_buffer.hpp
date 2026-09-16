#pragma once

#include <condition_variable>
#include <iostream>
#include <mutex>
#include <vector>


// потоко безопасный буфффер, собственно используется почти везде
// чтобы не надо было плодить примитивы разные для хендлинга много поточки, ожидания и проч.
// простой, пожалуй даже слишком, просто наработки были готовые скопипастил в интернете где-то
// переписал под себя
template<typename TItem>
class BufferRingThreadSafe
{
    public:
    BufferRingThreadSafe()  { items_.resize(capacity_);}
    BufferRingThreadSafe (int size):items_(size), capacity_(size), head_index_(0), tail_index_(0), count_(0)
    {
    }

    bool push(TItem item)
    {
        std::unique_lock<std::mutex> ul {mutex_};
        cv_full_.wait(ul, [this]() {
            return this->closed_ || this->count_ < this->capacity_;
        });
        if (closed_)
        {
            return false;
        }
        items_[tail_index_]= std::move(item);
        tail_index_ = (tail_index_ + 1) % capacity_;
        ++count_;
        cv_empty_.notify_one();
        return true;
    }
    TItem pop()
    {
        std::unique_lock<std::mutex> ul {mutex_};
        cv_empty_.wait(ul, [this]() {
            return this->closed_ || this->count_ > 0;
        });
        if (count_ == 0)
        {
            return TItem{};
        }
        TItem pop_item = std::move(items_[head_index_]);
        head_index_ = (head_index_ + 1) % capacity_;
        --count_;
        cv_full_.notify_one();
        return pop_item;
    }
    void close()
    {
        std::lock_guard<std::mutex> lg {mutex_};
        closed_ = true;
        cv_empty_.notify_all();
        cv_full_.notify_all();
    }
    bool isClosed() const
    {
        std::lock_guard<std::mutex> lg {mutex_};
        return closed_;
    }
    bool hasElements()
    {
        std::lock_guard<std::mutex> lg {mutex_};
        return count_>0;
    }
    std::size_t size()
    {
        std::lock_guard<std::mutex> lg {mutex_};
        return count_;
    }
    void resize(std::size_t new_size)
    {
        std::lock_guard<std::mutex> lg {mutex_};
        items_.resize(new_size);
        capacity_ = new_size;
    }

    private:
    std::vector<TItem> items_;
    std::size_t  capacity_ = 10;
    std::size_t count_ = 0 ;
    std::size_t head_index_=0;
    std::size_t tail_index_=0;
    bool closed_ = false;

    mutable std::mutex mutex_;
    std::condition_variable cv_full_;
    std::condition_variable cv_empty_;
};