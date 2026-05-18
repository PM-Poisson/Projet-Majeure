"""
Falaise 3D — champ de densité volumétrique
===========================================
Représentation :
  density > 0  →  terre
  density < 0  →  air
  density ≈ 0  →  surface (eau au niveau de l'eau)

Structure : tableau numpy (NX, NY, NZ)
  X = largeur (gauche → droite)
  Y = profondeur (avant → arrière)
  Z = hauteur   (bas → haut)

La falaise est un trapèze en coupe XZ, extrudé le long de Y,
puis perturbé par un bruit de Perlin 3D.
"""

import sys
import math
import numpy as np
from PyQt5.QtWidgets import (
    QApplication, QMainWindow, QWidget,
    QVBoxLayout, QHBoxLayout, QSlider, QLabel,
    QGroupBox, QSizePolicy, QFrame
)
from PyQt5.QtCore import Qt

import matplotlib
matplotlib.use("Qt5Agg")
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure
import matplotlib.colors as mcolors
import matplotlib.patches as mpatches


# ═══════════════════════════════════════════════════════════════════
#  Bruit de Perlin 3D (implémentation minimaliste, sans dépendance)
# ═══════════════════════════════════════════════════════════════════

class Perlin3D:
    """Bruit de Perlin 3D classique (table de permutation fixe)."""

    def __init__(self, seed: int = 0):
        rng = np.random.default_rng(seed)
        p = np.arange(256, dtype=int)
        rng.shuffle(p)
        self._p = np.tile(p, 2)          # table de 512 entrées

    @staticmethod
    def _fade(t):
        return t * t * t * (t * (t * 6 - 15) + 10)

    @staticmethod
    def _lerp(a, b, t):
        return a + t * (b - a)

    def _grad(self, h, x, y, z):
        h = h & 15
        u = np.where(h < 8, x, y)
        v = np.where(h < 4, y, np.where((h == 12) | (h == 14), x, z))
        return np.where(h & 1, -u, u) + np.where(h & 2, -v, v)

    def noise(self, x, y, z):
        """Bruit de Perlin pour des scalaires ou tableaux numpy."""
        p = self._p
        xi = np.floor(x).astype(int) & 255
        yi = np.floor(y).astype(int) & 255
        zi = np.floor(z).astype(int) & 255
        xf = x - np.floor(x)
        yf = y - np.floor(y)
        zf = z - np.floor(z)
        u, v, w = self._fade(xf), self._fade(yf), self._fade(zf)

        aaa = p[p[p[xi    ] + yi    ] + zi    ]
        aba = p[p[p[xi    ] + yi + 1] + zi    ]
        aab = p[p[p[xi    ] + yi    ] + zi + 1]
        abb = p[p[p[xi    ] + yi + 1] + zi + 1]
        baa = p[p[p[xi + 1] + yi    ] + zi    ]
        bba = p[p[p[xi + 1] + yi + 1] + zi    ]
        bab = p[p[p[xi + 1] + yi    ] + zi + 1]
        bbb = p[p[p[xi + 1] + yi + 1] + zi + 1]

        x1 = self._lerp(self._grad(aaa, xf,     yf,     zf    ),
                        self._grad(baa, xf - 1, yf,     zf    ), u)
        x2 = self._lerp(self._grad(aba, xf,     yf - 1, zf    ),
                        self._grad(bba, xf - 1, yf - 1, zf    ), u)
        y1 = self._lerp(x1, x2, v)

        x1 = self._lerp(self._grad(aab, xf,     yf,     zf - 1),
                        self._grad(bab, xf - 1, yf,     zf - 1), u)
        x2 = self._lerp(self._grad(abb, xf,     yf - 1, zf - 1),
                        self._grad(bbb, xf - 1, yf - 1, zf - 1), u)
        y2 = self._lerp(x1, x2, v)

        return self._lerp(y1, y2, w)

    def fbm(self, x, y, z, octaves=5, lacunarity=2.0, gain=0.5):
        """Mouvement Brownien fractionnaire 3D."""
        val = 0.0
        amp, freq = 1.0, 1.0
        for _ in range(octaves):
            val  += amp * self.noise(x * freq, y * freq, z * freq)
            amp  *= gain
            freq *= lacunarity
        return val


# ═══════════════════════════════════════════════════════════════════
#  Champ de densité 3D
# ═══════════════════════════════════════════════════════════════════

