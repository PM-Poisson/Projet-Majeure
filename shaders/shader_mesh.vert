#version 120

uniform mat4 camera_projection;
uniform mat4 camera_modelview;
uniform mat4 normal_matrix;

varying vec4 position_3d_original;
varying vec4 position_3d_modelview;

varying vec3 normal;
varying vec4 color;

#version 330 core

layout(location=0) in vec3 position;
layout(location=1) in vec2 uv;

out vec2 frag_uv;
out float height;

uniform float time;

void main (void)
{
    gl_Position = camera_projection*camera_modelview*gl_Vertex;

    position_3d_original = gl_Vertex;
    position_3d_modelview = camera_modelview*gl_Vertex;
    color = gl_Color;

    vec4 normal4d = normal_matrix*vec4(normalize(gl_Normal),0);
    normal = normal4d.xyz;

    gl_TexCoord[0]=gl_MultiTexCoord0;

    float wave = 0.0;

    wave += 0.2*sin(4.0*position.x + time);
    wave += 0.1*cos(6.0*position.y + time*1.5);

    vec3 pos = position;
    pos.z += wave;

    height = wave;
    frag_uv = uv;

    gl_Position = camera_projection * camera_modelview * vec4(pos,1.0);
}


