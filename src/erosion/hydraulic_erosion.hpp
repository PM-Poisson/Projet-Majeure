#pragma once
#include <vector>
#include <cstdint>

struct RainErosionParams {
    int   numDroplets    = 30000;
    int   maxSteps       = 80;     // longue durée de vie → transport loin
    float inertia        = 0.5f;   // plus d'inertie → trajectoires plus lisses
    float gravity        = 3.0f;
    float evaporation    = 0.008f; // évaporation lente → dépose loin du sommet
    float capacity       = 8.0f;   // grande capacité → transporte beaucoup
    float erosionRate    = 0.04f;  // érosion faible → ne creuse pas trop
    float depositionRate = 0.6f;   // dépôt fort → aplatit et élargit
    float minSlope       = 0.001f; // seuil très bas → dépose même sur terrain plat
    float brushRadius    = 3.0f;   // grand pinceau → dépôts lisses et étalés
    float seaLevel       = 0.0f;
};

class HydraulicErosion {
public:
    void run(std::vector<float>& heights, int N, float worldSize,
             const RainErosionParams& p, uint32_t seed = 42);
private:
    float    interpolate    (const std::vector<float>& h, int N, float x, float z) const;
    struct Gradient { float gx, gz; };
    Gradient computeGradient(const std::vector<float>& h, int N, float x, float z) const;
    void     applyBrush     (std::vector<float>& h, int N,
                             float cx, float cz, float amount,
                             float radius, float seaLevel) const;
};
