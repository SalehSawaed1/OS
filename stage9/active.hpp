#pragma once
#include <mutex>
#include <condition_variable>
#include <deque>
#include <thread>
#include <atomic>
#include <utility>

template <typename T>
class ActiveObject {
public:
    using Handler = void(*)(T&&, void*);

    ActiveObject() = default;
    ~ActiveObject(){ stop(); }

    void start(Handler h, void* ctx, const char* name = nullptr) {
        handler_ = h; ctx_ = ctx; running_.store(true);
        thr_ = std::thread([this,name]{ loop(name); });
    }
    void post(T item) {
        {
            std::lock_guard<std::mutex> lk(m_);
            q_.push_back(std::move(item));
        }
        cv_.notify_one();
    }
    void stop() {
        bool expected = true;
        if (running_.compare_exchange_strong(expected, false)) {
            cv_.notify_all();
            if (thr_.joinable()) thr_.join();
        }
    }

private:
    void loop(const char* /*name*/) {
        while (running_.load()) {
            T item;
            {
                std::unique_lock<std::mutex> lk(m_);
                cv_.wait(lk, [&]{ return !running_.load() || !q_.empty(); });
                if (!running_.load()) break;
                item = std::move(q_.front()); q_.pop_front();
            }
            if (handler_) handler_(std::move(item), ctx_);
        }
        // drain remaining items if any (optional)
        while (true) {
            T item;
            {
                std::lock_guard<std::mutex> lk(m_);
                if (q_.empty()) break;
                item = std::move(q_.front()); q_.pop_front();
            }
            if (handler_) handler_(std::move(item), ctx_);
        }
    }

    std::mutex m_;
    std::condition_variable cv_;
    std::deque<T> q_;
    std::thread thr_;
    std::atomic<bool> running_{false};
    Handler handler_{nullptr};
    void* ctx_{nullptr};
};
