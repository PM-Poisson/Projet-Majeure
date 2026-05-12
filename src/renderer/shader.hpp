#pragma once
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

// Wrapper autour d'un programme GLSL (vertex + fragment, ou compute)
// Charge les sources depuis des fichiers, compile et linke automatiquement.

class Shader {
public:
    Shader() = default;
    ~Shader();

    // Interdit la copie
    Shader(const Shader&)            = delete;
    Shader& operator=(const Shader&) = delete;

    // Charge et compile un programme vertex + fragment
    bool load(const std::string& vertPath, const std::string& fragPath);

    // Charge et compile un compute shader (erosion GPU)
    bool loadCompute(const std::string& compPath);

    void use() const;
    void unuse() const;

    unsigned int id() const { return m_programID; }

    // Setters d'uniforms
    void setBool  (const std::string& name, bool  value) const;
    void setInt   (const std::string& name, int   value) const;
    void setFloat (const std::string& name, float value) const;
    void setVec2  (const std::string& name, const glm::vec2& v) const;
    void setVec3  (const std::string& name, const glm::vec3& v) const;
    void setVec4  (const std::string& name, const glm::vec4& v) const;
    void setMat4  (const std::string& name, const glm::mat4& m) const;

private:
    unsigned int m_programID = 0;

    std::string  readFile(const std::string& path) const;
    unsigned int compileShader(unsigned int type, const std::string& source) const;
    bool         linkProgram(unsigned int vert, unsigned int frag);
    bool         linkComputeProgram(unsigned int comp);
    int          uniformLocation(const std::string& name) const;
};