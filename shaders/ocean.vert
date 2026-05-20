#version 430 core

// ---------------------------------------------------------------------------
// ocean.vert — Vertex shader de l'océan avec vagues de Gerstner
//
// Le plan de base de l'océan est une grille XZ plate (Y=0).
// Ce shader anime chaque sommet par superposition de 5 vagues de Gerstner,
// plus réalistes que de simples sinusoïdes car les sommets décrivent des
// cercles (déplacement horizontal + vertical) au lieu de monter/descendre.
//
// Chaque vague de Gerstner est définie par :
//   - amplitude A    : hauteur max de la vague
//   - wavelength L   : longueur d'onde (en unités monde)
//   - speed c        : vitesse de propagation
//   - direction d    : vecteur XZ unitaire de propagation
//   - raideur Q      : 0 = sinusoïde pure, 1 = Gerstner pur (crêtes pointues)
//
// Les 5 vagues combinées donnent :
//   - une vague principale longue (effet de houle)
//   - une vague secondaire croisée (interférences)
//   - une vague tertiaire (complexité)
//   - deux vaguelettes haute fréquence (texture de surface)
// ---------------------------------------------------------------------------

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;

uniform mat4  uModel;
uniform mat4  uView;
uniform mat4  uProjection;
uniform float uTime;        // temps écoulé en secondes (pour l'animation)

out vec3  vWorldPos;
out vec3  vNormal;
out vec2  vUV;
out float vWaveHeight;   // hauteur Y totale des vagues (pour la couleur)

// ---------------------------------------------------------------------------
// gerstnerWave — calcule le déplacement et la contribution à la normale
// pour une vague de Gerstner unique
//
// Retourne le déplacement (dx, dy, dz).
// norm_contrib : contribution à la normale analytique (dérivée de Gerstner).
// ---------------------------------------------------------------------------
vec3 gerstnerWave(vec2 p, float amplitude, float wavelength,
                  float speed, vec2 direction,
                  float time, out vec3 norm_contrib)
{
    float k    = 2.0 * 3.14159265 / wavelength;  // nombre d'onde
    float f    = k * dot(direction, p) - speed * time;  // phase courante
    float Q    = 0.6;   // raideur : 0 = sinus, 1 = Gerstner pur

    float sinf = sin(f);
    float cosf = cos(f);

    // Déplacement de Gerstner :
    //   horizontal (XZ) : Q*A*d*cos(f)  — les crêtes se "penchent" en avant
    //   vertical   (Y)  : A*sin(f)
    vec3 disp = vec3(
        Q * amplitude * direction.x * cosf,
        amplitude * sinf,
        Q * amplitude * direction.y * cosf
    );

    // Normale analytique (dérivée partielle de la surface déformée)
    norm_contrib = vec3(
        -direction.x * k * amplitude * cosf,
         1.0 - Q * k * amplitude * sinf,
        -direction.y * k * amplitude * cosf
    );

    return disp;
}

void main() {
    vec3 pos  = aPosition;
    vec3 norm = vec3(0.0);
    vec3 norm_i;

    // 5 vagues superposées avec des paramètres variés pour un résultat naturel
    pos  += gerstnerWave(aPosition.xz, 1.8,  80.0, 1.5, normalize(vec2( 1.0,  0.3)), uTime, norm_i); norm += norm_i;
    pos  += gerstnerWave(aPosition.xz, 1.1,  55.0, 1.2, normalize(vec2( 0.7,  1.0)), uTime, norm_i); norm += norm_i;
    pos  += gerstnerWave(aPosition.xz, 0.6,  32.0, 0.9, normalize(vec2(-0.4,  0.8)), uTime, norm_i); norm += norm_i;
    pos  += gerstnerWave(aPosition.xz, 0.25, 14.0, 1.8, normalize(vec2( 0.9, -0.2)), uTime, norm_i); norm += norm_i;
    pos  += gerstnerWave(aPosition.xz, 0.15,  9.0, 2.2, normalize(vec2(-0.6, -0.7)), uTime, norm_i); norm += norm_i;

    vNormal     = normalize(norm);
    vWaveHeight = pos.y;     // hauteur totale après déformation
    vUV         = aUV;
    vWorldPos   = (uModel * vec4(pos, 1.0)).xyz;

    gl_Position = uProjection * uView * uModel * vec4(pos, 1.0);
}
