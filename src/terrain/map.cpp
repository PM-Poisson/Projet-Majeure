#include "map.hpp"
#include <algorithm>
#include <cmath>

float Heightmap::sample(float x, float y) const {
    // Clamp aux bords
    x = std::clamp(x, 0.0f, (float)(width  - 1));
    y = std::clamp(y, 0.0f, (float)(height - 1));

    int   x0 = (int)x, y0 = (int)y;
    int   x1 = std::min(x0 + 1, width  - 1);
    int   y1 = std::min(y0 + 1, height - 1);
    float tx = x - (float)x0;
    float ty = y - (float)y0;

    float v00 = at(x0, y0);
    float v10 = at(x1, y0);
    float v01 = at(x0, y1);
    float v11 = at(x1, y1);

    // Interpolation bilinéaire
    return (1-tx)*(1-ty)*v00 + tx*(1-ty)*v10
          +(1-tx)*   ty *v01 + tx*   ty *v11;
}

// glm::vec3 Heightmap::normalAt(int x, int y, float verticalScale) const {
//     // Différences finies centrées (ou unilatérales aux bords)
//     int xl = std::max(x - 1, 0);
//     int xr = std::min(x + 1, width  - 1);
//     int yd = std::max(y - 1, 0);
//     int yu = std::min(y + 1, height - 1);

//     float hL = at(xl, y)  * verticalScale;
//     float hR = at(xr, y)  * verticalScale;
//     float hD = at(x,  yd) * verticalScale;
//     float hU = at(x,  yu) * verticalScale;

//     // Gradient → normale
//     glm::vec3 n = glm::normalize(glm::vec3(hL - hR, 2.0f, hD - hU));
//     return n;
// }