#include <iostream>
#include <stdio.h>
#include "core/test.h"
// #include <GLFW/glfw3.h>

using namespace std;

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

int main() {
    std::cout << "Result of add(3, 5): " << add(3, 5) << std::endl;
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

    // glfwTerminate();
    return 0;
}
