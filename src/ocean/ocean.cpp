/**Projet majeure 4ETI - CPE Lyon - 2025/2026 */

#include <GL/glew.h>
#include "../UI/myWidgetGL.hpp"
#include "ocean.hpp"

#include "../lib/opengl/glutils.hpp"
#include "../lib/interface/camera_matrices.hpp"
#include "../lib/mesh/mesh_io.hpp"
#include "../lib/mesh/triangle_index.hpp"

#include <cmath>
#include <string>

// ------------------------------------------------------------------ //
//  load_scene : appelé une seule fois au démarrage                    //
// ------------------------------------------------------------------ //
void scene::load_scene()
{
    // Shader
    texture_default   = load_texture_file("data/white.jpg");
    shader_program_id = read_shader("shaders/ocean.vert",
                                    "shaders/ocean.frag");

    // --- Génération de la grille océan (CPU, statique) ---
    const int   N    = 50;      // résolution 50x50
    const float size = 20.0f;   // taille du plan en unités monde

    // Sommets + UV
    for(int i = 0; i < N; ++i)
    {
        for(int j = 0; j < N; ++j)
        {
            float u = i / float(N - 1);
            float v = j / float(N - 1);

            cpe::vec3 p;
            p.x() = size * (u - 0.5f);
            p.y() = size * (v - 0.5f);
            p.z() = 0.0f;

            ocean_mesh.add_vertex(p);
            ocean_mesh.add_texture_coord(cpe::vec2(u, v));
        }
    }

    // Triangles (deux par cellule)
    for(int i = 0; i < N - 1; ++i)
    {
        for(int j = 0; j < N - 1; ++j)
        {
            int k = i * N + j;
            //   k --- k+1
            //   |  \   |
            //  k+N - k+N+1
            ocean_mesh.add_triangle_index(cpe::triangle_index(k,     k + 1,   k + N));
            ocean_mesh.add_triangle_index(cpe::triangle_index(k + 1, k + N + 1, k + N));
        }
    }

    // Remplir couleurs/normales par défaut (requis par fill_vbo)
    ocean_mesh.fill_empty_field_by_default();

    // Envoi au GPU (une seule fois)
    ocean_mesh_opengl.fill_vbo(ocean_mesh);

    // Démarrage du timer
    timer.start();
}

// ------------------------------------------------------------------ //
//  draw_scene : appelé à chaque frame                                 //
// ------------------------------------------------------------------ //
void scene::draw_scene()
{
    glUseProgram(shader_program_id);                                        PRINT_OPENGL_ERROR();

    // Matrices caméra
    cpe::camera_matrices const& cam = pwidget->camera();
    glUniformMatrix4fv(get_uni_loc(shader_program_id, "camera_modelview"),
                       1, false, cam.modelview.pointer());                  PRINT_OPENGL_ERROR();
    glUniformMatrix4fv(get_uni_loc(shader_program_id, "camera_projection"),
                       1, false, cam.projection.pointer());                 PRINT_OPENGL_ERROR();
    glUniformMatrix4fv(get_uni_loc(shader_program_id, "normal_matrix"),
                       1, false, cam.normal.pointer());                     PRINT_OPENGL_ERROR();

    // Temps en secondes pour l'animation
    float t = timer.elapsed() / 1000.0f;
    glUniform1f(get_uni_loc(shader_program_id, "time"), t);                 PRINT_OPENGL_ERROR();

    // Dessin
    ocean_mesh_opengl.draw();                                               PRINT_OPENGL_ERROR();
}

// ------------------------------------------------------------------ //
//  Boilerplate                                                         //
// ------------------------------------------------------------------ //
scene::scene()
    : shader_program_id(0)
    , pwidget(nullptr)
{}

GLuint scene::load_texture_file(std::string const& filename)
{
    return pwidget->load_texture_file(filename);
}

void scene::set_widget(myWidgetGL* widget_param)
{
    pwidget = widget_param;
}
