# MÉTHODOLOGIE EXPÉRIMENTALE

## 1. Configuration Matérielle

### 1.1 Système informatique
- **Système d'exploitation** : Linux [À COMPLÉTER avec ta distribution exacte - ex: Ubuntu 22.04 LTS]
- **Noyau Linux** : [À COMPLÉTER - ex: 5.15.0-91-generic]
- **Processeur** : [À COMPLÉTER - ex: Intel Core i7-9700K @ 3.60GHz]
- **Mémoire RAM** : [À COMPLÉTER - ex: 16 GB DDR4]
- **Architecture** : x86_64

### 1.2 Caméra
- **Type** : Webcam USB [À COMPLÉTER avec le modèle exact]
- **Interface** : Video4Linux2 (V4L2)
- **Périphérique** : /dev/video0
- **Résolution d'acquisition** : [À COMPLÉTER - probablement 640x480 ou 1280x720]
- **Format de pixel** : [À COMPLÉTER - généralement YUYV ou MJPEG]
- **Profondeur de couleur** : 24 bits (RGB)

---

## 2. Environnement Logiciel

### 2.1 Langages et bibliothèques
- **Langage** : C++ (standard C++11)
- **Compilateur** : GCC/G++ [À COMPLÉTER avec version - ex: 11.4.0]
- **Bibliothèque de vision** : OpenCV [À COMPLÉTER avec version - ex: 4.5.4]
- **Bibliothèques système** : 
  - STL (Standard Template Library)
  - pthreads (pour le multithreading OpenCV)

### 2.2 Modules OpenCV utilisés
- `opencv_core` : Structures de données et opérations matricielles
- `opencv_imgproc` : Traitement d'image et conversions d'espace colorimétrique
- `opencv_highgui` : Interface graphique et affichage des fenêtres
- `opencv_videoio` : Capture vidéo depuis la caméra
- `opencv_objdetect` : Détection de visage (Haar Cascade Classifier)

---

## 3. Paramètres d'Acquisition

### 3.1 Paramètres temporels
- **Fréquence d'échantillonnage** : 15 FPS (frames par seconde)
- **Durée du buffer** : 10 secondes (150 frames)
- **Décalage temporel** : 1 seconde (15 frames) entre chaque estimation
- **Période de stabilisation** : 5 secondes (75 frames) pour l'auto-balance des blancs

### 3.2 Région d'intérêt (ROI)
- **ROI visage** : Détectée automatiquement via Haar Cascade (`haarcascade_frontalface_alt.xml`)
- **ROI front** : 
  - Position verticale : 10% sous le haut du visage
  - Hauteur : 25% de la hauteur totale du visage
  - Position horizontale : Centrée (25% de marge de chaque côté)
  - Largeur : 50% de la largeur totale du visage

### 3.3 Canaux colorimétriques
- **Espace colorimétrique** : RGB (Red, Green, Blue)
- **Canaux extraits** : 
  - Rouge (R) : Indice 2 en OpenCV BGR
  - Vert (G) : Indice 1 en OpenCV BGR
  - Bleu (B) : Indice 0 en OpenCV BGR

---

## 4. Traitement du Signal

### 4.1 Méthode de séparation de sources
- **Algorithme** : ICA (Independent Component Analysis) FastICA
- **Fonction de contraste** : tanh (tangente hyperbolique)
- **Nombre d'itérations** : 200
- **Critère de sélection** : Composante avec variance maximale

### 4.2 Prétraitement
1. **Centrage** : Soustraction de la moyenne pour chaque canal
2. **Blanchiment** : Décorrélation via décomposition en valeurs propres
3. **Normalisation** : (signal - moyenne) / écart-type

### 4.3 Analyse fréquentielle
- **Méthode** : Transformée de Fourier Rapide (FFT)
- **Algorithme** : Cooley-Tukey radix-2 decimation-in-time
- **Fenêtrage** : Fenêtre de Hann pour réduire les fuites spectrales
- **Padding** : Complément à la puissance de 2 supérieure avec des zéros
- **Tables trigonométriques** : Pré-calculées et réutilisées pour optimisation

### 4.4 Estimation de la fréquence cardiaque
- **Plage de fréquences** : 50-150 BPM (0.83-2.5 Hz)
- **Méthode** : Recherche du pic maximal dans le spectre de puissance
- **Résolution fréquentielle** : FPS / (2 × N) Hz, où N = taille du buffer après padding

