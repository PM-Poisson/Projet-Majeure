#include "hydraulic_erosion.hpp"
#include <random>
#include <cmath>
#include <algorithm>

float HydraulicErosion::interpolate(const std::vector<float>& h, int N,
                                     float x, float z) const {
    int x0=std::clamp((int)x,0,N-2), z0=std::clamp((int)z,0,N-2);
    float tx=x-x0, tz=z-z0;
    float h00=h[z0*N+x0],     h10=h[z0*N+x0+1];
    float h01=h[(z0+1)*N+x0], h11=h[(z0+1)*N+x0+1];
    return (h00*(1-tx)+h10*tx)*(1-tz)+(h01*(1-tx)+h11*tx)*tz;
}

HydraulicErosion::Gradient
HydraulicErosion::computeGradient(const std::vector<float>& h, int N,
                                   float x, float z) const {
    int x0=std::clamp((int)x,0,N-2), z0=std::clamp((int)z,0,N-2);
    float tx=x-x0, tz=z-z0;
    float h00=h[z0*N+x0],     h10=h[z0*N+x0+1];
    float h01=h[(z0+1)*N+x0], h11=h[(z0+1)*N+x0+1];
    return {(h10-h00)*(1-tz)+(h11-h01)*tz,
            (h01-h00)*(1-tx)+(h11-h10)*tx};
}

void HydraulicErosion::applyBrush(std::vector<float>& h, int N,
                                   float cx, float cz, float amount,
                                   float radius, float seaLevel) const {
    int r=(int)std::ceil(radius), ix=(int)cx, iz=(int)cz;
    struct Cell { int idx; float w; };
    std::vector<Cell> cells;
    float tw=0;
    for(int dz=-r;dz<=r;++dz) for(int dx=-r;dx<=r;++dx) {
        int nx=ix+dx, nz=iz+dz;
        if(nx<0||nx>=N||nz<0||nz>=N) continue;
        float d=std::sqrt(float(dx*dx+dz*dz));
        if(d>radius) continue;
        float w=radius-d;
        cells.push_back({nz*N+nx,w}); tw+=w;
    }
    if(tw<1e-8f) return;

    for(auto& c:cells) {
        float delta=amount*(c.w/tw);
        float cur=h[c.idx];
        if(delta>0.0f) {
            if(cur>seaLevel) h[c.idx]=cur+delta;
        } else {
            if(cur>seaLevel) h[c.idx]=std::max(seaLevel, cur+delta);
        }
    }
}

void HydraulicErosion::run(std::vector<float>& heights, int N,
                            float, const RainErosionParams& p, uint32_t seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> rpos(1.0f,(float)(N-2));

    for(int drop=0; drop<p.numDroplets; ++drop) {
        float x=rpos(rng), z=rpos(rng);
        float dx=0, dz=0, speed=1, water=1, sediment=0;

        for(int step=0; step<p.maxSteps; ++step) {
            float h0=interpolate(heights,N,x,z);
            if(h0<=p.seaLevel) break;

            auto [gx,gz]=computeGradient(heights,N,x,z);
            dx=dx*p.inertia-gx*(1-p.inertia);
            dz=dz*p.inertia-gz*(1-p.inertia);
            float len=std::sqrt(dx*dx+dz*dz);
            if(len<1e-6f){
                std::uniform_real_distribution<float> j(-1,1);
                dx=j(rng); dz=j(rng); len=std::sqrt(dx*dx+dz*dz);
            }
            dx/=len; dz/=len;

            float nx2=x+dx, nz2=z+dz;
            if(nx2<1||nx2>=N-2||nz2<1||nz2>=N-2) break;

            float h1=interpolate(heights,N,nx2,nz2);
            float dh=h1-h0;

            float slope=std::max(p.minSlope,-dh);
            float cap=slope*speed*water*p.capacity;

            if(sediment>cap || dh>0.0f) {
                float amt;
                if(dh>0.0f) amt = std::min(sediment, dh);
                else        amt = (sediment-cap)*p.depositionRate;
                amt=std::max(0.f,amt);
                sediment-=amt;
                applyBrush(heights,N,x,z,amt,p.brushRadius,p.seaLevel);
            } else {
                float amt=std::min((cap-sediment)*p.erosionRate, -dh*0.5f);
                amt=std::max(0.f,amt);
                sediment+=amt;
                applyBrush(heights,N,x,z,-amt,p.brushRadius,p.seaLevel);
            }

            speed=std::min(std::sqrt(std::max(0.f,speed*speed+(-dh)*p.gravity)),8.f);
            water*=(1-p.evaporation);
            if(water<0.005f) break;
            x=nx2; z=nz2;
        }
    }
}