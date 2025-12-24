#pragma once

#include <functional>

class Job {
public:
    Job(std::function<void(void*)> func, void* param)
    : _func(func), _param(param) {}

    void Run() {
        _func(_param);
    }
private:
    std::function<void(void*)> _func;
    void* _param;

};