class DensityField:
    """
    Génère le tableau 3D de densité.

    Paramètres géométriques du trapèze (coordonnées normalisées [0,1]) :
      cliff_base_x  : bord droit de la falaise au sol
      cliff_top_x   : bord droit de la falaise au sommet
      cliff_height  : hauteur normalisée du sommet de la falaise
      water_z       : niveau d'eau normalisé
    """

    def __init__(
        self,
        nx: int = 80,
        ny: int = 60,
        nz: int = 60,
        cliff_base_x: float = 0.72,
        cliff_top_x:  float = 0.48,
        cliff_height: float = 0.65,
        water_z:      float = 0.18,
        noise_amplitude: float = 0.08,
        noise_scale:     float = 3.5,
        seed: int = 42,
    ):
        self.nx, self.ny, self.nz = nx, ny, nz
        self.cliff_base_x  = cliff_base_x
        self.cliff_top_x   = cliff_top_x
        self.cliff_height  = cliff_height
        self.water_z       = water_z
        self.noise_amplitude = noise_amplitude
        self.noise_scale     = noise_scale

        self.perlin = Perlin3D(seed)
        self.density = self._build()

    # ── Bord droit du trapèze à la hauteur z_norm ─────────────────
    def _trapezoid_right_edge(self, z_norm: np.ndarray) -> np.ndarray:
        """
        Interpolation linéaire entre la base et le sommet du trapèze.
        Au-dessus de cliff_height → pas de bord (tout est air).
        """
        t = np.clip(z_norm / self.cliff_height, 0.0, 1.0)
        return self.cliff_base_x + (self.cliff_top_x - self.cliff_base_x) * t

    # ── Construction du champ ──────────────────────────────────────
    def _build(self) -> np.ndarray:
        nx, ny, nz = self.nx, self.ny, self.nz

        # Coordonnées normalisées pour chaque voxel
        xs = (np.arange(nx) + 0.5) / nx   # [0, 1]
        ys = (np.arange(ny) + 0.5) / ny
        zs = (np.arange(nz) + 0.5) / nz

        # Grilles 3D (shape nx × ny × nz)
        X, Y, Z = np.meshgrid(xs, ys, zs, indexing="ij")

        # ── 1. Densité du trapèze ──────────────────────────────────
        # Bord droit du trapèze pour chaque hauteur z
        right_edge = self._trapezoid_right_edge(Z)  # (nx, ny, nz)

        # distance signée horizontale au bord droit : positif = intérieur
        dist_right = right_edge - X

        # Au-dessus du sommet : tout est air
        above_top = Z > self.cliff_height
        dist_right = np.where(above_top, dist_right - 1.0, dist_right)

        # ── 2. Bruit de Perlin 3D ─────────────────────────────────
        noise = self.perlin.fbm(
            X * self.noise_scale,
            Y * self.noise_scale,
            Z * self.noise_scale,
            octaves=5,
        ) * self.noise_amplitude

        # ── 3. Densité finale ─────────────────────────────────────
        density = dist_right + noise

        # ── 4. Eau : on écrase les voxels à z ≈ water_z et density < 0
        # (l'eau remplit l'air sous le niveau d'eau et hors de la terre)
        # On encode l'eau comme density == 0 exactement
        # → on marque les cases "air sous le niveau d'eau" avec 0
        water_mask = (density < 0) & (Z <= self.water_z)
        density = np.where(water_mask, 0.0, density)

        return density.astype(np.float32)

    # ── Accesseurs pratiques ───────────────────────────────────────

    @property
    def earth_mask(self) -> np.ndarray:
        return self.density > 0

    @property
    def water_mask(self) -> np.ndarray:
        return self.density == 0.0

    @property
    def air_mask(self) -> np.ndarray:
        return self.density < 0

    def slice_xz(self, iy: int) -> np.ndarray:
        """Coupe verticale (vue de côté) à la profondeur iy."""
        return self.density[:, iy, :]          # (nx, nz)

    def slice_xy(self, iz: int) -> np.ndarray:
        """Coupe horizontale (vue de dessus) à la hauteur iz."""
        return self.density[:, :, iz]          # (nx, ny)

    def slice_yz(self, ix: int) -> np.ndarray:
        """Coupe frontale (vue de face) à la position ix."""
        return self.density[ix, :, :]          # (ny, nz)


# ═══════════════════════════════════════════════════════════════════
#  Colormap personnalisée : air / eau / terre
# ═══════════════════════════════════════════════════════════════════

