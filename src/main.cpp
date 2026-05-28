#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>

#include "terrain/scene.hpp"
#include "ocean/ocean.hpp"
#include "renderer/camera.hpp"
#include "trees/tree_system.hpp"
#include "fish/fish_system.hpp"

struct AppState {
    int   width=1280, height=720;
    Camera camera;
    double lastMouseX=0,lastMouseY=0;
    bool   firstMouse=true, rightButton=false;
    float  lastFrame=0,deltaTime=0,time=0;
    IslandParams         islandParams;
    TreePlacementParams  treeParams;
    FishParams           fishParams;
    bool   regenerate=false, wireframe=false;
};
static AppState g;

void cbResize  (GLFWwindow*,int w,int h)            {g.width=w;g.height=h;glViewport(0,0,w,h);}
void cbMouseBtn(GLFWwindow*,int btn,int action,int) {if(btn==GLFW_MOUSE_BUTTON_RIGHT)g.rightButton=(action==GLFW_PRESS);}
void cbScroll  (GLFWwindow*,double,double yo)       {if(!ImGui::GetIO().WantCaptureMouse)g.camera.onMouseScroll(float(yo));}
void cbCursor  (GLFWwindow*,double xp,double yp) {
    if(g.firstMouse){g.lastMouseX=xp;g.lastMouseY=yp;g.firstMouse=false;}
    float dx=float(xp-g.lastMouseX),dy=float(yp-g.lastMouseY);
    g.lastMouseX=xp;g.lastMouseY=yp;
    if(!ImGui::GetIO().WantCaptureMouse)g.camera.onMouseMove(dx,dy,g.rightButton);
}
void cbKey(GLFWwindow* win,int key,int,int action,int) {
    if(key==GLFW_KEY_ESCAPE&&action==GLFW_PRESS)glfwSetWindowShouldClose(win,true);
    if(key==GLFW_KEY_F     &&action==GLFW_PRESS)g.camera.toggleMode();
    if(key==GLFW_KEY_TAB   &&action==GLFW_PRESS){
        g.wireframe=!g.wireframe;
        glPolygonMode(GL_FRONT_AND_BACK,g.wireframe?GL_LINE:GL_FILL);
    }
}
void processKeys(GLFWwindow* win){
    if(ImGui::GetIO().WantCaptureKeyboard)return;
    g.camera.onKeyboard(
        glfwGetKey(win,GLFW_KEY_W)==GLFW_PRESS,glfwGetKey(win,GLFW_KEY_S)==GLFW_PRESS,
        glfwGetKey(win,GLFW_KEY_A)==GLFW_PRESS,glfwGetKey(win,GLFW_KEY_D)==GLFW_PRESS,
        glfwGetKey(win,GLFW_KEY_E)==GLFW_PRESS,glfwGetKey(win,GLFW_KEY_Q)==GLFW_PRESS,
        g.deltaTime);
}

