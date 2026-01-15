#include "application.h"
#include "loggersystem.h"
#include "eventsystem.h"
#include "memorysystem.h"
#include "jobsystem.h"

#include <GLFW/glfw3.h>

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

void mtApplication::run() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
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
    glfwSetKeyCallback(window, key_callback);
    while (!glfwWindowShouldClose(window)) {
        glfwSwapBuffers(window);
        glfwWaitEventsTimeout(1.0/60.0); 
    }
}
