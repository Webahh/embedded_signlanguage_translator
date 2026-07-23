
## 7 Entwicklung und Implementierung

### 7.1 Aufbau und Aufbereitung des Datensatzes

#### 7.1.1 Rohvideos

Der Datensatz basiert auf einer eigens erstellten Sammlung von Videoaufnahmen der deutsche Gebärdensprache (DGS) und wurde speziell für die Klassifikation statischer Fingeralphabetzeichen konzipiert. Die Rohvideos wurden manuell aufgezeichnet und liegen im MP4-Format vor.

Die Benennungskonvention der Videodateien lautet `alph_[og|fw]_[Label][Nummer].mp4`, wobei `[og|fw]` den aufzeichnenden protokolliert, `[Label]` das jeweilige Fingersprachezeichen und `[Nummer]` die fortlaufende Nummer innerhalb eines Labels spezifiziert. Die Speicherung erfolgt im Verzeichnis `ml_pipeline/resources/videos_prototype/`.

Der Datensatz umfasst 226 Rohvideos, die auf 26 Klassen verteilt sind. Die Klassenauswahl beschränkt sich auf die 24 statischen Buchstaben A–Y (ohne J und Z, da diese dynamische Handbewegungen erfordern und somit mit dem statischen Ansatz des Klassifikators nicht kompatibel sind), das Sonderzeichen SCH sowie die Leerklasse NONE, die die Unbestimmtheit des Ergebnisses darstellt. Die Anzahl der Originaufnahmen pro Klasse variiert zwischen 3 und 5, wobei 16 Klassen mit je 5, 8 Klassen mit je 4 und 2 Klassen (F, NONE) mit je 3 Aufnahmen vertreten sind.

#### 7.1.2 Frame-Extraktion und Handdetektion

#TODO MediaPipe Hand/Palm Detection

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

#TODO Pickle Inspect Gegenüberstellung der Gesten nach Augmentation 

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

### 7.2 Entwicklung und Training des Klassifikationsmodells

#### 7.2.1 Problemdefinition

Das zu entwickelnde Modell soll die Fähigkeit besitzen, Gesten bestimmten Gruppen zuzuordnen. Eine Gruppe entspricht dabei immer einer Geste. Diese Einteilung von Datenpunkten (Gesten) in vordefinierte Klassen (Gebärdenalphabet-Zeichen) beschreibt Klassifikationsmodelle. Für jede Geste sollen die Wahrscheinlichkeiten für die zuzuordnenden Klassen ausgegeben werden.

Es liegt keine Binäreklassifikation vor, da mehr als zwei Klassen zu unterscheiden sind. Mit den 26 Klassen (NONE, A–Y ohne J und Z, SCH) handelt es sich um eine Multiklassen-Klassifikation.

#### 7.2.2 Datenanalyse

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

#### 7.2.3 Modellauswahl

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

#### 7.2.4 Modellarchitektur

Das Modell nutzt die Keras Sequential API:

![[Sequential MLP.drawio.png]]
\[ Eigene Darstellung]

Das Klassifikationsmodell setzt sich aus sieben Schichten zusammen, die sequenziell eine direkte Informationsflussrichtung von der Eingabe zur Ausgabe aufweisen. Die Architektur folgt dem Prinzip der schrittweisen Transformation und Abstraktion der Eingabemerkmale. Nachdem der Input-Tensor in eine eindimensionale Darstellung überführt wurde, transformiert die erste Dense-Schicht die Rohmerkmale in einen repräsentativen Feature-Raum. Die nachfolgende Dense-Schicht erweitert diesen Raum, um komplexere Inter-Feature-Beziehungen modellieren zu können. Zwischen den Dense-Schichten werden Dropout-Schichten eingebunden um Zufall in das Modell zu bringen, um Overfitting zu verhindern. Die finale Dense-Schicht projeziert die gelernten Repräsentationen auf die 26 Ausgabeklassen.

