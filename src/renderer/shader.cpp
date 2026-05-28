#include "shader.hpp"
#include <glad/glad.h>
#include <fstream>
#include <sstream>
#include <iostream>

// ---------------------------------------------------------------------------
// Destructeur
// ---------------------------------------------------------------------------

Shader::~Shader() {
    if (m_programID) glDeleteProgram(m_programID);
}

// ---------------------------------------------------------------------------
// Lecture de fichier
// ---------------------------------------------------------------------------

std::string Shader::readFile(const std::string& path) const {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[Shader] Impossible d'ouvrir : " << path << "\n";
        return "";
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

// ---------------------------------------------------------------------------
// Compilation d'un shader individuel
// ---------------------------------------------------------------------------

unsigned int Shader::compileShader(unsigned int type, const std::string& source) const {
    unsigned int shader = glCreateShader(type);
    const char*  src    = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        const char* typeName = (type == GL_VERTEX_SHADER)   ? "VERTEX"
                             : (type == GL_FRAGMENT_SHADER) ? "FRAGMENT"
                                                            : "COMPUTE";
        std::cerr << "[Shader] Erreur compilation " << typeName << " :\n" << log << "\n";
    }
    return shader;
}

// ---------------------------------------------------------------------------
// Link vertex + fragment
// ---------------------------------------------------------------------------

bool Shader::linkProgram(unsigned int vert, unsigned int frag) {
    m_programID = glCreateProgram();
    glAttachShader(m_programID, vert);
    glAttachShader(m_programID, frag);
    glLinkProgram(m_programID);

    int success;
    glGetProgramiv(m_programID, GL_LINK_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetProgramInfoLog(m_programID, sizeof(log), nullptr, log);
        std::cerr << "[Shader] Erreur link :\n" << log << "\n";
        return false;
    }
    glDeleteShader(vert);
    glDeleteShader(frag);
    return true;
}

// ---------------------------------------------------------------------------
// Link compute shader
// ---------------------------------------------------------------------------

bool Shader::linkComputeProgram(unsigned int comp) {
    m_programID = glCreateProgram();
    glAttachShader(m_programID, comp);
    glLinkProgram(m_programID);

    int success;
    glGetProgramiv(m_programID, GL_LINK_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetProgramInfoLog(m_programID, sizeof(log), nullptr, log);
        std::cerr << "[Shader] Erreur link compute :\n" << log << "\n";
        return false;
    }
    glDeleteShader(comp);
    return true;
}

// ---------------------------------------------------------------------------
// API publique
// ---------------------------------------------------------------------------

bool Shader::load(const std::string& vertPath, const std::string& fragPath) {
    std::string vertSrc = readFile(vertPath);
    std::string fragSrc = readFile(fragPath);
    if (vertSrc.empty() || fragSrc.empty()) return false;

    unsigned int vert = compileShader(GL_VERTEX_SHADER,   vertSrc);
    unsigned int frag = compileShader(GL_FRAGMENT_SHADER, fragSrc);
    return linkProgram(vert, frag);
}

bool Shader::loadCompute(const std::string& compPath) {
    std::string src = readFile(compPath);
    if (src.empty()) return false;
    unsigned int comp = compileShader(GL_COMPUTE_SHADER, src);
    return linkComputeProgram(comp);
}

void Shader::use()   const { glUseProgram(m_programID); }
void Shader::unuse() const { glUseProgram(0); }

// ---------------------------------------------------------------------------
// Uniforms
// ---------------------------------------------------------------------------

int Shader::uniformLocation(const std::string& name) const {
    int loc = glGetUniformLocation(m_programID, name.c_str());
    if (loc == -1)
        std::cerr << "[Shader] Uniform introuvable : " << name << "\n";
    return loc;
}

void Shader::setBool (const std::string& n, bool  v) const { glUniform1i (uniformLocation(n), (int)v); }
void Shader::setInt  (const std::string& n, int   v) const { glUniform1i (uniformLocation(n), v); }
void Shader::setFloat(const std::string& n, float v) const { glUniform1f (uniformLocation(n), v); }
void Shader::setVec2 (const std::string& n, const glm::vec2& v) const { glUniform2fv(uniformLocation(n), 1, glm::value_ptr(v)); }
void Shader::setVec3 (const std::string& n, const glm::vec3& v) const { glUniform3fv(uniformLocation(n), 1, glm::value_ptr(v)); }
void Shader::setVec4 (const std::string& n, const glm::vec4& v) const { glUniform4fv(uniformLocation(n), 1, glm::value_ptr(v)); }
void Shader::setMat4 (const std::string& n, const glm::mat4& m) const { glUniformMatrix4fv(uniformLocation(n), 1, GL_FALSE, glm::value_ptr(m)); }