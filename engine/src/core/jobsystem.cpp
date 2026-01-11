#include <iostream>
#include <coroutine>
#include "loggersystem.h"
#include "jobsystem.h"

template<> MT_API mtJobSystem* Singleton<mtJobSystem>::_instance = nullptr;

struct _JobTask {
    struct promise_type {
        _JobTask get_return_object() {
            return _JobTask{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };

    std::coroutine_handle<promise_type> _handle;
    explicit _JobTask(std::coroutine_handle<promise_type> handle)
        : _handle(handle) {}

    void resume() {
        if (_handle && !_handle.done()) {
            _handle.resume();
        }
    }
};

_JobTask JobTaskFunc(std::queue<mtJob>& jobQueue) {
    while (!jobQueue.empty()) {
        mtJob job = jobQueue.front();
        jobQueue.pop();
        job.run();
    }
    co_return;
}

b8 mtJobSystem::initialize() {
    //std::cout << "JobSystem initialized with " << _numThreads << " threads." << std::endl;
    MT_LOG_INFO("JobSystem initialized with {} threads.", _numThreads);
    // Initialize job system with specified number of threads
    return true;
}

b8 mtJobSystem::shutdown() {
    // Shutdown job system and clean up resources
    return true;
}

void mtJobSystem::addJob(mtJob& job) {
    // Add job to the job queue for execution
    _jobQueue.push(job);
}

void mtJobSystem::runJobs() {
    // Run all jobs in the job queue
    auto jobTask = JobTaskFunc(_jobQueue); 
    jobTask.resume();
}
