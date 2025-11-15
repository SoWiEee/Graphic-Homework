#include "Application.h"
#include "UIManager.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <stdexcept>
#include <string>

// --- Application Public ---

void Application::run() {
    try {
        init();
        mainLoop();
        cleanup();
    }
    catch (const std::exception& e) {
        std::cerr << "An unrecoverable error occurred: " << e.what() << std::endl;
        if (m_Window) {
            glfwDestroyWindow(m_Window);
        }
        glfwTerminate();
    }
}

// --- Application Private ---

void Application::init() {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    std::string windowTitle = "S11259043";
    m_Window = glfwCreateWindow(m_WindowWidth, m_WindowHeight, windowTitle.c_str(), NULL, NULL);
    if (!m_Window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(m_Window);
    glfwSetWindowUserPointer(m_Window, this); // 讓我們能在 C-style callback 中取得 'this'

    // set callbacks
    glfwSetKeyCallback(m_Window, keyCallback);
    glfwSetFramebufferSizeCallback(m_Window, framebufferSizeCallback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        throw std::runtime_error("Failed to initialize GLAD");
    }

    glViewport(0, 0, m_WindowWidth, m_WindowHeight);
    glEnable(GL_DEPTH_TEST); // 深度測試
    glEnable(GL_CULL_FACE);  // 剔除背面

    m_UIManager.init(m_Window);

    try {
        m_Shader.load("gasket.vert", "gasket.frag");
    }
    catch (const std::exception& e) {
        throw std::runtime_error(std::string("Shader load error: ") + e.what());
    }

    m_Gasket.init();

    // Z=2 -> (0,0,0)
    // 並稍微向上移動 (Y 軸)，俯視角
    m_Camera.setPosition(glm::vec3(0.0f, 0.0f, 2.0f));
    m_Camera.setTarget(glm::vec3(0.0f, 0.0f, 0.0f));

    // init state
    m_SubdivisionLevel = 0;
    m_LevelChanged = true;
}

void Application::mainLoop() {
    while (!glfwWindowShouldClose(m_Window)) {
		// unput handling
        glfwPollEvents();

        m_UIManager.beginFrame();

        // draw UI
        int previousLevel = m_SubdivisionLevel;
        m_UIManager.drawContextMenu(m_SubdivisionLevel);

        // Level changed
        if (m_SubdivisionLevel != previousLevel) {
            m_LevelChanged = true;
        }

        // Geometry update
        if (m_LevelChanged) {
            m_Gasket.generate(m_SubdivisionLevel);
            m_LevelChanged = false; // reset flag
        }

        // Rendering
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        m_Shader.use();

        // MVP
        glm::mat4 projection = m_Camera.getProjectionMatrix((float)m_WindowWidth / (float)m_WindowHeight);
        glm::mat4 view = m_Camera.getViewMatrix();
        glm::mat4 model = glm::mat4(1.0f); // 單位矩陣

        m_Shader.setMat4("u_MVP", projection * view * model);

        // draw 3D gasket
        m_Gasket.draw();

        // draw ImGui
        m_UIManager.endFrame();

        glfwSwapBuffers(m_Window);
    }
}

void Application::cleanup()
{
    m_UIManager.cleanup();
    m_Gasket.cleanup();
    m_Shader.cleanup();

    if (m_Window) {
        glfwDestroyWindow(m_Window);
    }
    glfwTerminate();
}

// Static Callbacks
void Application::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (!app) return;

    // 'q' or 'Q' to exit
    if ((key == GLFW_KEY_Q && action == GLFW_PRESS)) {
        glfwSetWindowShouldClose(window, true);
    }
}

void Application::framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app) {
        glViewport(0, 0, width, height);
        app->m_WindowWidth = width;
        app->m_WindowHeight = height;
    }
}