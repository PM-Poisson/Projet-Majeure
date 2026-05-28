#version 430 core

in vec3  vWorldPos;
in vec3  vNormal;
in vec2  vUV;
in float vHeight;
in float vSlope;

out vec4 fragColor;

uniform float uSeaLevel;
uniform vec3  uLightDir;
uniform vec3  uCameraPos;

// ---------------------------------------------------------------------------
// Micro-texture procedurale
// ---------------------------------------------------------------------------
float cheapNoise(vec2 uv) {
    return fract(sin(dot(uv * 43.0, vec2(127.1, 311.7))) * 43758.5453);
}
float smoothNoise(vec2 uv) {
    vec2 i = floor(uv);
    vec2 f = fract(uv);
    f = f * f * (3.0 - 2.0 * f);
    float a = cheapNoise(i),                b = cheapNoise(i + vec2(1,0));
    float c = cheapNoise(i + vec2(0,1)),    d = cheapNoise(i + vec2(1,1));
    return mix(mix(a,b,f.x), mix(c,d,f.x), f.y);
}

// ---------------------------------------------------------------------------
// Palettes
// ---------------------------------------------------------------------------
vec3 colorSeabed(float depth) {
    vec3 rock     = vec3(0.18, 0.20, 0.22);
    vec3 sediment = vec3(0.28, 0.32, 0.30);
    return mix(rock, sediment, clamp(depth * 0.3 + smoothNoise(vUV * 80.0) * 0.15, 0.0, 1.0));
}
vec3 colorIntertidal() {
    return mix(vec3(0.55, 0.47, 0.32), vec3(0.76, 0.68, 0.48), smoothNoise(vUV * 120.0) * 0.2);
}
vec3 colorCliff() {
    vec3 dark  = vec3(0.32, 0.30, 0.28);
    vec3 light = vec3(0.72, 0.70, 0.66);
    vec3 c = mix(dark, light, smoothNoise(vUV * 40.0));
    c += smoothNoise(vUV * 200.0) * 0.08;
    float wetness = 1.0 - smoothstep(0.1, 0.4, vSlope);
    return mix(c, dark * 0.6, wetness * 0.6);
}
vec3 colorPlateau() {
    vec3 grass = vec3(0.28, 0.42, 0.18);
    vec3 soil  = vec3(0.38, 0.30, 0.20);
    vec3 rock  = vec3(0.60, 0.58, 0.55);
    vec3 base  = mix(grass, soil, smoothNoise(vUV * 60.0) * 0.4);
    return mix(base, rock, smoothstep(0.6, 0.85, 1.0 - vSlope));
}

// ---------------------------------------------------------------------------
// Eclairage Phong
// ---------------------------------------------------------------------------
vec3 applyLighting(vec3 albedo) {
    vec3  L    = normalize(uLightDir);
    float diff = max(dot(vNormal, L), 0.0);
    vec3  V    = normalize(uCameraPos - vWorldPos);
    vec3  H    = normalize(L + V);
    float spec = pow(max(dot(vNormal, H), 0.0), 32.0) * (1.0 - vSlope) * 0.3;
    return albedo * (0.25 + diff * 0.75) + vec3(spec);
}

// ---------------------------------------------------------------------------
// Main — seuils en valeurs absolues (hauteur monde en unites)
// ---------------------------------------------------------------------------
void main() {
    float h = vHeight;
    vec3 albedo;

    if (h < uSeaLevel - 1.0) {
        albedo = colorSeabed(uSeaLevel - h);
    } else if (h < uSeaLevel + 2.0) {
        float t = (h - (uSeaLevel - 1.0)) / 3.0;
        albedo  = mix(colorSeabed(1.0), colorIntertidal(), smoothstep(0.0, 1.0, t));
    } else if (h < uSeaLevel + 15.0) {
        float t = (h - (uSeaLevel + 2.0)) / 13.0;
        albedo  = mix(colorIntertidal(), colorCliff(), smoothstep(0.0, 0.3, t));
    } else {
        float t = clamp((h - (uSeaLevel + 15.0)) / 10.0, 0.0, 1.0);
        albedo  = mix(colorCliff(), colorPlateau(), smoothstep(0.0, 1.0, t));
    }

    vec3 color = applyLighting(albedo);

    // Brume atmospherique
    float fog = exp(-length(uCameraPos - vWorldPos) * 0.002);
    color = mix(vec3(0.72, 0.80, 0.88), color, fog);

    fragColor = vec4(color, 1.0);
}
