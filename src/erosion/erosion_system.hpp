#pragma once
#include "hydraulic_erosion.hpp"
#include <vector>
#include <cstdint>

struct ThermalErosionParams {
    int   iterations  = 12;
    float talusAngle  = 35.0f;
    float erosionRate = 0.5f;
};

struct ErosionParams {
    bool enabled = false;
    ThermalErosionParams thermal;
    RainErosionParams    rain;
};

class ErosionSystem {
public:
    void apply(std::vector<float>& heights, int N,
               float worldSize, const ErosionParams& p,
               uint32_t seed = 42);
private:
    void applyThermal(std::vector<float>& h, int N,
                      float worldSize, const ThermalErosionParams& p);
};