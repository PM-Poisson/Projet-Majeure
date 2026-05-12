#include "map.hpp"
#include <glad/glad.h>
#include <algorithm>
#include <stdexcept>
#include <cmath>

Heightmap::Heightmap(int width, int height)
    : m_width(width), m_height(height), m_data(width * height, 0.0f)
{
    // Cree la texture GPU R32F (format float simple precision)
    glGenTextures(1, &m_textureID);
    glBindTexture(GL_TEXTURE_2D, m_textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F,
                 m_width, m_height, 0,
                 GL_RED, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
}

Heightmap::~Heightmap() {
    if (m_textureID) glDeleteTextures(1, &m_textureID);
}

// ---------------------------------------------------------------------------
// Acces CPU
// ---------------------------------------------------------------------------

float Heightmap::get(int x, int z) const {
    x = std::clamp(x, 0, m_width  - 1);
    z = std::clamp(z, 0, m_height - 1);
    return m_data[idx(x, z)];
}

void Heightmap::set(int x, int z, float value) {
    if (x < 0 || x >= m_width || z < 0 || z >= m_height) return;
    m_data[idx(x, z)] = value;
}

float Heightmap::getInterpolated(float x, float z) const {
    int   x0 = (int)std::floor(x),  z0 = (int)std::floor(z);
    int   x1 = x0 + 1,              z1 = z0 + 1;
    float tx  = x - (float)x0,      tz = z - (float)z0;

    float h00 = get(x0, z0), h10 = get(x1, z0);
    float h01 = get(x0, z1), h11 = get(x1, z1);

    // Interpolation bilineaire
    float h0 = h00 + tx * (h10 - h00);
    float h1 = h01 + tx * (h11 - h01);
    return h0 + tz * (h1 - h0);
}

// ---------------------------------------------------------------------------
// Calcul des normales (differences finies centrales)
// ---------------------------------------------------------------------------

glm::vec3 Heightmap::getNormal(int x, int z) const {
    float scale = 1.0f;   // echelle horizontale du terrain en world units
    float hL = get(x - 1, z);
    float hR = get(x + 1, z);
    float hD = get(x, z - 1);
    float hU = get(x, z + 1);

    glm::vec3 n(
        (hL - hR) / (2.0f * scale),
        1.0f,
        (hD - hU) / (2.0f * scale)
    );
    return glm::normalize(n);
}

// ---------------------------------------------------------------------------
// Synchronisation GPU
// ---------------------------------------------------------------------------

void Heightmap::uploadToGPU() {
    glBindTexture(GL_TEXTURE_2D, m_textureID);
    glTexSubImage2D(GL_TEXTURE_2D, 0,
                    0, 0, m_width, m_height,
                    GL_RED, GL_FLOAT, m_data.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Heightmap::downloadFromGPU() {
    glBindTexture(GL_TEXTURE_2D, m_textureID);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, m_data.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

// ---------------------------------------------------------------------------
// Classification roche / mer
// ---------------------------------------------------------------------------

bool Heightmap::isRock(int x, int z, float seaLevel) const {
    return get(x, z) > seaLevel;
}