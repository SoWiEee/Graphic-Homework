#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"


GLuint shaderProgram;
GLuint VAO, VBO_positions, VBO_colors;
std::vector<glm::vec3> g_positions;
std::vector<glm::vec3> g_colors;

// MVP
glm::mat4 model_matrix(1.0f);
glm::mat4 view_matrix(1.0f);
glm::mat4 projection_matrix(1.0f);
GLint modelLoc, viewLoc, projLoc;

int g_subdivision_level = 0;

// mouse
bool g_is_dragging = false;
double g_last_mouse_x = 0.0;
double g_last_mouse_y = 0.0;
float g_rotate_x = 0.0f;
float g_rotate_y = 0.0f;

// vertex
glm::vec3 initial_points[4] = {
    glm::vec3(0.0f,  0.8165f, 0.0f),
    glm::vec3(-0.8165f, -0.4082f, 0.4714f),
    glm::vec3( 0.8165f, -0.4082f, 0.4714f),
    glm::vec3( 0.0f,  -0.4082f, -0.9428f)
};

glm::vec3 base_colors[4] = {
    glm::vec3(1.0f, 0.0f, 0.0f), // red
    glm::vec3(0.0f, 1.0f, 0.0f), // green
    glm::vec3(0.0f, 0.0f, 1.0f), // blue
    glm::vec3(1.0f, 1.0f, 0.0f)  // yellow
};


// shader
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


void checkShaderError(GLuint shader, std::string type) {
    GLint success;
    GLchar infoLog[1024];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "[X] Shader compile error (" << type << ")\n" << infoLog << std::endl;
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "[X] Shader link error (" << type << ")\n" << infoLog << std::endl;
        }
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

// --- 3D Gasket 遞迴函式 (和之前相同) ---
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

    divide_tetrahedron(a,    m_ab, m_ac, m_ad, level - 1);
    divide_tetrahedron(m_ab, b,    m_bc, m_bd, level - 1);
    divide_tetrahedron(m_ac, m_bc, c,    m_cd, level - 1);
    divide_tetrahedron(m_ad, m_bd, m_cd, d,    level - 1);
}

void updateGasketData() {
    g_positions.clear();
    g_colors.clear();

    std::cout << "Generate Level " << g_subdivision_level << " 的 Gasket..." << std::endl;
    divide_tetrahedron(
        initial_points[0], initial_points[1],
        initial_points[2], initial_points[3],
        g_subdivision_level
    );
    std::cout << "[*] Generation completed, total vertex: " << g_positions.size() << std::endl;

    // 上傳新資料到 GPU
    glBindBuffer(GL_ARRAY_BUFFER, VBO_positions);
    glBufferData(GL_ARRAY_BUFFER, g_positions.size() * sizeof(glm::vec3), g_positions.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_colors);
    glBufferData(GL_ARRAY_BUFFER, g_colors.size() * sizeof(glm::vec3), g_colors.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// error callback
void error_callback(int error, const char* description) {
    std::cerr << "GLFW 錯誤: " << description << std::endl;
}

// keyboard callback
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // 讓 ImGui 優先處理
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);

    // 如果 ImGui 不想用鍵盤，我們才處理
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureKeyboard) {
        return;
    }

    // 符合作業要求: 'q' 或 'Q' 退出
    if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
        std::cout << "使用者要求退出..." << std::endl;
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

// 視窗重塑回呼
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    if (height == 0) height = 1;
    float aspect = (float)width / (float)height;

    glViewport(0, 0, width, height);

    projection_matrix = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
}

// mouse click ballback
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    // 讓 ImGui 優先處理
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);

    // 如果 ImGui 不想用滑鼠，我們才處理
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) {
        return;
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        g_is_dragging = true;
        glfwGetCursorPos(window, &g_last_mouse_x, &g_last_mouse_y);
    } else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        g_is_dragging = false;
    }
}

// mouse move ballback
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    // 讓 ImGui 優先處理
    ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);

    if (g_is_dragging) {
        // 如果 ImGui 正在使用滑鼠 (例如拖動選單)，我們就不旋轉
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse) {
            g_is_dragging = false; // 停止拖曳
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

// ImGui 滾輪回呼
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
}


