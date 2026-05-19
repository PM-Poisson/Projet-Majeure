#version 430 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;

uniform mat4  uModel;
uniform mat4  uView;
uniform mat4  uProjection;
uniform float uTime;

out float vWaveHeight;
out vec2  vUV;
out vec3  vWorldPos;

void main() {
    vec3 pos = aPosition;

    float wave = 0.0;
    wave += 1.2 * sin(0.018 * pos.x + uTime * 0.8);
    wave += 0.7 * cos(0.025 * pos.z + uTime * 1.1);
    wave += 0.4 * sin(0.012 * pos.x + 0.020 * pos.z + uTime * 0.6);

    pos.y += wave;

    vWaveHeight = wave;
    vUV         = aUV;
    vWorldPos   = (uModel * vec4(pos, 1.0)).xyz;

    gl_Position = uProjection * uView * uModel * vec4(pos, 1.0);
}
