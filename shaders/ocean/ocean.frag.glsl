#version 330 core

// --- Entrée depuis le vertex shader ---
in struct fragment_data {
    vec3 position;
    vec3 normal;
    vec3 color;
    vec2 uv;
    float height;
} fragment;

// --- Sortie ---
layout(location = 0) out vec4 FragColor;

// --- Uniforms CGP standard ---
uniform mat4 view;
uniform vec3 light;

// Material Phong (envoyé automatiquement par CGP)
struct phong_structure {
    float ambient;
    float diffuse;
    float specular;
    float specular_exponent;
};
struct material_structure {
    vec3  color;
    float alpha;
    phong_structure phong;
};
uniform material_structure material;

void main()
{
    // Couleur océan : dégradé bleu profond → bleu clair selon la hauteur
    vec3 deep  = vec3(0.0,  0.15, 0.4);
    vec3 crest = vec3(0.15, 0.5,  0.8);
    float t = clamp((fragment.height + 0.5) / 1.0, 0.0, 1.0);
    vec3 ocean_color = mix(deep, crest, t) * material.color;

    // Éclairage Phong simple
    vec3 N = normalize(fragment.normal);
    vec3 L = normalize(light - fragment.position);
    float diffuse = max(dot(N, L), 0.0);

    mat3 O = transpose(mat3(view));
    vec3 last_col = vec3(view * vec4(0.0, 0.0, 0.0, 1.0));
    vec3 cam_pos  = -O * last_col;
    vec3 V = normalize(cam_pos - fragment.position);
    vec3 R = reflect(-L, N);
    float specular = 0.0;
    if(diffuse > 0.0)
        specular = pow(max(dot(R, V), 0.0), material.phong.specular_exponent);

    vec3 color = (material.phong.ambient + material.phong.diffuse * diffuse) * ocean_color
               + material.phong.specular * specular * vec3(1.0);

    FragColor = vec4(color, material.alpha);
}
