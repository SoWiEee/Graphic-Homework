#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Camera.h"
#include "Shader.h"
#include "TetraGasket.h"
#include "UIManager.h"

class Application {
public:
    void run();

private:
    void init();
    void mainLoop();
    void cleanup();

    // static Callbacks
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);

    GLFWwindow* m_Window = nullptr;
    UIManager m_UIManager;
    Camera m_Camera;
    Shader m_Shader;
    TetraGasket m_Gasket;

    // 狀態
    int WindowWidth = 1280;
    int WindowHeight = 720;
    int SubdivisionLevel = 0; // 初始 subdivision level = 0
    bool LevelChanged = true; // 標記 level 是否改變，以便重新產生
};