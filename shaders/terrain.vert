#version 430 core

// ---------------------------------------------------------------------------
// terrain.vert — Vertex shader de l'île
//
// Reçoit les données du VBO (position, normale, UV) et les transmet
// au fragment shader après transformation dans l'espace écran.
// Calcule aussi vSlope (pente) utilisé pour choisir le biome dans terrain.frag.
// ---------------------------------------------------------------------------

layout(location = 0) in vec3 aPosition;  // position monde (Y = hauteur)
layout(location = 1) in vec3 aNormal;    // normale (calculée par diff. finies CPU)
layout(location = 2) in vec2 aUV;        // coordonnées de texture [0,1]

uniform mat4  uModel;
uniform mat4  uView;
uniform mat4  uProjection;
uniform float uSeaLevel;     // niveau de la mer (transmis au fragment shader)

out vec3  vWorldPos;   // position en espace monde (pour l'éclairage)
out vec3  vNormal;     // normale transformée
out vec2  vUV;         // UV pour la micro-texture procédurale
out float vHeight;     // hauteur Y brute (pour les seuils de biome)
out float vSlope;      // dot(normal, up) : 1=plat, 0=vertical

void main() {
    vec4 worldPos = uModel * vec4(aPosition, 1.0);
    vWorldPos = worldPos.xyz;

    // Transformation de la normale (inverse-transposée de la model matrix)
    vNormal = normalize(mat3(transpose(inverse(uModel))) * aNormal);

    vUV     = aUV;
    vHeight = aPosition.y;  // hauteur avant transformation (= hauteur monde si uModel=I)

    // Pente : 1.0 = surface horizontale (plateau), 0.0 = surface verticale (falaise)
    vSlope = clamp(dot(vNormal, vec3(0.0, 1.0, 0.0)), 0.0, 1.0);

    gl_Position = uProjection * uView * worldPos;
}
