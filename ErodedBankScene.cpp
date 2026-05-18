#include "ErodedBankScene.h"
#include <QPainterPath>
#include <QPen>
#include <QBrush>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QtMath>
#include <QRandomGenerator>

// ─── Layer colours (inspired by real sediment stratigraphy) ────────────────
static const QColor kSky1       {102, 178, 230};
static const QColor kSky2       { 30,  80, 140};
static const QColor kGrass      { 55,  90,  30};
static const QColor kGrassDark  { 35,  60,  20};
static const QColor kTopsoil    { 80,  50,  20};
static const QColor kSand       {190, 160,  90};
static const QColor kSandDark   {160, 130,  70};
static const QColor kClay       {140, 100,  60};
static const QColor kBedrock    { 80,  70,  60};
static const QColor kBedrockDark{ 55,  50,  45};
static const QColor kWater1     { 30,  90, 150, 200};
static const QColor kWater2     { 10,  50, 110, 230};
static const QColor kFoam       {220, 235, 255, 180};
static const QColor kPebble1    {120, 110, 100};
static const QColor kPebble2    {160, 148, 130};
static const QColor kRoot       {100,  65,  30};

ErodedBankScene::ErodedBankScene(QWidget *parent) : QWidget(parent)
{
    setMinimumSize(800, 500);
}

// ─── Profile functions ──────────────────────────────────────────────────────

double ErodedBankScene::bankSurface(double xNorm) const
{
    // Top of the bank (grass/topsoil surface)
    // xNorm in [0,1], 0 = left, 1 = right
    double base = 0.30 * m_H;
    // Gentle undulation + some collapse near the water edge
    double noise = fbm(xNorm * 4.0, 4, 1.0, 1.0, 0);
    double collapse = 0.0;
    // Near the right side (water edge) the bank dips due to erosion
    if (xNorm > 0.55) {
        double t = (xNorm - 0.55) / 0.45;
        collapse = t * t * 0.12 * m_H;
    }
    return base + noise * 0.04 * m_H + collapse;
}

double ErodedBankScene::bankUndercut(double xNorm) const
{
    // The ceiling of the hydraulic undercut cavity near the waterline
    // Only relevant in right portion of the scene (0.45 to 1.0)
    double waterLine = m_waterY;
    double noise = fbm(xNorm * 6.0 + 10.0, 3, 1.0, 1.0, 7);
    return waterLine - 0.06 * m_H - noise * 0.03 * m_H;
}

double ErodedBankScene::bedrockY(double xNorm) const
{
    double base = 0.72 * m_H;
    double noise = fbm(xNorm * 3.0 + 5.0, 3, 1.0, 1.0, 3);
    return base + noise * 0.04 * m_H;
}

// ─── paint ──────────────────────────────────────────────────────────────────

void ErodedBankScene::paintEvent(QPaintEvent *)
{
    m_W = width();
    m_H = height();
    m_waterY = static_cast<int>(0.58 * m_H);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    drawSky(p, QRectF(0, 0, m_W, m_H));
    drawDistantTerrain(p, m_W, m_H);
    drawErodedBank(p, m_W, m_H);
    drawWater(p, m_W, m_H);
    drawFoam(p, m_W, m_H);
    drawPebbles(p, m_W, m_H);
    drawExposedRoots(p, m_W, m_H);
    drawVegetation(p, m_W, m_H);
}

// ─── Sky ────────────────────────────────────────────────────────────────────

void ErodedBankScene::drawSky(QPainter &p, const QRectF &r)
{
    QLinearGradient sky(r.topLeft(), r.bottomLeft());
    sky.setColorAt(0.0, QColor(15, 55, 120));
    sky.setColorAt(0.35, QColor(70, 140, 210));
    sky.setColorAt(0.7, QColor(145, 195, 235));
    sky.setColorAt(1.0, QColor(185, 215, 240));
    p.fillRect(r, sky);

    // Sun glow
    QRadialGradient sun(m_W * 0.15, m_H * 0.12, m_H * 0.25);
    sun.setColorAt(0.0, QColor(255, 245, 200, 120));
    sun.setColorAt(0.4, QColor(255, 220, 100, 50));
    sun.setColorAt(1.0, Qt::transparent);
    p.fillRect(QRectF(0, 0, m_W, m_H * 0.5), sun);

    // Soft clouds
    p.setPen(Qt::NoPen);
    auto drawCloud = [&](double cx, double cy, double rx, double ry, int alpha) {
        for (int i = -2; i <= 2; ++i) {
            double x = cx + i * rx * 0.4;
            double y = cy - std::abs(i) * ry * 0.3;
            QRadialGradient cg(x, y, rx * 0.7);
            cg.setColorAt(0.0, QColor(255, 255, 255, alpha));
            cg.setColorAt(1.0, Qt::transparent);
            p.setBrush(cg);
            p.drawEllipse(QPointF(x, y), rx * 0.7, ry * 0.7);
        }
    };
    drawCloud(m_W * 0.25, m_H * 0.10, 90, 28, 100);
    drawCloud(m_W * 0.60, m_H * 0.07, 70, 22, 80);
    drawCloud(m_W * 0.82, m_H * 0.13, 55, 18, 70);
}