def make_colormap():
    """
    Renvoie une colormap et une norm pour afficher les densités :
      density < 0  →  dégradé ciel (bleu clair → blanc)
      density = 0  →  bleu eau
      density > 0  →  dégradé terre (beige → brun foncé)
    """
    # On discrétise en 3 zones
    colors_air   = [(0.53, 0.81, 0.98), (0.85, 0.93, 0.98)]   # ciel
    colors_water = [(0.10, 0.45, 0.75)]                         # eau
    colors_earth = [(0.75, 0.60, 0.38), (0.38, 0.24, 0.08)]   # terre

    all_colors = colors_air + colors_water + colors_earth
    cmap = mcolors.LinearSegmentedColormap.from_list("scene", all_colors, N=256)
    return cmap


CMAP = make_colormap()


def density_to_rgba(data: np.ndarray) -> np.ndarray:
    """
    Convertit un tableau 2D de densités en image RGBA.
      air   (< 0)  → bleu ciel
      eau   (= 0)  → bleu eau
      terre (> 0)  → brun
    """
    h, w = data.shape
    img = np.zeros((h, w, 4), dtype=np.float32)

    air   = data < 0
    water = data == 0.0
    earth = data > 0

    # Air : dégradé selon la hauteur (on normalise density entre -1 et 0)
    d_air = np.clip(-data, 0, 1)
    img[air, 0] = 0.53 + 0.32 * d_air[air]
    img[air, 1] = 0.78 + 0.15 * d_air[air]
    img[air, 2] = 0.95 + 0.05 * d_air[air]
    img[air, 3] = 1.0

    # Eau
    img[water, 0] = 0.10
    img[water, 1] = 0.45
    img[water, 2] = 0.78
    img[water, 3] = 1.0

    # Terre : dégradé selon la densité
    d_earth = np.clip(data / 0.5, 0, 1)
    img[earth, 0] = 0.75 - 0.37 * d_earth[earth]
    img[earth, 1] = 0.55 - 0.31 * d_earth[earth]
    img[earth, 2] = 0.30 - 0.22 * d_earth[earth]
    img[earth, 3] = 1.0

    return img


# ═══════════════════════════════════════════════════════════════════
#  Canvas matplotlib
# ═══════════════════════════════════════════════════════════════════

class SliceCanvas(FigureCanvas):
    """Affiche une coupe 2D du champ de densité."""

    def __init__(self, title: str, xlabel: str, ylabel: str, parent=None):
        self.fig = Figure(figsize=(4, 4), facecolor="#1a1a2e")
        super().__init__(self.fig)
        self.setParent(parent)
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)

        self.ax = self.fig.add_subplot(111)
        self.ax.set_facecolor("#1a1a2e")
        self.ax.set_title(title, color="white", fontsize=10, pad=6)
        self.ax.set_xlabel(xlabel, color="#aaaaaa", fontsize=8)
        self.ax.set_ylabel(ylabel, color="#aaaaaa", fontsize=8)
        self.ax.tick_params(colors="#777777", labelsize=7)
        for spine in self.ax.spines.values():
            spine.set_edgecolor("#444466")

        self._im = None

    def update_slice(self, data: np.ndarray, label: str = ""):
        """Met à jour l'image avec une nouvelle coupe."""
        img = density_to_rgba(data.T[::-1])   # Z vers le haut, transposé

        if self._im is None:
            self._im = self.ax.imshow(
                img, aspect="auto", interpolation="nearest",
                extent=[0, data.shape[0], 0, data.shape[1]]
            )
        else:
            self._im.set_data(img)
            self._im.set_extent([0, data.shape[0], 0, data.shape[1]])

        if label:
            self.ax.set_title(label, color="white", fontsize=10, pad=6)

        self.fig.tight_layout(pad=0.5)
        self.draw()


class StatsCanvas(FigureCanvas):
    """Affiche un histogramme de la distribution des densités."""

    def __init__(self, parent=None):
        self.fig = Figure(figsize=(4, 2.5), facecolor="#1a1a2e")
        super().__init__(self.fig)
        self.setParent(parent)
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)
        self.setFixedHeight(180)

        self.ax = self.fig.add_subplot(111)
        self._setup_axes()

    def _setup_axes(self):
        self.ax.set_facecolor("#12122a")
        self.ax.set_title("Distribution des densités", color="white", fontsize=9)
        self.ax.set_xlabel("densité", color="#aaaaaa", fontsize=7)
        self.ax.set_ylabel("voxels", color="#aaaaaa", fontsize=7)
        self.ax.tick_params(colors="#777777", labelsize=7)
        for spine in self.ax.spines.values():
            spine.set_edgecolor("#444466")

    def update_stats(self, density: np.ndarray):
        self.ax.cla()
        self._setup_axes()

        d = density.ravel()
        # Air
        self.ax.hist(d[d < 0], bins=40, color=(0.53, 0.78, 0.95, 0.8),
                     label=f"Air  ({(d<0).sum():,})")
        # Eau
        n_water = (d == 0).sum()
        if n_water:
            self.ax.axvline(0, color=(0.1, 0.5, 0.9), linewidth=2,
                            label=f"Eau  ({n_water:,})")
        # Terre
        self.ax.hist(d[d > 0], bins=40, color=(0.65, 0.42, 0.18, 0.8),
                     label=f"Terre ({(d>0).sum():,})")

        self.ax.legend(fontsize=7, facecolor="#222244", labelcolor="white",
                       framealpha=0.8)
        self.fig.tight_layout(pad=0.4)
        self.draw()


