#version 430 core

// ---------------------------------------------------------------------------
// ocean.vert
//
// Simulation de vagues par superposition de vagues de Gerstner.
// Chaque vague a une direction, longueur d'onde, amplitude et vitesse propres.
//
// Repere : XZ = plan horizontal, Y = hauteur (convention de votre projet)
// ---------------------------------------------------------------------------

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
// Une vague de Gerstner
//
// Parametres :
//   p         : position XZ du sommet (avant deformation)
//   amplitude : hauteur max de la vague
//   wavelength: longueur d'onde (metres)
//   speed     : vitesse de propagation
//   direction : direction de propagation (vecteur XZ normalise)
//
// Retourne la hauteur Y et modifie pos pour le deplacement horizontal (effet
// Gerstner : les sommets decrivent des cercles, pas juste monter/descendre)
// ---------------------------------------------------------------------------
vec3 gerstnerWave(vec2 p, float amplitude, float wavelength,
                  float speed, vec2 direction,
                  float time, out vec3 normal_contrib)
{
    float k     = 2.0 * 3.14159265 / wavelength;   // nombre d'onde
    float c     = speed;                             // vitesse de phase
    float f     = k * dot(direction, p) - c * time; // phase
    float Q     = 0.6;                               // raideur (0=sinusoide, 1=Gerstner pur)

    // Deplacement de Gerstner :
    //   horizontal (XZ) : Q * A * direction * cos(f)
    //   vertical   (Y)  :     A             * sin(f)
    float sinf  = sin(f);
    float cosf  = cos(f);

    vec3 disp;
    disp.x = Q * amplitude * direction.x * cosf;
    disp.y = amplitude * sinf;
    disp.z = Q * amplitude * direction.y * cosf;

    // Contribution a la normale analytique (derivee de Gerstner)
    normal_contrib = vec3(
        -direction.x * k * amplitude * cosf,
         1.0 - Q * k * amplitude * sinf,
        -direction.y * k * amplitude * cosf
    );

    return disp;
}

void main()
{
    vec3 pos = aPosition;
    vec3 norm = vec3(0.0, 0.0, 0.0);
    vec3 norm_i;

    // --- 5 vagues de Gerstner avec des directions et parametres varies ---
    // Vague principale (longue, vient du large)
    pos  += gerstnerWave(aPosition.xz, 1.8,  80.0, 1.5,
                         normalize(vec2(1.0,  0.3)), uTime, norm_i);
    norm += norm_i;

    // Vague secondaire (croise la principale)
    pos  += gerstnerWave(aPosition.xz, 1.1,  55.0, 1.2,
                         normalize(vec2(0.7,  1.0)), uTime, norm_i);
    norm += norm_i;

    // Vague tertiaire (direction opposee, interference)
    pos  += gerstnerWave(aPosition.xz, 0.6,  32.0, 0.9,
                         normalize(vec2(-0.4, 0.8)), uTime, norm_i);
    norm += norm_i;

    // Vaguelette haute frequence (detail de surface)
    pos  += gerstnerWave(aPosition.xz, 0.25, 14.0, 1.8,
                         normalize(vec2(0.9, -0.2)), uTime, norm_i);
    norm += norm_i;

    // Vaguelette supplementaire (texture de surface)
    pos  += gerstnerWave(aPosition.xz, 0.15,  9.0, 2.2,
                         normalize(vec2(-0.6, -0.7)), uTime, norm_i);
    norm += norm_i;

    // Normale finale normalisee
    vNormal     = normalize(norm);
    vWaveHeight = pos.y;
    vUV         = aUV;
    vWorldPos   = (uModel * vec4(pos, 1.0)).xyz;

    gl_Position = uProjection * uView * uModel * vec4(pos, 1.0);
}