// ─── Distant terrain (opposite bank) ────────────────────────────────────────

void ErodedBankScene::drawDistantTerrain(QPainter &p, int W, int H)
{
    // Far hills
    QPainterPath hills;
    hills.moveTo(0, H);
    int steps = 200;
    for (int i = 0; i <= steps; ++i) {
        double t  = i / static_cast<double>(steps);
        double y  = 0.32 * H + fbm(t * 5.0 + 20.0, 4, 1.0, 1.0, 9) * 0.08 * H;
        if (i == 0) hills.lineTo(0, y);
        else        hills.lineTo(t * W, y);
    }
    hills.lineTo(W, H);
    hills.closeSubpath();

    QLinearGradient hillGrad(0, 0.25 * H, 0, 0.40 * H);
    hillGrad.setColorAt(0.0, QColor(60, 100, 55, 180));
    hillGrad.setColorAt(1.0, QColor(40,  75, 40, 200));
    p.fillPath(hills, hillGrad);

    // Distant treeline silhouette
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(35, 65, 30, 200));
    int treeCount = 60;
    // seed deterministic
    for (int i = 0; i < treeCount; ++i) {
        double tx = (i + 0.5) / treeCount;
        double tyBase = 0.29 * H + fbm(tx * 5.0 + 20.0, 4, 1.0, 1.0, 9) * 0.08 * H;
        double th = (0.05 + fbm(tx * 12.0 + 3.0, 2, 1.0, 1.0, 11) * 0.07) * H;
        double tw = th * (0.4 + fbm(tx * 8.0 + 1.0, 1, 1.0, 1.0, 13) * 0.3);
        // Pine silhouette (triangle)
        QPolygonF tri;
        tri << QPointF(tx * W, tyBase)
            << QPointF(tx * W - tw / 2, tyBase + th)
            << QPointF(tx * W + tw / 2, tyBase + th);
        p.drawPolygon(tri);
    }
}

// ─── Eroded bank (main attraction) ──────────────────────────────────────────

