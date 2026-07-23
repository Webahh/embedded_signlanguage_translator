
## 7 Entwicklung und Implementierung

### 7.1 Aufbau und Aufbereitung des Datensatzes

#### 7.1.1 Rohvideos

Der Datensatz basiert auf einer eigens erstellten Sammlung von Videoaufnahmen der deutsche Gebärdensprache (DGS) und wurde speziell für die Klassifikation statischer Fingeralphabetzeichen konzipiert. Die Rohvideos wurden manuell aufgezeichnet und liegen im MP4-Format vor.

Die Benennungskonvention der Videodateien lautet `alph_[og|fw]_[Label][Nummer].mp4`, wobei `[og|fw]` den aufzeichnenden protokolliert, `[Label]` das jeweilige Fingersprachezeichen und `[Nummer]` die fortlaufende Nummer innerhalb eines Labels spezifiziert. Die Speicherung erfolgt im Verzeichnis `ml_pipeline/resources/videos_prototype/`.

Der Datensatz umfasst 226 Rohvideos, die auf 26 Klassen verteilt sind. Die Klassenauswahl beschränkt sich auf die 24 statischen Buchstaben A–Y (ohne J und Z, da diese dynamische Handbewegungen erfordern und somit mit dem statischen Ansatz des Klassifikators nicht kompatibel sind), das Sonderzeichen SCH sowie die Leerklasse NONE, die die Unbestimmtheit des Ergebnisses darstellt. Die Anzahl der Originaufnahmen pro Klasse variiert zwischen 3 und 5, wobei 16 Klassen mit je 5, 8 Klassen mit je 4 und 2 Klassen (F, NONE) mit je 3 Aufnahmen vertreten sind.

#### 7.1.2 Frame-Extraktion und Hand-Erkennung

Zur Verarbeitung der Rohvideos wird jeder Einzelrahmen (Frame) mittels OpenCV Videocapture einzeln extrahiert und ohne vorherige Vorfilterung der Erkennungspipeline zugeführt.

Die Hand-Erkennung und Landmark-Extraktion erfolgt mithilfe des MediaPipe-Handdetektors (Lugaresi et al., 2019) in der Konfiguration für statische Bildanalyse mit einer maximalen Detektionskapazität von 2 Händen pro Frame und einer Mindestkonfidenz von 70 %. Pro Hand werden 21 Landmarks extrahiert, die den Handgelenkspunkt (WRIST) sowie die Gelenke der fünf Finger (Daumen: CMC, MCP, IP, TIP; Indexfinger: MCP, PIP, DIP, TIP; Mittelfinger: MCP, PIP, DIP, TIP; Ringfinger: MCP, PIP, DIP, TIP; Kleinfinger: MCP, PIP, DIP, TIP) umfassen.

#### 7.1.3 Augmentation

Die Extrahierte Gesten Daten werden im folgenden schritt mittels Augmentation vervielfacht und gegen Overfitting angepasst. Die Augmentationspipeline bietet  die folgenden Augmentationsoptionen:
- Translate: addiert einen Offset auf die Gesture Datem
- Spiegelung: Spiegelt gesten horizontal
- Scale: scaliert die gesten
- Jitter: Fügt der Geste zufälige individuelle  Fehler hinzu
- Zoom: Zufälliger zoom der Geste
- DropFrames: Skips frames in order to simulate missing Frames
Für die verwendeten Optionen sind:
- Mirror
- 2xTranslation
- 5xZoom

Mit hilfe der 

#### 7.1.4 Normalisierung und Interpolation

Die extrahierten Landmarks werden für eine effiziente und positionsinvariante Darstellung normalisiert. Hierbei werden alle Landmark-Koordinaten relativ zum Handgelenk positioniert und in den int16-Zahlenbereich (−32.768 bis 32.767) umgerechnet. Die Landmark-Daten werden als Dictionary mit den 21 benannten Schlüsseln und den zugehörigen \[x, y, z]-Koordinaten abgebildet. Zusätzlich wird die absolute Handgelenkposition (`wrist_pos`) in Skalierung auf den int16-Bereich sowie die Bounding-Box der Hand in Pixelkoordinaten (`hand_area`) gespeichert. Die absoluten Landmark-Positionen in Pixelkoordinaten (`landmark_pos`) werden ebenfalls erfasst.

