#version 330 core

// --- Entrée depuis le vertex shader ---
in float height;

// --- Sortie ---
out vec4 FragColor;

void main()
{
    // Couleur de base : bleu océan
    vec3 deep  = vec3(0.0, 0.15, 0.4);   // bleu profond
    vec3 crest = vec3(0.2, 0.5,  0.8);   // bleu clair sur les crêtes

    // Mélange selon la hauteur de la vague (normalisée ~[-0.5, 0.5])
    float t = clamp((height + 0.5) / 1.0, 0.0, 1.0);
    vec3 color = mix(deep, crest, t);

    FragColor = vec4(color, 1.0);
}