# ═══════════════════════════════════════════════════════════════════
#  Fenêtre principale
# ═══════════════════════════════════════════════════════════════════

class MainWindow(QMainWindow):

    def __init__(self, field: DensityField):
        super().__init__()
        self.field = field
        self.setWindowTitle("Falaise 3D — Champ de densité")
        self.setStyleSheet("""
            QMainWindow, QWidget { background: #12122a; color: white; }
            QGroupBox { border: 1px solid #444466; border-radius: 4px;
                        margin-top: 8px; font-size: 9pt; color: #aaaacc; }
            QGroupBox::title { subcontrol-origin: margin; left: 8px; }
            QLabel { color: #ccccee; font-size: 8pt; }
            QSlider::groove:horizontal { background: #2a2a4a; height: 4px;
                                         border-radius: 2px; }
            QSlider::handle:horizontal { background: #6688cc; width: 14px;
                                         height: 14px; margin: -5px 0;
                                         border-radius: 7px; }
        """)

        self._build_ui()
        self._refresh_all()

    # ── Construction de l'interface ────────────────────────────────

    def _build_ui(self):
        nx, ny, nz = self.field.nx, self.field.ny, self.field.nz
        central = QWidget()
        self.setCentralWidget(central)
        root = QHBoxLayout(central)
        root.setSpacing(8)
        root.setContentsMargins(8, 8, 8, 8)

        # ── Colonne gauche : coupes XZ et XY ──────────────────────
        left = QVBoxLayout()
        left.setSpacing(6)

        # Vue de côté (XZ)
        grp_xz = QGroupBox("Vue de côté  XZ  (profondeur Y)")
        vl = QVBoxLayout(grp_xz)
        self.canvas_xz = SliceCanvas("", "X →", "Z ↑")
        vl.addWidget(self.canvas_xz)
        hl = QHBoxLayout()
        hl.addWidget(QLabel("Y :"))
        self.slider_y = QSlider(Qt.Horizontal)
        self.slider_y.setRange(0, ny - 1)
        self.slider_y.setValue(ny // 2)
        self.lbl_y = QLabel(f"{ny//2}")
        self.slider_y.valueChanged.connect(self._on_y_changed)
        hl.addWidget(self.slider_y); hl.addWidget(self.lbl_y)
        vl.addLayout(hl)
        left.addWidget(grp_xz)

        # Vue de dessus (XY)
        grp_xy = QGroupBox("Vue de dessus  XY  (hauteur Z)")
        vl2 = QVBoxLayout(grp_xy)
        self.canvas_xy = SliceCanvas("", "X →", "Y →")
        vl2.addWidget(self.canvas_xy)
        hl2 = QHBoxLayout()
        hl2.addWidget(QLabel("Z :"))
        self.slider_z = QSlider(Qt.Horizontal)
        self.slider_z.setRange(0, nz - 1)
        self.slider_z.setValue(int(self.field.water_z * nz))
        self.lbl_z = QLabel(f"{int(self.field.water_z*nz)}")
        self.slider_z.valueChanged.connect(self._on_z_changed)
        hl2.addWidget(self.slider_z); hl2.addWidget(self.lbl_z)
        vl2.addLayout(hl2)
        left.addWidget(grp_xy)

        root.addLayout(left, stretch=3)

        # ── Colonne droite : vue YZ + stats ───────────────────────
        right = QVBoxLayout()
        right.setSpacing(6)

        grp_yz = QGroupBox("Vue de face  YZ  (position X)")
        vl3 = QVBoxLayout(grp_yz)
        self.canvas_yz = SliceCanvas("", "Y →", "Z ↑")
        vl3.addWidget(self.canvas_yz)
        hl3 = QHBoxLayout()
        hl3.addWidget(QLabel("X :"))
        self.slider_x = QSlider(Qt.Horizontal)
        self.slider_x.setRange(0, nx - 1)
        self.slider_x.setValue(int(self.field.cliff_top_x * nx))
        self.lbl_x = QLabel(f"{int(self.field.cliff_top_x*nx)}")
        self.slider_x.valueChanged.connect(self._on_x_changed)
        hl3.addWidget(self.slider_x); hl3.addWidget(self.lbl_x)
        vl3.addLayout(hl3)
        right.addWidget(grp_yz)

        # Légende
        grp_leg = QGroupBox("Légende")
        hl_leg = QHBoxLayout(grp_leg)
        for color, label in [
            ("#87CEFA", "Air (density < 0)"),
            ("#1A73C4", "Eau (density = 0)"),
            ("#8B6914", "Terre (density > 0)"),
        ]:
            dot = QFrame()
            dot.setFixedSize(12, 12)
            dot.setStyleSheet(f"background:{color}; border-radius:6px;")
            hl_leg.addWidget(dot)
            hl_leg.addWidget(QLabel(label))
            hl_leg.addSpacing(8)
        right.addWidget(grp_leg)

        # Statistiques
        grp_stats = QGroupBox("Statistiques")
        vl4 = QVBoxLayout(grp_stats)
        self.canvas_stats = StatsCanvas()
        vl4.addWidget(self.canvas_stats)

        # Comptages
        d = self.field.density
        n_tot = d.size
        n_earth = int((d > 0).sum())
        n_water = int((d == 0).sum())
        n_air   = int((d < 0).sum())
        info = QLabel(
            f"Grille : {self.field.nx} × {self.field.ny} × {self.field.nz}"
            f"  ({n_tot:,} voxels)\n"
            f"Terre : {n_earth:,}  |  Eau : {n_water:,}  |  Air : {n_air:,}"
        )
        info.setAlignment(Qt.AlignCenter)
        info.setStyleSheet("color:#9999bb; font-size:8pt;")
        vl4.addWidget(info)
        right.addWidget(grp_stats)

        root.addLayout(right, stretch=2)

    # ── Refresh ────────────────────────────────────────────────────

    def _refresh_all(self):
        iy = self.slider_y.value()
        iz = self.slider_z.value()
        ix = self.slider_x.value()
        self._update_xz(iy)
        self._update_xy(iz)
        self._update_yz(ix)
        self.canvas_stats.update_stats(self.field.density)

    def _update_xz(self, iy: int):
        data = self.field.slice_xz(iy)
        xn = (iy + 0.5) / self.field.ny
        self.canvas_xz.update_slice(
            data, f"Vue de côté XZ  —  Y={iy} ({xn:.2f})")

    def _update_xy(self, iz: int):
        data = self.field.slice_xy(iz)
        zn = (iz + 0.5) / self.field.nz
        self.canvas_xy.update_slice(
            data, f"Vue de dessus XY  —  Z={iz} ({zn:.2f})")

    def _update_yz(self, ix: int):
        data = self.field.slice_yz(ix)
        xn = (ix + 0.5) / self.field.nx
        self.canvas_yz.update_slice(
            data, f"Vue de face YZ  —  X={ix} ({xn:.2f})")

    # ── Slots ──────────────────────────────────────────────────────

    def _on_y_changed(self, v: int):
        self.lbl_y.setText(str(v))
        self._update_xz(v)

    def _on_z_changed(self, v: int):
        self.lbl_z.setText(str(v))
        self._update_xy(v)

    def _on_x_changed(self, v: int):
        self.lbl_x.setText(str(v))
        self._update_yz(v)


# ═══════════════════════════════════════════════════════════════════
#  Point d'entrée
# ═══════════════════════════════════════════════════════════════════

if __name__ == "__main__":
    print("Génération du champ de densité 3D…")
    field = DensityField(
        nx=80, ny=60, nz=60,
        cliff_base_x=0.72,
        cliff_top_x=0.48,
        cliff_height=0.65,
        water_z=0.18,
        noise_amplitude=0.08,
        noise_scale=3.5,
        seed=42,
    )
    n = field.density
    print(f"  Grille : {field.nx}×{field.ny}×{field.nz}  ({n.size:,} voxels)")
    print(f"  Terre  : {(n>0).sum():,} voxels")
    print(f"  Eau    : {(n==0).sum():,} voxels")
    print(f"  Air    : {(n<0).sum():,} voxels")
    print(f"  density.min={n.min():.3f}  max={n.max():.3f}")

    app = QApplication(sys.argv)
    win = MainWindow(field)
    win.resize(1300, 750)
    win.show()
    sys.exit(app.exec_())