Der Input-Layer empfängt einen tensor der Form `(88, 1)`. Dieser Tensor repräsentiert die normalisierten Merkmale zweier Hände mit je 44 Werten. Pro enthalten sind 21 MediaPipe-Hand-Landmarks jeweils als `(x,y)`-Koordinate sowie die absolute Handgelenkposition ebenfalls als `(x,y)`. Alle Koordianten sind Handgelenksrelativ normalisiert und auf den int15-Bereich skaliert, sodass der Input-Wertebereich \[-1, 1] beträgt.

Der Flatten-Layer überführt den mehrdimensionalen Input-Tensor in einen eindimensionalen Vektor der Länge 88. Dense-Schichten erwarten als Eingabe einen flachen Vektor, sodass die räumliche Struktur des Tensors (2 Hände x 44 Werte) für die nachfolgende Gewichtsmatrix aufgelöst werden kann.

Der erste Dense-Layer mit 88 Neuronen und ReLU-Aktivierung führt per-Feature-Transformationen durch. Das bedeutet durch die  Gewichtsmatrix (88 × 88) und den Bias-Vektor wird jedes Eingabemerkmal einzeln transformiert. Die ReLU-Aktivierung (`f(x) = max(0, x)`) fügt Nicht-Linearität hinzu und ermöglicht es dem Modell, nicht-triviale Muster in den Landmark-Daten zu erkennen. (Sharma et al., 2020) Die Wahl von 88 Neuronen hält die Kapazität zunächst auf gleicher Dimension wie der Input und zwingt das Modell, die Merkmale in einem äquivalenten Raum abzubilden, bevor eine Dimensionserweiterung erfolgt.

Der Dropout-Layer (0.25) deaktiviert während des Trainings zufällig 25% der Ausgaben des vorherigen Dense-Layers. Jedes Neuron wir mit einer Wahrscheinlichkeit von 0.25 auf den Wert 0 gesetzt. Die Summe über alle Neuronen bleibt dabei Gleich. Dies verhindert die Ko-Adaptierung von Neuronen, bei der sich mehrere Neuronen auf ein einzelnes Eingabemuster spezialisieren und dadurch das Modell anfällig für Overfitting wird. (Ying, 2019) 

Der zweite Dense-layer mit 128 Neuronen und ReLU-Aktivierung erweitert den Feature-Raum von 88 auf 128 Dimensionen. Durch die Erhöhung der Kapazität kann des Modell Inter-Feature-Beziehungen modellieren, die über die per-Feature-Transformationen des vorherigen Layers hinausgehen. Typische Beziehungen umfassen:
- Intra-Hand-Abhängigkeiten: Korrelation zwischen benachbarten Landmarks des Selben fingers (z.B. die Auslenkung von PIP und DIP des Indexfingers), zwischen verschiedenen fingern einer Hand sowie  die Orientierung der Hand und deren Rotation basierend auf der Handflächen features. 
- Inter-Hand-Beziehungen: Zusammenhänge zwischen den Landmarks beider Hände, die bei einhändigen Gesten durch symmetrische oder asymmetrische Konfigurationen entstehen können.
- Komplexe Muster: Nicht-lineare Kombinationen mehrerer Landmarks, die erst in der erweiterten Repräsentation als discriminative Merkmale erkennbar werden.

Der zweite Dropout-Layer (0.5) wendet eine stärkere Regularisierung an als der erste Dropout-Layer. Die Rate von 50 % ist bewusst höher gewählt, da der vorherige Dense-Layer mit 128 Neuronen eine größere Modellkapazität aufweist und die Gefahr des Overfitting steigt. Die stärkere Regularisierung vor der Ausgabeschicht stellt sicher, dass das Modell nur die robustesten Feature-Repräsentationen für die finale Klassifikation verwendet.

Der dritte Dense-Layer mit 26 Neuronen und Softmax-Aktivierung bildet die finale Klassifikationsschicht. Die 26 Ausgabeneuronen entsprechen den 26 Klassen (NONE, A–Y ohne J und Z, SCH). Die Softmax-Funktion $$σ(z_i) = \frac{e^{z_i}}{Σ e^{z_j}}$$normalisiert (Sharma et al., 2020). Die Rohausgaben (Logits) in Wahrscheinlichkeiten, deren Summe exakt 1 ergibt. Jedes Ausgabeneuron liefert damit die Wahrscheinlichkeit dafür, dass der Input der entsprechenden Gebärdensprache-Klasse angehört.

