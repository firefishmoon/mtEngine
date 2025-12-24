#include <iostream>
#include <stdio.h>
#include <coroutine>
#include "core/test.h"
#include <thread>
#include <chrono>

#include "core/singleton.h"
#include "core/jobsystem.h"
// #include <GLFW/glfw3.h>

using namespace std;

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

struct TimerAwaiter {
    std::chrono::milliseconds _duration;

    TimerAwaiter(std::chrono::milliseconds duration) : _duration(duration) {}

    bool await_ready() const noexcept { return false; }

    void await_suspend(std::coroutine_handle<> handle) {
        std::thread([handle, this]() {
            std::cout << "before task" << std::endl;
            std::this_thread::sleep_for(_duration);
            std::cout << "after task" << std::endl;
            handle.resume();
        }).detach();
    }

    void await_resume() noexcept {}
};

struct Task {
    struct promise_type {
        Task get_return_object() {
            return Task{};
        }
        std::suspend_never initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };
};

Task TaskFunc() {
    co_await TimerAwaiter{std::chrono::seconds(1)};
    std::cout << "after TaskFunc" << std::endl;
}

void test_job() {
    Job job = Job {[](void*) {
        std::cout << "RunJob" << std::endl;
        for (int i = 0; i < 5; ++i) {
            Job innerJob = Job {[](void* param) {
                int index = *static_cast<int*>(param);
                std::cout << "  Inner Job " << index << " running." << std::endl;
            }, new int(i)};
            JobSystem::instance()->addJob(innerJob);
        }
    }, NULL};
    // job.Run();
    JobSystem::instance()->initialize(4);
    JobSystem::instance()->addJob(job);
    JobSystem::instance()->runJobs();
}

int main() {
    std::cout << "Result of add(3, 5): " << add(3, 5) << std::endl;

    test_job();
    // TaskFunc();
    // glfwInit();
    // glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    // glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // printf("%s\n", "whatever");
    // cout << "hello cpp!" << endl;

    // GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    // if (window == NULL) {
    //     std::cout << "Failed to create GLFW window" << std::endl;
    //     glfwTerminate();
    //     return -1;
    // }

    // while (!glfwWindowShouldClose(window)) {
    //     glfwPollEvents();
    //     glfwSwapBuffers(window);
    // }
    std:this_thread::sleep_for(std::chrono::seconds(2));
    // glfwTerminate();
    return 0;
}
