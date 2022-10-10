#pragma once
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
//---------------------------------------------------------------------------
class ThreadPool{
public:
    ThreadPool(size_t num_threads = 0)
    :num_threads_(num_threads), stop_all(false)
    {
        if(num_threads_ == 0){
            num_threads_ = GetSupportedHardwareThreads();
        }
        worker_threads_.reserve(num_threads_);
        for (size_t i = 0; i < num_threads_; ++i) {
            worker_threads_.emplace_back([this] { this->Run(); });
        }
    }

    ~ThreadPool() noexcept {
        stop_all = true;
        cv_job_q_.notify_all();

        for(auto& thread : worker_threads_){
            if(thread.joinable()){
                thread.join();
            }
        }
    }

    template<class F, class... Args>
    auto EnqueueJob(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
        if (stop_all) {
            throw std::runtime_error("ThreadPool 사용 중지됨");
        }
        using return_type = decltype(f(args...));
        using wrapped_task = std::packaged_task<return_type()>;
        
        // Perfect Forwarding을 이용해 작업 생성
        auto task = std::make_shared<wrapped_task>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        std::future<return_type> result = task->get_future();

        // 작업 큐에 추가
        {
            std::lock_guard<std::mutex> lock(m_job_q_); // 큐 보호용 락가드
            jobs_.push([task = std::move(task)] { (*task)(); }); // 작업 큐에 추가
        }
        cv_job_q_.notify_one(); // 다른 스레드에 알리기

        return result;
    }

    size_t GetNumThreads() const{
        return num_threads_;
    }

    static size_t GetSupportedHardwareThreads(){
        return std::thread::hardware_concurrency();
    }

private:
    // 총 Worker 쓰레드의 개수.
    size_t num_threads_;
    // Worker 쓰레드를 보관하는 벡터.
    std::vector<std::thread> worker_threads_;
    // 할일들을 보관하는 job 큐.
    std::queue<std::function<void()>> jobs_;
    // 위의 job 큐를 위한 cv 와 m.
    std::condition_variable cv_job_q_;
    std::mutex m_job_q_;

    // 모든 쓰레드 종료
    bool stop_all;

    void Run() {
        std::function<void()> job;
        while(true){    
            {
                std::unique_lock<std::mutex> lock(m_job_q_);
            
                // 작업 큐가 비어있지 않거나 모든 쓰레드가 종료된 경우가 아니라면 대기한다.
                cv_job_q_.wait(lock, [this]() { return !this->jobs_.empty() || stop_all; });
                
                // 쓰레드가 모두 종료된 경우거나 작업 큐가 비어있다면 종료한다.
                if (stop_all && this->jobs_.empty()) {
                    return;
                }

                // 맨 앞의 job 을 뺀다.
                job = std::move(jobs_.front());
                jobs_.pop();
            }

            // 해당 job 을 수행한다 :)
            job();
        }
    }
};
//---------------------------------------------------------------------------