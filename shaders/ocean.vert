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

// Hash déterministe : même position → même valeur, toujours entre 0 et 1
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

void main()
{
    // Vagues de base sin/cos
    float wave = 0.0;
    wave += 0.25 * sin(3.0 * position.x + time * 1.2);
    wave += 0.15 * cos(5.0 * position.y + time * 0.8);
    wave += 0.10 * sin(2.0 * position.x + 4.0 * position.y + time);

    // Perturbation locale : vagues occasionnellement plus grandes/petites
    // floor(... * 0.4) discrétise l'espace en cellules d'environ 2.5 unités
    // chaque cellule a un identifiant unique stable via hash
    vec2  cell     = floor(position.xy * 0.4);
    float local_id = hash(cell);
    // pulse lent et déphasé aléatoirement par cellule (période ~25s)
    float pulse    = sin(time * 0.25 + local_id * 6.2832);
    // step(0.5, local_id) : active la perturbation sur ~la moitié des cellules
    float extra    = 0.35 * pulse * step(0.5, local_id);
    wave += extra;

    vec3 pos = position;
    pos.z += wave;

    height = wave;

    gl_Position = camera_projection * camera_modelview * vec4(pos, 1.0);
}
