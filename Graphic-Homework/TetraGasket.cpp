#include "TetraGasket.h"

static const float sqrt6_4 = glm::sqrt(6.0f) / 4.0f;
static const float sqrt3_3 = glm::sqrt(3.0f) / 3.0f;
static const float sqrt6_12 = glm::sqrt(6.0f) / 12.0f;
static const float n_sqrt3_6 = -glm::sqrt(3.0f) / 6.0f;

static const glm::vec3 baseVertices[4] = {
    glm::vec3(0.0f, 0.0f, sqrt6_4),                 // v[0]
    glm::vec3(0.0f, sqrt3_3, sqrt6_12),             // v[1]
    glm::vec3(-0.5f, n_sqrt3_6, sqrt6_12),          // v[2]
    glm::vec3(0.5f, n_sqrt3_6, sqrt6_12)            // v[3]
};

static const glm::vec3 faceColors[4] = {
    glm::vec3(1.0f, 0.0f, 0.0f), // Red
    glm::vec3(0.0, 1.0, 0.0),    // Green
    glm::vec3(0.0f, 0.0f, 1.0f), // Blue
    glm::vec3(0.0f, 0.0f, 0.0f)  // Black
};

void TetraGasket::init()
{
    glCreateVertexArrays(1, &m_VAO);
    glCreateBuffers(1, &m_VBO_Position);
    glCreateBuffers(1, &m_VBO_Color);

    glEnableVertexArrayAttrib(m_VAO, 0);
    glVertexArrayAttribFormat(m_VAO, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(m_VAO, 0, 0);

    glEnableVertexArrayAttrib(m_VAO, 1);
    glVertexArrayAttribFormat(m_VAO, 1, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(m_VAO, 1, 1);

    glVertexArrayVertexBuffer(m_VAO, 0, m_VBO_Position, 0, sizeof(glm::vec3));
    glVertexArrayVertexBuffer(m_VAO, 1, m_VBO_Color, 0, sizeof(glm::vec3));
}

// --- 這是新的 Generate ---
void TetraGasket::generate(int level)
{
    m_Positions.clear();
    m_Colors.clear();

    // 呼叫遞迴函式
    dividePyramid(baseVertices[0], baseVertices[1], baseVertices[2], baseVertices[3], level);

    m_VertexCount = m_Positions.size();

    if (m_VertexCount > 0)
    {
        glNamedBufferData(m_VBO_Position, m_VertexCount * sizeof(glm::vec3), m_Positions.data(), GL_DYNAMIC_DRAW);
        glNamedBufferData(m_VBO_Color, m_VertexCount * sizeof(glm::vec3), m_Colors.data(), GL_DYNAMIC_DRAW);
    }
}

// --- 這是新的遞迴函式 ---
void TetraGasket::dividePyramid(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, const glm::vec3& v4, int level)
{
    if (level == 0) {
        // 遞迴終點：繪製這個小四面體
        drawTetra(v1, v2, v3, v4);
    }
    else
    {
        // 計算 6 個邊的中點
        glm::vec3 m12 = 0.5f * (v1 + v2);
        glm::vec3 m13 = 0.5f * (v1 + v3);
        glm::vec3 m14 = 0.5f * (v1 + v4);
        glm::vec3 m23 = 0.5f * (v2 + v3);
        glm::vec3 m24 = 0.5f * (v2 + v4);
        glm::vec3 m34 = 0.5f * (v3 + v4);

        // 遞迴呼叫 4 個角落的小四面體
        // (這移除了中間的八面體)
        dividePyramid(v1, m12, m13, m14, level - 1);
        dividePyramid(m12, v2, m23, m24, level - 1);
        dividePyramid(m13, m23, v3, m34, level - 1);
        dividePyramid(m14, m24, m34, v4, level - 1);
    }
}

void TetraGasket::drawTetra(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, const glm::vec3& v4)
{
    addTriangle(v1, v2, v3, faceColors[0]); // Red
    addTriangle(v4, v3, v2, faceColors[3]); // Black
    addTriangle(v1, v4, v2, faceColors[2]); // Blue
    addTriangle(v1, v3, v4, faceColors[1]); // Green
}

void TetraGasket::addTriangle(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, const glm::vec3& color)
{
    m_Positions.push_back(p1);
    m_Positions.push_back(p2);
    m_Positions.push_back(p3);

    m_Colors.push_back(color);
    m_Colors.push_back(color);
    m_Colors.push_back(color);
}

void TetraGasket::draw()
{
    if (m_VertexCount > 0)
    {
        glBindVertexArray(m_VAO);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)m_VertexCount);
        glBindVertexArray(0);
    }
}

void TetraGasket::cleanup()
{
    if (m_VBO_Color != 0) glDeleteBuffers(1, &m_VBO_Color);
    if (m_VBO_Position != 0) glDeleteBuffers(1, &m_VBO_Position);
    if (m_VAO != 0) glDeleteVertexArrays(1, &m_VAO);
}