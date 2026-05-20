#include "erosion_system.hpp"
#include <cmath>
#include <algorithm>
#include <vector>

void ErosionSystem::applyThermal(std::vector<float>& h, int N,
                                  float worldSize,
                                  const ThermalErosionParams& p) {
    const float cellSize  = worldSize / float(N - 1);
    const float threshold = std::tan(p.talusAngle * 3.14159265f / 180.0f) * cellSize;

    static const int DX[4] = { 1,-1, 0, 0};
    static const int DZ[4] = { 0, 0, 1,-1};

    std::vector<float> delta(N * N, 0.0f);

    for (int iter = 0; iter < p.iterations; ++iter) {
        std::fill(delta.begin(), delta.end(), 0.0f);

        for (int z = 1; z < N - 1; ++z) {
            for (int x = 1; x < N - 1; ++x) {
                float hc = h[z * N + x];
                float totalDiff = 0.0f;
                for (int d = 0; d < 4; ++d) {
                    float diff = hc - h[(z+DZ[d])*N+(x+DX[d])];
                    if (diff > threshold) totalDiff += (diff - threshold);
                }
                if (totalDiff < 1e-8f) continue;
                for (int d = 0; d < 4; ++d) {
                    int nx2 = x+DX[d], nz2 = z+DZ[d];
                    float diff = hc - h[nz2*N+nx2];
                    if (diff > threshold) {
                        float move = p.erosionRate * 0.5f * (diff - threshold)
                                     * (diff - threshold) / totalDiff;
                        delta[z*N+x]      -= move;
                        delta[nz2*N+nx2]  += move;
                    }
                }
            }
        }
        for (int i = 0; i < N*N; ++i) h[i] += delta[i];
    }
}

void ErosionSystem::apply(std::vector<float>& heights, int N,
                           float worldSize, const ErosionParams& p,
                           uint32_t seed) {
    if (!p.enabled) return;
    applyThermal(heights, N, worldSize, p.thermal);
    HydraulicErosion hydraulic;
    hydraulic.run(heights, N, worldSize, p.rain, seed);
}
