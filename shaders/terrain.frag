#version 430 core

// ---------------------------------------------------------------------------
// terrain.frag
//
// Colorisation par biomes selon la hauteur RELATIVE au heightScale :
//   - Fond marin      : h < seaLevel
//   - Plage sableuse  : juste au-dessus de seaLevel (bande etroite)
//   - Rochers humides : transition plage -> falaise (pente forte)
//   - Falaise         : pente forte, hauteur moyenne
//   - Herbe / plateau : sommet, pente faible
//   - Roche nue       : sommet, pente forte
//
// Les seuils sont exprimes en fraction de heightScale pour etre
// independants des parametres de generation.
// ---------------------------------------------------------------------------

in vec3  vWorldPos;
in vec3  vNormal;
in vec2  vUV;
in float vHeight;    // hauteur Y en unites monde
in float vSlope;     // dot(normal, up) : 1=plat, 0=vertical

out vec4 fragColor;

uniform float uSeaLevel;
uniform float uHeightScale;  // amplitude max du terrain (pour normaliser les seuils)
uniform vec3  uLightDir;
uniform vec3  uCameraPos;

// ---------------------------------------------------------------------------
// Micro-texture procedurale (evite les textures externes)
// ---------------------------------------------------------------------------
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}
float noise2(vec2 p) {
    vec2 i = floor(p), f = fract(p);
    f = f*f*(3.0-2.0*f);
    return mix(mix(hash(i),          hash(i+vec2(1,0)), f.x),
               mix(hash(i+vec2(0,1)),hash(i+vec2(1,1)), f.x), f.y);
}
// Deux octaves pour plus de detail
float tex(vec2 uv, float freq) {
    return noise2(uv*freq)*0.65 + noise2(uv*freq*2.3)*0.35;
}

// ---------------------------------------------------------------------------
// Palettes de biomes
// ---------------------------------------------------------------------------

// Fond marin : roche sombre, plus claire en eau peu profonde
vec3 colorSeabed(float relDepth) {
    // relDepth : 0 = juste sous la surface, 1 = profond
    vec3 shallow = vec3(0.22, 0.28, 0.30);
    vec3 deep    = vec3(0.10, 0.14, 0.18);
    float n = tex(vUV, 60.0) * 0.2;
    return mix(shallow, deep, clamp(relDepth + n, 0.0, 1.0));
}

// Plage sableuse : sable dore, plus humide pres de l'eau
vec3 colorBeach(float relH) {
    // relH : 0 = bord de l'eau, 1 = haut de la plage
    vec3 wet  = vec3(0.62, 0.54, 0.36);   // sable mouille
    vec3 dry  = vec3(0.82, 0.74, 0.52);   // sable sec
    float n   = tex(vUV, 90.0) * 0.15;
    return mix(wet, dry, clamp(relH + n, 0.0, 1.0));
}

// Rochers de la cote : roche sombre avec algues
vec3 colorCoastalRock() {
    vec3 base   = vec3(0.35, 0.33, 0.30);
    vec3 algae  = vec3(0.22, 0.30, 0.18);   // algues vertes
    vec3 bright = vec3(0.58, 0.55, 0.50);
    float n = tex(vUV, 45.0);
    vec3 c = mix(base, bright, n);
    // Algues dans les zones les plus basses / humides
    float wetness = 1.0 - smoothstep(0.1, 0.45, vSlope);
    return mix(c, algae, wetness * 0.55);
}

// Falaise : calcaire / granite gris, texture verticale
vec3 colorCliff() {
    vec3 light = vec3(0.68, 0.65, 0.60);
    vec3 dark  = vec3(0.30, 0.28, 0.25);
    float n = tex(vUV, 35.0);
    vec3 c = mix(dark, light, n);
    // Stries verticales (anisotropie de la roche)
    float stripe = noise2(vUV * vec2(5.0, 80.0)) * 0.08;
    c += stripe;
    // Zones tres inclinées plus sombres / humides
    float wetness = 1.0 - smoothstep(0.05, 0.35, vSlope);
    return mix(c, dark * 0.55, wetness * 0.5);
}

// Herbe : vert avec variation de sol et rochers selon pente
vec3 colorGrass() {
    vec3 grass    = vec3(0.24, 0.40, 0.14);
    vec3 soil     = vec3(0.40, 0.30, 0.18);
    vec3 drygrass = vec3(0.52, 0.50, 0.28);  // herbe seche / haute
    float n = tex(vUV, 55.0);
    return mix(mix(grass, drygrass, n*0.4), soil, (1.0-n)*0.25);
}

