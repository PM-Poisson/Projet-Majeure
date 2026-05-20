# Coastal Erosion — Génération procédurale d'un paysage côtier

Projet Majeure 4ETI — CPE Lyon 2025/2026

Simulation en temps réel d'une île côtière avec océan animé, générée entièrement de manière procédurale. Deux méthodes de génération de terrain sont implémentées et sélectionnables depuis l'interface.

---

## Compilation

### Prérequis système

```bash
sudo apt install build-essential cmake pkg-config \
    libwayland-dev wayland-protocols libxkbcommon-dev \
    libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libxext-dev
```

### Dépendances (git submodules)

```bash
git clone <votre-repo>
cd coastal-erosion
git submodule update --init --recursive
```

GLAD n'est pas un submodule : générez-le sur https://glad.dav1d.de avec :
- Language : C/C++, Specification : OpenGL, Profile : Core, Version : 4.3+, ✓ Generate a loader

Placez ensuite `glad/include/` → `libs/glad/include/` et `glad/src/glad.c` → `libs/glad/src/glad.c`.

### Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -G "Unix Makefiles"
make -j4
```

### Lancement

```bash
./coastal-erosion
```

Le programme cherche les shaders dans `shaders/` (copié automatiquement dans le dossier build par CMake).

---

## Utilisation

### Navigation dans la scène

| Action | Contrôle |
|---|---|
| Rotation de la caméra | Clic droit + glisser |
| Zoom | Molette |
| Basculer mode caméra (Orbite/FPS) | `F` |
| Déplacement FPS (mode FPS) | `W` `A` `S` `D` + `E`/`Q` (haut/bas) |
| Affichage wireframe | `TAB` |
| Quitter | `Échap` |

### Panneau de contrôle (ImGui)

Affiché en haut à gauche. Toutes les modifications sont appliquées après avoir cliqué **Régénérer**.

**Méthode de génération** (boutons radio) :
- *Simplex + Domain Warping* : bruit fractal avec torsion du domaine
- *Diamond-Square [Galin 2019]* : subdivision récursive fractale

**Forme de l'île** :
- *Rayon plateau* : taille du plateau central par rapport à la grille
- *Largeur plage* : épaisseur de la zone de transition (petit = falaise abrupte, grand = plage douce)
- *Asymétrie* : amplitude de la variation angulaire du rayon (0 = île ronde, haut = une face en falaise, l'autre en plage)
- *Hauteur max* : amplitude verticale maximale du terrain en unités monde

**Paramètres Simplex** (visibles si méthode Simplex) :
- *Fréquence bruit* : fréquence spatiale du bruit de base
- *Octaves* : nombre de couches de détail (fBm)
- *Domain warp* : intensité de la torsion du domaine (0 = bruit classique)

**Paramètres Diamond-Square** (visibles si méthode Diamond-Square) :
- *Subdivisions* : nombre de niveaux de subdivision, grille résultante (2^n+1)²
- *Rugosité H* : exposant de Hurst — bas = rugueux, haut = lisse

**Ocean** :
- *Niveau ocean* : décalage vertical du plan de base de l'océan. Les vagues de Gerstner ajoutent ~4 unités par-dessus. Ajuster pour aligner le niveau de la mer avec la plage du terrain.

**Seed** : graine aléatoire. Changer la valeur puis cliquer Régénérer produit une île différente.

---

## Architecture du projet

```
coastal-erosion/
├── src/
│   ├── main.cpp                  — boucle principale GLFW + ImGui
│   ├── terrain/
│   │   ├── bruit.hpp/cpp         — bruit Simplex 2D + fBm + domain warping
│   │   ├── map.hpp/cpp           — heightmap CPU + texture GPU (GL_R32F)
│   │   ├── terrain.hpp/cpp       — TerrainGenerator (profil côtier linéaire)
│   │   ├── scene.hpp/cpp         — IslandScene : île procédurale (profil radial)
│   │   └── diamond_square.hpp/cpp — algorithme Diamond-Square
│   ├── ocean/
│   │   └── ocean.hpp/cpp         — grille animée par vagues de Gerstner
│   └── renderer/
│       ├── shader.hpp/cpp        — chargement/compilation shaders GLSL
│       ├── mesh.hpp/cpp          — VAO/VBO/EBO OpenGL
│       └── camera.hpp/cpp        — caméra orbitale + FPS
├── shaders/
│   ├── terrain.vert / terrain.frag  — rendu de l'île (biomes par hauteur/pente)
│   └── ocean.vert / ocean.frag      — rendu de l'océan (Gerstner + Fresnel)
├── libs/                         — dépendances (git submodules)
│   ├── glfw/   ├── glad/   ├── glm/   ├── imgui/   └── stb/
└── CMakeLists.txt
```

---

## Méthodes implémentées

### Génération de terrain

#### Bruit Simplex + Domain Warping

Le bruit Simplex 2D (implémentation originale de Stefan Gustavson) est accumulé en plusieurs octaves pour obtenir un mouvement brownien fractionnaire (fBm). Le domain warping consiste à évaluer le bruit dans un espace préalablement déformé par un autre bruit, ce qui produit des formes très organiques (méandres, irrégularités côtières).

Le masque d'île est un profil sigmoïde radial asymétrique : le rayon de l'île varie selon l'angle grâce à un bruit basse fréquence (2 octaves), créant un côté à falaise et un côté à plage.

#### Diamond-Square (Galin et al. 2019, Section 3.1.1)

Algorithme de subdivision récursive de Fournier, Fussell et Carpenter [FFC82b]. Sur une grille de taille (2^n+1)² :

- **Étape Diamond** : le centre de chaque carré reçoit la moyenne de ses 4 coins plus une perturbation aléatoire d'écart-type σ.
- **Étape Square** : le milieu de chaque arête reçoit la moyenne de ses voisins en croix plus une perturbation.

À chaque niveau k, σ_k = σ_0 × 2^(−k×H) où H est l'exposant de Hurst. H contrôle la dimension fractale du résultat (H≈0.3 = très rugueux, H≈0.8 = lisse).

La heightmap résultante est redimensionnée par interpolation bilinéaire puis le même masque d'île que la méthode Simplex lui est appliqué.

#### Pipeline commun

Les deux méthodes produisent une heightmap brute dans [-1, 1] sur laquelle est appliqué :
1. Le masque radial asymétrique (forme de l'île)
2. `shoreProfile` : trois sous-zones (mer → plage → falaise → plateau)
3. `cliffProfile` : courbe en S qui aplatit les sommets et les fonds tout en rendant la transition abrupte autour du niveau de la mer

### Colorisation par biomes (terrain.frag)

Les seuils de couleur sont exprimés en fractions de `heightScale` pour être indépendants des paramètres de génération :

| Zone | Hauteur relative | Critère de pente |
|---|---|---|
| Fond marin | < seaLevel | — |
| Plage sableuse | 0–7% | pente douce → sable, pentu → rocher |
| Rochers côtiers | 7–22% | roche sombre avec algues |
| Falaise | 22–50% | calcaire gris avec stries verticales |
| Plateau herbeux | > 50% | herbe si plat, roche si pentu |

### Simulation de l'océan (ocean.vert)

Superposition de 5 vagues de Gerstner avec des directions, longueurs d'onde et vitesses différentes. Contrairement aux sinusoïdes simples, les vagues de Gerstner déplacent les sommets à la fois verticalement et horizontalement, produisant le profil asymétrique réaliste des vagues (crête pointue, creux plat). Les normales analytiques sont calculées directement dans le vertex shader.

Le fragment shader ajoute un effet Fresnel simplifié (réflexion du ciel en angle rasant) et de la mousse blanche sur les crêtes.

---

## Dépendances

| Bibliothèque | Rôle |
|---|---|
| GLFW | Fenêtre et gestion des inputs |
| GLAD | Chargement des fonctions OpenGL |
| GLM | Mathématiques 3D (vecteurs, matrices) |
| Dear ImGui | Interface de contrôle temps réel |
| stb | Utilitaires image (non utilisé activement) |