void ErodedBankScene::drawErodedBank(QPainter &p, int W, int H)
{
    const int steps = W;

    // Helper: build a profile polygon from two arrays
    auto makePolyBetween = [&](
        const std::vector<double> &topY,
        const std::vector<double> &botY,
        int from, int to) -> QPainterPath
    {
        QPainterPath path;
        path.moveTo(from, topY[from]);
        for (int x = from; x <= to; ++x)
            path.lineTo(x, topY[x]);
        for (int x = to; x >= from; --x)
            path.lineTo(x, botY[x]);
        path.closeSubpath();
        return path;
    };

    // --- Build all Y profiles ---
    std::vector<double> surfY(steps + 1), bedrY(steps + 1), sandY(steps + 1),
                    clayY(steps + 1), undercutY(steps + 1);

    for (int x = 0; x <= steps; ++x) {
        double t  = x / static_cast<double>(W);
        surfY[x]  = bankSurface(t);
        bedrY[x]  = bedrockY(t);
        // Sand layer: midway between surface and bedrock, shifted up
        sandY[x]  = lerp(surfY[x], bedrY[x], 0.45 + fbm(t * 5.0, 2, 1.0, 1.0, 17) * 0.08);
        clayY[x]  = lerp(surfY[x], bedrY[x], 0.25 + fbm(t * 4.0 + 2.0, 2, 1.0, 1.0, 21) * 0.06);
        undercutY[x] = bankUndercut(t);
    }

    // --- 1. Bedrock layer (darkest, bottom) ---
    {
        QPainterPath path;
        path.moveTo(0, H);
        for (int x = 0; x <= steps; ++x) path.lineTo(x, bedrY[x]);
        path.lineTo(W, H); path.closeSubpath();
        QLinearGradient g(0, 0.65 * H, 0, H);
        g.setColorAt(0.0, kBedrockDark);
        g.setColorAt(1.0, QColor(40, 35, 30));
        p.fillPath(path, g);
    }

    // --- 2. Sand/silt layer ---
    {
        QPainterPath path = makePolyBetween(sandY, bedrY, 0, steps);
        QLinearGradient g(0, 0.5 * H, 0, 0.72 * H);
        g.setColorAt(0.0, kSand);
        g.setColorAt(1.0, kSandDark);
        p.fillPath(path, g);

        // Horizontal stratification lines
        p.setPen(QPen(QColor(140, 115, 65, 100), 1.0));
        int nLines = 6;
        for (int l = 1; l < nLines; ++l) {
            double frac = l / static_cast<double>(nLines);
            QPainterPath line;
            bool first = true;
            for (int x = 0; x <= steps; ++x) {
                double y = lerp(sandY[x], bedrY[x], frac) + fbm(x * 0.02 + l, 2, 1.0, 1.0, 50+l) * 3.0;
                if (first) { line.moveTo(x, y); first = false; }
                else        line.lineTo(x, y);
            }
            p.drawPath(line);
        }
    }

    // --- 3. Clay layer ---
    {
        QPainterPath path = makePolyBetween(clayY, sandY, 0, steps);
        QLinearGradient g(0, 0.35 * H, 0, 0.55 * H);
        g.setColorAt(0.0, QColor(160, 110, 65));
        g.setColorAt(1.0, kClay);
        p.fillPath(path, g);
    }

    // --- 4. Topsoil layer ---
    {
        QPainterPath path = makePolyBetween(surfY, clayY, 0, steps);
        QLinearGradient g(0, 0.25 * H, 0, 0.45 * H);
        g.setColorAt(0.0, QColor(60, 38, 15));
        g.setColorAt(1.0, kTopsoil);
        p.fillPath(path, g);
    }

    // --- 5. Hydraulic undercut cavity (eroded hollow near waterline) ---
    // This is the signature feature of hydraulic bank erosion from the paper
    {
        // The undercut starts around x = 0.45*W and deepens to the right
        // It carves the clay/sand layer from below at water level
        int startX = static_cast<int>(0.38 * W);
        QPainterPath undercutPath;
        undercutPath.moveTo(startX, undercutY[startX]);
        for (int x = startX; x <= steps; ++x)
            undercutPath.lineTo(x, undercutY[x]);
        // Bottom of cavity is the waterline + some depth
        for (int x = steps; x >= startX; --x) {
            double bot = m_waterY + fbm(x * 0.01 + 8.0, 2, 1.0, 1.0, 33) * 0.02 * H;
            undercutPath.lineTo(x, bot);
        }
        undercutPath.closeSubpath();

        // Fill cavity with dark shadow colour
        QLinearGradient cavGrad(0, m_waterY - 0.08 * H, 0, m_waterY + 0.02 * H);
        cavGrad.setColorAt(0.0, QColor(25, 18, 12, 230));
        cavGrad.setColorAt(1.0, QColor(10,  8,  5, 200));
        p.fillPath(undercutPath, cavGrad);

        // Ceiling texture — exposed raw soil
        p.setPen(QPen(QColor(90, 60, 30, 80), 0.8));
        for (int x = startX; x < steps; x += 4) {
            double cx = x + (std::sin(x * 0.3) * 2.0);
            double cy = undercutY[std::min(x, steps)];
            p.drawLine(QPointF(cx, cy), QPointF(cx + 3, cy + fbm(x * 0.05, 2, 1.0, 1.0, 44) * 5.0));
        }
    }

    // --- 6. Grass cap on top ---
    {
        // Grass thickness ~4% of H, irregular
        QPainterPath grassPath;
        grassPath.moveTo(0, surfY[0]);
        for (int x = 1; x <= steps; ++x) {
            double y = surfY[x] - 0.01 * H - fbm(x * 0.05, 3, 1.0, 1.0, 55) * 0.015 * H;
            grassPath.lineTo(x, y);
        }
        for (int x = steps; x >= 0; --x)
            grassPath.lineTo(x, surfY[x]);
        grassPath.closeSubpath();

        QLinearGradient gg(0, 0.25 * H, 0, 0.35 * H);
        gg.setColorAt(0.0, QColor(70, 110, 35));
        gg.setColorAt(1.0, kGrassDark);
        p.fillPath(grassPath, gg);
    }

    // --- 7. Thin dark outline / shadow edge at bank surface ---
    {
        p.setPen(QPen(QColor(20, 12, 5, 160), 1.5));
        QPainterPath outline;
        outline.moveTo(0, surfY[0]);
        for (int x = 1; x <= steps; ++x) outline.lineTo(x, surfY[x]);
        p.drawPath(outline);
    }

    // --- 8. Vertical crack lines (desiccation cracks) ---
    p.setPen(QPen(QColor(50, 30, 10, 90), 0.8));
    for (int c = 0; c < 30; ++c) {
        double tx = 0.02 + fbm(c * 1.7 + 100.0, 1, 1.0, 1.0, 77) * 0.95;
        int cx = static_cast<int>(tx * W);
        double topCy = surfY[cx] + 2.0;
        double botCy = topCy + (8 + fbm(c * 2.3 + 200.0, 1, 1.0, 1.0, 78) * 25.0);
        p.drawLine(QPointF(cx, topCy),
                   QPointF(cx + fbm(c * 0.9, 1, 1.0, 1.0, 79) * 4.0 - 2.0, botCy));
    }
}

