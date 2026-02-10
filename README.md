# MÉTHODOLOGIE EXPÉRIMENTALE

## 1. Configuration Matérielle

### 1.1 Système informatique

* **Système d'exploitation** : Linux Ubuntu 22.04.5 LTS (Jammy Jellyfish)
* **Noyau Linux** : 6.8.0-94-generic
* **Processeur** : Intel® Core™ i5-3340M @ 2.70 GHz
  (2 cœurs physiques, 4 threads)
* **Mémoire RAM** : 4 GB (3.7 GiB utilisables)
* **Architecture** : x86_64
* **Carte graphique** : Intel 3rd Gen Core Processor Integrated Graphics

---

### 1.2 Caméra

* **Type** : Webcam intégrée (modèle exact non spécifié)
* **Interface** : Video4Linux2 (V4L2)
* **Périphérique** : `/dev/video0` (présent mais accès non confirmé lors du relevé)
* **Résolution d'acquisition** : Non déterminée (outils `v4l2-utils` non installés)
* **Format de pixel** : Non déterminé (probablement YUYV ou MJPEG)
* **Profondeur de couleur** : 24 bits (RGB, après conversion OpenCV)

> Remarque : les informations détaillées sur la caméra (résolutions, formats supportés) n’étaient pas accessibles au moment du relevé système.

---

## 2. Environnement Logiciel

### 2.1 Langages et bibliothèques

* **Langage** : C++ (standard C++11)
* **Compilateur** : GCC / G++ 11.4.0
* **Bibliothèque de vision** : OpenCV 4.5.4
* **Python** : Python 3.10.12 (scripts annexes et tests)
* **Bibliothèques système** :

  * STL (Standard Template Library)
  * pthreads (multithreading OpenCV)

---

### 2.2 Modules OpenCV utilisés

* `opencv_core` : Structures de données et opérations matricielles
* `opencv_imgproc` : Traitement d'image et conversions d'espace colorimétrique
* `opencv_highgui` : Interface graphique et affichage
* `opencv_videoio` : Capture vidéo depuis la caméra
* `opencv_objdetect` : Détection de visage (Haar Cascade Classifier)

---

## 3. Paramètres d'Acquisition

### 3.1 Paramètres temporels

* **Fréquence d'échantillonnage** : 15 FPS
* **Durée du buffer** : 10 secondes (150 frames)
* **Décalage temporel** : 1 seconde (15 frames)
* **Période de stabilisation** : 5 secondes (75 frames)

---

### 3.2 Région d'intérêt (ROI)

* **ROI visage** : Détectée automatiquement via Haar Cascade (`haarcascade_frontalface_alt.xml`)
* **ROI front** :

  * Position verticale : 10 % sous le haut du visage
  * Hauteur : 25 % de la hauteur du visage
  * Position horizontale : Centrée
  * Largeur : 50 % de la largeur du visage

---

### 3.3 Canaux colorimétriques

* **Espace colorimétrique** : RGB
* **Canaux extraits** :

  * Rouge (R) : indice 2 (OpenCV BGR)
  * Vert (G) : indice 1 (OpenCV BGR)
  * Bleu (B) : indice 0 (OpenCV BGR)

---

## 4. Traitement du Signal

### 4.1 Séparation de sources

* **Algorithme** : FastICA
* **Fonction de contraste** : tanh
* **Nombre d'itérations** : 200
* **Critère de sélection** : variance maximale

---

### 4.2 Prétraitement

1. Centrage (soustraction de la moyenne)
2. Blanchiment (décorrélation)
3. Normalisation :

```math
(signal - \mu) / \sigma
```

---

### 4.3 Analyse fréquentielle

* **Méthode** : FFT
* **Algorithme** : Cooley-Tukey radix-2
* **Fenêtrage** : Fenêtre de Hann
* **Padding** : Zéro-padding à la puissance de 2 supérieure

---

### 4.4 Estimation de la fréquence cardiaque

* **Plage fréquentielle** : 50–150 BPM (0.83–2.5 Hz)
* **Méthode** : Pic maximal du spectre de puissance
* **Conversion** :

```math
HR_{BPM} = f_{Hz} \times 60
```

---

## 5. Conditions Expérimentales

### 5.1 Environnement

* **Éclairage** : Éclairage intérieur ambiant
* **Distance caméra-sujet** : ~50–70 cm
* **Position** : Assis, face caméra
* **Arrière-plan** : Intérieur standard

---

### 5.2 Conditions du sujet

* Sujet au repos
* Immobilité
* Respiration normale
* Pas de parole

---

### 5.3 Durée des tests

* **Stabilisation** : 5 s
* **Première estimation** : 10 s
* **Estimations suivantes** : Toutes les 1 s
* **Durée totale** : 60 s

---

## 6. Pipeline de Traitement

```text
Acquisition vidéo
  → Détection visage
    → Extraction ROI front
      → Moyenne RGB
        → Buffer circulaire
          → ICA
            → Normalisation
              → FFT
                → Pic fréquentiel
                  → HR (BPM)
```

---

## 7. Références

* Viola & Jones – Rapid Object Detection using Haar-like Features
* Poh et al., 2010 – Non-contact, automated cardiac pulse measurements
* Verkruysse et al., 2008 – Remote plethysmographic imaging