#### 7.2.5 Verlustfunktion und Optimierung

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

#### 7.2.6 Trainingsprozess

**Hyperparameter:**

| Parameter        | Wert                          | Begründung                                          |
| ---------------- | ----------------------------- | --------------------------------------------------- |
| Optimizer        | Adam                          | Adaptiver Optimierer für kleine Datensätze          |
| Learning Rate    | 0,00001                       | Stabile Konvergenz                                  |
| Verlustfunktion  | SparseCategoricalCrossentropy | Integer-kodiertete Labels                           |
| Metrik           | sparse_categorical_accuracy   | Anteil korrekt klassifizierter Samples              |
| Epochen          | 20                            | Maximaler Trainingzeitraum                          |
| Validation Split | 0,2 (80/20)                   | Unabhängige Evaluierung                             |
| Batch Size       | 128                           | Balance zwischen Gradient-Noise und Generalisierung |
| Early Stopping   | patience=3                    | Stoppt bei 3 Epochen ohne Verbesserung              |

**Early Stopping:** Das Training wird automatisch beendet, wenn sich die Validierungsverluste über 3 aufeinanderfolgende Epochen nicht verbessern. Dies verhindert Overfitting ohne manuelle Nachsteuerung.

#### 7.2.7 Evaluierung

**Trainingsergebnisse:**

| Metrik                      | Wert    |
| --------------------------- | ------- |
| Trainingsgenauigkeit        | 98,34 % |
| Validierungsgenauigkeit     | 99,21 % |
| Finaler Trainingsverlust    | 0,0754  |
| Finaler Validierungsverlust | 0,0319  |

**Confusionsmatrix**:
![[confusion_matrix.png]]
![[confusion_matrix_normalized.png]]

Das Modell zeigt keine Anzeichen von Overfitting: Der Validierungsverlust sinkt kontinuierlich über alle 20 Epochen.

**Modellgröße:** Das quantisierte INT8-Modell benötigt nur 31,83 KB Speicher und inferiert in durchschnittlich 0,005 ms – ideal für Embedded-Einsatz.


### 7.3 Quantisierung und Konvertierung der Modelle

### 7.4 Embedded Projektstruktur/Treiber

### 7.5 Kamera - Display Pipeline

### 7.6 Ablaufsteuerung und Software Scheduler

### 7.7 NPU Integration & AI Interface

### 7.8 Vor- und Nachverarbeitungsschritte

Die drei neuronalen Netze verwenden unterschiedliche Eingabedaten und liefern Ausgaben, die nicht unmittelbar durch das jeweils nachfolgende Modell verarbeitet werden können. Daher sind zwischen den einzelnen Inferenzschritten mehrere Vor- und Nachverarbeitungen erforderlich. Diese umfassen sowohl hardwaregestützte Operationen innerhalb der DCMIPP als auch durch den Hauptprozessor ausgeführte Transformationen der Bild- und Landmark-Daten.

Die DCMIPP übernimmt die grundlegende Aufbereitung der Kamerabilder. Die nachgelagerten Verarbeitungsschritte umfassen die Nachverarbeitung der Handdetektion, die Bildung der Region of Interest, die Vor- und Nachverarbeitung des Landmark-Modells sowie die Aufbereitung der Landmark-Koordinaten für das Klassifikationsmodell.

#### 7.8.1 DCMIPP Embedded Processing

Die Kamera überträgt die aufgenommenen Bilder als RAW-Bayer-Daten mit einer Auflösung von 2592 x 1944 Pixeln über die CSI-Schnittstelle. Da weder die Displayausgabe noch die neuronalen Netze diese Daten unmittelbar verarbeiten können, übernimmt die Digital Camera Interface Pixel Pipeline (DCMIPP) einen wesentlichen Teil der erforderlichen Bildvorverarbeitung.

