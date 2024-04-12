#pragma once

#include <atomic>
#include <cstddef>
#include <condition_variable>
#include <mutex>
#include <queue>

namespace reactor {
namespace queue {

template<typename T>
class LockedQueue
{
public:
    LockedQueue()
        : queue_()
        , cv_() {}
    LockedQueue(LockedQueue const &) =default;
    LockedQueue(LockedQueue &&) =default;
    LockedQueue & operator=(LockedQueue const &) = default;
    LockedQueue & operator=(LockedQueue &&) = default;
    ~LockedQueue() {}

    void push(T const & v)
    {
        std::lock_guard<std::mutex> lk(mx_);
        queue_.push(v);
        size_.fetch_add(1, std::memory_order_release);
        cv_.notify_one();
    }

    T pop()
    {
        std::unique_lock<std::mutex> lk(mx_);
        cv_.wait(lk, [&]{ return !queue_.empty(); });
        T & ret = queue_.front();
        size_.fetch_sub(1, std::memory_order_acquire);
        lk.unlock();
        queue_.pop();

        return ret;
    }

    bool empty()
    {
        std::lock_guard<std::mutex> lk(mx_);
        return size_.load(std::memory_order_acquire);
    }

private:
    std::queue<T>           queue_;
    std::mutex              mx_;
    std::condition_variable cv_;
    std::atomic<size_t>     size_;
};


template<typename T>
class LockFreeQueue
{
public:
    LockFreeQueue(size_t capacity)
            :   capacity_(capacity),
                size_(0),
                head_(-1),
                tail_(-1),
                tail_updated_(false),
                buffer_(0)
        {
            if (is_power_of_two(capacity))
                capacity_ = capacity;
            else
                capacity_ = round_up_to_power_of_two(capacity);

            buffer_.reserve(capacity_);
        }
    
    LockFreeQueue(LockFreeQueue&& queue)
        : size_(queue.size_),
          capacity_(queue.capacity_),
          tail_(queue.tail_.load(std::memory_order_acquire)),
          head_(queue.head_.load(std::memory_order_acquire)),
          buffer_(std::move(queue.buffer_))
    {
        queue.size_ = 0;
        queue.capacity_ = 0;
        tail_.store(-1, std::memory_order_release);
        head_.store(-1, std::memory_order_release);
        queue.buffer_.clear();
        queue.buffer_.shrink_to_fit();
    }       

    LockFreeQueue(const LockFreeQueue& queue) = delete;
    LockFreeQueue & operator=(LockFreeQueue && ) = delete;
    LockFreeQueue & operator=(const LockFreeQueue & ) = delete;
    ~LockFreeQueue();

private:
    bool is_power_of_two(size_t& size) { return (size > 0) && ((size & (size - 1)) == 0); }

    size_t round_up_to_power_of_two(size_t & size) {
        size_t power = 1;
        while (power < size) {
            power <<= 1;
        }

        return power;
    }

private:
    size_t capacity_;
    size_t size_;
    std::atomic<int64_t> head_;
    std::atomic<int64_t> tail_;
    std::atomic<bool> tail_updated_;
    std::vector<T> buffer_;
};

} // namespace queue
} // namespace reactor