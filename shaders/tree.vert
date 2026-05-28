#version 430 core

// Attributs du mesh (partagés entre toutes les instances)
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aColor;

// Attributs per-instance (divisor=1)
layout(location = 3) in vec3  iPos;       // position monde du tronc
layout(location = 4) in float iScale;     // facteur d'échelle global
layout(location = 5) in vec3  iTint;      // teinte multiplicative
layout(location = 6) in float iRotation;  // rotation autour de Y (radians)

uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vWorldPos;
out vec3 vNormal;
out vec3 vColor;

void main() {
    // Rotation Y dans l'espace local de l'arbre
    float c = cos(iRotation);
    float s = sin(iRotation);
    mat3 rotY = mat3(
        c, 0.0, -s,
        0.0, 1.0, 0.0,
        s, 0.0,  c
    );

    vec3 scaled  = aPos * iScale;
    vec3 rotated = rotY * scaled;
    vec3 world   = rotated + iPos;

    vWorldPos = world;
    vNormal   = normalize(rotY * aNormal);
    vColor    = aColor * iTint;

    gl_Position = uProjection * uView * vec4(world, 1.0);
}