void renderUI(int treeCount) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos ({10,10},   ImGuiCond_Once);
    ImGui::SetNextWindowSize({320,640}, ImGuiCond_Once);
    ImGui::Begin("Coastal Erosion");

    ImGui::Text("Camera : %s  (F pour basculer)",
        g.camera.mode()==Camera::Mode::Orbit?"Orbite":"FPS");
    ImGui::Text("TAB: wireframe  |  clic droit: rotation");
    ImGui::Separator();

    IslandParams& p = g.islandParams;

    ImGui::Text("Forme de l'ile");
    ImGui::SliderFloat("Rayon",             &p.islandRadius, 0.15f, 0.65f);
    ImGui::SliderFloat("Abruption falaise", &p.shoreBlend,   0.03f, 0.30f, "%.3f");
    ImGui::SliderFloat("Hauteur max",       &p.heightScale,  5.0f,  80.0f);
    ImGui::Separator();

    ImGui::Text("Detail de surface");
    ImGui::SliderFloat("Freq bruit",   &p.noiseFreq,    0.001f, 0.008f, "%.4f");
    ImGui::SliderInt  ("Octaves",      &p.octaves,      2, 10);
    ImGui::SliderFloat("Domain warp",  &p.warpStrength, 0.0f, 3.0f);
    ImGui::InputScalar("Seed", ImGuiDataType_U32, &p.seed);
    ImGui::Separator();

    ImGui::Checkbox("Erosion pluie", &p.erosion.enabled);
    if (p.erosion.enabled) {
        ImGui::Indent(8.f);
        ImGui::TextColored({0.4f,0.8f,1.f,1.f},"[ Pluie ]");
        ImGui::SliderInt  ("Nb gouttes",    &p.erosion.rain.numDroplets,    1000, 100000);
        ImGui::SliderFloat("Erosion",       &p.erosion.rain.erosionRate,    0.01f, 0.3f,  "%.2f");
        ImGui::SliderFloat("Depot",         &p.erosion.rain.depositionRate, 0.1f,  1.0f,  "%.2f");
        ImGui::SliderFloat("Capacite",      &p.erosion.rain.capacity,       1.0f,  15.0f, "%.1f");
        ImGui::SliderInt  ("Duree vie",     &p.erosion.rain.maxSteps,       20, 120);
        ImGui::SliderFloat("Rayon pinceau", &p.erosion.rain.brushRadius,    1.f, 6.f,     "%.1f");
        ImGui::TextColored({1.f,0.7f,0.3f,1.f},"[ Thermique ]");
        ImGui::SliderInt  ("Cycles",        &p.erosion.thermal.cycles,       1, 6,    "%d (pluie→talus)");
        ImGui::SliderFloat("Angle talus",   &p.erosion.thermal.talusAngle,  10.f,60.f,"%.0f deg");
        ImGui::SliderInt  ("Iterations",    &p.erosion.thermal.iterations,  1, 20);
        ImGui::Unindent(8.f);
    }
    ImGui::Separator();

    TreePlacementParams& t = g.treeParams;
    ImGui::Checkbox("Arbres", &t.enabled);
    if (t.enabled) {
        ImGui::Indent(8.f);
        ImGui::TextColored({0.4f,0.9f,0.4f,1.f}, "[ Foret ] %d arbres", treeCount);
        ImGui::SliderFloat("Densite",         &t.density,      0.0f, 1.0f,  "%.2f");
        ImGui::SliderFloat("Distance min",    &t.minDistance,  0.6f, 4.0f,  "%.1f");
        ImGui::SliderFloat("Espacement",      &t.gridSpacing,  1.0f, 4.0f,  "%.1f");
        ImGui::SliderFloat("Hauteur min",     &t.minHeight,    0.5f, 10.f,  "%.1f");
        ImGui::SliderFloat("Hauteur max",     &t.maxHeight,    10.f, 60.f,  "%.1f");
        ImGui::SliderFloat("Pente max (cos)", &t.maxSlopeCos,  0.3f, 0.95f, "%.2f");
        ImGui::SliderFloat("Echelle min",     &t.minScale,     0.3f, 1.5f,  "%.2f");
        ImGui::SliderFloat("Echelle max",     &t.maxScale,     0.5f, 3.0f,  "%.2f");
        ImGui::SliderFloat("Freq clairieres", &t.densityFreq,     0.005f, 0.10f, "%.3f");
        ImGui::SliderFloat("Biais clairieres",&t.clearingBias,    0.0f,   0.7f,  "%.2f (haut=plus vide)");
        ImGui::SliderFloat("Contraste",       &t.clearingContrast,0.5f,   5.0f,  "%.1f (haut=bords nets)");
        ImGui::Unindent(8.f);
    }


    // ---- Section Poissons ----
    FishParams& fp = g.fishParams;
    ImGui::Checkbox("Poissons", &fp.enabled);
    if (fp.enabled) {
        ImGui::Indent(8.f);
        ImGui::TextColored({0.3f,0.8f,1.f,1.f}, "[ Bancs ]");
        ImGui::SliderInt  ("Nb bancs",       &fp.numSchools,     1, 5);
        ImGui::SliderInt  ("Poissons/banc",  &fp.fishPerSchool,  5, 50);
        ImGui::SliderFloat("Vitesse max",    &fp.maxSpeed,       1.0f, 10.0f, "%.1f");
        ImGui::SliderFloat("Cohesion",       &fp.cohesionForce,  0.0f, 3.0f,  "%.2f");
        ImGui::SliderFloat("Separation",     &fp.separationForce,0.0f, 5.0f,  "%.2f");
        ImGui::SliderFloat("Exploration",    &fp.explorationForce,0.0f,2.0f,  "%.2f");
        ImGui::SliderFloat("Taille",         &fp.fishScale,      0.1f, 1.0f,  "%.2f");
        ImGui::Unindent(8.f);
    }

    ImGui::Separator();
    if(ImGui::Button("Regenerer",{-1,0})) g.regenerate=true;
    ImGui::Separator();
    ImGui::Text("FPS : %.1f", ImGui::GetIO().Framerate);
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