// ─── Water ───────────────────────────────────────────────────────────────────

void ErodedBankScene::drawWater(QPainter &p, int W, int H)
{
    int wY = m_waterY;

    // Main water body
    QPainterPath waterPath;
    waterPath.moveTo(0, wY);
    // Gentle surface ripple
    for (int x = 0; x <= W; ++x) {
        double t = x / static_cast<double>(W);
        double ripple = std::sin(t * 18.0 + 1.0) * 1.5 + std::sin(t * 7.3 + 0.5) * 2.0;
        waterPath.lineTo(x, wY + ripple);
    }
    waterPath.lineTo(W, H);
    waterPath.lineTo(0, H);
    waterPath.closeSubpath();

    QLinearGradient wg(0, wY, 0, H);
    wg.setColorAt(0.00, QColor(40, 110, 170, 210));
    wg.setColorAt(0.15, QColor(25,  80, 140, 230));
    wg.setColorAt(0.50, QColor(15,  55, 110, 245));
    wg.setColorAt(1.00, QColor( 8,  35,  75, 255));
    p.fillPath(waterPath, wg);

    // Specular shimmer lines
    p.setPen(QPen(QColor(200, 230, 255, 60), 1.0));
    for (int s = 0; s < 20; ++s) {
        double ty = wY + 5 + s * (H - wY - 5) / 25.0;
        double xStart = fbm(s * 3.1 + 40.0, 1, 1.0, 1.0, 90) * 0.6 * W;
        double xLen   = (0.03 + fbm(s * 2.7 + 41.0, 1, 1.0, 1.0, 91) * 0.12) * W;
        p.drawLine(QPointF(xStart, ty), QPointF(xStart + xLen, ty));
    }

    // Sky reflection band near surface
    QLinearGradient refl(0, wY, 0, wY + 0.06 * H);
    refl.setColorAt(0.0, QColor(100, 170, 220, 80));
    refl.setColorAt(1.0, Qt::transparent);
    p.fillRect(QRectF(0, wY, W, 0.06 * H), refl);
}

// ─── Foam at bank base ───────────────────────────────────────────────────────

void ErodedBankScene::drawFoam(QPainter &p, int W, int /*H*/)
{
    // Foam patches where water meets the bank
    p.setPen(Qt::NoPen);
    for (int i = 0; i < 18; ++i) {
        double tx   = 0.3 + fbm(i * 1.3 + 60.0, 1, 1.0, 1.0, 95) * 0.65;
        double ty   = m_waterY + 3.0 + fbm(i * 2.1 + 61.0, 1, 1.0, 1.0, 96) * 6.0;
        double rw   = (5 + fbm(i * 0.9 + 62.0, 1, 1.0, 1.0, 97) * 18.0);
        double rh   = rw * 0.35;
        QRadialGradient foam(tx * W, ty, rw);
        foam.setColorAt(0.0, QColor(245, 250, 255, 160));
        foam.setColorAt(1.0, Qt::transparent);
        p.setBrush(foam);
        p.drawEllipse(QPointF(tx * W, ty), rw, rh);
    }
}

// ─── Pebbles / gravel ────────────────────────────────────────────────────────

