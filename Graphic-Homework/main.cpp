#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <iomanip>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

GLuint shaderProgram = 0;
GLuint VAO = 0, VBO_positions = 0, VBO_colors = 0;

std::vector<glm::vec3> g_positions;
std::vector<glm::vec3> g_colors;

// 變換
glm::mat4 model_matrix(1.0f);
glm::mat4 view_matrix(1.0f);
glm::mat4 projection_matrix(1.0f);
GLint modelLoc = -1, viewLoc = -1, projLoc = -1;

// 細分層級
int g_subdivision_level = 0;
int g_last_submitted_level = -1; // 用來偵測變化

// 滑鼠互動
bool g_is_dragging = false;
double g_last_mouse_x = 0.0;
double g_last_mouse_y = 0.0;
float g_rotate_x = 0.0f;
float g_rotate_y = 0.0f;

// 四面體 (Tetrahedron) 的 4 個初始頂點
glm::vec3 initial_points[4] = {
    glm::vec3(0.0f,  0.8165f, 0.0f),
    glm::vec3(-0.8165f, -0.4082f, 0.4714f),
    glm::vec3(0.8165f, -0.4082f, 0.4714f),
    glm::vec3(0.0f,  -0.4082f, -0.9428f)
};

// 4 個面的基礎顏色
glm::vec3 base_colors[4] = {
    glm::vec3(1.0f, 0.0f, 0.0f), // 紅
    glm::vec3(0.0f, 1.0f, 0.0f), // 綠
    glm::vec3(0.0f, 0.0f, 1.0f), // 藍
    glm::vec3(1.0f, 1.0f, 0.0f)  // 黃
};

const char* vertexShaderSource = R"glsl(
    #version 450 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aColor;
    out vec3 ourColor;
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    void main() {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
        ourColor = aColor;
    }
)glsl";

const char* fragmentShaderSource = R"glsl(
    #version 450 core
    in vec3 ourColor;
    out vec4 FragColor;
    void main() {
        FragColor = vec4(ourColor, 1.0);
    }
)glsl";


// --- 輔助函式 ---
void checkShaderError(GLuint shader, std::string type) {
    GLint success;
    GLchar infoLog[1024];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) { glGetShaderInfoLog(shader, 1024, NULL, infoLog); std::cerr << "Shader COMPILE error (" << type << "):\n" << infoLog << std::endl; }
    }
    else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) { glGetProgramInfoLog(shader, 1024, NULL, infoLog); std::cerr << "Shader LINK error (" << type << "):\n" << infoLog << std::endl; }
    }
}

void initShaders() {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    checkShaderError(vertexShader, "VERTEX");

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    checkShaderError(fragmentShader, "FRAGMENT");

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    checkShaderError(shaderProgram, "PROGRAM");

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    modelLoc = glGetUniformLocation(shaderProgram, "model");
    viewLoc = glGetUniformLocation(shaderProgram, "view");
    projLoc = glGetUniformLocation(shaderProgram, "projection");
}

// --- 3D Gasket 遞迴函式 ---
void add_triangle(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 color) {
    g_positions.push_back(a);
    g_positions.push_back(b);
    g_positions.push_back(c);
    g_colors.push_back(color);
    g_colors.push_back(color);
    g_colors.push_back(color);
}

void draw_tetrahedron(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d) {
    add_triangle(a, b, c, base_colors[0]);
    add_triangle(a, c, d, base_colors[1]);
    add_triangle(a, d, b, base_colors[2]);
    add_triangle(b, d, c, base_colors[3]);
}

glm::vec3 midpoint(glm::vec3 p1, glm::vec3 p2) {
    return (p1 + p2) * 0.5f;
}

void divide_tetrahedron(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, int level) {
    if (level == 0) {
        draw_tetrahedron(a, b, c, d);
        return;
    }
    glm::vec3 m_ab = midpoint(a, b);
    glm::vec3 m_ac = midpoint(a, c);
    glm::vec3 m_ad = midpoint(a, d);
    glm::vec3 m_bc = midpoint(b, c);
    glm::vec3 m_bd = midpoint(b, d);
    glm::vec3 m_cd = midpoint(c, d);

    divide_tetrahedron(a, m_ab, m_ac, m_ad, level - 1);
    divide_tetrahedron(m_ab, b, m_bc, m_bd, level - 1);
    divide_tetrahedron(m_ac, m_bc, c, m_cd, level - 1);
    divide_tetrahedron(m_ad, m_bd, m_cd, d, level - 1);
}

