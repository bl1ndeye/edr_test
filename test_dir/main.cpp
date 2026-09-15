#include <iostream>
#include <vector>
#include <ranges>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <random>


template<typename TItem>
class BufferRingThreadSafe
{
    public:
    BufferRingThreadSafe() = default;
    BufferRingThreadSafe (int size):items_(size), capacity_(size), head_index_(0), tail_index_(0), count_(0)
    {

    }

    void push(TItem item)
    {
        std::unique_lock<std::mutex> ul {mutex_};
        cv_full_.wait(ul, [this]() {
            return this->count_ < this->capacity_;
        });
        items_[tail_index_]= std::move(item);
        tail_index_ = (tail_index_ + 1) % capacity_;
        ++count_;
        cv_empty_.notify_one();
    }
    TItem pop()
    {
        std::unique_lock<std::mutex> ul {mutex_};
        cv_empty_.wait(ul, [this]() {
            return this->count_>0;
        });
        TItem pop_item = std::move(items_[head_index_]);
        head_index_ = (head_index_ + 1) % capacity_;
        --count_;
        cv_full_.notify_one();
        return pop_item;
    }

    private:
    std::vector<TItem> items_;
    std::size_t  capacity_;
    std::size_t count_;
    std::size_t head_index_;
    std::size_t tail_index_;

    std::mutex mutex_;
    std::condition_variable cv_full_;
    std::condition_variable cv_empty_;
};


thread_local std::mt19937 generator(std::random_device{}());
std::uniform_int_distribution<int> distribution(1, 113);

struct PiskaSiska
{
    PiskaSiska()
    {
        value= distribution(generator);
    }
    ~PiskaSiska()
    {
        std::cout<<"destroying\t"<<value<<"\n";
    }
    std::size_t value;
};


int main() {

    // int first_int = 5;
    // auto&& first = first_int;
    // auto&& second=55; 
    // first = 22;
    // std::cout<< first_int<<std::endl;

    BufferRingThreadSafe<PiskaSiska> buffer{13};

    auto write = [&buffer](){
        buffer.push(PiskaSiska{});
    }; 
    auto read = [&buffer](){
        std::cout<<"value pop:\t"<<buffer.pop().value<<std::endl;
    }; 

    for (int i=0;i<10;++i)
    {
        //std::jthread jwrite1 {write};
        std::jthread jread1 {read};
    }
    
    return 0;
}
