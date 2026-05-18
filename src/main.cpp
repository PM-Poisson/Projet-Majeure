#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>
#include <memory>

#include "terrain/terrain.hpp"
#include "renderer/shader.hpp"
#include "renderer/camera.hpp"

// ---------------------------------------------------------------------------
// Etat global de la fenetre / interaction
// ---------------------------------------------------------------------------

struct AppState {
    int   width  = 1280;
    int   height = 720;

    Camera camera;

    double lastMouseX = 0.0, lastMouseY = 0.0;
    bool   firstMouse = true;
    bool   rightButtonHeld = false;

    float lastFrame = 0.0f;
    float deltaTime = 0.0f;

    // Parametres terrain (exposes dans ImGui)
    TerrainParams terrainParams;
    bool          regenerate = false;

    // Affichage
    bool wireframe = false;
};

static AppState g_app;

// ---------------------------------------------------------------------------
// Callbacks GLFW
// ---------------------------------------------------------------------------

void framebufferSizeCallback(GLFWwindow*, int w, int h) {
    g_app.width  = w;
    g_app.height = h;
    glViewport(0, 0, w, h);
}

void mouseButtonCallback(GLFWwindow*, int button, int action, int) {
    if (button == GLFW_MOUSE_BUTTON_RIGHT)
        g_app.rightButtonHeld = (action == GLFW_PRESS);
}

void cursorPosCallback(GLFWwindow*, double xpos, double ypos) {
    if (g_app.firstMouse) {
        g_app.lastMouseX = xpos;
        g_app.lastMouseY = ypos;
        g_app.firstMouse = false;
    }
    float dx = (float)(xpos - g_app.lastMouseX);
    float dy = (float)(ypos - g_app.lastMouseY);
    g_app.lastMouseX = xpos;
    g_app.lastMouseY = ypos;

    // Ne pas passer les deltas a la camera si ImGui capture la souris
    if (!ImGui::GetIO().WantCaptureMouse)
        g_app.camera.onMouseMove(dx, dy, g_app.rightButtonHeld);
}

void scrollCallback(GLFWwindow*, double, double yOffset) {
    if (!ImGui::GetIO().WantCaptureMouse)
        g_app.camera.onMouseScroll((float)yOffset);
}

void keyCallback(GLFWwindow* window, int key, int, int action, int) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (key == GLFW_KEY_F && action == GLFW_PRESS)
        g_app.camera.toggleMode();

    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
        g_app.wireframe = !g_app.wireframe;
        glPolygonMode(GL_FRONT_AND_BACK, g_app.wireframe ? GL_LINE : GL_FILL);
    }
}

// ---------------------------------------------------------------------------
// Traitement du clavier en continu (appele chaque frame)
// ---------------------------------------------------------------------------

void processKeyboard(GLFWwindow* window) {
    if (ImGui::GetIO().WantCaptureKeyboard) return;

    bool fwd   = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
    bool back  = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
    bool left  = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
    bool right = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
    bool up    = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;
    bool down  = glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS;

    g_app.camera.onKeyboard(fwd, back, left, right, up, down, g_app.deltaTime);
}

// ---------------------------------------------------------------------------
// Interface ImGui — panneau de controle du terrain
// ---------------------------------------------------------------------------

