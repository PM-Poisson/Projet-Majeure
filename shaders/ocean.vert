#version 330 core

// --- Entrées du VBO ---
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal_in;
layout(location = 2) in vec3 color_in;
layout(location = 3) in vec2 uv_in;

// --- Uniformes ---
uniform mat4 camera_modelview;
uniform mat4 camera_projection;
uniform float time;

// --- Sorties vers le fragment shader ---
out float height;

void main()
{
    // Déformation sin/cos en Z
    float wave = 0.0;
    wave += 0.25 * sin(3.0 * position.x + time * 1.2);
    wave += 0.15 * cos(5.0 * position.y + time * 0.8);
    wave += 0.10 * sin(2.0 * position.x + 4.0 * position.y + time);

    vec3 pos = position;
    pos.z += wave;

    height = wave; // transmis au fragment shader pour la couleur

    gl_Position = camera_projection * camera_modelview * vec4(pos, 1.0);
}
