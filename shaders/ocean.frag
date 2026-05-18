#version 430 core

in float vWaveHeight;
in vec2  vUV;
in vec3  vWorldPos;

out vec4 fragColor;

uniform float uTime;

void main() {
    // Bleu profond -> bleu clair selon la hauteur de vague
    vec3 deep  = vec3(0.02, 0.12, 0.35);
    vec3 crest = vec3(0.15, 0.45, 0.75);

    // Normalise entre -1 et 1 (amplitude max ~2.3)
    float t = clamp((vWaveHeight + 2.3) / 4.6, 0.0, 1.0);
    vec3 color = mix(deep, crest, t);

    // Legere mousse sur les cretes
    float foam = smoothstep(1.6, 2.3, vWaveHeight);
    color = mix(color, vec3(0.85, 0.92, 1.0), foam * 0.5);

    // Transparence legere (alpha 0.88 pour voir le fond sous l'eau)
    fragColor = vec4(color, 0.88);
}
