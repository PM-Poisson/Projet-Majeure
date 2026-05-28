#pragma once
#include <vector>
#include <glm/glm.hpp>

// Structure de donnees centrale : la heightmap
// Elle est stockee comme une texture GPU (GL_R32F) pour les compute shaders d'erosion,
// et comme un tableau CPU pour la generation initiale et le debug.

class Heightmap {
public:
    Heightmap(int width, int height);
    ~Heightmap();

    // Acces CPU
    float  get(int x, int z) const;
    void   set(int x, int z, float value);
    float  getInterpolated(float x, float z) const;  // bilineaire

    // Normales calculees a partir des gradients de la heightmap
    glm::vec3 getNormal(int x, int z) const;

    // Upload / download GPU <-> CPU
    void uploadToGPU();     // CPU -> texture OpenGL
    void downloadFromGPU(); // texture OpenGL -> CPU (apres erosion GPU)

    // Accesseurs
    int   width()  const { return m_width;  }
    int   height() const { return m_height; }
    unsigned int textureID() const { return m_textureID; }

    // Classifie un point : roche (> seaLevel) ou mer (< seaLevel)
    bool isRock(int x, int z, float seaLevel = 0.0f) const;

    // Retourne la heightmap sous forme d'image (pour export debug)
    std::vector<float> rawData() const { return m_data; }

private:
    int   m_width, m_height;
    std::vector<float> m_data;   // stockage CPU [row-major]
    unsigned int       m_textureID = 0;

    int idx(int x, int z) const { return z * m_width + x; }
};