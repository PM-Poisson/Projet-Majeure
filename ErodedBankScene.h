#pragma once

#include <QWidget>
#include <QPainter>
#include <QPolygonF>
#include <vector>
#include <cmath>

class ErodedBankScene : public QWidget
{
    Q_OBJECT

public:
    explicit ErodedBankScene(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    // --- Noise & geometry helpers ---
    double smooth(double t) const { return t * t * (3 - 2 * t); }

    double lerp(double a, double b, double t) const { return a + (b - a) * t; }

    // Simple 1-D value noise
    double valueNoise(double x, int seed = 0) const
    {
        int xi = static_cast<int>(std::floor(x));
        double xf = x - xi;
        auto hash = [&](int n) -> double {
            n = (n ^ (seed * 127 + 1)) * 1664525 + 1013904223;
            n = (n >> 16) ^ (n * 1664525);
            return (n & 0x7fffffff) / static_cast<double>(0x7fffffff);
        };
        return lerp(hash(xi), hash(xi + 1), smooth(xf));
    }

    double fbm(double x, int octaves, double freq, double amp, int seed = 0) const
    {
        double val = 0.0, a = amp, f = freq;
        for (int i = 0; i < octaves; ++i) {
            val += a * valueNoise(x * f, seed + i * 31);
            a   *= 0.5;
            f   *= 2.1;
        }
        return val;
    }

    // --- Drawing helpers ---
    void drawSky(QPainter &p, const QRectF &r);
    void drawDistantTerrain(QPainter &p, int W, int H);
    void drawErodedBank(QPainter &p, int W, int H);
    void drawWater(QPainter &p, int W, int H);
    void drawPebbles(QPainter &p, int W, int H);
    void drawVegetation(QPainter &p, int W, int H);
    void drawExposedRoots(QPainter &p, int W, int H);
    void drawFoam(QPainter &p, int W, int H);

    // Returns the surface Y of the eroded bank at column x
    // (the top of the water-cut undercut profile)
    double bankSurface(double xNorm) const;

    // Returns the undercut bottom Y (ceiling of the cavity)
    double bankUndercut(double xNorm) const;

    // Bedrock profile
    double bedrockY(double xNorm) const;

    int m_W{1200}, m_H{700};
    int m_waterY{0};   // filled in during paintEvent
};
