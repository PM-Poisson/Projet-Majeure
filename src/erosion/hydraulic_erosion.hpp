#pragma once
#include <vector>
#include <cstdint>

struct RainErosionParams {
    int   numDroplets    = 30000;
    int   maxSteps       = 80;
    float inertia        = 0.5f;
    float gravity        = 3.0f;
    float evaporation    = 0.008f;
    float capacity       = 8.0f;
    float erosionRate    = 0.15f;
    float depositionRate = 0.60f;
    float minSlope       = 0.001f;
    float brushRadius    = 3.0f;
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