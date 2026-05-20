#version 430 core

// ---------------------------------------------------------------------------
// ocean.frag
//
// Colorisation de l'ocean avec :
//   - Couleur par profondeur de vague (bleu profond -> bleu clair)
//   - Eclairage speculaire de Phong (reflets du soleil)
//   - Mousse sur les cretes
//   - Fresnel simplifie (surface reflechissante selon l'angle de vue)
// ---------------------------------------------------------------------------

in vec3  vWorldPos;
in vec3  vNormal;
in vec2  vUV;
in float vWaveHeight;

out vec4 fragColor;

uniform float uTime;
uniform vec3  uLightDir;
uniform vec3  uCameraPos;

void main()
{
    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightDir);
    vec3 V = normalize(uCameraPos - vWorldPos);

    // --- Couleur de base selon la hauteur de vague ---
    vec3 deep  = vec3(0.01, 0.10, 0.30);   // bleu profond (creux)
    vec3 mid   = vec3(0.05, 0.25, 0.55);   // bleu moyen
    vec3 crest = vec3(0.18, 0.52, 0.78);   // bleu clair (crete)

    // vWaveHeight varie approximativement dans [-3.5, 3.5] (somme des amplitudes)
    float t = clamp((vWaveHeight + 3.5) / 7.0, 0.0, 1.0);
    vec3 baseColor = t < 0.5
        ? mix(deep,  mid,   t * 2.0)
        : mix(mid,   crest, (t - 0.5) * 2.0);

    // --- Eclairage diffus ---
    float diff = max(dot(N, L), 0.0);

    // --- Speculaire (Blinn-Phong) : reflets du soleil sur l'eau ---
    vec3  H    = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), 180.0);   // exposant eleve = reflets nets

    // --- Fresnel simplifie : plus on regarde rasant, plus l'eau reflechit ---
    float fresnel = pow(1.0 - max(dot(N, V), 0.0), 3.0);
    vec3  skyColor = vec3(0.52, 0.72, 0.88);         // couleur du ciel reflchi
    vec3  fresnelColor = mix(baseColor, skyColor, fresnel * 0.5);

    // --- Mousse sur les cretes ---
    float foam = smoothstep(2.5, 3.8, vWaveHeight);
    vec3  foamColor = vec3(0.88, 0.94, 1.0);

    // --- Composition finale ---
    vec3 color = fresnelColor * (0.15 + diff * 0.7)  // ambiant + diffus
               + vec3(spec * 0.9)                     // speculaire (blanc brillant)
               + foam * foamColor * 0.6;              // mousse

    // Transparence : plus opaque sur les cretes, translucide dans les creux
    float alpha = mix(0.82, 0.95, t);

    fragColor = vec4(color, alpha);
}
