#version 430 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;

uniform mat4  uModel;
uniform mat4  uView;
uniform mat4  uProjection;
uniform float uTime;

out vec3  vWorldPos;
out vec3  vNormal;
out vec2  vUV;
out float vWaveHeight;

// ---------------------------------------------------------------------------
// Hash déterministe : même position → même valeur entre 0 et 1
// ---------------------------------------------------------------------------
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

// ---------------------------------------------------------------------------
// Une vague de Gerstner
// ---------------------------------------------------------------------------
vec3 gerstnerWave(vec2 p, float amplitude, float wavelength,
                  float speed, vec2 direction,
                  float time, out vec3 normal_contrib)
{
    float k     = 2.0 * 3.14159265 / wavelength;
    float c     = speed;
    float f     = k * dot(direction, p) - c * time;
    float Q     = 0.6;

    float sinf  = sin(f);
    float cosf  = cos(f);

    vec3 disp;
    disp.x = Q * amplitude * direction.x * cosf;
    disp.y = amplitude * sinf;
    disp.z = Q * amplitude * direction.y * cosf;

    normal_contrib = vec3(
        -direction.x * k * amplitude * cosf,
         1.0 - Q * k * amplitude * sinf,
        -direction.y * k * amplitude * cosf
    );

    return disp;
}

void main()
{
    vec3 pos  = aPosition;
    vec3 norm = vec3(0.0, 0.0, 0.0);
    vec3 norm_i;

    // --- 5 vagues de Gerstner ---
    pos  += gerstnerWave(aPosition.xz, 0.9,  40, 1.5,
                         normalize(vec2(1.0,  0.3)), uTime, norm_i);
    norm += norm_i;

    pos  += gerstnerWave(aPosition.xz, 0.55,  27.0, 1.2,
                         normalize(vec2(0.7,  1.0)), uTime, norm_i);
    norm += norm_i;

    pos  += gerstnerWave(aPosition.xz, 0.3,  16.0, 0.9,
                         normalize(vec2(-0.4, 0.8)), uTime, norm_i);
    norm += norm_i;

    pos  += gerstnerWave(aPosition.xz, 0.12, 7.0, 1.8,
                         normalize(vec2(0.9, -0.2)), uTime, norm_i);
    norm += norm_i;

    pos  += gerstnerWave(aPosition.xz, 0.075,  4.5, 2.2,
                         normalize(vec2(-0.6, -0.7)), uTime, norm_i);
    norm += norm_i;

    // --- Perturbation locale par hash ---
    // Cellules de ~25 unités monde (200/8 = 25)
    vec2  cell     = floor(aPosition.xz * 0.04);
    float local_id = hash(cell);
    // Pulse lent et déphasé par cellule (période ~25s)
    float pulse    = sin(uTime * 0.25 + local_id * 6.2832);
    // Actif sur ~50% des cellules, amplitude max 0.8 (cohérent avec Gerstner)
    float extra    = 1.5 * pulse * step(0.3, local_id);
    pos.y += extra;

    // Normale finale normalisée
    vNormal     = normalize(norm);
    vWaveHeight = pos.y;
    vUV         = aUV;
    vWorldPos   = (uModel * vec4(pos, 1.0)).xyz;

    gl_Position = uProjection * uView * uModel * vec4(pos, 1.0);
}