// --- 繪製 ImGui 選單 ---
void drawImGuiMenu() {
    // 在視窗的任何地方點擊右鍵，都會觸發這個彈出式選單
    // (ImGuiPopupFlags_MouseButtonRight)
    if (ImGui::BeginPopupContextWindow("HomeworkMenu"))
    {
        // 這是選單的主體
        // 符合作業要求: "Subdivision Level" 子選單
        if (ImGui::BeginMenu("Subdivision Level"))
        {
            // 幫我們建立 4 個選項
            for (int i = 0; i <= 3; ++i) {
                if (ImGui::MenuItem(("Level " + std::to_string(i)).c_str(), "", g_subdivision_level == i)) {
                    if (g_subdivision_level != i) {
                        g_subdivision_level = i;
                        updateGasketData(); // 重新產生 VBO
                    }
                }
            }
            ImGui::EndMenu();
        }

        ImGui::Separator(); // 分隔線

        // 符合作業要求: "Exit"
        if (ImGui::MenuItem("Exit (or press 'Q')")) {
            // (我們需要視窗指標來關閉)
            GLFWwindow* window = glfwGetCurrentContext();
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        ImGui::EndPopup();
    }
}

void initGL() {
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    // 2. 設定 OpenGL 狀態
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // 3. 編譯並連結著色器
    initShaders();

    // 4. 建立 VAO 和 VBOs
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO_positions);
    glGenBuffers(1, &VBO_colors);

    // 5. 設定 VAO (和之前相同)
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
    // 1. 初始化 GLFW
    glfwSetErrorCallback(error_callback);
    glfwInit();

    // 2. 設定 OpenGL 4.4 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Mac 需要
#endif

    // 3. 建立視窗 (並設定標題為學號)
    GLFWwindow* window = glfwCreateWindow(800, 600, "123456789", NULL, NULL); // <--- 請改成你的學號
    if (window == NULL) {
        std::cerr << "GLFW 視窗建立失敗" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // 啟用垂直同步

    // 4. 初始化 OpenGL (GLEW) 和我們的著色器/VBO
    try {
        initGL();
    } catch (const std::exception& e) {
        std::cerr << "發生嚴重錯誤: " << e.what() << std::endl;
        glfwTerminate();
        return -1;
    }

    // 5. 註冊 GLFW 回呼函式
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetScrollCallback(window, scroll_callback); // ImGui 需要

    // --- 初始化 Dear ImGui ---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark(); // 設定風格
    // 綁定 GLFW 和 OpenGL
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 440 core");

    // --- 設定視窗大小 (觸發一次 reshape) ---
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    framebuffer_size_callback(window, width, height);

    // 6. 進入主迴圈 (Game Loop)
    std::cout << "程式啟動. 按 'q' 退出, 或按右鍵開啟選單." << std::endl;
    while (!glfwWindowShouldClose(window)) {
        // (a) 處理輸入
        glfwPollEvents();

        // (b) 開始 ImGui 新的一幀
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // (c) 繪製我們的 ImGui 選單
        drawImGuiMenu();

        // (d) 繪製我們的 3D 場景
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shaderProgram);

        // 更新 MVP 矩陣
        view_matrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model_matrix = glm::mat4(1.0f);
        model_matrix = glm::rotate(model_matrix, glm::radians(g_rotate_x), glm::vec3(1.0f, 0.0f, 0.0f));
        model_matrix = glm::rotate(model_matrix, glm::radians(g_rotate_y), glm::vec3(0.0f, 1.0f, 0.0f));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view_matrix));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model_matrix));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection_matrix));

        // 繪製 Gasket
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)g_positions.size());
        glBindVertexArray(0);

        // (e) 繪製 ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // (f) 交換緩衝區
        glfwSwapBuffers(window);
    }

    // 7. 清理
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO_positions);
    glDeleteBuffers(1, &VBO_colors);
    glDeleteProgram(shaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
