#pragma once
#include <GLFW/glfw3.h>

class UIManager
{
public:
    void init(GLFWwindow* window);
    void beginFrame();
    void endFrame();
    void cleanup();

    // 我們的 UI 面板
    void drawContextMenu(int& subdivisionLevel);
};