#pragma once
#include <vector>

// Wrapper leger autour d'un VAO/VBO/EBO OpenGL.
// Attend des sommets au format : position(3) + normale(3) + uv(2) = 8 floats

class Mesh {
public:
    Mesh();
    ~Mesh();

    // Interdit la copie (ressources GPU)
    Mesh(const Mesh&)            = delete;
    Mesh& operator=(const Mesh&) = delete;

    // Upload des donnees vers le GPU
    void upload(const std::vector<float>&        vertices,
                const std::vector<unsigned int>& indices);

    // Dessin (appele dans la boucle de rendu)
    void draw() const;

    // Mise a jour partielle des positions (apres erosion)
    void updateVertices(const std::vector<float>& vertices);

    int indexCount() const { return m_indexCount; }
    unsigned int vao()    const { return m_vao; }

private:
    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;
    unsigned int m_ebo = 0;
    int          m_indexCount = 0;
};