Die DCMIPP wandelt die RAW-Bayer-Daten zunächst in ein RGB-Bild um. Hierzu werden die Demosaikierung der RAW10-Bilddaten, die Farbkorrektur, die Belichtungsanpassung und die Gammakorrektur innerhalb der Bildpipeline durchgeführt. Das Ergebnis wird im RGB888-Format ausgegeben. Dadurch müssen diese rechenintensiven Verarbeitungsschritte nicht nachträglich durch den Hauptprozessor auf den bereits gespeicherten Kamerabildern ausgeführt werden.

Die verwendeten Parameterwerte wurden empirisch bestimmt. Hierzu wurden die entsprechenden DCMIPP-Register schrittweise angepasst und die Auswirkungen auf das ausgegebene Kamerabild visuell bewertet. Einstellungen, die unter den vorgesehenen Einsatzbedingungen eine geeignete Bilddarstellung ergaben, wurden anschließend in die feste Konfiguration der Bildpipeline übernommen.

Aus dem gemeinsamen Kameradatenstrom werden zwei getrennte Ausgabepfade erzeugt. Pipe 1 dient der Displayausgabe und stellt gleichzeitig das Ausgangsbild für die Landmark-Erkennung bereit. Pipe 2 erzeugt dagegen das Eingabebild für die Handdeteketion.

Für Pipe 1 wird das Sensorbild zunächst auf das Seitenverhältnis des Displays zugeschnitten. Der horizontale Bildbereich bleibt vollständig erhalten, während das Bild vertikal zentriert beschnitten wird. Der entstandene Ausschnitt wird anschließend durch die Downscaling-Einheit der DCMIPP auf eine Auflösung von 800 x 480 Pixeln verkleinert und im RGB888-Format im externen PSRAM abgelegt. Die von Pipe 1 erzeugten Bilder werden über vier Hintergrundbildpuffer verwaltet. Die DCMIPP schreibt jeweils in den für die Aufnahme vorgesehenen Puffer, während weitere Puffer für die Displayausgabe, die KI-Verarbeitung und das Einzeichnen der Handregion beziehungsweise der Landmarks verwendet werden. Nach Abschluss eines Bildes wird die Zieladresse der DCMIPP auf den nächsten Aufnahmepuffer umgeschaltet.

Pipe 2 verarbeitet den vollständigen Sensorbereich und erzeugt das Eingabebild für die Handdetektion. Da deren Eingangsauflösung mit 192 x 192 Pixeln deutlich unterhalb der Sensorauflösung liegt, wird das Bild vor der eigentlichen Skalierung horizontal und vertikal dezimiert. Dadurch reduziert sich die Auflösung zunächst von 2592 x 1944 auf 1296 x 972 Pixel. Anschließend übernimmt der DCMIPP-Downsizer die Skalierung auf 192 x 192 Pixel. Da vor der Skalierung kein quadratischer Bildausschnitt gebildet wird, wird das Seitenverhältnis des vollständigen Sensorbildes dabei an die quadratische Modelleingabe angepasst. Auch dieser Ausgabepfad verwendet das RGB888-Format.

Für Pipe 2 wird der Double-Buffer-Modus der DCMIPP verwendet. Die erzeugten Bilder werden abwechselnd in zwei Eingabebildpuffern gespeichert. Während die DCMIPP einen Puffer mit einem neuen Bild beschreibt, kann der zuvor fertiggestellte Puffer für die Handdetektion verwendet werden. Der jeweils abgeschlossene Puffer wird über den Frame-Callback bestimmt und für die weitere Verarbeitung markiert.

Die Konfiguration der beiden Bildpfade wird jeweils durch eine globale Konfigurationsstruktur festgelegt. Die darin enthaltenen Parameter wurden aus den Anforderungen der beiden Verarbeitungspfade und den verwendeten Bildformaten abgeleitet.

