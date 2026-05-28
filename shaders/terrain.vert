#version 430 core

// ---------------------------------------------------------------------------
// terrain.vert — Vertex shader du terrain cotier
// Entrees : position, normale, uv (layout identique au Mesh)
// Sorties : fragments colories dans terrain.frag
// ---------------------------------------------------------------------------

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;

// Matrices de transformation
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

// Niveau de la mer (pour le fragment shader)
uniform float uSeaLevel;

// Sorties vers le fragment shader
out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUV;
out float vHeight;       // hauteur world-space (pour la colorisation)
out float vSlope;        // pente (dot produit normal/up) pour distinguer falaises/plateaux

void main() {
    vec4 worldPos = uModel * vec4(aPosition, 1.0);
    vWorldPos     = worldPos.xyz;
    vNormal       = normalize(mat3(transpose(inverse(uModel))) * aNormal);
    vUV           = aUV;
    vHeight       = aPosition.y;   // hauteur locale avant model transform

    // Pente : 1.0 = plat, 0.0 = vertical (falaise)
    vSlope = clamp(dot(vNormal, vec3(0.0, 1.0, 0.0)), 0.0, 1.0);

    gl_Position = uProjection * uView * worldPos;
}