int main() {
    if(!glfwInit()){std::cerr<<"Echec GLFW\n";return -1;}
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* win=glfwCreateWindow(g.width,g.height,"Coastal Erosion",nullptr,nullptr);
    if(!win){std::cerr<<"Echec fenetre\n";glfwTerminate();return -1;}
    glfwMakeContextCurrent(win); glfwSwapInterval(1);

    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){std::cerr<<"Echec GLAD\n";return -1;}
    std::cout<<"OpenGL "<<glGetString(GL_VERSION)<<"\n";

    glfwSetFramebufferSizeCallback(win,cbResize);
    glfwSetMouseButtonCallback(win,cbMouseBtn);
    glfwSetCursorPosCallback(win,cbCursor);
    glfwSetScrollCallback(win,cbScroll);
    glfwSetKeyCallback(win,cbKey);

    IMGUI_CHECKVERSION(); ImGui::CreateContext(); ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(win,true);
    ImGui_ImplOpenGL3_Init("#version 430");

    glEnable(GL_DEPTH_TEST);
    glViewport(0,0,g.width,g.height);

    const glm::vec3 lightDir = glm::normalize(glm::vec3(0.6f,1.f,0.4f));

    Ocean ocean;
    ocean.worldSize = 200.f;
    ocean.init();

    IslandScene island;
    island.init(g.islandParams);

    TreeSystem trees;
    trees.init();
    trees.generate(island.heights(), island.gridN(),
                   island.worldSize(), island.seaLevel(),
                   g.islandParams.seed, g.treeParams);

    FishSystem fish;
    fish.init();
    fish.spawn(island.heights(), island.gridN(),
               island.worldSize(), island.seaLevel(),
               g.islandParams.seed, g.fishParams);

    while(!glfwWindowShouldClose(win)){
        float now=float(glfwGetTime());
        g.deltaTime=now-g.lastFrame; g.lastFrame=now; g.time=now;
        glfwPollEvents(); processKeys(win);

        if(g.regenerate){
            island.regenerate(g.islandParams);
            trees.generate(island.heights(), island.gridN(),
                           island.worldSize(), island.seaLevel(),
                           g.islandParams.seed, g.treeParams);
            fish.spawn(island.heights(), island.gridN(),
                       island.worldSize(), island.seaLevel(),
                       g.islandParams.seed, g.fishParams);
            g.regenerate=false;
        }

        // Simulation des bancs de poissons
        fish.update(g.deltaTime, island.heights(), island.gridN(), island.worldSize());

        glClearColor(0.52f,0.72f,0.88f,1.f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        float     aspect = float(g.width)/float(g.height);
        glm::mat4 view   = g.camera.viewMatrix();
        glm::mat4 proj   = g.camera.projectionMatrix(aspect);
        glm::vec3 camPos = g.camera.position();

        island.draw(view, proj, lightDir, camPos);
        trees.draw (view, proj, lightDir, camPos);
        fish.draw  (view, proj, lightDir, camPos);
        // ocean.draw prend maintenant lightDir et cameraPos (version main branch)
        ocean.draw (view, proj, g.time, lightDir, camPos);
        renderUI(trees.count());
        glfwSwapBuffers(win);
    }

    ImGui_ImplOpenGL3_Shutdown(); ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext(); glfwDestroyWindow(win); glfwTerminate();
    return 0;
}