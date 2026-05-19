#version 330 core

// --- Entrées VBO (noms CGP) ---
layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec3 vertex_normal;
layout(location = 2) in vec3 vertex_color;
layout(location = 3) in vec2 vertex_uv;

// --- Uniforms CGP standard ---
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// --- Uniform custom : temps pour l'animation ---
uniform float time;

// --- Sorties vers le fragment shader ---
out struct fragment_data {
    vec3 position;
    vec3 normal;
    vec3 color;
    vec2 uv;
    float height; // hauteur de vague pour la couleur
} fragment;

// Hash déterministe : même position → même valeur entre 0 et 1
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

void main()
{
    // Vagues de base sin/cos
    float wave = 0.0;
    wave += 0.25 * sin(3.0 * vertex_position.x + time * 1.2);
    wave += 0.15 * cos(5.0 * vertex_position.y + time * 0.8);
    wave += 0.10 * sin(2.0 * vertex_position.x + 4.0 * vertex_position.y + time);

    // Perturbation locale par cellule
    vec2  cell     = floor(vertex_position.xy * 0.4);
    float local_id = hash(cell);
    float pulse    = sin(time * 0.25 + local_id * 6.2832);
    float extra    = 0.35 * pulse * step(0.5, local_id);
    wave += extra;

    // Position déformée
    vec3 pos = vertex_position;
    pos.z += wave;

    // Passage en world space
    vec4 position_world = model * vec4(pos, 1.0);

    // Normale approximée (on garde la normale plane pour l'instant)
    mat4 model_normal = transpose(inverse(model));
    vec4 normal_world = model_normal * vec4(vertex_normal, 0.0);

    // Transmission au fragment shader
    fragment.position = position_world.xyz;
    fragment.normal   = normal_world.xyz;
    fragment.color    = vertex_color;
    fragment.uv       = vertex_uv;
    fragment.height   = wave;

    gl_Position = projection * view * position_world;
}
