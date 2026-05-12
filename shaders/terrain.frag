#version 430 core

// ---------------------------------------------------------------------------
// terrain.frag — Fragment shader du terrain cotier
//
// Colorisation procedurale par couches :
//   < seaLevel            -> fond marin (sable humide / roche sous-marine)
//   seaLevel .. +2m       -> zone intertidale (sable mouille)
//   +2m .. +15m           -> falaise (roche grise, en fonction de la pente)
//   > +15m                -> sommet (herbe ou roche nue selon la pente)
// ---------------------------------------------------------------------------

in vec3  vWorldPos;
in vec3  vNormal;
in vec2  vUV;
in float vHeight;
in float vSlope;

out vec4 fragColor;

// Parametres de scene
uniform float uSeaLevel;
uniform float uHeightScale;    // amplitude max du terrain (pour normaliser)
uniform vec3  uLightDir;       // direction vers la lumiere (normalisee)
uniform vec3  uCameraPos;

// ---------------------------------------------------------------------------
// Utilitaires
// ---------------------------------------------------------------------------

// Melange lineaire entre deux couleurs avec un smooth step
vec3 blend(vec3 a, vec3 b, float t) {
    float s = smoothstep(0.0, 1.0, t);
    return mix(a, b, s);
}

// Bruit simple base sur les UV (simule une micro-texture sans texture sampler)
float cheapNoise(vec2 uv) {
    return fract(sin(dot(uv * 43.0, vec2(127.1, 311.7))) * 43758.5453);
}

float smoothNoise(vec2 uv) {
    vec2 i = floor(uv);
    vec2 f = fract(uv);
    f = f * f * (3.0 - 2.0 * f);
    float a = cheapNoise(i);
    float b = cheapNoise(i + vec2(1.0, 0.0));
    float c = cheapNoise(i + vec2(0.0, 1.0));
    float d = cheapNoise(i + vec2(1.0, 1.0));
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

// ---------------------------------------------------------------------------
// Palette de couleurs
// ---------------------------------------------------------------------------

// Fond marin : roche sombre avec reflets bleu-vert
vec3 colorSeabed(float depth) {
    vec3 rock     = vec3(0.18, 0.20, 0.22);
    vec3 sediment = vec3(0.28, 0.32, 0.30);
    float noise   = smoothNoise(vUV * 80.0) * 0.15;
    return mix(rock, sediment, clamp(depth * 0.3 + noise, 0.0, 1.0));
}

// Zone intertidale : sable mouille brun-dore
vec3 colorIntertidal() {
    vec3 wet    = vec3(0.55, 0.47, 0.32);
    vec3 dry    = vec3(0.76, 0.68, 0.48);
    float noise = smoothNoise(vUV * 120.0) * 0.2;
    return mix(wet, dry, noise);
}

// Falaise : roche calcaire grise avec variation de teinte
vec3 colorCliff(float height) {
    vec3 baseRock  = vec3(0.52, 0.50, 0.47);   // calcaire gris-beige
    vec3 darkRock  = vec3(0.32, 0.30, 0.28);   // roche sombre (humide, algues)
    vec3 lightRock = vec3(0.72, 0.70, 0.66);   // roche claire (soleil)

    float noise1 = smoothNoise(vUV * 40.0);
    float noise2 = smoothNoise(vUV * 200.0) * 0.08;

    vec3 c = mix(darkRock, lightRock, noise1);
    c += noise2;

    // Les zones tres inclinées (vSlope < 0.3) sont plus sombres/humides
    float wetness = 1.0 - smoothstep(0.1, 0.4, vSlope);
    c = mix(c, darkRock * 0.6, wetness * 0.6);

    return c;
}

// Plateau : herbe ou roche nue selon la pente
vec3 colorPlateau(float height) {
    vec3 grass    = vec3(0.28, 0.42, 0.18);
    vec3 soil     = vec3(0.38, 0.30, 0.20);
    vec3 rockBare = vec3(0.60, 0.58, 0.55);

    float noise = smoothNoise(vUV * 60.0);

    // Pente douce -> herbe, pente forte -> roche nue
    vec3 groundBase = mix(grass, soil, noise * 0.4);
    return mix(groundBase, rockBare, smoothstep(0.6, 0.85, 1.0 - vSlope));
}

// ---------------------------------------------------------------------------
// Eclairage Phong simplifie
// ---------------------------------------------------------------------------

vec3 applyLighting(vec3 albedo) {
    vec3  L         = normalize(uLightDir);
    float diff      = max(dot(vNormal, L), 0.0);
    float ambient   = 0.25;

    // Specular (reflets sur la roche mouillée)
    vec3  V         = normalize(uCameraPos - vWorldPos);
    vec3  H         = normalize(L + V);
    float spec      = pow(max(dot(vNormal, H), 0.0), 32.0)
                      * (1.0 - vSlope) * 0.3;  // seulement sur les falaises

    return albedo * (ambient + diff * 0.75) + vec3(spec);
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

void main() {
    float h = vHeight;   // hauteur locale

    vec3 albedo;

    if (h < uSeaLevel - 1.0) {
        // Fond marin profond
        float depth = uSeaLevel - h;
        albedo = colorSeabed(depth);

    } else if (h < uSeaLevel + 2.0) {
        // Zone intertidale : transition fond -> plage
        float t = (h - (uSeaLevel - 1.0)) / 3.0;
        albedo  = mix(colorSeabed(1.0), colorIntertidal(), smoothstep(0.0, 1.0, t));

    } else if (h < uSeaLevel + 15.0) {
        // Falaise
        float t = (h - (uSeaLevel + 2.0)) / 13.0;
        // A la base de la falaise : transition plage -> roche
        albedo  = mix(colorIntertidal(), colorCliff(h), smoothstep(0.0, 0.3, t));

    } else {
        // Plateau / sommet
        float t = clamp((h - (uSeaLevel + 15.0)) / 10.0, 0.0, 1.0);
        albedo  = mix(colorCliff(h), colorPlateau(h), smoothstep(0.0, 1.0, t));
    }

    // Eclairage
    vec3 color = applyLighting(albedo);

    // Legere brume atmospherique sur les grandes distances
    float dist = length(uCameraPos - vWorldPos);
    float fog  = exp(-dist * 0.002);
    vec3  fogColor = vec3(0.72, 0.80, 0.88);
    color = mix(fogColor, color, fog);

    fragColor = vec4(color, 1.0);
}
