#pragma once

#include <condition_variable>
#include <chrono>
#include <iostream>
#include <mutex>
#include <vector>

static constexpr auto TIMEOUT_DURATION = std::chrono::seconds{10};

template<typename TItem>
class BufferRingThreadSafe
{
    public:
    BufferRingThreadSafe()  { items_.resize(capacity_);}
    BufferRingThreadSafe (int size):items_(size), capacity_(size), head_index_(0), tail_index_(0), count_(0)
    {
    }

    void push(TItem item)
    {
        std::unique_lock<std::mutex> ul {mutex_};
        bool ready_to_push = cv_full_.wait_for(ul,TIMEOUT_DURATION ,[this]() {
            return this->count_ < this->capacity_;
        });
        if (!ready_to_push)
        {
            std::cerr<<"BufferRingThreadSafe::push timeouted"<<std::endl;
            return;
        }
        items_[tail_index_]= std::move(item);
        tail_index_ = (tail_index_ + 1) % capacity_;
        ++count_;
        cv_empty_.notify_one();
    }
    TItem pop()
    {
        std::unique_lock<std::mutex> ul {mutex_};
        bool ready_to_pop = cv_empty_.wait_for(ul, TIMEOUT_DURATION, [this]() {
            return this->count_>0;
        });
        if (!ready_to_pop)
        {
            std::cerr<<"BufferRingThreadSafe::pop timeouted"<<std::endl;
            return TItem{};
        }
        TItem pop_item = std::move(items_[head_index_]);
        head_index_ = (head_index_ + 1) % capacity_;
        --count_;
        cv_full_.notify_one();
        return pop_item;
    }
    bool hasElements(){return count_>0;}
    std::size_t size() const { return count_;}
    void resize(std::size_t new_size) 
    {
        items_.resize(new_size); 
        capacity_ = new_size;
    }
    private:
    std::vector<TItem> items_;
    std::size_t  capacity_ = 10;
    std::size_t count_ = 0 ;
    std::size_t head_index_;
    std::size_t tail_index_;

    std::mutex mutex_;
    std::condition_variable cv_full_;
    std::condition_variable cv_empty_;
};