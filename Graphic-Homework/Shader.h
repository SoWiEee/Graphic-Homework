#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

class Shader
{
public:
    GLuint ID = 0; // 程式 ID

    Shader() = default;

    // 載入、編譯並連結
    void load(const char* vertexPath, const char* fragmentPath);

    // 啟用 Shader
    void use();

    // 清理
    void cleanup();

    // Uniform 輔助函式
    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setMat4(const std::string& name, const glm::mat4& mat) const;

private:
    // 檢查編譯/連結錯誤
    void checkCompileErrors(GLuint shader, std::string type);
};