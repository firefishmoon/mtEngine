#pragma once

#include <functional>
#include <queue>
#include "../defines.h"
#include "singleton.h"

class MT_API mtJob {
public:
    mtJob(std::function<void(void*)> func, void* param)
    : _func(func), _param(param) {}

    void run() {
        _func(_param);
    }
private:
    std::function<void(void*)> _func;
    void* _param;

};

class MT_API mtJobSystem : public Singleton<mtJobSystem> {
public:
    mtJobSystem() = default;
    ~mtJobSystem() = default;

    void initialize(unsigned int numThreads);
    void shutdown();

    void addJob(mtJob& job);
    void runJobs();
private:
    std::queue<mtJob> _jobQueue; 
};
