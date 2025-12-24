#pragma once

#include <functional>
#include <queue>
#include "singleton.h"

class Job {
public:
    Job(std::function<void(void*)> func, void* param)
    : _func(func), _param(param) {}

    void run() {
        _func(_param);
    }
private:
    std::function<void(void*)> _func;
    void* _param;

};

class JobSystem : public Singleton<JobSystem> {
public:
    JobSystem() = default;
    ~JobSystem() = default;

    void initialize(unsigned int numThreads);
    void shutdown();

    void addJob(Job& job);
    void runJobs();
private:
    std::queue<Job> _jobQueue; 
};
