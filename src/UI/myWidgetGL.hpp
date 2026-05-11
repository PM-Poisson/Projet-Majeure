#pragma once

#ifndef MY_WIDGET_GL_HPP
#define MY_WIDGET_GL_HPP

#include <GL/glew.h>

#include <QtOpenGL/QGLWidget>
#include <QMouseEvent>

#include "../lib/interface/navigator_tool.hpp"
#include "../lib/mesh/mesh.hpp"
#include "../lib/opengl/mesh_opengl.hpp"
#include "../lib/opengl/axes_helper.hpp"
#include "../lib/interface/camera_matrices.hpp"
#include "../ocean/ocean.hpp"


/** Qt Widget to render OpenGL scene */
class myWidgetGL : public QGLWidget
{
    Q_OBJECT

public:

    myWidgetGL(const QGLFormat& format, QGLWidget *parent = 0);
    ~myWidgetGL();

    /** Set the drawing on/off */
    void change_draw_state();
    /** Set the wireframe on/off */
    void wireframe(bool est_actif);
    /** Get the current cameras values */
    cpe::camera_matrices const& camera() const;

    /** Load a texture given by its filename */
    GLuint load_texture_file(std::string const& filename);

protected:

    void initializeGL();
    void paintGL();
    void resizeGL(int width, int height);

    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void timerEvent(QTimerEvent *event);
    void keyPressEvent(QKeyEvent *event);

private:

    void setup_camera();
    void setup_opengl();
    void setup_glew();
    void print_current_opengl_context() const;
    void draw_axes();

    cpe::navigator_tool nav;

    /** La scène océan */
    scene scene_3d;

    bool draw_state;

    cpe::axes_helper axes;
    cpe::camera_matrices camera_data;
};

#endif
