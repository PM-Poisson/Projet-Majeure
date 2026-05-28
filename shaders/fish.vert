#version 430 core

// Mesh statique du poisson (position + normale)
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

// Par instance (divisor=1) : matrice modèle complète + couleur du banc
layout(location = 2) in vec4 iMat0;
layout(location = 3) in vec4 iMat1;
layout(location = 4) in vec4 iMat2;
layout(location = 5) in vec4 iMat3;
layout(location = 6) in vec3 iColor;

uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vWorldPos;
out vec3 vNormal;
out vec3 vColor;

void main() {
    mat4 model = mat4(iMat0, iMat1, iMat2, iMat3);

    vec4 worldPos = model * vec4(aPos, 1.0);
    vWorldPos = worldPos.xyz;

    // Normale dans l'espace monde (matrice normale = inverse transposée)
    // Comme on n'a pas de non-uniform scale, on peut utiliser directement mat3(model)
    vNormal = normalize(mat3(model) * aNormal);

    vColor = iColor;

    gl_Position = uProjection * uView * worldPos;
}
