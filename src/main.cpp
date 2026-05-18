#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>
#include <memory>

#include "terrain/scene.hpp"   // IslandScene
#include "ocean/ocean.hpp"     // Ocean
#include "renderer/camera.hpp"

// ---------------------------------------------------------------------------
// Etat global
// ---------------------------------------------------------------------------
struct AppState {
    int   width  = 1280;
    int   height = 720;

    Camera camera;

    double lastMouseX  = 0.0, lastMouseY = 0.0;
    bool   firstMouse  = true;
    bool   rightButton = false;

    float lastFrame = 0.0f;
    float deltaTime = 0.0f;
    float time      = 0.0f;

    IslandParams islandParams;
    bool         regenerate  = false;
    bool         wireframe   = false;
};

static AppState g;

// ---------------------------------------------------------------------------
// Callbacks
// ---------------------------------------------------------------------------
void cbResize(GLFWwindow*, int w, int h) {
    g.width = w; g.height = h;
    glViewport(0, 0, w, h);
}
void cbMouseBtn(GLFWwindow*, int btn, int action, int) {
    if (btn == GLFW_MOUSE_BUTTON_RIGHT)
        g.rightButton = (action == GLFW_PRESS);
}
void cbCursor(GLFWwindow*, double xp, double yp) {
    if (g.firstMouse) { g.lastMouseX = xp; g.lastMouseY = yp; g.firstMouse = false; }
    float dx = float(xp - g.lastMouseX);
    float dy = float(yp - g.lastMouseY);
    g.lastMouseX = xp; g.lastMouseY = yp;
    if (!ImGui::GetIO().WantCaptureMouse)
        g.camera.onMouseMove(dx, dy, g.rightButton);
}
void cbScroll(GLFWwindow*, double, double yo) {
    if (!ImGui::GetIO().WantCaptureMouse)
        g.camera.onMouseScroll(float(yo));
}
void cbKey(GLFWwindow* win, int key, int, int action, int) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(win, true);
    if (key == GLFW_KEY_F && action == GLFW_PRESS)
        g.camera.toggleMode();
    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
        g.wireframe = !g.wireframe;
        glPolygonMode(GL_FRONT_AND_BACK, g.wireframe ? GL_LINE : GL_FILL);
    }
}
void processKeys(GLFWwindow* win) {
    if (ImGui::GetIO().WantCaptureKeyboard) return;
    g.camera.onKeyboard(
        glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS,
        glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS,
        glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS,
        glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS,
        glfwGetKey(win, GLFW_KEY_E) == GLFW_PRESS,
        glfwGetKey(win, GLFW_KEY_Q) == GLFW_PRESS,
        g.deltaTime
    );
}

// ---------------------------------------------------------------------------
// UI ImGui
// ---------------------------------------------------------------------------

void renderUI() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos (ImVec2(10, 10),   ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(300, 340),  ImGuiCond_Once);
    ImGui::Begin("Coastal Erosion");

    ImGui::Text("Camera : %s  (F)",
        g.camera.mode() == Camera::Mode::Orbit ? "Orbite" : "FPS");
    ImGui::Text("TAB : wireframe  |  clic droit : rotation");
    ImGui::Separator();

    IslandParams& p = g.islandParams;
    ImGui::SliderFloat("Hauteur max",   &p.heightScale,  5.0f,  80.0f);
    ImGui::SliderFloat("Rayon ile",     &p.islandRadius, 0.1f,  0.6f);
    ImGui::SliderFloat("Blend rivage",  &p.shoreBlend,   0.02f, 0.3f);
    ImGui::SliderFloat("Freq bruit",    &p.noiseFreq,    0.001f,0.01f,"%.4f");
    ImGui::SliderInt  ("Octaves",       &p.octaves,      1, 10);
    ImGui::SliderFloat("Domain warp",   &p.warpStrength, 0.0f,  3.0f);
    ImGui::InputScalar("Seed", ImGuiDataType_U32, &p.seed);

    if (ImGui::Button("Regenerer", ImVec2(-1, 0)))
        g.regenerate = true;

    ImGui::Separator();
    ImGui::Text("FPS : %.1f", ImGui::GetIO().Framerate);
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main() {
    if (!glfwInit()) { std::cerr << "Echec GLFW\n"; return -1; }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* win = glfwCreateWindow(g.width, g.height,
                                       "Coastal Erosion", nullptr, nullptr);
    if (!win) { std::cerr << "Echec fenetre\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Echec GLAD\n"; return -1;
    }
    std::cout << "OpenGL " << glGetString(GL_VERSION) << "\n";

    glfwSetFramebufferSizeCallback(win, cbResize);
    glfwSetMouseButtonCallback    (win, cbMouseBtn);
    glfwSetCursorPosCallback      (win, cbCursor);
    glfwSetScrollCallback         (win, cbScroll);
    glfwSetKeyCallback            (win, cbKey);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(win, true);
    ImGui_ImplOpenGL3_Init("#version 430");

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, g.width, g.height);

    // --- Init scenes ---
    Ocean ocean;
    ocean.worldSize = 200.0f;
    ocean.init();

    IslandScene island;
    island.init(g.islandParams);

    const glm::vec3 lightDir = glm::normalize(glm::vec3(0.6f, 1.0f, 0.4f));

    // --- Boucle principale ---
    while (!glfwWindowShouldClose(win)) {
        float now    = float(glfwGetTime());
        g.deltaTime  = now - g.lastFrame;
        g.lastFrame  = now;
        g.time       = now;

        glfwPollEvents();
        processKeys(win);

        if (g.regenerate) {
            island.regenerate(g.islandParams);
            g.regenerate = false;
        }

        glClearColor(0.52f, 0.72f, 0.88f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float     aspect = float(g.width) / float(g.height);
        glm::mat4 view   = g.camera.viewMatrix();
        glm::mat4 proj   = g.camera.projectionMatrix(aspect);
        glm::vec3 camPos = g.camera.position();

        // 1. Ile (opaque) — depth buffer bloquera l'ocean au-dessus
        island.draw(view, proj, lightDir, camPos);

        // 2. Ocean (semi-transparent) — sous l'ile grace au depth test
        ocean.draw(view, proj, g.time);

        // ---- UI ----
        renderUI();

        glfwSwapBuffers(win);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}