| Parameter                         | Pipe 1                         | Pipe 2      | Begründung                                                                                                                                                                                   |
| --------------------------------- | ------------------------------ | ----------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `output_width`<br>`output_height` | 800 x 480                      | 192 x 192   | Entsprechen der Displayauflösung, bzw. Eingangsgröße der Handdetection.                                                                                                                      |
| `output_format`                   | RGB888                         | RGB888      | Display und KI-Vorverarbeitung arbeiten mit drei Farbkanälen und jeweils 8 Bit pro Kanal.                                                                                                    |
| `output_bpp`                      | 3                              | 3           | RGB888 benötigt drei Byte pro Pixel (für jeden Kanal 8 Bit)<br>Der Wert wird unter anderem zur Berechnung des Speicher-Pitchs verwendet.                                                     |
| `enable_crop`                     | aktiviert                      | aktiviert   | Pipe 1 benötigt einen Zuschnitt auf das Seitenverhältnis des Displays. <br><br>Pipe 2 verarbeitet den vollständigen Sensorbereich.                                                           |
| `crop_x`                          | 0                              | 0           | Horizontal wird kein Bereich abgeschnitten.                                                                                                                                                  |
| `crop_y`                          | Zentriert vertikaler Zuschnitt | 0           | Pipe 1 entfernt oben und unten Bildbereiche, um das Sensorformat ohne Verzerrung an 800 x 480 anzupassen. <br><br>Pipe 2 verwendet das vollständige Bild.                                    |
| `crop_width`                      | 2592                           | 2592        | Die vollständige Sensorbreite wird verwendet.                                                                                                                                                |
| `crop_height`                     | ca. 1555                       | 1944        | Pipe 1 erhält das Displayseitenverhältnis<br>5 : 3. <br><br>Pipe  2 behält die vollständige Senorhöhe.                                                                                       |
| `enable_downsize`                 | aktiviert                      | aktiviert   | Beide Ausgaben sind wesentlich kleiner als das Sensorbild.                                                                                                                                   |
| `enable_decimate`                 | deaktiviert                    | aktiviert   | Nur Pipe 2 benötigt vor dem Downscaling eine zusätzliche Halbierung.                                                                                                                         |
| `decimate_h`<br>`decimate_v`      | -                              | 1, 1        | Die Auflösung wird horizontal und vertikal jeweils durch zwei geteilt.                                                                                                                       |
| `enable_swap`                     | deaktiviert                    | deaktiviert | Die Farbkanäle sind bereits korrekt. Ein Swap ist nicht notwendig.                                                                                                                           |
| `enable_gamma`                    | aktiviert                      | aktiviert   | Die Gammakorrektur verbessert die Helligkeitsdarstellung.                                                                                                                                    |
| `enable_dbm`                      | deaktiviert                    | aktiviert   | Pipe 1 verwendet eine softwareseitige Rotation von vier Bildpuffern, da die Anwendung mehr als nur 2 Puffer benötigt.<br><br>Pipe 2 verwendet zwei hardwareseitig wechselnde Eingabepuffer.  |
Tabelle X: Konfigurationsparameter der DCMIPP-Bildpfade

Zusätzlich zur Erzeugung des Displaybildes werden für Pipe 1 drei Statistikkanäle aktiviert. Diese erfassen Helligkeits- und Farbwerte innerhalb eines zentralen Bildbereichs mit einer Größe von 1296 x 972 Pixeln. Die ermittelten Statistiken werden von der automatischen Belichtungsregelung ausgewertet und zur Anpassung der Belichtungszeit des Kamerasensors verwendet.

Durch die hardwaregestützte Vorverarbeitung werden nur die tatsächlich benötigten Bildauflösungen in den externen Speicher geschrieben. Dies reduziert sowohl den Speicherbedarf als auch die vom Hauptprozessor zu verarbeitende Datenmenge. Die weiteren auf der CPU ausgeführten Vor- und Nachverarbeitungsschritte können dadurch unmittelbar auf den bereits aufbereiteten RGB888-Bilddaten arbeiten.

#### 7.8.2 Palm Detection Postprocessing

#### 7.8.3 Hand Landmark Preprocessing

#### 7.8.4 Hand Landmark Postprocessing

#### 7.8.5 Klassifizierungsmodell Preprocessing

#### 7.8.6 Klassifizierungsmodell Postprocessing

### 7.9 Visualisierung und Benutzerausgabe