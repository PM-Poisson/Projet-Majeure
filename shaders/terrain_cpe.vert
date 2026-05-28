#version 330 core

// --- Entrees du VBO (identiques a l'ocean) ---
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal_in;
layout(location = 2) in vec3 color_in;
layout(location = 3) in vec2 uv_in;

// --- Uniformes (meme nommage que ocean.vert) ---
uniform mat4 camera_modelview;
uniform mat4 camera_projection;
uniform mat4 normal_matrix;
uniform float sea_level;
uniform float height_scale;

// --- Sorties vers le fragment shader ---
out vec3 v_normal;
out vec2 v_uv;
out float v_height;   // hauteur Z en unites monde
out float v_slope;    // pente estimee (1=plat, 0=vertical)

void main()
{
    v_uv     = uv_in;
    v_height = position.z;   // Z = hauteur dans la convention de votre collegue

    // Normale transformee
    v_normal = normalize((normal_matrix * vec4(normal_in, 0.0)).xyz);

    // Pente : produit scalaire de la normale avec l'axe vertical (Z ici)
    v_slope = clamp(dot(v_normal, vec3(0.0, 0.0, 1.0)), 0.0, 1.0);

    gl_Position = camera_projection * camera_modelview * vec4(position, 1.0);
}
