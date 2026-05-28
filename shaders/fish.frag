#version 430 core

in vec3 vWorldPos;
in vec3 vNormal;
in vec3 vColor;

uniform vec3 uLightDir;
uniform vec3 uCameraPos;

out vec4 fragColor;

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightDir);
    vec3 V = normalize(uCameraPos - vWorldPos);
    vec3 H = normalize(L + V);

    // Diffus Lambert + ambient
    float diff = max(dot(N, L), 0.0) * 0.75 + 0.25;

    // Spéculaire Blinn-Phong (brillance des écailles)
    float spec = pow(max(dot(N, H), 0.0), 48.0) * 0.5;

    // Sous-surface scattering approximé : teinte légèrement dorée sur les
    // faces éclairées, plus sombre dans les creux
    vec3 color = vColor * diff + vec3(spec * 0.8, spec * 0.7, spec * 0.5);

    // Légère transparence pour l'effet sous-marin
    fragColor = vec4(color, 0.92);
}