// Roche nue des sommets / pentes fortes en altitude
vec3 colorSummitRock() {
    vec3 base  = vec3(0.55, 0.52, 0.48);
    vec3 light = vec3(0.75, 0.72, 0.68);
    float n = tex(vUV, 30.0);
    return mix(base, light, n * 0.6);
}

// ---------------------------------------------------------------------------
// Eclairage Blinn-Phong
// ---------------------------------------------------------------------------
vec3 applyLighting(vec3 albedo) {
    vec3  L    = normalize(uLightDir);
    vec3  V    = normalize(uCameraPos - vWorldPos);
    vec3  H    = normalize(L + V);
    float diff = max(dot(vNormal, L), 0.0);
    // Speculaire uniquement sur les surfaces humides / rocheuses
    float specPow = mix(8.0, 64.0, 1.0 - vSlope);
    float spec    = pow(max(dot(vNormal, H), 0.0), specPow) * 0.25 * (1.0 - vSlope);
    return albedo * (0.22 + diff * 0.78) + vec3(spec);
}

// ---------------------------------------------------------------------------
// Main
//
// Seuils relatifs a heightScale :
//   fond marin        : h < seaLevel
//   plage             : seaLevel .. seaLevel + 0.06 * heightScale
//   rocher cotier     : seaLevel + 0.06*hs .. seaLevel + 0.18*hs  (si pente forte)
//   falaise / pente   : seaLevel + 0.06*hs .. seaLevel + 0.50*hs  (si pente forte)
//   transition herbe  : seaLevel + 0.35*hs .. seaLevel + 0.55*hs
//   plateau herbeux   : > seaLevel + 0.55 * heightScale
//
// La pente (vSlope) intervient pour separer falaise et plage en pente douce.
// ---------------------------------------------------------------------------
void main() {
    float h   = vHeight;
    float hs  = uHeightScale;
    float sea = uSeaLevel;

    // Seuils absolus derives de heightScale
    float beachTop  = sea + 0.07 * hs;   // haut de la plage
    float rockTop   = sea + 0.22 * hs;   // haut des rochers cotiers
    float cliffTop  = sea + 0.50 * hs;   // bas du plateau
    float grassBot  = sea + 0.42 * hs;   // debut transition herbe

    vec3 albedo;

    if (h < sea) {
        // --- Fond marin ---
        float depth = clamp((sea - h) / (0.5 * hs), 0.0, 1.0);
        albedo = colorSeabed(depth);

    } else if (h < beachTop) {
        // --- Plage ---
        // Selon la pente : sable si plat, rocher si pentu
        float relH   = (h - sea) / (beachTop - sea);   // [0,1] dans la bande de plage
        float slopeBlend = smoothstep(0.55, 0.80, vSlope);  // 1=plat, 0=pentu
        vec3 sand  = colorBeach(relH);
        vec3 crock = colorCoastalRock();
        albedo = mix(crock, sand, slopeBlend);

    } else if (h < rockTop) {
        // --- Rochers cotiers / falaise basse ---
        float t = (h - beachTop) / (rockTop - beachTop);
        // En bas et pentu : rochers humides ; plus haut : falaise seche
        albedo = mix(colorCoastalRock(), colorCliff(), smoothstep(0.2, 0.8, t));

    } else if (h < cliffTop) {
        // --- Falaise ---
        // Zone de transition vers le plateau : si pente douce, de l'herbe commence
        float t         = (h - rockTop) / (cliffTop - rockTop);
        float grassHere = smoothstep(grassBot, cliffTop, h) * smoothstep(0.3, 0.7, vSlope);
        vec3 cliff = colorCliff();
        vec3 grass = colorGrass();
        albedo = mix(cliff, grass, grassHere * t);

    } else {
        // --- Plateau ---
        // Herbe sur les zones plates, roche nue sur les pentes
        float rockFactor = smoothstep(0.55, 0.80, 1.0 - vSlope);
        float n = tex(vUV, 40.0) * 0.1;
        albedo = mix(colorGrass(), colorSummitRock(), rockFactor + n);
    }

    // Eclairage
    vec3 color = applyLighting(albedo);

    // Brume atmospherique legere
    float dist = length(uCameraPos - vWorldPos);
    float fog  = exp(-dist * 0.0015);
    color = mix(vec3(0.70, 0.78, 0.88), color, fog);

    fragColor = vec4(color, 1.0);
}