Für eine einheitliche Darstellung aller Sequenzen erfolgt eine FPS-Normalisierung auf 60 FPS mittels linearer Interpolation zwischen den Frames. Frames, in denen eine Hand nicht erkannt wird, werden durch leere Handstrukturen mit der Sentinel-Koordinate \[−100, −100] repräsentiert und bei der Interpolation als Sprungpunkte behandelt.

#### 7.1.5 Datenstrukturen

Die verarbeiteten Geste- und Handdaten werden in den folgenden Datenstrukturen abgebildet:

**Gesture** - Repräsentation einer vollständigen Geste-Sequenz:
- `label`: Klasse der Geste (str)
- `fps`: Aktuelle Bildwiederholrate (float)
- `frames`: Liste von Frames, wobei jeder Frame eine Liste von zwei Hand-Objekten `[left, right]` enthält

**Hand** - Darstellung einer einzelnen Hand in einem Frame (frozen dataclass):
- `left_hand`: Seitenzugehörigkeit (bool)
- `wrist_pos`: Absolute Handgelenkposition in int16-Skalierung `[x, y]`
- `landmarks`: Dictionary mit 21 benannten Landmarks relativ zum Handgelenk
- `hand_area`: Bounding-Box in Pixelkoordinaten `(x, y, width, height)`
- `landmark_pos`: Absolute Landmark-Positionen in Pixelkoordinaten (21 × \[x, y])

#### 7.1.6 Speicherung

Die verarbeiteten Geste-Daten werden im Pickle-Format (`.pkl`) abgelegt. Der Dateiname folgt dem Muster `{Label}_{Augmentierungstyp}_{UUID}.pkl`, wobei die UUID die ersten vier Hexadezimalzeichen eines UUID4 darstellt. Die Speicherung erfolgt im Verzeichnis `resources/gestures/`. Die Verarbeitung der Videodateien und die Speicherung der Ergebnisse werden mittels Multiprocessing unter Auslastung von 80 % der verfügbaren CPU-Kerne parallelisiert.

### 7.3 Entwicklung und Training des Klassifikationsmodells

#### 7.3.1 Problemdefinition

Das zu entwickelnde Modell soll die Fähigkeit besitzen, Gesten bestimmten Gruppen zuzuordnen. Eine Gruppe entspricht dabei immer einer Geste. Diese Einteilung von Datenpunkten (Gesten) in vordefinierte Klassen (Gebärdenalphabet-Zeichen) beschreibt Klassifikationsmodelle. Für jede Geste sollen die Wahrscheinlichkeiten für die zuzuordnenden Klassen ausgegeben werden.

Es liegt keine Binäreklassifikation vor, da mehr als zwei Klassen zu unterscheiden sind. Mit den 26 Klassen (NONE, A–Y ohne J und Z, SCH) handelt es sich um eine Multiklassen-Klassifikation.

#### 7.3.2 Datenanalyse

**Input-Format:** Der Klassifikator empfängt einen 88-dimensionalen Feature-Vektor. Dieser setzt sich nach Ausgabe des Hand-Landmark Modells aus

- 44 Werten für die linke Hand (22 Gelenke × 2 Koordinaten)
- 44 Werten für die rechte Hand (22 Gelenke × 2 Koordinaten)

zusammen.

Die 22 Gelenke pro Hand umfassen die 21 MediaPipe-Landmarks sowie die absolute Handgelenkposition. Koordinaten werden wrist-relativ normalisiert und auf den int16-Bereich (−32.768 bis 32.767) skaliert.

**Klassenverteilung:** Der Datensatz umfasst 26 Klassen:

-  NONE (Index 0): Keine Hand erkannt
- A–Y (Indizes 1–24): 24 statische Buchstaben (J und Z ausgeschlossen, da diese dynamische Handbewegungen erfordern)
- SCH (Index 25): Sonderzeichen

Die Klassenverteilung ist leicht ungleichmäßig (16 Klassen mit 185, 8 Klassen mit 148, NONE mit 111, F mit 110 Dateien).

