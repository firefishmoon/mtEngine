#include "application.h"
#include "loggersystem.h"
#include "eventsystem.h"
#include "memorysystem.h"
#include "jobsystem.h"

#include "../render/rendersystem.h"
#define GLFW_INCLUDE_VULKAN
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

template<> MT_API mtApplication* Singleton<mtApplication>::_instance = nullptr;

b8 mtApplication::initialize() {
    mtLoggerSystem::instance();
    mtLoggerSystem::getInstance()->initialize();
    
    mtMemorySystem::instance();
    mtMemorySystem::getInstance()->initialize();
    mtJobSystem::instance(4);
    mtJobSystem::getInstance()->initialize();

    mtEventSystem::instance();
    mtEventSystem::getInstance()->instance();

    mtRenderSettings renderSettings {mtBackendAPI::VULKAN};
    mtRenderSystem::instance(renderSettings);

    
    MT_LOG_INFO("Application Initialized");
    return true;
}

b8 mtApplication::shutdown() {
    mtEventSystem::getInstance()->shutdown();
    mtJobSystem::getInstance()->shutdown();
    mtMemorySystem::getInstance()->shutdown();
    mtLoggerSystem::getInstance()->shutdown();
 
    MT_LOG_INFO("Application Shutdown");
    return true;
}

static void key_callback(GLFWwindow *win, int key, int sc, int act, int mods) {
    mtEventType eventType;
    if (act == GLFW_PRESS)
        eventType = mtEventType::KEYBOARD_PRESS;
    else if (act == GLFW_RELEASE)
        eventType = mtEventType::KEYBOARD_RELEASE;
    else // GLFW_REPEAT
        eventType = mtEventType::KEYBOARD_REPEAT;

    if (key == GLFW_KEY_ESCAPE && act == GLFW_PRESS)
        glfwSetWindowShouldClose(win, GLFW_TRUE);
    else {
        MT_LOG_DEBUG("Key event: key={}, scancode={}, action={}, mods={}", key, sc, act, mods);
        mtEventSystem::getInstance()->emitEvent({eventType, static_cast<u32>(key)});
    }
}

static void window_size_callback(GLFWwindow* window, int width, int height) {
    MT_LOG_DEBUG("Window resized: width={}, height={}", width, height);
    mtEventSystem::getInstance()->emitEvent({mtEventType::WINDOW_RESIZE, static_cast<u32>(width), static_cast<u32>(height)});
}

void mtApplication::run() {
    const double FPS = 60.0;
    const double FRAME_DT = 1.0 / FPS;
    double lastTime = 0.0;
    double accumulator = 0.0;
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(
        _config.width, 
        _config.height, 
        _config.title.c_str(), 
        NULL, 
        NULL);
    if (window == NULL) {
        MT_LOG_FATAL("Failed to create GLFW window");
        glfwTerminate();
        return;
    }

    _platformData.hwnd = glfwGetWin32Window(window);
    _platformData.hInstance = GetModuleHandle(NULL);
    if (!mtRenderSystem::getInstance()->initialize()) {
        MT_LOG_FATAL("Failed to initialize Render System");
        return;
    }
    
    glfwSetKeyCallback(window, key_callback);
    glfwSetWindowSizeCallback(window, window_size_callback);
    lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - lastTime;
        lastTime = currentTime;
        glfwWaitEventsTimeout(FRAME_DT);
        accumulator += deltaTime;
        while (accumulator >= FRAME_DT) {
            mtEventSystem::getInstance()->emitEvent({mtEventType::FRAME, static_cast<f32>(FRAME_DT)});
            accumulator -= FRAME_DT;
        }
        glfwSwapBuffers(window);
    }
}
