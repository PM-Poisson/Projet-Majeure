#pragma once
#include "hydraulic_erosion.hpp"
#include <vector>
#include <cstdint>

struct ThermalErosionParams {
    int   iterations  = 12;      // passes par cycle (moins par cycle, mais plusieurs cycles)
    float talusAngle  = 30.0f;  // angle de repos (degrés) — abaissé pour île humide/volcanique
    float erosionRate = 0.5f;
    int   cycles      = 5;      // nombre de cycles pluie→thermique (plus = plus lisse)
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