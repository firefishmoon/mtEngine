#pragma once

#include <functional>
#include "../defines.h"
#include "../common/singleton.h"
#include "core/std_wrapper.h"

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
    explicit mtJobSystem(u32 numThreads) : _numThreads(numThreads) {};
    ~mtJobSystem() = default;

    b8 initialize() override;
    b8 shutdown() override;

    void addJob(mtJob& job);
    void runJobs();
private:
    mtQueue<mtJob> _jobQueue; 
    u32 _numThreads;
};