---

## 5. Conditions Expérimentales

### 5.1 Environnement
- **Éclairage** : [À COMPLÉTER - ex: Lumière ambiante naturelle de bureau, ~500 lux]
- **Distance caméra-sujet** : [À COMPLÉTER - ex: ~50 cm]
- **Position du sujet** : Assis, face à la caméra, immobile
- **Arrière-plan** : [À COMPLÉTER - ex: Mur blanc uniforme]

### 5.2 Conditions du sujet
- **État** : Au repos, après 5 minutes d'acclimatation
- **Consignes** : 
  - Rester immobile pendant l'acquisition
  - Respiration normale
  - Éviter de parler
  - Maintenir une expression faciale neutre

### 5.3 Durée des tests
- **Temps de stabilisation** : 5 secondes (auto-balance caméra)
- **Première estimation** : 10 secondes après détection du visage
- **Acquisitions suivantes** : Toutes les 1 seconde (avec buffer glissant)
- **Durée totale** : [À COMPLÉTER selon votre protocole - ex: 60 secondes]

---

## 6. Validation et Contrôle Qualité

### 6.1 Critères de rejet
- Buffer incomplet (< 150 frames)
- Perte de détection du visage
- Valeurs aberrantes (HR < 40 ou > 200 BPM)

### 6.2 Affichage en temps réel
- **Fenêtres de visualisation** :
  1. Image brute avec rectangles de ROI (visage en vert, front en bleu)
  2. ROI du visage
  3. ROI du front
  4. Signal temporel normalisé (ICA ou canal vert)
  5. Spectre de puissance avec zoom sur bande cardiaque

### 6.3 Indicateurs affichés
- Pourcentage de remplissage du buffer
- Fréquence cardiaque estimée (BPM)
- Messages d'état (détection face, estimation HR, avertissements)

---

## 7. Pipeline de Traitement

```
1. Acquisition vidéo (15 FPS)
   ↓
2. Détection du visage (Haar Cascade)
   ↓
3. Extraction ROI front (25% hauteur, 50% largeur)
   ↓
4. Calcul moyennes RGB par frame
   ↓
5. Stockage dans buffers circulaires (R, G, B)
   ↓
6. ICA sur 150 frames (10 secondes)
   ↓
7. Normalisation du signal sélectionné
   ↓
8. Fenêtrage de Hann
   ↓
9. FFT avec padding à puissance de 2
   ↓
10. Spectre de puissance
    ↓
11. Recherche du pic dans [50-150] BPM
    ↓
12. Affichage HR et décalage buffer (1 sec)
```

---

## 8. Formules Mathématiques Clés

### 8.1 Normalisation
```
signal_normalisé[i] = (signal[i] - μ) / σ
où μ = moyenne, σ = écart-type
```

### 8.2 Fenêtre de Hann
```
w[n] = 0.5 × (1 - cos(2π × n / (N-1)))
signal_fenêtré[n] = signal[n] × w[n]
```

### 8.3 Spectre de puissance
```
P[k] = √(Re[k]² + Im[k]²)
où Re[k], Im[k] sont les parties réelle/imaginaire de la FFT
```

### 8.4 Conversion indice → BPM
```
f_Hz = indice_pic × (FPS / (2 × N))
HR_BPM = f_Hz × 60
```

---

## 9. Optimisations Implémentées

1. **Tables trigonométriques pré-calculées** : cos/sin pour FFT
2. **Buffer circulaire** : Décalage de 1 sec au lieu de vider complètement
3. **Détection unique du visage** : ROI fixe après première détection
4. **Réutilisation des allocations mémoire** : Évite new/delete répétés
5. **ICA sélective** : Active/désactive via flag `USE_ICA`

---

## 10. Références des Algorithmes

- **Détection de visage** : Viola-Jones Haar Cascade (OpenCV implementation)
- **ICA** : FastICA avec fonction de contraste tanh
- **FFT** : Cooley-Tukey radix-2 algorithm
- **rPPG** : Méthode inspirée de Poh et al. (2010) et Verkruysse et al. (2008)

---

## ANNEXE : Commande pour récupérer les informations système

Pour compléter les sections marquées [À COMPLÉTER], exécutez le script fourni :

```bash
chmod +x collect_system_info.sh
./collect_system_info.sh > system_info.txt
```

Puis remplissez les informations manquantes avec les valeurs obtenues.