**Normalisierung:** Alle Koordinaten werden durch den Maximalwert des int16-Bereichs (POS_MAX = 32.767) dividiert, sodass die Werte im Bereich \[−1, 1] liegen.

#### 7.3.3 Modellauswahl

Für die Modellauswahl wurden verschiedene Ansätze evaluiert, wobei die Anforderungen des Embedded-Systems (schnelle Inferenz, kleine Modellgröße) und die Input-Struktur (88D-feature-Vektor aus MediaPipe-Landmarks) im Vordergrund standen.

Ein mehrschichtiges Perceptron (MLP) wurde als geeignetstes Modell
identifiziert. Die Input-Struktur war dafür Ausschlaggebend. Die MediaPipe wandelt die räumliche Information der Hand in strukturierte Landmarks um. Daher ist eine convolutionale
Verarbeitung (CNN) unnötig. Studien belegen, dass CNNs bei
Landmark-basiertem Input keine signifikanten Genauigkeitsvorteile
gegenüber MLPs aufweisen. (Fritz, 2025)

Random Forest und SVM wurden ebenfalls in Betracht gezogen, erreichen jedoch in der Literatur geringere Genauigkeiten (70–75 % bzw. 61–76 %) bei vergleichbaren Tasks. Zudem fehlen beiden Modellen nativ kalibrierte Wahrscheinlichkeiten: Random Forest gibt nur harte Klassifikationen aus, während SVM ein zusätzliches Platt-Scaling für Wahrscheinlichkeiten erfordert. (Rahman et al., 2025). Es ist anzumerken das SVM und Random forest auf einer anderen Datenbasis (Photoplethysmography (PPG)) trainiert worden sind.

Das MLP erfüllt es die Anforderungen an Ressourcenbeschränkungen und Echtzeitfähigkeit. Die Softmax-Ausgabeschicht liefert direkt kalibrierte Klassenwahrscheinlichkeiten, die für die nachfolgende Verarbeitung benötigt werden. Während direkte Quantisierung zu Genauigkeitsverlust führen kann, zeigen Studien, dass INT8-Quantisierung bei MLP-Modellen mit geeigneten Techniken (LayerNorm, Kalibrierung) Verluste von unter 1 % erreicht. Ein akzeptabler Kompromiss für den Embedded-Einsatz (blogdeveloperspot, 2025).

Ferner bestätigt der Ansatz von Google MediaPipe Model Maker die
Wahl: Dort werden Dense Layers (MLP) als Standard-Ansatz für
Landmark-basierte Gesture Recognition eingesetzt, was die
praktische Bewährtheit dieser Architektur unterstreicht.

#### 7.3.4 Modellarchitektur

Das Modell nutzt die Keras Sequential API:

```
             Input
           (88 × 1)
               │
               ▼
          Flatten
         (88 values)
               │
               ▼
      Dense(88, ReLU)
               │
               ▼
       Dropout(25%)
               │
               ▼
     Dense(128, ReLU)
               │
               ▼
       Dropout(50%)
               │
               ▼
 Dense(label_count, Softmax)
               │
               ▼
     Class probabilities
```
\[ Eigene Darstellung]

**Schichten-Erklärung:**

Kurzfassungen zu den verschieden Schicht-Typen. `Input` nimmt einen Input Tensor mit bestimmten Vektoreigenschaften entgegen. `Flatten` wandelt mehrdimensionale Tensoren (wie z.B 2D-Bilder) in einen einzigen, eindimensionalen Vektor. `Dense` fügt eine Schicht Neuronen mit entsprechender Aktivierungsfunktion sowie vollständiger Verknüpfung zum vorherigen Layer.  `Dropout(%)` setzt die Eingaben eines Layers zufällig auf 0  mit einer Wahrscheinlichkeit von `%`.

| Schicht            | Funktion                                                   |
| ------------------ | ---------------------------------------------------------- |
| Input(88, 1)       | Empfängt den normalisierten Feature-Vektor                 |
| Flatten            | Reduziert die Dimension für die Dense-Schicht              |
| Dense(88, ReLU)    | Lernt per-Feature-Transformationen                         |
| Dropout(0.25)      | Verhindert Overfitting (25% deaktiviert)                   |
| Dense(128, ReLU)   | Erweitert die Repräsentation für Inter-Feature-Beziehungen |
| Dropout(0.5)       | Stärkere Regularisierung vor der Ausgabe                   |
| Dense(26, Softmax) | Gibt Klassenwahrscheinlichkeiten aus (Summe = 1)           |