void renderUI() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_Once);
    ImGui::Begin("Terrain");

    ImGui::Text("Camera : %s  (F pour basculer)",
                g_app.camera.mode() == Camera::Mode::Orbit ? "Orbite" : "FPS");
    ImGui::Text("TAB : wireframe | clic droit : rotation");
    ImGui::Separator();

    TerrainParams& p = g_app.terrainParams;

    ImGui::Text("Bruit");
    ImGui::SliderFloat("Frequence",    &p.noiseFreq,    0.001f, 0.01f,  "%.4f");
    ImGui::SliderInt  ("Octaves",      &p.octaves,      1, 10);
    ImGui::SliderFloat("Lacunarite",   &p.lacunarity,   1.5f, 3.0f);
    ImGui::SliderFloat("Gain",         &p.gain,         0.3f, 0.7f);
    ImGui::SliderFloat("Domain warp",  &p.warpStrength, 0.0f, 3.0f);
    ImGui::Separator();

    ImGui::Text("Profil cotier");
    ImGui::SliderFloat("Hauteur max",  &p.heightScale,  5.0f,  100.0f);
    ImGui::SliderFloat("Niveau mer",   &p.seaLevel,    -10.0f,  10.0f);
    ImGui::SliderFloat("Blend cote",   &p.coastBlend,   0.05f,  0.8f);
    ImGui::SliderFloat("Offset cote",  &p.shoreOffset, -0.4f,   0.4f);
    ImGui::Separator();

    ImGui::InputScalar("Seed", ImGuiDataType_U32, &p.seed);

    if (ImGui::Button("Regenerer", ImVec2(-1, 0)))
        g_app.regenerate = true;

    ImGui::Separator();
    ImGui::Text("FPS : %.1f", ImGui::GetIO().Framerate);

    ImGui::End();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// ---------------------------------------------------------------------------
// Point d'entree
// ---------------------------------------------------------------------------

int main() {
    // ---- Init GLFW ----
    if (!glfwInit()) {
        std::cerr << "Echec init GLFW\n";
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(
        g_app.width, g_app.height,
        "Coastal Erosion", nullptr, nullptr);
    if (!window) {
        std::cerr << "Echec creation fenetre GLFW\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);  // vsync

    // ---- Init GLAD ----
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Echec init GLAD\n";
        return -1;
    }
    std::cout << "OpenGL " << glGetString(GL_VERSION) << "\n";

    // ---- Callbacks ----
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetMouseButtonCallback    (window, mouseButtonCallback);
    glfwSetCursorPosCallback      (window, cursorPosCallback);
    glfwSetScrollCallback         (window, scrollCallback);
    glfwSetKeyCallback            (window, keyCallback);

    // ---- Init ImGui ----
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 430");

    // ---- Etat OpenGL ----
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glViewport(0, 0, g_app.width, g_app.height);

    // ---- Shader terrain ----
    Shader terrainShader;
    if (!terrainShader.load("shaders/terrain.vert", "shaders/terrain.frag")) {
        std::cerr << "Echec chargement shader terrain\n";
        return -1;
    }

    // ---- Generation du terrain ----
    auto terrain = std::make_unique<TerrainGenerator>(g_app.terrainParams);
    terrain->generate();

    // ---- Boucle principale ----
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = (float)glfwGetTime();
        g_app.deltaTime    = currentFrame - g_app.lastFrame;
        g_app.lastFrame    = currentFrame;

        glfwPollEvents();
        processKeyboard(window);

        // Regeneration si demandee depuis ImGui
        if (g_app.regenerate) {
            terrain->regenerate(g_app.terrainParams);
            g_app.regenerate = false;
        }

        // ---- Rendu ----
        glClearColor(0.53f, 0.73f, 0.87f, 1.0f);  // ciel bleu clair
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float aspect = (float)g_app.width / (float)g_app.height;

        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view  = g_app.camera.viewMatrix();
        glm::mat4 proj  = g_app.camera.projectionMatrix(aspect);

        terrainShader.use();
        terrainShader.setMat4 ("uModel",       model);
        terrainShader.setMat4 ("uView",        view);
        terrainShader.setMat4 ("uProjection",  proj);
        terrainShader.setFloat("uSeaLevel",    g_app.terrainParams.seaLevel);
        terrainShader.setFloat("uHeightScale", g_app.terrainParams.heightScale);
        terrainShader.setVec3 ("uLightDir",    glm::normalize(glm::vec3(0.6f, 1.0f, 0.4f)));
        terrainShader.setVec3 ("uCameraPos",   g_app.camera.position());

        terrain->mesh().draw();
        terrainShader.unuse();

        // ---- UI ----
        renderUI();

        glfwSwapBuffers(window);
    }

    // ---- Nettoyage ----
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}