#version 430 core

// ---------------------------------------------------------------------------
// ocean.frag — Fragment shader de l'océan
//
// Calcule la couleur finale de chaque fragment de l'eau avec :
//   - Couleur de base interpolée selon la hauteur de vague (creux → crête)
//   - Éclairage spéculaire Blinn-Phong (reflets nets du soleil sur l'eau)
//   - Effet Fresnel simplifié (l'eau réfléchit davantage le ciel en angle rasant)
//   - Mousse blanche sur les crêtes les plus hautes
//   - Transparence variable (plus opaque sur les crêtes)
// ---------------------------------------------------------------------------

in vec3  vWorldPos;
in vec3  vNormal;
in vec2  vUV;
in float vWaveHeight;  // hauteur Y totale des vagues

out vec4 fragColor;

uniform float uTime;
uniform vec3  uLightDir;
uniform vec3  uCameraPos;

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightDir);
    vec3 V = normalize(uCameraPos - vWorldPos);

    // Couleur de base : bleu profond dans les creux, bleu clair sur les crêtes
    // vWaveHeight varie approximativement dans [-3.5, 3.5]
    vec3 deep  = vec3(0.01, 0.10, 0.30);
    vec3 mid   = vec3(0.05, 0.25, 0.55);
    vec3 crest = vec3(0.18, 0.52, 0.78);
    float t = clamp((vWaveHeight + 3.5) / 7.0, 0.0, 1.0);
    vec3 baseColor = t < 0.5
        ? mix(deep,  mid,   t * 2.0)
        : mix(mid,   crest, (t - 0.5) * 2.0);

    // Diffus (lumière directionnelle)
    float diff = max(dot(N, L), 0.0);

    // Spéculaire Blinn-Phong : exposant élevé = reflets nets et localisés
    vec3  H    = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), 180.0);

    // Fresnel simplifié : angle rasant => réflexion du ciel
    float fresnel    = pow(1.0 - max(dot(N, V), 0.0), 3.0);
    vec3  skyColor   = vec3(0.52, 0.72, 0.88);
    vec3  fresnelCol = mix(baseColor, skyColor, fresnel * 0.5);

    // Mousse blanche sur les crêtes (vWaveHeight > ~2.5)
    float foam     = smoothstep(2.5, 3.8, vWaveHeight);
    vec3  foamCol  = vec3(0.88, 0.94, 1.0);

    // Composition finale
    vec3 color = fresnelCol * (0.15 + diff * 0.7)
               + vec3(spec * 0.9)
               + foam * foamCol * 0.6;

    // Transparence : plus opaque sur les crêtes (t=1), plus translucide dans les creux
    float alpha = mix(0.82, 0.95, t);

    fragColor = vec4(color, alpha);
}
