/**Projet majeure 4ETI - CPE Lyon - 2025/2026 */

#pragma once

#ifndef OCEAN_HPP
#define OCEAN_HPP

#include <GL/glew.h>

#include "../lib/3d/vec3.hpp"
#include "../lib/3d/vec2.hpp"
#include "../lib/mesh/mesh.hpp"
#include "../lib/opengl/mesh_opengl.hpp"
#include "../lib/interface/camera_matrices.hpp"

#include <QElapsedTimer>

class myWidgetGL;

class scene
{
public:

    scene();

    /** \brief Method called only once at the beginning */
    void load_scene();

    /** \brief Method called at every frame */
    void draw_scene();

    /** Set the pointer to the parent Widget */
    void set_widget(myWidgetGL* widget_param);

private:

    /** Load a texture from a given file and returns its id */
    GLuint load_texture_file(std::string const& filename);

    /** Access to the parent object */
    myWidgetGL* pwidget;

    /** Default id for the texture (white texture) */
    GLuint texture_default;

    /** The id of the shader to draw the ocean */
    GLuint shader_program_id;

    /** Ocean mesh (CPU, static) */
    cpe::mesh        ocean_mesh;
    cpe::mesh_opengl ocean_mesh_opengl;

    /** Timer for animation */
    QElapsedTimer timer;
};

#endif
