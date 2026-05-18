#version 330 core

// --- Entrees depuis le vertex shader ---
in vec3  v_normal;
in vec2  v_uv;
in float v_height;
in float v_slope;

// --- Uniformes ---
uniform float sea_level;
uniform float height_scale;

// --- Sortie ---
out vec4 FragColor;

// ---------------------------------------------------------------------------
// Micro-texture procedurale (evite d'avoir besoin de fichiers .png)
// ---------------------------------------------------------------------------
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}
float noise2d(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(
        mix(hash(i), hash(i + vec2(1,0)), f.x),
        mix(hash(i + vec2(0,1)), hash(i + vec2(1,1)), f.x),
        f.y
    );
}

// ---------------------------------------------------------------------------
// Palettes
// ---------------------------------------------------------------------------
vec3 color_rock(float h) {
    vec3 dark  = vec3(0.30, 0.28, 0.26);
    vec3 light = vec3(0.62, 0.60, 0.56);
    float n = noise2d(v_uv * 40.0);
    vec3 c = mix(dark, light, n);
    // Zones humides (falaise) plus sombres
    float wetness = 1.0 - smoothstep(0.1, 0.4, v_slope);
    return mix(c, dark * 0.6, wetness * 0.5);
}

vec3 color_beach() {
    vec3 wet = vec3(0.55, 0.47, 0.32);
    vec3 dry = vec3(0.76, 0.68, 0.48);
    return mix(wet, dry, noise2d(v_uv * 80.0) * 0.5 + 0.25);
}

vec3 color_grass(float h) {
    vec3 grass    = vec3(0.25, 0.42, 0.15);
    vec3 soil     = vec3(0.36, 0.28, 0.18);
    vec3 rockBare = vec3(0.58, 0.56, 0.52);
    vec3 base = mix(grass, soil, noise2d(v_uv * 50.0) * 0.4);
    // Roche nue sur les pentes fortes
    return mix(base, rockBare, smoothstep(0.55, 0.80, 1.0 - v_slope));
}

// ---------------------------------------------------------------------------
// Eclairage directionnel simple
// ---------------------------------------------------------------------------
vec3 apply_light(vec3 albedo) {
    vec3 L       = normalize(vec3(0.6, 0.4, 1.0));   // direction lumiere (Z = haut)
    float diff   = max(dot(v_normal, L), 0.0);
    float ambient = 0.28;
    return albedo * (ambient + diff * 0.72);
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
void main()
{
    float h = v_height;
    vec3 albedo;

    if (h < sea_level + 0.05) {
        // Juste sous la surface : sable de plage mouille
        albedo = color_beach();

    } else if (h < sea_level + 0.25) {
        // Zone de plage / estran
        float t = (h - sea_level) / 0.25;
        albedo = mix(color_beach(), color_rock(h), smoothstep(0.4, 1.0, t));

    } else if (h < sea_level + height_scale * 0.55) {
        // Falaise rocheuse
        albedo = color_rock(h);

    } else {
        // Sommet : herbe ou roche selon la pente
        float t = clamp((h - sea_level - height_scale * 0.55) / (height_scale * 0.2), 0.0, 1.0);
        albedo = mix(color_rock(h), color_grass(h), smoothstep(0.0, 1.0, t));
    }

    FragColor = vec4(apply_light(albedo), 1.0);
}