// --- 資料更新函式 ---
void updateGasketData() {
    g_positions.clear();
    g_colors.clear();
    std::cout << "[*] Generating Level " << g_subdivision_level << " Gasket..." << std::endl;
    divide_tetrahedron(
        initial_points[0], initial_points[1],
        initial_points[2], initial_points[3],
        g_subdivision_level
    );
    std::cout << "[*] Generation completed, total vertices: " << g_positions.size() << std::endl;

    // 上傳新資料到 GPU
    glBindVertexArray(VAO); // 確保 VAO 是綁定的
    glBindBuffer(GL_ARRAY_BUFFER, VBO_positions);
    glBufferData(GL_ARRAY_BUFFER, g_positions.size() * sizeof(glm::vec3), g_positions.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_colors);
    glBufferData(GL_ARRAY_BUFFER, g_colors.size() * sizeof(glm::vec3), g_colors.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    g_last_submitted_level = g_subdivision_level;
}


// --- [新] ImGui 選單繪製函式 ---
void drawImGuiPopup(GLFWwindow* window) {
    // 點擊右鍵彈出選單
    if (ImGui::BeginPopupContextWindow("HomeworkMenu"))
    {
        if (ImGui::BeginMenu("Subdivision Level"))
        {
            // 幫我們建立 4 個選項
            for (int i = 0; i <= 3; ++i) {
                // (label, shortcut, is_selected, is_enabled)
                if (ImGui::MenuItem(("Level " + std::to_string(i)).c_str(), "", g_subdivision_level == i, true)) {
                    g_subdivision_level = i; // 更新層級
                }
            }
            ImGui::EndMenu();
        }

        ImGui::Separator(); // 分隔線

        // 符合作業要求: "Exit"
        if (ImGui::MenuItem("Exit (or press 'Q')")) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        ImGui::EndPopup();
    }
}


// GLFW Callbacks

void error_callback(int error, const char* description) {
    std::cerr << "GLFW 錯誤: " << description << std::endl;
}

// 符合作業要求: 'q' 或 'Q' 退出
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // 檢查 GUI 是否正在使用鍵盤 (例如在文字框中打字)
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureKeyboard) {
        return; // ImGui 正在用，我們就不要處理
    }

    if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
        std::cout << "[*] User requested exit..." << std::endl;
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    if (height == 0) height = 1;
    float aspect = (float)width / (float)height;
    glViewport(0, 0, width, height);
    projection_matrix = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
}

// 處理滑鼠點擊 (用於旋轉)
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    // 檢查 GUI 是否正在使用滑鼠
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) {
        return; // ImGui 正在用 (例如點擊選單)，我們就不要處理
    }

    // 處理場景拖曳
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        g_is_dragging = true;
        glfwGetCursorPos(window, &g_last_mouse_x, &g_last_mouse_y);
    }
    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        g_is_dragging = false;
    }
}

// 處理滑鼠拖曳 (用於旋轉)
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    if (g_is_dragging) {
        // 再次檢查 ImGui (例如拖曳選單)
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse) {
            g_is_dragging = false;
            return;
        }

        float deltaX = (float)(xpos - g_last_mouse_x);
        float deltaY = (float)(ypos - g_last_mouse_y);

        g_rotate_y += deltaX * 0.5f;
        g_rotate_x += deltaY * 0.5f;

        g_last_mouse_x = xpos;
        g_last_mouse_y = ypos;
    }
}

// ImGui 需要滾輪回呼
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    // ImGui 的 'Init' 函式會自動處理這個
    // 我們不需要做任何事
}


void initGL() {
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        throw std::runtime_error("GLAD 初始化失敗");
    }
    std::cout << "[*] OpenGL Version: " << glGetString(GL_VERSION) << std::endl;

    // 2. 設定 OpenGL 狀態
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // 深灰色背景
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND); // ImGui 需要
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // ImGui 需要

    // 3. 編譯並連結著色器
    initShaders();

    // 4. 建立 VAO 和 VBOs
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO_positions);
    glGenBuffers(1, &VBO_colors);

    // 5. 設定 VAO (Vertex Array Object)
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_positions);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_colors);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // 6. 第一次產生資料 (Level 0)
    updateGasketData();
}

int main(int argc, char** argv) {
    // init GLFW
    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        std::cerr << "[X] GLFW init failed" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "S11259043", NULL, NULL);
    if (!window) {
        std::cerr << "[X] GLFW window create failed" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // 啟用 VSync

    try {
        initGL();
    }
    catch (const std::exception& e) {
        std::cerr << "[X] Fatal error: " << e.what() << std::endl;
        glfwTerminate();
        return -1;
    }

    // register callbacks
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // --- 初始化 Dear ImGui ---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark(); // theme
    // [關鍵] 綁定 GLFW 和 OpenGL，並 "安裝回呼"
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 450 core");


    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    framebuffer_size_callback(window, width, height);

    // main loop
    std::cout << "程式啟動. 按 'q' 退出, 或按右鍵開啟選單." << std::endl;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // --- ImGui: New Frame ---
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // --- 繪製你的彈出式選單 ---
        drawImGuiPopup(window);

        // (檢查選單是否改變了層級)
        if (g_subdivision_level != g_last_submitted_level) {
            updateGasketData();
        }

        // --- 繪製 3D 場景 ---
        glfwGetFramebufferSize(window, &width, &height); // 取得當前大小
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shaderProgram);

        // update MVP
        view_matrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f));

        g_rotate_y += 0.1f; // 自動繞 Y 軸旋轉

        model_matrix = glm::mat4(1.0f);
        model_matrix = glm::rotate(model_matrix, glm::radians(g_rotate_x), glm::vec3(1.0f, 0.0f, 0.0f));
        model_matrix = glm::rotate(model_matrix, glm::radians(g_rotate_y), glm::vec3(0.0f, 1.0f, 0.0f));

        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view_matrix));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model_matrix));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection_matrix));

        // bind VAO and draw
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(g_positions.size()));
        glBindVertexArray(0);

        // --- ImGui Render ---
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO_positions) glDeleteBuffers(1, &VBO_positions);
    if (VBO_colors) glDeleteBuffers(1, &VBO_colors);
    if (shaderProgram) glDeleteProgram(shaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}