#### 7.3.5 Verlustfunktion und Optimierung

**Verlustfunktion: SparseCategoricalCrossentropy**

Die Verlustfunktion wird als SparseCategoricalCrossentropy gewählt, weil:

- Labels sind Integer-kodiert (nicht One-Hot)
- Spart Speicherplatz gegenüber One-Hot-Kodierung
- Gradientenverhalten ist numerisch stabil

Die Formel lautet:

$$L = -\sum(y_{true} \cdot \log(y_{pred}))$$

wobei $y_{true}$ die Integer-Klasse und $y_{pred}$ die Softmax-Ausgabe ist.

**Optimierer: Adam**

Adam (Adaptive Moment Estimation) wird mit einer sehr kleinen Lernrate von 0,00001 verwendet, um:

- Stabile Konvergenz auf kleinem Datensatz zu gewährleisten
- Per-Parameter-Lernraten automatisch anzupassen
- Die Adaptive Momentum-Schätzung für schnelleres Training zu nutzen

#### 7.3.6 Trainingsprozess

**Hyperparameter:**

| Parameter | Wert | Begründung |
|-----------|------|------------|
| Optimizer | Adam | Adaptiver Optimierer für kleine Datensätze |
| Learning Rate | 0,00001 | Stabile Konvergenz |
| Verlustfunktion | SparseCategoricalCrossentropy | Integer-kodiertete Labels |
| Metrik | sparse_categorical_accuracy | Anteil korrekt klassifizierter Samples |
| Epochen | 20 | Maximaler Trainingzeitraum |
| Validation Split | 0,2 (80/20) | Unabhängige Evaluierung |
| Batch Size | 128 | Balance zwischen Gradient-Noise und Generalisierung |
| Early Stopping | patience=3 | Stoppt bei 3 Epochen ohne Verbesserung |

**Early Stopping:** Das Training wird automatisch beendet, wenn sich die Validierungsverluste über 3 aufeinanderfolgende Epochen nicht verbessern. Dies verhindert Overfitting ohne manuelle Nachsteuerung.

**Augmentation:** Die Trainingsdaten werden mittels einer Augmentationspipeline vervielfacht:

- Mirror: Horizontale Spiegelung (vertauscht links/rechts)
- Random Translate: ±10.922 int16-Einheiten (2× pro Geste)
- Random Zoom: Faktor 0,5–1,5 (5× pro Geste)

#### 7.3.7 Evaluierung

**Trainingsergebnisse:**

| Metrik | Wert |
|--------|------|
| Trainingsgenauigkeit | 98,34 % |
| Validierungsgenauigkeit | 99,21 % |
| Finaler Trainingsverlust | 0,0754 |
| Finaler Validierungsverlust | 0,0319 |

Das Modell zeigt keine Anzeichen von Overfitting: Der Validierungsverlust sinkt kontinuierlich über alle 20 Epochen.

**Modellgröße:** Das quantisierte INT8-Modell benötigt nur 31,83 KB Speicher und inferiert in durchschnittlich 0,005 ms – ideal für Embedded-Einsatz.


### 7.4 Quantisierung und Konvertierung der Modelle

### 7.5 Implementierung der Kamerapipeline

### 7.6 Handflächen- und Landmark-Erkennung

### 7.7 Region of Interest


### 7.8 Vor- und Nachverarbeitungsschritte

#### 7.8.1 DCMIPP Embedded Processing

#### 7.8.2 Palm Detection Postprocessing

#### 7.8.3 Hand Landmark Preprocessing

#### 7.8.4 Hand Landmark Postprocessing

#### 7.8.5 Klassifizierungsmodell Preprocessing

#### 7.8.6 Klassifizierungsmodell Postprocessing

### 7.9 Integration der neuronalen Netze auf dem Embedded-System

### 7.10 Ablaufsteuerung und Software-Scheduler

### 7.11 Visualisierung und Benutzerausgabe