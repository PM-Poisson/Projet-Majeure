#version 430 core

in vec3 vWorldPos;
in vec3 vNormal;
in vec3 vColor;

uniform vec3 uLightDir;    // direction de la lumière (normalisée)
uniform vec3 uCameraPos;

out vec4 FragColor;

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightDir);

    // Lambert avec ambient floor
    float diffuse = max(dot(N, L), 0.0) * 0.7 + 0.3;

    // Petit spéculaire pour les highlights du feuillage
    vec3 V = normalize(uCameraPos - vWorldPos);
    vec3 R = reflect(-L, N);
    float spec = pow(max(dot(V, R), 0.0), 16.0) * 0.08;

    vec3 color = vColor * diffuse + vec3(spec);
    FragColor  = vec4(color, 1.0);
}