void ErodedBankScene::drawPebbles(QPainter &p, int W, int H)
{
    // Gravel bar just at waterline and slightly underwater
    p.setPen(QPen(QColor(60, 55, 48), 0.5));
    int count = 120;
    for (int i = 0; i < count; ++i) {
        double tx = fbm(i * 1.1 + 200.0, 1, 1.0, 1.0, 100) * 0.85;
        double ty = m_waterY + 8 + fbm(i * 2.3 + 201.0, 1, 1.0, 1.0, 101) * 0.12 * H;
        double rx = 3.0 + fbm(i * 1.7 + 202.0, 1, 1.0, 1.0, 102) * 8.0;
        double ry = rx * (0.5 + fbm(i * 0.8 + 203.0, 1, 1.0, 1.0, 103) * 0.4);
        double angle = fbm(i * 0.5 + 204.0, 1, 1.0, 1.0, 104) * 180.0;

        // Two-tone pebble
        QRadialGradient pg(tx * W - rx * 0.3, ty - ry * 0.3, rx * 0.8);
        bool light = (i % 3 != 0);
        pg.setColorAt(0.0, light ? kPebble2 : kPebble1);
        pg.setColorAt(1.0, light ? kPebble1 : kBedrockDark);
        p.setBrush(pg);

        p.save();
        p.translate(tx * W, ty);
        p.rotate(angle);
        p.drawEllipse(QPointF(0, 0), rx, ry);
        p.restore();
    }
}

// ─── Exposed roots ───────────────────────────────────────────────────────────

void ErodedBankScene::drawExposedRoots(QPainter &p, int W, int H)
{
    // Roots dangling from the undercut area — a classic visual sign of erosion
    p.setPen(Qt::NoPen);
    int rCount = 25;
    int startX = static_cast<int>(0.40 * W);
    for (int i = 0; i < rCount; ++i) {
        double tx  = startX + fbm(i * 1.9 + 300.0, 1, 1.0, 1.0, 110) * (W - startX);
        double ty0 = bankUndercut(tx / W) - 2.0;
        double ty1 = ty0 + (8 + fbm(i * 2.1 + 301.0, 1, 1.0, 1.0, 111) * 0.08 * H);
        double thick = 1.0 + fbm(i * 0.7 + 302.0, 1, 1.0, 1.0, 112) * 2.5;
        double wobble = fbm(i * 1.3 + 303.0, 1, 1.0, 1.0, 113) * 6.0 - 3.0;

        QPen rp(kRoot, thick);
        rp.setCapStyle(Qt::RoundCap);
        p.setPen(rp);
        QPainterPath root;
        root.moveTo(tx, ty0);
        root.cubicTo(tx + wobble, (ty0 + ty1) * 0.5,
                     tx - wobble, (ty0 + ty1) * 0.5 + 5,
                     tx + wobble * 0.3, ty1);
        p.drawPath(root);
    }
}

// ─── Vegetation (grass tufts, sedges) ───────────────────────────────────────

void ErodedBankScene::drawVegetation(QPainter &p, int W, int H)
{
    // Grass blades on the bank top
    int blades = 300;
    for (int i = 0; i < blades; ++i) {
        double tx   = fbm(i * 1.1 + 400.0, 1, 1.0, 1.0, 120) * 0.92;
        double base = bankSurface(tx) - 0.012 * H;
        double len  = (0.025 + fbm(i * 2.3 + 401.0, 1, 1.0, 1.0, 121) * 0.05) * H;
        double lean = (fbm(i * 0.9 + 402.0, 1, 1.0, 1.0, 122) - 0.5) * 0.6; // sideways lean
        double tipX = tx * W + lean * len;
        double tipY = base - len;

        QColor blade = (i % 5 == 0) ? QColor(90, 130, 40) :
                       (i % 3 == 0) ? QColor(55,  95, 28) :
                                      QColor(70, 112, 35);

        QPen bp(blade, 0.8 + fbm(i * 0.4, 1, 1.0, 1.0, 123) * 0.8);
        bp.setCapStyle(Qt::RoundCap);
        p.setPen(bp);
        p.drawLine(QPointF(tx * W, base), QPointF(tipX, tipY));
    }

    // A few taller sedge plants at the erosion edge
    int sedges = 12;
    for (int i = 0; i < sedges; ++i) {
        double tx  = 0.40 + fbm(i * 1.5 + 500.0, 1, 1.0, 1.0, 130) * 0.55;
        double by  = bankSurface(tx) - 0.005 * H;
        double ht  = (0.07 + fbm(i * 2.0 + 501.0, 1, 1.0, 1.0, 131) * 0.07) * H;
        double cx  = tx * W;

        // Stem
        QPen sp(QColor(50, 85, 25), 1.2);
        sp.setCapStyle(Qt::RoundCap);
        p.setPen(sp);
        p.drawLine(QPointF(cx, by), QPointF(cx + (fbm(i, 1, 1.0, 1.0, 132) - 0.5) * 6.0, by - ht));

        // Seed head (ellipse)
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(100, 75, 30, 200));
        double hx = cx + (fbm(i, 1, 1.0, 1.0, 132) - 0.5) * 6.0;
        double hy = by - ht;
        p.drawEllipse(QPointF(hx, hy), 3.5, 7.0);
    }
}
