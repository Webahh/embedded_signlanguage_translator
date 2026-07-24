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

#### 7.1.5 Datenstrukturen und Speicherung

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

Das MLP erfüllt die Anforderungen an Ressourcenbeschränkungen und Echtzeitfähigkeit. Die Softmax-Ausgabeschicht liefert direkt kalibrierte Klassenwahrscheinlichkeiten, die für die nachfolgende Verarbeitung benötigt werden. Während direkte Quantisierung zu Genauigkeitsverlust führen kann, zeigen Studien, dass INT8-Quantisierung bei MLP-Modellen mit geeigneten Techniken (LayerNorm, Kalibrierung) Verluste von unter 1 % erreicht. Ein akzeptabler Kompromiss für den Embedded-Einsatz (blogdeveloperspot, 2025).

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

Für das Training des Modells wird die Verlustfunktion Sparse Categorical Crossentropy verwendet. Diese eignet sich insbesondere, da die Zielklassen als Integer und nicht als One-Hot-Vektoren kodiert vorliegen. Dadurch wird der Speicherbedarf reduziert und gleichzeitig eine numerisch stabile Berechnung der Gradienten ermöglicht. Die Verlustfunktion berechnet die Abweichung zwischen den tatsächlichen Klassen und den durch die Softmax-Ausgabe vorhergesagten Klassenwahrscheinlichkeiten und lässt sich durch folgende Formel beschreiben:

$$L = -\log(p_{y_{true}})$$
(What Is Sparse Categorical Crossentropy, 10:49:00+00:00) wobei $p_{y_{true}}$​​ die vom Modell vorhergesagte Wahrscheinlichkeit der tatsächlichen Klasse bezeichnet. 

#### 7.2.6 Trainingsprozess

Um einen Trainingsprozess durchlaufen zu können müssen Hyperparameter bestimmt werden. Zentral sind Optimizer, Lernrate, Verlustfunktion, Epochen, Trainingssplit, Batch Size und Frühzeitiges Stoppen.

Der Adam-Optimizer (Adaptive Moment Estimation) kombiniert die Vorteile von Momentum und adaptiven Lernraten (RMSProp). Er berechnet für jeden Parameter individuelle Lernraten auf Basis des ersten und zweiten Moments der Gradienten. Dadurch konvergiert das Modell in der Regel schneller und stabiler als klassische Optimierungsverfahren wie Stochastic Gradient Descent (SGD). Insbesondere bei kleinen oder mittelgroßen Datensätzen sowie komplexen neuronalen Netzen liefert Adam häufig robuste Ergebnisse, da weniger aufwendige Hyperparameteranpassungen erforderlich sind. (A. G. et al., 2026)

Die Lernrate bestimmt die Größe der Aktualisierungsschritte während der Optimierung. Eine sehr kleine Lernrate von $10^{-5}$ reduziert das Risiko, das Minimum der Verlustfunktion zu überschreiten oder instabile Trainingsverläufe zu erzeugen. Der Nachteil einer geringeren Lernrate besteht in einer längeren Trainingsdauer, die jedoch durch eine stabilere Konvergenz ausgeglichen wird.

Als Verlustfunktion eignet sich Sparse Categorical Crossentropy für Mehrklassenklassifikationen, bei denen die Zielklassen als Ganzzahlen (z. B. 0, 1, 2, …) codiert sind. Im Gegensatz zur Categorical Crossentropy ist keine One-Hot-Kodierung der Labels erforderlich, wodurch Speicherbedarf und Vorverarbeitungsaufwand reduziert werden. Die Verlustfunktion misst die Abweichung zwischen den vorhergesagten Klassenwahrscheinlichkeiten und den tatsächlichen Klassen und liefert somit eine geeignete Optimierungsgrundlage für Klassifikationsaufgaben.

Die Sparse categorical Accuracy Metrik gibt den Anteil der korrekt klassifizierten Beispiele an und stellt damit eine leicht interpretierbare Leistungskennzahl dar. Da die Zielklassen als Integer vorliegen, passt sie direkt zur verwendeten Verlustfunktion SparseCategoricalCrossentropy. Während die Verlustfunktion zur Optimierung dient, ermöglicht die Accuracy eine intuitive Bewertung der Modellleistung während des Trainings und auf den Validierungsdaten.

Eine Epoche entspricht einem vollständigen Durchlauf des Trainingsdatensatzes. Die Wahl von 20 Epochen bietet dem Modell ausreichend Möglichkeiten, die zugrunde liegenden Muster zu erlernen, ohne die Trainingszeit unnötig zu verlängern. Da zusätzlich Early Stopping eingesetzt wird, dient dieser Wert als maximale Obergrenze. Das Training endet automatisch früher, sobald keine Verbesserung der Validierungsleistung mehr festgestellt wird.

Ein weiterer Hyperparameter ist die Aufteilung der Trainingsdaten in Training, Validation und Testdaten. Durch die Aufteilung des Datensatzes in 64 % Trainingsdaten, 16% Validierungsdaten und 20% Testdaten kann die Generalisierungsfähigkeit des Modells während des Trainings überprüft werden. Die Validierungsdaten werden nicht für das Lernen verwendet und ermöglichen daher eine unabhängige Beurteilung der Modellleistung.

Die Batch Size legt fest, wie viele Trainingsbeispiele gleichzeitig verarbeitet werden, bevor eine Aktualisierung der Modellgewichte erfolgt. Eine Batchgröße von 128 stellt einen guten Kompromiss zwischen Rechenleistung, Speicherbedarf und Trainingsstabilität dar. Größere Batches ermöglichen eine effizientere Nutzung von GPU Hardware dar und liefern stabilere Gradienten, während kleinere Batches zwar stärkeres Rauschen erzeugen, jedoch teilweise eine bessere Generalisierung fördern. Die gewählte Batchgröße bietet daher eine ausgewogene Balance zwischen Trainingsgeschwindigkeit und Modellqualität.

Early Stopping dient der Vermeidung von Overfitting, indem das Training automatisch beendet wird, sobald sich die Validierungsleistung über mehrere Epochen hinweg nicht mehr verbessert. Mit einer Patience von 3 werden kleinere Schwankungen der Validierungsverluste toleriert, bevor das Training gestoppt wird. Dadurch wird verhindert, dass unnötig lange trainiert wird oder das Modell beginnt, sich zu stark an die Trainingsdaten anzupassen. Gleichzeitig wird das Modell mit der besten Validierungsleistung gespeichert bzw. verwendet.

Zusammenfassend:

| Parameter        | Wert                            |
| ---------------- | ------------------------------- |
| Optimizer        | Adam                            |
| Learning Rate    | 0,00001                         |
| Verlustfunktion  | Sparse Categorical Crossentropy |
| Metrik           | Sparse Categorical Accuracy     |
| Epochen          | 20                              |
| Validation Split | 0,2 (80/20)                     |
| Batch Size       | 128                             |
| Early Stopping   | 3                               |

#### 7.2.7 Evaluierung

Die Evaluierung der Ergebnisse des Machine Learning Modells sind in Kapitel 8 zu finden.
#TODO Füge kapitelreferenz ein!

### 7.3 Quantisierung, Konvertierung und Deployment der Modelle

Das folgende Kapitel beschreibt die Quantisierung des Tensorflow Fingeralphabet Modells, die Convertierung der Quantisierten Handdedektion-, Landmarkerkennung- und Fingeralphabet- Modelle in NPU ausführbare Binaries. Im letzten Schritt wird das Flashen der Modelle auf den STM32N650DK beschrieben.

#### 7.3.1 Quantisierung von Tensorflow Modellen

Da die Modelle zur Handdetektion und Landmark-Erkennung bereits in einer quantisierten Form vorliegen, ist für diese keine weitere Quantisierung erforderlich. Lediglich das Modell zur Erkennung des Fingeralphabets muss für den Einsatz auf der Zielhardware quantisiert werden. Die Quantisierung erfolgt im Rahmen der Konvertierung des trainierten Keras-Modells in das TensorFlow-Lite-Format (TFLite). Dieser Schritt ist notwendig, da die Generierung der Embedded-Binärdateien mit STEdgeAI ausschließlich Modelle im TensorFlow-Lite- oder ONNX-Format unterstützt.

Bei der Konvertierung wird eine Post-Training-Quantisierung (PTQ) durchgeführt. Hierbei werden die Gewichte und Aktivierungen des bereits trainierten Modells von Gleitkommazahlen (Float32) auf 8-Bit-Ganzzahlen (INT8) abgebildet. Da die eingesetzte Neural Processing Unit (NPU) ausschließlich vollständig INT8-quantisierte Modelle unterstützt, müssen sowohl die internen Operationen als auch die Ein- und Ausgabedaten des Modells diesem Format entsprechen.

Zunächst wird das trainierte Keras-Modell geladen und ein Tensorflow-Lite-Konverter erzeugt.

```python
model = keras.models.load_model(MODEL_DIR)
converter = tf.lite.TFLiteConverter.from_keras_model(model)
```

Anschließend werden die Standardoptimierungen des Konverters aktiviert, wodurch unter anderem die Quantisierung ermöglicht wird:

```python
converter.optimization = [tf.lite.Optimize.DEFAULT]
```

Für die Kalibrierung der Quantisierung wird ein repräsentativer Datensatz bereitgestellt. Hierfür werden die normalisierten Trainingsdaten verwendet.

```python
converter.representative_dataset = representative_dataset
```

Während der Kalibrierung bestimmt TensorFlow Lite anhand dieser Daten die Wertebereiche der Aktivierungen, um geeignete Skalierungsfaktoren und Nullpunkte für die Quantisierung zu berechnen. Sind die Trainingsdaten nicht verfügbar, wird ersatzweise ein Datensatz aus gleichverteilten Zufallswerten im Bereich `[-1, 1]` verwendet.

Da die Zielhardware ausschließlich INT8-Operationen unterstützt, wird die Konvertierung entsprechend eingeschränkt und zusätzlich die Ein- und Ausgabedatentyp des Modells auf 8-Bit-Ganzzahlen festgelegt:
```python
converter.target_spec.supported_ops = [
    tf.lite.OpsSet.TFLITE_BUILTINS_INT8
]

converter.inference_input_type = tf.uint8
converter.inference_output_type = tf.uint8
```

Abschließend wird das Modell in das Tensorflow-Lite-Format konvertiert und als `.tflite`-Datei gespeichert.

Ergebnis ist ein vollständig quantisiertes TensorFlow-Lite-Modell, das den Anforderungen der eingesetzten NPU entspricht und anschließend mit STEdgeAI in eine für das Embedded-System ausführbare Binärdatei überführt werden kann. Durch die Quantisierung werden der Speicherbedarf und der Rechenaufwand reduziert, während die Modellgenauigkeit durch die Verwendung repräsentativer Kalibrierungsdaten weitgehend erhalten bleibt.
#### 7.3.2 Konvertierung INT8-Quantisierte-Modelle -> Embedded binaries

Nach der Quantisierung liegen alle drei Modelle (Handdetektion, Handlandmark-Erkennung und Fingeralphabet-Klassifikation) im TensorFlow-Lite-Format mit einer INT8-Quantisierung vor. Der nächste Verarbeitungsschritt ist diese Modelle mithilfe der ST Edge AI Core CLI in für die Zielhardware ausführbare Binärdateien zu überführen. Die Verwendung der Kommandozeilenschnittstelle (CLI) wurde bewusst der grafischen Benutzeroberfläche (GUI) vorgezogen, da sich hierdurch der gesamte Konvertierungsprozess automatisieren lässt. Auf diese Weise können Skripte erstellt werden, welche die Generierung aller Modellartefakte reproduzierbar und mit einem einzelnen Aufruf durchführen.

Die Umwandlungsstruktur:
```
models/
├── source/                           ← Raw TFLite models (input)
│   ├── 033_palm_detection_full_quant_pc_ff_od.tflite
│   ├── 033_hand_landmark_full_quant_pc_uf_handl.tflite
│   └── fingeralphabet_model_int8.tflite
│
├── my_mpools/                        ← Memory pool definitions per model
│   ├── palm_detection.mpool
│   ├── hand_landmark.mpool
│   └── fingeralphabet.mpool
│
├── user_neuralart.json   ← Per-model config (links model → mpool+compiler flags)
├── generate_n6_models.sh ← Orchestrator script (calls stedgeai, copies outputs)
├── copy_n6_models.sh     ← Copies generated files into the firmware project tree
│
├── generated/            ← Output: C source + headers ready for compilation
│   ├── palm/
│   │   ├── palm_detection_model_v3.c         ← Weight data as C arrays (ECBLOBs)
│   │   ├── palm_detection_model_v3_ecblobs.h ← Header declaring weight arrays
│   │   ├── stai_palm_detection_model_v3.c    ← ST AI runtime wrapper
│   │   └── stai_palm_detection_model_v3.h    ← Public API header
│   ├── landmark/
│   │   ├── hand_landmark_model_v3.c
│   │   ├── hand_landmark_model_v3_ecblobs.h
│   │   ├── stai_hand_landmark_model_v3.c
│   │   └── stai_hand_landmark_model_v3.h
│   └── finger/
│       ├── fingeralphabet_model_v3.c
│       ├── fingeralphabet_model_v3_ecblobs.h
│       ├── stai_fingeralphabet_model_v3.c
│       └── stai_fingeralphabet_model_v3.h
│
└── st_ai_output/                 ← Intermediate build artifacts (gitignored)
    └── *.raw, *.bin, *.hex       ← Binary weight blobs + Intel HEX for flashing
```

**Modell Profile**
In der Struktur definiert `user_neuralart.json` wie die einzelnen Modelle zu behandeln sind. Sie enthält einen Abschnitt `Profiles` mit jeweils einem Eintrag pro Modell:

```json
{
  "Profiles": {
    "palm_detection_model_v3": {
      "memory_pool": "./my_mpools/palm_detection.mpool",
      "options": "--enable-epoch-controller
			     "-O3" 
			     "--all-buffers-info"
			     "--cache-maintenance"
			     "..."
    }
  }
}
```

`memory_pool`
Verweist auf die .mpool-Datei, die das Speicherlayout der NPU für dieses Modell definiert (welche SRAM-Bänke, Flash-Bereiche, Adressen und Größen).

`options` - ST Edge AI Core compiler flags

| Flag                                | Effekt                                                                                                                                                                                                                                  |
| ----------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| --enable-epoch-controller           | Ermöglicht die epochenbasierte Ausführung der NPU (schichtweise Ablaufplanung)                                                                                                                                                          |
| -O3/-O2                             | Optimierungsstufe für den NPU-Codegenerator                                                                                                                                                                                             |
| --all-buffers-info                  | Vollständige Puffer-Metadaten für die Laufzeit ausgeben                                                                                                                                                                                 |
| --cache-maintenance<br>--Ocache-opt | Befehle zum Leeren/Ungültigmachen des Caches einfügen                                                                                                                                                                                   |
| --Oauto-sched                       | Automatisches planen der Epochen (default/Ohne verweis)                                                                                                                                                                                 |
| --native-float                      | Verwendet hardware float operationen wenn möglich                                                                                                                                                                                       |
| --enable-virtual-mem-pools          | Erlaubt dem linker Tensoren über mehrere physische Speicherregionen zu verteilen                                                                                                                                                        |
| --Omax-ca-pipe \<num>               | Legt die maximale Anzahl \<num> an Compute-&-Accumulate- (CA-)Pipelines bzw. parallelen Hardware-Ausführungspfaden fest, die der Compiler für eine bestimmte Schicht oder Operation eines neuronalen Netzes gleichzeitig zuweisen darf. |
| --Oconv-split-kw                    | Aufteilen von Faltungskernen auf die verfügbaren NPU-MAC-Arrays                                                                                                                                                                         |
\[(ST Neural-ART Compiler Primer, n.d.)]

**Memory-Pool-Definition**
Die .mpool Dateien beschreiben die STM32N6 Hardware Speicherabbild der NPU. Alle .mpool Dateien haben den identischen Aufbau. Der einzige wesentliche Unterschied zwischen den Modellen besteht in der xSPI2 (flash) Region (Basisadresse und Größe) an welcher die Modelle gespeichert sind.

Hauptbereiche:

`params` - Globale Einschränkungen:
```json
{ "paramname": "max_onchip_sram_size", "value": "1024", "magnitude": "KBYTES"}
```
Beschränkt die maximale verfügbare onchip sram größe auf 1024 KB für die NPU.  

`cacheinfo` - Beschreibt den cache
`mempools` - Speicherbereich Konfiguration pro Modell auf der NPU.

| Name      | Region   | Adresse    | Größe | Rolle                           |
| --------- | -------- | ---------- | ----- | ------------------------------- |
| npuRAM3   | AXISRAM3 | 0x34200000 | 448KB | NPU activation scratch          |
| npuRAM4   | AXISRAM4 | 0x34270000 | 448KB | NPU activation scratch          |
| npuRAM5   | AXISRAM5 | 0x342e0000 | 448KB | NPU activation scratch          |
| npuRAM6   | AXISRAM6 | 0x34350000 | 448KB | NPU activation scratch          |
| hyperRAM  | xSPI1    | 0x90000000 | 0MB   | External HyperRAM (Deaktiviert) |
| octoFlash | xSPI2    | 0x71000000 | 2MB   | Weight storage in OctoSPI flash |
Die vier AXISRAM-Bänke sind On-Chip-SRAM-Speicher mit hohem Durchsatz und geringer Latenz, die für Laufzeit-Aktivierungen (Zwischentensoren) genutzt werden. octoFlash ist zur Laufzeit schreibgeschützt (ACC_READ) und speichert die Modellgewichte bzw. -konstanten. Die Einstellung constants_preferred: "true" weist das Tool an, konstante Daten in diesem Bereich abzulegen.

Die Flash-Basisadressen unterscheiden sich je nach Modell, um den 16-MB-xSPI2-Flash zu partitionieren:

| Modell        | Flash Start | Flash Size |
| ------------- | ----------- | ---------- |
| Finger        | 0x71000000  | 2 MB       |
| Handdedektion | 0x71200000  | 4 MB       |
| Handlandmark  | 0x71600000  | 10 MB      |
**Script - Generierung**
Das `generate_n6_models.sh`-Script führt für jedes Modell 3 Schritte durch dann Kopiert und Builded anschließend das Projekt.

Schritt 1 - STedgeAI generation:

```bash
stedgeai generate \
    --name palm_detection_model_v3 \
    --model source/033_palm_detection_full_quant_pc_ff_od.tflite \
    --target stm32n6 \
    --st-neural-art "palm_detection_model_v3@user_neuralart.json" \
    --input-data-type uint8 \
    --output st_ai_output/
```

Dies liest das Tensorflow Lite modell ein, lädt die `palm_detection_model_v3` Konfiguration in `user_neuralart.json` um den Memory-Pool und Compiler optionen zu erhalten und generiert:
- Eine .c-Datei mit Gewichtsdaten als C-Arrays (die rohen Binärdaten sind eingebettet)
- Ein Header namens \_ecblobs.h, der diese Arrays mit Attributen für Linker-Sektionen deklariert
- stai\_\*.c / stai\_\*.h - der ST-AI-Runtime-API-Wrapper (Zuweisung, Laden, Ausführung der Inferenz)
- Ein *\_atonbuf.xSPI2.raw file - Der Binär-Blob mit den Gewichten für die Flash-Programmierung

Schritt 2 -  arm-none-eabi-objcopy:

Konvertiert .raw Gewichts-Blob in eine Intel .hex Datei mit der korrekten Basisadresse (z.B. 0x71000000). Diese Hex-Datei kann via STM32CubeProgrammer auf den Microkontroller geflashed werden.

Schritt 3 - copy_n6_models.sh

Kopiert die generierten Dateien in das STM32N6570DK-Embedded-Projekt:
- \*.c and \*\_ecblobs.h -> embedded/STM32N6570DK/FSBL/Src/AI/\<Model>/Src|Inc/
- stai_\*.c/h (with --stai flag)
- \*\.bin weight blobs (with --weights flag) -> FSBL/Assets/AI/

Zudem wird \*\_ecblobs.h gepatcht, um `#include "ecblob_sections.h"` einzufügen. Dadurch wird sichergestellt, dass die Gewichts-Arrays im ECBLOBS-Linker-Abschnitt (0x72000000, 8 MB) landen, anstatt das interne ROM (255 KB) zu sprengen.

#### 7.3.3 Deployment  

Die Firmware erstellt mit arm-none-eabi-gcc ecblobs.bin, kopiert sie nach Assets/AI/ und führt anschließend flash_models.sh aus, um alles auf das Board zu programmieren.

**Generierte Dateien**

| Datei                    | Nutzen                                                                                                | Embedded Nutzung                              |
| ------------------------ | ----------------------------------------------------------------------------------------------------- | --------------------------------------------- |
| \*\_model\_v3.c          | Gewichtskonstanten als C-Arrays mit ECBLOB-Linker-Sektionen                                           | Ja - compiliert zu ecblobs.bin                |
| \*\_model\_v3\_ecblobs.h | Header-Datei zur Deklaration der Symbole für das Gewichts-Array (angepasst mittels ecblob_sections.h) | Ja - Includiert von .c                        |
| stai\_\*\_model\_v3.c    | ST-KI-Laufzeit-Wrapper - Funktionen stai Allocate(), stai Load() und stai Run()                       | Ja - Inferenz Eintrittspunkt                  |
| stai\_\*\_model\_v3.h    | Öffentlicher API-Header für den Laufzeit-Wrapper                                                      | Ja - Includiert durhc Anwendung               |
| \*\_data.hex             | Intel-HEX-Datei des Gewichts-Blobs an der Flash-Adresse (für CubeProgrammer)                          | Ja - Wird Seperat auf xSPI2 geflashed         |
| \*\_atonbuf.xSPI2.raw    | Binärer Rohdaten-Gewichts-Blob (Zwischenschritt)                                                      | Indirekt - Wird zu .hex oder .bin convertiert |
Die drei Modelle werden in einer Pipeline auf der MCU verwendet: palm_detection -> hand_landmark -> fingeralphabet. Für die Kompilierung und Ausführung jedes Modells werden alle vier zugehörigen Dateien (.c, \_ecblobs.h, stai\_\*.c, stai\_\*.h) benötigt.

### 7.4 Softwaregrundstruktur und hardwarenahe Basistreiber

Die Umsetzung der Kamera-, Display- und KI-Verarbeitung setzt zunächst eine funktionsfähige hardwarenahe Basissoftware voraus. Hierzu wurden grundlegende Treiber für die Initialisierung und Steuerung des Mikrocontrollers sowie seiner Peripherie entwickelt. Diese abstrahieren die direkten Registerzugriffe und stellen den übergeordneten Softwarekomponenten einheitliche Funktionen zur Verfügung. Zu den grundlegenden Komponenten gehören insbesondere die Taktsteuerung, die Konfiguration der Ein- und Ausgänge sowie Treiber für UART, Timer und I2C.

Die Implementierung greift auf den Cortex Microcontroller Software Interface Standard (CMSIS) zurück. Das sind bereitgestellte Prozessor- und Gerätedefinitionen für Cortex Microcontroller. CMSIS stellt dabei unter anderem die Registerstrukturen, Interruptnummern und Funktionen für den Zugriff auf den Cortex-M55-Prozessorkern bereit.  (CMSIS: Introduction, n.d.) Die eigentliche Konfiguration der Peripherie wurde dagegen überwiegend durch projektspezifische Treiber umgesetzt. Dadurch konnten die benötigten Funktionen gezielt an die Anforderungen des Systems angepasst und nicht benötigte Bestandteile umfangreicherer Abstraktionsschichten vermieden werden. 

Die Basissoftware bildet die unterste anwendungsspezifische Softwareschicht des Systems. Auf ihr bauen die Treiber und Komponenten für Kamera, Display, externe Speicher und neuronalen Beschleuniger auf. Erst durch diese Schichtung können die übergeordneten Funktionen, beispielsweise die Kamera-Display-Pipeline und die KI-Verarbeitungskette, unabhängig von einzelnen Registerzugriffen strukturiert umgesetzt werden. (Washizaki, 2025, S. 3-4 f.).

#### 7.4.1 Aufbau der Embedded-Software

Mit der Integration der Kamera, des Displays, der externen Speicher und der neuronalen Netze umfasst die Firmware mehrere Komponenten mit unterschiedlichen Aufgaben und Hardwareabhängigkeiten. Der Quellcode wurde daher in getrennte Bereiche gegliedert. Die Struktur orientiert sich an den Prinzipien der Modularisierung und der Trennung von Zuständigkeiten. Dabei werden unterschiedliche Funktionen in eigenständigen Komponenten mit definierten Schnittstellen zusammengefasst und Implementierungsdetails gegenüber den übergeordneten Komponenten gekapselt (Washizaki, 2025, S. 3-4 f.). Tabelle X zeigt die grundlegende Projektstruktur.

| Verzeichnis          | Inhalt und Aufgabe                                                                                                                                                                                                               |
| -------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `Src/Core`           | Enthält den Programmeinstieg, die übergeordnete Systeminitialisierung, zentrale Konfiguration und die Definition der Verarbeitungsaufgaben.                                                                                      |
| `Src/Drivers/CMSIS`  | Stellt die Definitionen des Cortex-M55-Prozessorkerns, gerätespezifische Registerstrukturen, Interruptnummern und Funktionen für den Zugriff auf den Prozessorkern bereit.                                                       |
| `Src/Drivers/HAL`    | Enthält ausgewählte Komponenten der von STMicroelectronics bereitgestellten Hardware Abstraction Layer. Diese werden in diesem Projekt lediglich für eine vereinfachte Anbindung der Neural Processing Unit Middleware benötigt. |
| `Src/Drivers/Simple` | Beinhaltet die selbst entwickelten hardwarenahen Teiber. Diese kapseln die Registerzugriffe und stellen den übergeordneten Systemkomponenten einheitliche Schnittstellen zur Verfügung.                                          |
| `Src/AI`             | Enthält die Einbindung der neuronalen Netze, die Schnittstellen zur NPU, sowie die modellbezogenen Vor- und Nachverarbeitungsschritte.                                                                                           |
| `Src/Assets`         | Stellt statische Ressourcen für Anzeigen und Benutzeroberfläche bereit.                                                                                                                                                          |
| `Src/Middleware`     | Externe Bibliothek für die Verwendung der Neural Processing Unit.                                                                                                                                                                |
Tabelle X: Aufbau der Embedded-Software

Ein Schwerpunkt der Eigenentwicklung liegt auf der Treiberschicht im Verzeichnis `Src/Drivers/Simple`. Ihre öffentlichen Schnittstellen werden in den Headerdateien des Unterverzeichnisses `Inc` bereitgestellt, während sich die Implementierungen in `Src` befinden. Über diese Schnittstellen können die übergeordneten Komponenten beispielsweise Takte aktivieren, Ein- und Ausgänge konfigurieren, Zeitfunktionen verwenden oder Daten über serielle Schnittstellen übertragen, ohne die dafür erforderlichen Registerzugriffe selbst auszuführen.

#### 7.4.2 Zentrale Konfiguration und Systeminitialisierung

Die Konfiguration und Initialisierung der Systemkomponenten werden an zentralen Stellen der Firmware zusammengeführt. Dadurch müssen hardwarespezifische Parameter nicht innerhalb der einzelnen Anwendungskomponenten festgelegt werden. Gleichzeitig wird eine definierte Initialisierungsreihenfolge sichergestellt, da mehrere Komponenten von zuvor eingerichteten Taktquellen, Speicherbereichen oder Basistreibern abhängig sind.

Die Dateien `config.c/h` enthalten die systemweit verwendeten Konfigurationswerte und Konfigurationsstrukturen. Dazu gehören unter anderem Bildauflösungen, Speicherzuordnungen, Schnittstellenparameter sowie Einstellungen der Kamera-, DCMIPP- und Displaykomponenten. Die Konfiguration wird damit von der eigentlichen Treiberimplementierung getrennt. Die jeweiligen Treiber erhalten die benötigten Parameter über definierte Strukturen und übertragen sie während der Initialisierung in die zugehörigen Hardwareregister.

| Datei        | Aufgabe                                                                                  |
| ------------ | ---------------------------------------------------------------------------------------- |
| `main.c`     | Einstiegspunkt der Software und Übergabe an die übergeordnete Anwendungsinitialisierung. |
| `app.c`      | Koordination der Initialisierung der einzelnen Hardware- und Softwarekomponenten.        |
| `config.c/h` | Definition & Instanzierung zentraler Konfigurationsstruktuten.                           |
| `tasks.c`    | Definition der später durch den Software-Scheduler ausgeführten Verarbeitungsaufgaben.   |
Tabelle X: Aufgaben der zentralen Core-Dateien

Nach dem Systemstart wird zunächst die Adresse der Interruptvektortabelle in das Vector Table Offset Register des Prozessors eingetragen. Anschließend erfolgt die Konfiguration des Resource Isolation Framework Security Controllers (RIFSC). Dieser verwaltet die Zugriffsrechte auf interne Speicherbereiche und Peripheriekomponenten des STM32N6570. Eine fehlerhafte Zuordnung kann dazu führen, dass einzelne Komponenten trotz aktivierter Taktversorgung nicht auf benötigte Register oder Speicherbereiche zugreifen können. Die Zugriffsrechte werden daher eingerichtet, bevor die entsprechenden Hardwarekomponenten initialisiert werden. (Resource Isolation Framework Overview - Stm32mpu, n.d.)

Darauf folgen die Konfiguration der Spannungsversorgung sowie die Einrichtung der System- und Peripherietakte. Zusätzlich werden die benötigten Takte für den Energiesparmodus aktiviert. Dies ist insbesondere für die asynchrone Ausführung der neuronalen Netze relevant, da der Prozessorkern während der Inferenz durch Warteanweisungen vorübergehend in einen Energiesparzustand wechseln kann. Die von der NPU und den externen Speichern benötigten Takte müssen währenddessen weiterhin aktiv bleiben. Dieses Verhalten wird auch in der Dokumentation zur Ausführung neuronaler Netze auf dem STM32N6 beschrieben (STM32N6 Example Projects & Tips for Creating New Projects, n.d.).

Nach der grundlegenden Systemkonfiguration werden der Instruktions- und der Datencache aktiviert. Der Instruktioncache reduziert die Zugriffzeiten beim Laden von Programmcode. Der Datencache beschleunigt den Zugriff auf häufig verwendete Daten, erfordert jedoch bei gemeinsam durch Prozessor und Peripherie genutzten Speicherbereiche eine explizite Cache-Synchronisation. Dies betrifft beispielsweise die Ein- und Ausgabepuffer der neuronalen Netze. Die erforderlichen Cache-Operationen werden deshalb innerhalb der jeweiligen Treiber bzw. Modulschnittstellen ausgeführt.

Im nächsten Schritt werden die serielle Debugschnittstelle, die Zeitbasis und ein zu Diagnosezwecken verwendeter GPIO-Ausgang initialisiert. Die Debugschnittstelle ermöglicht es, Statusmeldungen und Fehlercodes bereits während der nachfolgenden Initialisierung auszugeben. Dadurch können insbesondere Fehler bei der Einrichtung der externen Speicher oder der Kamerapipeline frühzeitig erkannt werden. 

Anschließend werden der externe PSRAM- und der NOR-Flash-Speicher über die XSPI-Schnittstellen eingerichtet. Der PSRAM wird vor allem für die Aufnahme der Bild- und Anzeigepuffer verwendet, während der NOR-Flash die für die neuronalen Netze benötigten Modell- und Laufzeitdaten enthält. Da das Display und die Kamerapipeline auf diese Speicherbereiche zugreifen, müssen die externen Speicher vor diesen Komponenten initialisiert werden.

Daraufhin wird die Kamera initialisiert und die Bildverarbeitung über die DCMIPP-Komponente gestartet. Dabei werden zwei Verarbeitungspfade verwendet. Der erste Pfad stellt die aufgenommenen Kamerabilder für die Anzeige bereit. Der zweite Pfad erzeugt die Eingabebilder für die Verarbeitung durch die neuronalen Netze. Durch diese Trennung können Anzeige und KI-Pipeline unterschiedliche Bildauflösungen und Speicherbereiche verwenden. Nach dem Start der Kamerapipeline werden der Touchcontroller und die grafische Benutzeroberfläche eingerichtet. Die Benutzeroberfläche stellt unter anderem Auswahl- und Anzeigeelemente für die einzelnen Verarbeitungsstufen bereit. Über diese können die Ergebnisse der Handerkennung, der Landmarkenerkennung und der Fingeralphabetklassifikation getrennt dargestellt und konfiguriert werden.

Im Anschluss wird die KI-Laufzeitumgebung initialisiert. Dabei werden die NPU, die zugehörige Middleware sowie die Ein- und Ausgabepuffer der eingebundenen neuronalen Netze vorbereitet. Schlägt dieser Schritt fehl, wird der weitere Programmablauf gestoppt, da die eigentliche Anwendung ohne die neuronalen Netze nicht vollständig ausgeführt werden kann. Abschließend wird der Software-Scheduler gestartet, um die eigentliche Ablaufsteuerung zu verwalten. Dazu wird Genaueres in Kapitel 7.6 beschrieben. Die Initialisierungsreihenfolge lässt sich damit wie folgt zusammenfassen:

| Schritt | Komponente                    | Zweck                                                    |
| ------- | ----------------------------- | -------------------------------------------------------- |
| 1       | Interruptvektortabelle        | Zuordnung der verwendeten Interruptbehandlungen          |
| 2       | RIFSC                         | Freigabe der benötigten Speicher- und Peripheriezugriffe |
| 3       | Spannungsversorgung und Takte | Grundlage für alle weiteren Hardwarekomponenten          |
| 4       | Instruktions- und Datencache  | Beschleunigung der Code- und Datenzugriffe               |
| 5       | Debugschnittstelle            | Diagnose                                                 |
| 6       | PSRAM und NOR-Flash           | Bereitstellung der Bild- und Modelldaten                 |
| 7       | LTDC und LTDC-Layer           | Ausgabe des Kamerabildes und der Benutzeroberfläche      |
| 8       | Kamera und DCMIPP             | Aufnahme und Aufbereitung der Bilddaten                  |
| 0       | Touch und Benutzeroberfläche  | Interaktion und Ergebnisdarstellung                      |
| 10      | KI-Laufzeitumgebung           | Vorbereitung der NPI und der neuronalen Netze            |
| 11      | Software-Scheduler            | Zyklische Ausführung der Verarbeitungsaufgaben           |
Tabelle X: Initialisierungreihenfolge nach Systemstart

#### 7.4.3 Registerbasierte Treiberentwicklung

Die Hardwareanbindung wurde überwiegend durch projektspezifische, registerbasierte Treiber umgesetzt. Diese greifen mithilfe der von CMSIS bereitgestellten Registerstrukturen und Bitmasken direkt auf die Peripherieregister des Mikrocontrollers zu. Die Konfiguration erfolgt entsprechend den Vorgaben des Referenzhandbuchs des STM32N657. Die direkten Registerzugriffe werden hinter einheitlichen Treiberfunktionen gekapselt. Übergeordnete Komponenten müssen daher weder die verwendeten Register noch die Position der einzelnen Registerfelder kennen. Konfigurationsparameter werden überwiegend in Strukturen zusammengefasst und bei der Initialisierung an den jeweiligen Treiber übergeben.

Das Vorgehen zeigt sich beispielsweise im GPIO-Treiber. Die Funktion `GPIO_Config()` aktiviert zunächst den Takt des ausgewählten GPIO-Ports und überträgt anschließend die übergebene Konfiguration in die Register `MODER`, `OTYPER` und `PUPDR`:

```c
void GPIO_Config(GPIO_TypeDef *GPIOX, uint32_t pinNr, GPIO_cfg_TypeDef cfg){
	RCC_enable_GPIO(GPIOX);

	GPIOX->MODER  = (GPIOX->MODER & ~(3U << (2U * pinNr)))
			  | (cfg.mode << (2U * pinNr));

	GPIOX->OTYPER = (GPIOX->OTYPER & ~(1U << (pinNr))) 
			  | (cfg.otyp << (pinNr));

	GPIOX->PUPDR  = (GPIOX->PUPDR & ~(3U << (2U * pinNr))) 
			  | (cfg.pupdr << (2U * pinNr));

if (cfg.mode == GPIO_MODE_AF) {
	GPIO_set_af(GPIOX, pinNr, cfg.af, cfg.speed);
	}
}
```

Durch die Maskierung werden ausschließlich die zum ausgewählten Anschluss gehörenden Registerfelder verändert. Bei Verwendung einer Alternativfunktion übernimmt die interne Hilfsfunktion `GPIO_set_af()`zusätzlich die Konfiguration des `AFR`- und des `OSPEEDR`-Registers.

Die Konfiguration erfolgt mithilfe der Struktur `GPIO_cfg_TypeDef`. Diese fasst den Betriebsmodus, den Ausgangstyp, die Pull-up- beziehungsweise Pull-down-Konfiguration, die Ausgangsgeschwindigkeit und die Nummer der Alternativfunktion zusammen. Für wiederkehrende Anwendungsfälle werden passende Konfigurationen vorab definiert und anschließend gemeinsam mit dem GPIO-Port und der Pinnummer an `GPIO_Config()` übergeben. Dadurch bleibt die eigentliche Registerkonfiguration unabhängig von der späteren Verwendung des Anschlusses. Der GPIO-Treiber muss beispielsweise nicht wissen, ob ein Anschluss für I²C, USART oder eine andere Peripherie vorgesehen ist. Er überträgt lediglich die in der Konfigurationsstruktur enthaltenen Parameter in die entsprechenden Register. Die Zuordnung einer konkreten Schnittstelle zu den benötigten Anschlüssen und Alternativfunktionen erfolgt außerhalb des Treibers.

Dieses Prinzip wird auch bei weiteren Treibern verwendet. Hardwareparameter werden in Konfigurationsstrukturen zusammengefasst, während die Treiberfunktionen deren Prüfung und Übertragung in die Register übernehmen. Dadurch können unterschiedliche Instanzen derselben Peripherie mit einer gemeinsamen Implementierung konfiguriert werden. Gleichzeitig bleiben die hardwarespezifischen Registerzugriffe auf die jeweilige Treiberschicht begrenzt.
#### 7.4.4 Grundlegende Systemtreiber

Die projektspezifische Treiberschicht umfasst sowohl grundlegende Systemtreiber als auch funktionsspezifische Komponenten. Als grundlegende Systemtreiber werden im Rahmen dieser Arbeit diejenigen Komponenten eingeordnet, die von mehreren übergeordneten Modulen verwendet werden und zunächst keinen eigenständigen Anwendungszweck erfüllen. Sie stellen unter anderem die Taktversorgung, den Zugriff auf Ein- und Ausgänge, Zeitfunktionen sowie serielle Kommunikations- und Speicherschnittstellen bereit.

Die Treiber für Kamera, DCMIPP, Display und Benutzereingabe bauen auf diesen grundlegenden Funktionen auf. Ebenso werden die Komponenten für den Software-Scheduler und die NPU-Anbindung getrennt behandelt. Tabelle X beschränkt sich daher auf die allgemeinen Systemtreiber der Firmware.

| Datei            | Aufgabe                                                                                            |
| ---------------- | -------------------------------------------------------------------------------------------------- |
| `simple_rcc.c`   | Konfiguration der Energieversorgung sowie Aktivierung und Auswahl der System- und Peripheretakte.  |
| `simple_rifsc.c` | Konfiguration der Zugriffsrechte und Sicherheitsattribute der verwendeten Systemressourcen.        |
| `simple_gpio.c`  | Konfiguration und Ansteuerung der digitalen Ein- und Ausgänge sowie ihrer alternativen Funktionen. |
| `simple_timer.c` | Konfiguration der Timer und Bereitstellung von einer Systemzeit und einer Verzögerungsfunktion.    |
| `simple_usart.c` | Konfiguration der seriellen Schnittstelle zur Übertragung von Diagnose- und Debugdaten.            |
| `simple_i2c.c`   | Kommunikation mit externen Komponenten, insbesondere die Kamera und der Touchcontroller.           |
| `simple_xspi.c`  | Initialisierung und Ansteuerung des externen PSRAM und NOR-Flash über die XSPI-Schnittstellen.     |
Tabelle X: Grundlegende Systemtreiber der Embedded-Software

Die grundlegenden Systemtreiber bauen teilweise aufeinander auf. Der RCC-Treiber aktiviert zunächst die benötigten Peripherietakte, während der RIFSC die erforderlichen Zugriffsrechte auf Speicher- und Peripheriebereiche bereitstellt. Darauf aufbauend konfiguriert der GPIO-Treiber die Ein- und Ausgänge sowie die alternativen Pin-Funktionen für Schnittstellen wie USART, I²C und XSPI. Die Timer-, Kommunikations- und Speichertreiber können anschließend von den übergeordneten Komponenten verwendet werden. Die funktionsspezifischen Treiber für Kamera, Display, Ablaufsteuerung und NPU werden in den folgenden Kapiteln im Zusammenhang mit ihrer jeweiligen Verwendung beschrieben.

#### 7.4.5 Einordnung der Basistreiber in die übergeordneten Komponenten

Die funktionsspezifischen Komponenten der Firmware bauen jeweils auf mehreren grundlegenden Systemtreibern auf. Tabelle X ordnet den übergeordneten Komponenten die von ihnen verwendeten Basistreiber zu.

| Übergeordnete Komponente               | Verwendete Basistreiber                      |
| -------------------------------------- | -------------------------------------------- |
| Kamera- und Bildpipeline               | RCC, RIFSC, GPIO, I2C, Timer, XSPI und USART |
| Ablaufsteuerung und Software Scheduler | RCC, Timer und USART                         |
| NPU- und KI-Modul                      | RCC, RIFSC, XSPI und USART                   |
| Status-LED                             | RCC, GPIO und Timer                          |
| Touch-Eingabe                          | RCC, GPIO, I2C, Timer und USART              |
Tabelle X: Zuordnung der Basistreiber zu den übergeordneten Softwarekomponenten

Die Zuordnung zeigt, dass insbesondere die RCC-, und GPIO-Treiber von mehreren Systembereichen gemeinsam verwendet werden. Die Basistreiber stellen damit die gemeinsame hardwarenahe Grundlage bereit, während die übergeordneten Komponenten deren Funktionen zu anwendungsspezifischen Abläufen kombinieren.

### 7.5 Kamera - Display Pipeline

#### 7.5.1 Initialisierung des Kamerasensors

#### 7.5.2 Übertragung der Kameradaten über CSI & DCMIPP

#### 7.5.3 Verwaltung der Bildpuffer
(XSPI PSRAM etc...)

#### 7.5.4 LTDC-Konfiguration und Verwaltung der Displayebenen

### 7.6 Ablaufsteuerung und Software Scheduler

### 7.7 Integration der NPU und einheitliche KI-Schnittstelle

Die Ausführung der drei neuronalen Netze erfolgt überwiegend auf dem im STM32N657 integrierten Neural-ART Accelerator. Für dessen Verwendung müssen zunächst die zugehörigen Takt- und Speicherbereiche sowie die ATON-Laufzeitumgebung eingerichtet werden. Anschließend werden die drei konvertierten Netzwerkinstanzen für die Handdetektion, Landmark-Erkennung und Fingeralphabetklassifikation initialisiert.

Um die Middleware-spezifischen Aufrufe nicht unmittelbar in der Ablaufsteuerung verwenden zu müssen, wurde mit `simple_ai` eine gemeinsame KI-Schnittstelle umgesetzt. Diese fasst die Initialisierung der KI-Komponenten zusammen, vereinheitlicht die Statusrückgaben und stellt Funktionen für die Ausführung der neuronalen Netze bereit.
#### 7.7.1 Initialisierung des Neural-ART Accelerators

Die Initialisierung der KI-Komponenten erfolgt zentral durch `AI_Init()`. Die Funktion prüft zunächst, ob die Initialisierung bereits durchgeführt wurde. In diesem Fall wird unmittelbar `AI_STATUS_OK` zurückgegeben. Dadurch kann die Funktion mehrfach aufgerufen werden, ohne die Laufzeitumgebung und Netzwerkinstanzen erneut einzurichten. Zu Beginn werden die benötigten Takt- und Speicherbereiche aktiviert. Neben den zugehörigen Peripherietakten betrifft das die AXI-SRAM-Bereiche 3 bis 6.

Innerhalb dieser Funktion wird außerdem die Konfiguration des NPU-Caches vorgenommen. Die anschließende Initialisierung des CacheAXI erfolgt über `HAL_CACHEAXI_Init()`. CacheAXI stellt die für die NPU erforderliche Cache-Infrastruktur bereit. Da die Initialisierung durch die von STMicroelectronics bereitgestellte HAL-Komponente erfolgt, bildet sie eine Ausnahme von der ansonsten überwiegend registerbasierten Treiberimplementierung. Schlägt dieser Schritt fehl, wird die Initialisierung mit `AI_STATUS_CACHEAXI_ERROR` abgebrochen.  

Der verwendete `CACHEAXI-HAL`-Treiber erwartet für seine interne Zeitüberwachung die Funktion `HAL_GetTick()`. Da im Projekt keine separate HAL-Zeitbasis verwendet wird, wurde mit `ai_hal_adapter.c` ein kleiner Adapter umgesetzt. Die Funktion `HAL_GetTick()` greift auf den bereits vorhandenen Millisekundenzähler des Software-Schedulers zurück. Dadurch kann der `CACHEAXI`-Treiber seine vorgesehenen Zeitüberwachungen verwenden, ohne zusätzlich den vollständigen HAL-Zeitbasismechanismus in das Projekt zu integrieren. 

Nach erfolgreicher Einrichtung der Hardware wird `LL_ATON_RT_RuntimeInit()` aufgerufen. Diese Funktion initialisiert die ATON-Laufzeitumgebung, über die die konvertierten neuronalen Netze ausgeführt werden. Anschließend werden die drei Netzwerkinstanzen nacheinander initialisiert:

1. Handdetektion über `PALM_Init()`
2. Landmark-Erkennung über `LANDMARK_Init()`
3. Fingeralphabetklassifikation über `FINGERALPHABET_Init()`

Jede Initialisierungsfunktion ermittelt die vom jeweiligen Netzwerk verwendeten Ein- und Ausgabepuffer und bereitet dessen internen Zustand vor. Gibt eine der Funktionen einen Fehlerstatus zurück, wird die weitere Initialisierung abgebrochen und der Status an die aufrufende Komponente weitergereicht. Erst nach erfolgreicher Initialisierung aller drei Modelle wird `ai_initialized` gesetzt und `AI_STATUS_OK` zurückgegeben.

#### 7.7.2 Einheitliche KI-Schnittstelle

Die von der ATON-Middleware bereitgestellten Funktionen sind an die Laufzeitumgebung und die jeweils erzeugten Netzwerkinstanzen gekoppelt. Um diese Abhängigkeiten nicht unmittelbar in die Ablaufsteuerung und die Vor- beziehungsweise Nachverarbeitung zu übernehmen, wurde mit `simple_ai` eine gemeinsame KI-Schnittstelle implementiert. Sie bündelt die allgemeinen Funktionen zur Initialisierung und Ausführung der neuronalen Netze und stellt einheitliche Statuswerte für alle Modellkomponenten bereit.

Für die Rückgabe von Fehlerzuständen wird der gemeinsame Aufzählungstyp `AI_Status_TypeDef` verwendet. Dadurch können die zentralen und modellspezifischen KI-Funktionen ihre Ergebnisse in einer einheitlichen Form an die aufrufenden Komponenten weitergeben. Die Ablaufsteuerung muss somit keine unterschiedlichen Fehlerdarstellungen der einzelnen Modelle auswerten.

| Statuswert                    | Bedeutung                                                        |
| ----------------------------- | ---------------------------------------------------------------- |
| `AI_STATUS_OK`                | Die aufgerufene Funktion wurde erfolgreich ausgeführt.           |
| `AI_STATUS_NOT_INITIALIZED`   | Die benötigte KI-Komponente wurde noch nicht initialisiert.      |
| `AI_STATUS_CACHEAXI_ERROR`    | Bei der Einrichtung des CacheAXI ist ein Fehler aufgetreten.     |
| `AI_STATUS_INVALID_BUFFER`    | Ein benötigter Ein- oder Ausgabepuffer ist ungültig.             |
| `AI_STATUS_PREPROCESS_ERROR`  | Die Vorverarbeitung der Eingabedaten ist fehlgeschlagen.         |
| `AI_STATUS_POSTPROCESS_ERROR` | Die Nachverarbeitung der Modellausgabe ist fehlgeschlagen.       |
| `AI_STATUS_RUNTIME_ERROR`     | Während der Ausführung eines Modells ist ein Fehler aufgetreten. |
Tabelle X: Einheitliche Statuswerte der KI-Komponenten

Die eigentliche Ausführung einer Netzwerkinstanz wird durch `AI_RuntimeRunNetwork()` gekapselt. Der Funktion wird ein Zeiger auf die jeweilige `NN_Instance_TypeDef` übergeben. Dadurch kann dieselbe Funktion für die Handdetektion, die Landmark-Erkennung und die Fingeralphabetklassifikation verwendet werden. Ist der übergebene Zeiger ungültig, wird die Ausführung unmittelbar abgebrochen. Intern ruft die Funktion wiederholt `LL_ATON_RT_RunEpochBlock()` auf. Die ATON-Laufzeitumgebung verarbeitet das Netzwerk dabei abschnittsweise und liefert nach jedem Aufruf einen Status zurück. Mit `LL_ATON_RT_WFE` wird signalisiert, dass zunächst auf ein Hardwareereignis gewartet werden muss. In diesem Fall erfolgt das Warten über `LL_ATON_OSAL_WFE()`. Der Status `LL_ATON_RT_NO_WFE` zeigt dagegen an, dass die Verarbeitung ohne vorheriges Warten fortgesetzt werden kann. Die Aufrufe werden wiederholt, bis die Laufzeitumgebung einen abschließenden Status zurückgibt.

Die Funktion liefert nur dann `true`, wenn die Netzwerkausführung mit `LL_ATON_RT_DONE` beendet wurde. Alle anderen abschließenden Zustände werden als fehlgeschlagene Ausführung behandelt und durch `false` signalisiert. Für die aufrufenden Modellkomponenten entsteht dadurch eine vereinfachte Schnittstelle, bei der die einzelnen ATON-Rückgabewerte nicht außerhalb von `simple_ai` ausgewertet werden müssen.

Die modellspezifischen Komponenten kapseln darauf aufbauend die jeweiligen Netzwerkinstanzen sowie deren Ein- und Ausgabepuffer. Sie prüfen ihren Initialisierungszustand und die übergebenen Puffer, führen die Netzwerkinstanz über `AI_RuntimeRunNetwork()` aus und übersetzen das Ergebnis in einen Wert des gemeinsamen Typs `AI_Status_TypeDef`. Die Vor- und Nachverarbeitung verbleibt hingegen in den jeweiligen Modellkomponenten, da sie sich zwischen Handdetektion, Landmark-Erkennung und Fingeralphabetklassifikation unterscheidet.

Damit bildet `simple_ai` die gemeinsame Verbindung zwischen den erzeugten Netzwerkinstanzen und der übergeordneten Ablaufsteuerung. Middleware-spezifische Details der Netzwerkausführung bleiben innerhalb dieser Schnittstelle gekapselt, während die modellspezifischen Unterschiede weiterhin in den zugehörigen Komponenten behandelt werden.

### 7.8 Vor- und Nachverarbeitungsschritte

Die drei neuronalen Netze verwenden unterschiedliche Eingabedaten und liefern Ausgaben, die nicht unmittelbar durch das jeweils nachfolgende Modell verarbeitet werden können. Daher sind zwischen den einzelnen Inferenzschritten mehrere Vor- und Nachverarbeitungen erforderlich. Diese umfassen sowohl hardwaregestützte Operationen innerhalb der DCMIPP als auch durch den Hauptprozessor ausgeführte Transformationen der Bild- und Landmark-Daten.

Die DCMIPP übernimmt die grundlegende Aufbereitung der Kamerabilder. Die nachgelagerten Verarbeitungsschritte umfassen die Nachverarbeitung der Handdetektion, die Bildung der Region of Interest, die Vor- und Nachverarbeitung des Landmark-Modells sowie die Aufbereitung der Landmark-Koordinaten für das Klassifikationsmodell.

#### 7.8.1 DCMIPP-gestützte Bildvorverarbeitung

Die Kamera überträgt die aufgenommenen Bilder als RAW-Bayer-Daten mit einer Auflösung von 2592x1944 Pixeln über die CSI-Schnittstelle. Da weder die Displayausgabe noch die neuronalen Netze diese Daten unmittelbar verarbeiten können, übernimmt die Digital Camera Interface Pixel Pipeline (DCMIPP) einen wesentlichen Teil der erforderlichen Bildvorverarbeitung.

Die DCMIPP wandelt die RAW-Bayer-Daten zunächst in ein RGB-Bild um. Hierzu werden die Demosaikierung der RAW10-Bilddaten, die Farbkorrektur, die Belichtungsanpassung und die Gammakorrektur innerhalb der Bildpipeline durchgeführt (STMicroelectronics, n.d.). Das Ergebnis wird im RGB888-Format ausgegeben. Dadurch müssen diese rechenintensiven Verarbeitungsschritte nicht nachträglich durch den Hauptprozessor auf den bereits gespeicherten Kamerabildern ausgeführt werden.

Die verwendeten Parameterwerte wurden empirisch bestimmt. Hierzu wurden die entsprechenden DCMIPP-Register schrittweise angepasst und die Auswirkungen auf das ausgegebene Kamerabild visuell bewertet. Einstellungen, die unter den vorgesehenen Einsatzbedingungen eine geeignete Bilddarstellung ergaben, wurden anschließend in die feste Konfiguration der Bildpipeline übernommen.

Aus dem gemeinsamen Kameradatenstrom werden zwei getrennte Ausgabepfade erzeugt. Pipe 1 dient der Displayausgabe und stellt gleichzeitig das Ausgangsbild für die Landmark-Erkennung bereit. Pipe 2 erzeugt dagegen das Eingabebild für die Handdeteketion.

Für Pipe 1 wird das Sensorbild zunächst auf das Seitenverhältnis des Displays zugeschnitten. Der horizontale Bildbereich bleibt vollständig erhalten, während das Bild vertikal zentriert beschnitten wird. Der entstandene Ausschnitt wird anschließend durch die Downscaling-Einheit der DCMIPP auf eine Auflösung von 800x480 Pixeln verkleinert und im RGB888-Format im externen PSRAM abgelegt. Die von Pipe 1 erzeugten Bilder werden über vier Hintergrundbildpuffer verwaltet. Die DCMIPP schreibt jeweils in den für die Aufnahme vorgesehenen Puffer, während weitere Puffer für die Displayausgabe, die KI-Verarbeitung und das Einzeichnen der Handregion beziehungsweise der Landmarks verwendet werden. Nach Abschluss eines Bildes wird die Zieladresse der DCMIPP auf den nächsten Aufnahmepuffer umgeschaltet.

Pipe 2 verarbeitet den vollständigen Sensorbereich und erzeugt das Eingabebild für die Handdetektion. Da deren Eingangsauflösung mit 192x192 Pixeln deutlich unterhalb der Sensorauflösung liegt, wird das Bild vor der eigentlichen Skalierung horizontal und vertikal dezimiert. Dadurch reduziert sich die Auflösung zunächst von 2592x1944 auf 1296x972 Pixel. Anschließend übernimmt der DCMIPP-Downsizer die Skalierung auf 192x192 Pixel. Da vor der Skalierung kein quadratischer Bildausschnitt gebildet wird, wird das Seitenverhältnis des vollständigen Sensorbildes dabei an die quadratische Modelleingabe angepasst. Auch dieser Ausgabepfad verwendet das RGB888-Format.

Für Pipe 2 wird der Double-Buffer-Modus der DCMIPP verwendet. Die erzeugten Bilder werden abwechselnd in zwei Eingabebildpuffern gespeichert. Während die DCMIPP einen Puffer mit einem neuen Bild beschreibt, kann der zuvor fertiggestellte Puffer für die Handdetektion verwendet werden. Der jeweils abgeschlossene Puffer wird über den Frame-Callback bestimmt und für die weitere Verarbeitung markiert.

Die Konfiguration der beiden Bildpfade wird jeweils durch eine globale Konfigurationsstruktur festgelegt. Die darin enthaltenen Parameter wurden aus den Anforderungen der beiden Verarbeitungspfade und den verwendeten Bildformaten abgeleitet.

| Parameter                         | Pipe 1                         | Pipe 2      | Begründung                                                                                                                                                                                  |
| --------------------------------- | ------------------------------ | ----------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `output_width`<br>`output_height` | 800x480                        | 192x192     | Entsprechen der Displayauflösung, bzw. Eingangsgröße der Handdetektion.                                                                                                                     |
| `output_format`                   | RGB888                         | RGB888      | Display und KI-Vorverarbeitung arbeiten mit drei Farbkanälen und jeweils 8 Bit pro Kanal.                                                                                                   |
| `output_bpp`                      | 3                              | 3           | RGB888 benötigt drei Byte pro Pixel (für jeden Kanal 8 Bit). Der Wert wird unter anderem zur Berechnung des Speicher-Pitchs verwendet.                                                      |
| `enable_crop`                     | aktiviert                      | aktiviert   | Pipe 1 benötigt einen Zuschnitt auf das Seitenverhältnis des Displays. <br><br>Pipe 2 verarbeitet den vollständigen Sensorbereich.                                                          |
| `crop_x`                          | 0                              | 0           | Horizontal wird kein Bereich abgeschnitten.                                                                                                                                                 |
| `crop_y`                          | Zentriert vertikaler Zuschnitt | 0           | Pipe 1 entfernt oben und unten Bildbereiche, um das Sensorformat ohne Verzerrung an 800x480 anzupassen. <br><br>Pipe 2 verwendet das vollständige Bild.                                     |
| `crop_width`                      | 2592                           | 2592        | Die vollständige Sensorbreite wird verwendet.                                                                                                                                               |
| `crop_height`                     | ca. 1555                       | 1944        | Pipe 1 erhält das Displayseitenverhältnis<br>5:3. <br><br>Pipe  2 behält die vollständige Sensorhöhe.                                                                                       |
| `enable_downsize`                 | aktiviert                      | aktiviert   | Beide Ausgaben sind wesentlich kleiner als das Sensorbild.                                                                                                                                  |
| `enable_decimate`                 | deaktiviert                    | aktiviert   | Nur Pipe 2 benötigt vor dem Downscaling eine zusätzliche Halbierung.                                                                                                                        |
| `decimate_h`<br>`decimate_v`      | -                              | 1, 1        | Die Auflösung wird horizontal und vertikal jeweils durch zwei geteilt.                                                                                                                      |
| `enable_swap`                     | deaktiviert                    | deaktiviert | Die Farbkanäle sind bereits korrekt. Ein Swap ist nicht notwendig.                                                                                                                          |
| `enable_gamma`                    | aktiviert                      | aktiviert   | Die Gammakorrektur verbessert die Helligkeitsdarstellung.                                                                                                                                   |
| `enable_dbm`                      | deaktiviert                    | aktiviert   | Pipe 1 verwendet eine softwareseitige Rotation von vier Bildpuffern, da die Anwendung mehr als  zwei Puffer benötigt.<br><br>Pipe 2 verwendet zwei hardwareseitig wechselnde Eingabepuffer. |
Tabelle X: Konfigurationsparameter der DCMIPP-Bildpfade

Zusätzlich zur Erzeugung des Displaybildes werden für Pipe 1 drei Statistikkanäle aktiviert. Diese erfassen Helligkeits- und Farbwerte innerhalb eines zentralen Bildbereichs mit einer Größe von 1296x972 Pixeln. Die ermittelten Statistiken werden von der automatischen Belichtungsregelung ausgewertet und zur Anpassung der Belichtungszeit des Kamerasensors verwendet.

Durch die hardwaregestützte Vorverarbeitung werden nur die tatsächlich benötigten Bildauflösungen in den externen Speicher geschrieben. Dies reduziert sowohl den Speicherbedarf als auch die vom Hauptprozessor zu verarbeitende Datenmenge. Die weiteren auf der CPU ausgeführten Vor- und Nachverarbeitungsschritte können dadurch unmittelbar auf den bereits aufbereiteten RGB888-Bilddaten arbeiten.

#### 7.8.2 Nachverarbeitung der Handdetektion

**Ankerbasierte Modellausgabe**

Das Modell zur Handdetektion verwendet ein ankerbasiertes Detektionsverfahren (Zhang et al., 2020). Hierfür sind in der Ankertabelle `pd_anchors` insgesamt 2016 Ankerpositionen definiert. Diese repräsentieren unterschiedliche Positionen und Größen möglicher Handregionen innerhalb des 192x192 Pixel großen Eingabebildes. Für jeden Anker gibt das Modell einen Konfidenzwert sowie mehrere Regressionswerte aus. Der Konfidenzwert beschreibt, wie wahrscheinlich sich im Bereich des jeweiligen Ankers eine Hand befindet. Die Regressionswerte enthalten die Abweichungen zwischen dem Anker und der vorhergesagten Handregion sowie die Positionen der zugehörigen Schlüsselpunkte.  (QUELLE?)

Die Anker bilden damit ein festes Suchraster über dem Eingabebild. Das Modell muss die Position und Ausdehnung einer Hand nicht vollständig unabhängig bestimmen, sondern sagt für jeden Anker die räumlichen Abweichungen zur tatsächlichen Handregion voraus. Da mehrere benachbarte Anker auf dieselbe Hand reagieren können, entstehen üblicherweise mehrere ähnliche Erkennungskandidaten, die in der anschließenden Nachverarbeitung bereinigt werden müssen.

**Auswahl und Filterung der Erkennungskandidaten**

Für jeden der 2016 Anker wird zunächst geprüft, ob der ausgegebene Konfidenzwert den festgelegten Schwellwert erreicht. Die Konfidenzwerte liegen als Logits vor und müssen für die weitere Bewertung durch eine Sigmoidfunktion in einen Wahrscheinlichkeitswert zwischen 0 und 1 überführt werden (Sharma et al., 2020):
$$ \sigma(x) = \frac{1}{1 + \mathrm{e}^{-x}} $$
Eine direkte Berechnung würde die Exponentialfunktion `expf()` erfordern. Da die verwendete Floating Point Unit keine Exponentialfunktion als direkte Hardwareoperation bereitstellt, müsste diese durch eine mathematische Bibliotheksroutine softwareseitig berechnet werden. Zur Verringerung des Rechenaufwands wird die Sigmoidfunktion daher durch die folgende rationale Funktion approximiert: #TODO PRÜFEN, ggf. PADE APPROX!
$$\hat{\sigma}(x)=\frac{0{,}5+0{,}25x}{1-0{,}25x+0{,}125x^2}$$
Für Eingabewerte kleiner als -8 beziehungsweise größer als 8 wird unmittelbar der Wert 0 beziehungsweise 1 zurückgegeben. Innerhalb dieses Bereichs benötigt die Berechnung lediglich Additionen, Multiplikationen und eine Division. Um die Sigmoidapproximation nicht für alle 2016 Anker ausführen zu müssen, wird der festgelegte Konfidenzschwellwert zunächst in den Logit-Raum überführt (Athavale et al., 2024):
$$ t_{\mathrm{Logit}} = \ln\left(\frac{t}{1-t}\right) $$
Die vom Modell ausgegebenen Logits können dadurch unmittelbar mit dem berechneten Logit-Schwellwert verglichen werden. Nur Kandidaten, die diesen Schwellwert erreichen oder überschreiten, werden vollständig dekodiert und mithilfe der approximierten Sigmoidfunktion in einen Wahrscheinlichkeitswert umgerechnet. Anschließend werden die Regressionswerte der verbleibenden Kandidaten unter Berücksichtigung der jeweils zugehörigen Ankerposition dekodiert. Dabei werden der Mittelpunkt, die Breite und Höhe der vorhergesagten Handregion sowie die vom Modell ausgegebenen Schlüsselpunkte bestimmt. Die berechneten Koordinaten und Abmessungen werden auf die Größe des Modelleingangs normiert. Kandidaten mit einer ungültigen Breite oder Höhe sowie nicht endlichen Zahlenwerten werden verworfen.

Um den Speicherbedarf und den Rechenaufwand der weiteren Verarbeitung zu begrenzen, werden höchstens 20 Erkennungskandidaten zwischengespeichert. Diese Obergrenze wird durch die Konstante `PALM_PP_MAX_CANDIDATES` festgelegt. Solange die maximale Anzahl noch nicht erreicht ist, wird jeder gültige Kandidat übernommen. Ist der Kandidatenspeicher bereits vollständig belegt, wird zunächst der Kandidat mit der geringsten Wahrscheinlichkeit ermittelt. Dieser wird nur dann ersetzt, wenn der neue Kandidat eine höhere Wahrscheinlichkeit aufweist. Auf diese Weise werden aus den Ergebnissen der 2016 Ankerpositionen ausschließlich die 20 wahrscheinlichsten Kandidaten für die weitere Verarbeitung berücksichtigt.

Die ausgewählten Kandidaten werden anschließend absteigend nach ihrer Wahrscheinlichkeit sortiert. Da mehrere benachbarte Anker auf dieselbe Hand reagieren können, entstehen häufig mehrere stark überlappende Handregionen. Zur Entfernung dieser Mehrfacherkennungen wird eine Non-Maximum Suppression (NMS) durchgeführt (Bodla et al., 2017). Hierzu wird jeder Kandidat mit den bereits übernommenen Handregionen verglichen. Als Maß für ihre räumliche Überschneidung dient die Intersection over Union (IoU). Sie beschreibt das Verhältnis zwischen der Schnittfläche und der Vereinigungsfläche zweier Begrenzungsrahmen (Rezatofighi et al., 2019):
$$ \operatorname{IoU}(A,B) =\frac{\left|A \cap B\right|} {\left|A \cup B\right|} $$
Die Kandidaten werden in absteigender Reihenfolge ihrer Wahrscheinlichkeit verarbeitet. Erreicht oder überschreitet die IoU eines Kandidaten mit einer bereits übernommenen Handregion den festgelegten Schwellwert, wird der Kandidat verworfen. Dadurch bleibt von mehreren stark überlappenden Erkennungen in der Regel nur der Kandidat mit der höchsten Wahrscheinlichkeit erhalten. Nach Abschluss der Non-Maximum Suppression wird der stärkste verbleibende Kandidat als Ergebnis der Handdetektion übernommen. Das Ergebnis enthält die Wahrscheinlichkeit, den Mittelpunkt und die Abmessungen der erkannten Handregion, die zugehörigen Schlüsselpunkte sowie den verwendeten Ankerindex.

Um kurzzeitige Fehldetektionen und einzelne Aussetzer zu reduzieren, wird das Ergebnis zusätzlich über mehrere Ausführungen hinweg gefiltert. Eine Hand gilt erst dann als bestätigt, wenn in zwei aufeinanderfolgenden Auswertungen eine gültige Detektion vorliegt. Bei einer ungültigen Detektion wird der positive Zähler zurückgesetzt. Umgekehrt wird der erkannte Zustand erst aufgehoben, wenn in drei aufeinanderfolgenden Auswertungen keine gültige Handdetektion vorliegt. Einzelne Aussetzer führen dadurch nicht unmittelbar zum Verlust der erkannten Handregion.

**Erzeugung der initialen Region of Interest**

Aus der bestätigten Handdetektion wird die initiale Region of Interest für das Handlandmark-Modell erzeugt. Die Handdetektion wird auf dem von Pipe 2 bereitgestellten Bild ausgeführt, während der Eingabeausschnitt für das Landmark-Modell aus dem Displaybild von Pipe 1 entnommen wird. Aufgrund der unterschiedlichen Auflösungen und Bildausschnitte müssen die normierten Koordinaten der Handdetektion zunächst in das Koordinatensystem von Pipe 1 transformiert werden.

In horizontaler Richtung bilden beide Bildpfade die vollständige Sensorbreite ab. Die horizontale Position und Breite können deshalb unmittelbar mit der Breite des Displaybildes skaliert werden. Pipe 1 verwendet jedoch einen vertikal zentrierten Ausschnitt des Sensorbildes. Bei der Transformation der vertikalen Koordinaten müssen daher zusätzlich die Höhe und der Offset dieses Ausschnitts berücksichtigt werden. Neben der Begrenzungsbox stellt das Modell mehrere Schlüsselpunkte der Hand bereit. Zwei dieser Punkte werden zur Bestimmung ihrer Orientierung verwendet. Aus ihrer relativen Lage wird der Rotationswinkel berechnet: QUELLE?
$$\alpha = \frac{\pi}{2} - \operatorname{atan2}(-\Delta y,\Delta x)$$
Der berechnete Winkel wird anschließend auf den Bereich von $-\pi$ bis $\pi$ normiert. Kann kein gültiger Winkel bestimmt werden, wird eine Rotation von 0 verwendet. Da die Begrenzungsbox der Handdetektion hauptsächlich die Handfläche umfasst, wird sie für die Landmark-Erkennung erweitert. Ihr Mittelpunkt wird entlang der rotierten lokalen Vertikalachse um die Hälfte der ursprünglichen Höhe in Richtung der Finger verschoben. Anschließend wird die längere Seite der Begrenzungsbox bestimmt und mit dem Faktor 2,6 skaliert. Der berechnete Wert wird für die Breite und Höhe verwendet, sodass eine quadratische Region entsteht.

Aus dem Mittelpunkt, der Seitenlänge und dem Rotationswinkel werden abschließend die vier Eckpunkte der Region berechnet. Die so erzeugte Region bildet die Grundlage für den 224x224 Pixel großen Eingabeausschnitt des Handlandmark-Modells. Dessen Erzeugung wird im folgenden Abschnitt zur Vorverarbeitung der Landmark-Erkennung beschrieben.
#### 7.8.3 Vorverarbeitung der Landmark-Erkennung

Die aus der Handdetektion erzeugte Region of Interest besitzt eine variable Position, Größe und Rotation innerhalb des von Pipe 1 bereitgestellten Kamerabildes. Das Handlandmark-Modell erwartet dagegen ein quadratisches Eingabebild mit einer festen Auflösung von 224x224 Pixeln. Daher muss die Region aus dem Kamerabild entnommen, entsprechend ihrer Rotation ausgerichtet und auf die Eingangsgröße des Modells übertragen werden. Hierzu wird für jedes Pixel des Zielbildes die zugehörige Position innerhalb der Region bestimmt. Die Zielkoordinaten werden zunächst auf den Bereich von −0,5 bis 0,5 normiert und anschließend mit der Breite und Höhe der Region skaliert. Unter Berücksichtigung des Rotationswinkels werden die lokalen Koordinaten in das Koordinatensystem des Kamerabildes überführt:
$$\begin{aligned}
x_{\mathrm{Quelle}}
&=
c_x+x_{\mathrm{lokal}}\cos(\alpha)
-y_{\mathrm{lokal}}\sin(\alpha),\\
y_{\mathrm{Quelle}}
&=
c_y+x_{\mathrm{lokal}}\sin(\alpha)
+y_{\mathrm{lokal}}\cos(\alpha).
\end{aligned}
$$
Dabei beschreiben $c_x$ und $c_y$ den Mittelpunkt und $\alpha$ den Rotationswinkel der Region. Dadurch werden Skalierung, Position und Rotation gleichzeitig berücksichtigt. Damit wird die Hand unabhängig von ihrer Lage im ursprünglichen Kamerabild in eine einheitliche Ausrichtung überführt.

Die Transformation führt häufig zu Quellkoordinaten, die zwischen den ganzzahligen Pixelpositionen des Kamerabildes liegen. Der benötigte Farbwert wird deshalb durch eine bilineare Interpolation aus den vier benachbarten Pixeln berechnet. Dadurch werden Skalierungs- und Rotationsartefakte gegenüber einer einfachen Auswahl des nächstgelegenen Pixels reduziert. Reicht die Region über den Rand des Kamerabildes hinaus, werden die außerhalb des Bildes liegenden Bereiche im Modelleingang schwarz aufgefüllt. Bei der Adressierung der Bilddaten wird außerdem die im Speicher verwendete Zeilenlänge berücksichtigt. (QUELLE)

#### 7.8.4 Nachverarbeitung der Landmark-Erkennung

**Rücktransformation der Landmarks**

Das Handlandmark-Modell gibt für jeden der 21 Handlandmarks drei Koordinaten innerhalb des 224x224 Pixel großen Modelleingangs aus. Da dieser Eingabeausschnitt gegenüber dem ursprünglichen Kamerabild skaliert, verschoben und rotiert wurde, können die ausgegebenen Koordinaten nicht unmittelbar für die Anzeige oder die nachfolgende Zeichenklassifikation verwendet werden. Sie werden deshalb zunächst in das Koordinatensystem des von Pipe 1 bereitgestellten Kamerabildes zurücktransformiert.

Hierzu werden die x- und y-Koordinaten der Landmarks auf den Mittelpunkt des Modelleingangs bezogen und entsprechend der Größe der aktuellen Region of Interest skaliert. Anschließend werden sie anhand des Rotationswinkels der Region ausgerichtet und um deren Mittelpunkt verschoben. Die Transformation entspricht damit der Umkehrung der zuvor durchgeführten Erzeugung des Modelleingangs. Abschließend werden die x- und y-Koordinaten durch die Breite beziehungsweise Höhe des Kamerabildes geteilt und dadurch auf den Bereich des gesamten Bildes normiert. Die vom Modell ausgegebene z-Koordinate wird unverändert übernommen.

Die zurücktransformierten Landmark-Koordinaten werden sowohl für die Anzeige und Zeichenklassifikation als auch für die Aktualisierung der Region of Interest verwendet. Dadurch kann die Hand in den nachfolgenden Kamerabildern anhand der bereits ermittelten Landmarks weiterverfolgt werden, ohne erneut eine vollständige Handdetektion ausführen zu müssen.

**Aktualisierung der Tracking-Region***

Für die Bestimmung der nächsten Tracking-Region werden nicht alle 21 Landmarks verwendet. Insbesondere die Fingerspitzen haben sich in Tests abhängig vom dargestellten Handzeichen stark bewegt und haben dadurch die Größe und Position der Region unnötig verändert. Stattdessen werden zwölf vergleichsweise stabile Punkte aus dem Bereich des Handgelenks, der Handfläche und der Fingerbasen berücksichtigt. Aus ihren minimalen und maximalen x- und y-Koordinaten wird zunächst eine Bounding-Box bestimmt. Deren Mittelpunkt, Breite und Höhe bilden die Grundlage der neuen Region.

Die Orientierung der Hand wird über eine zentrale Handflächenachse bestimmt. Hierzu wird zunächst der Mittelpunkt der vier Fingergrundgelenke von Zeige-, Mittel-, Ring- und kleinem Finger berechnet. Die Verbindung zwischen dem Handgelenk und diesem Mittelpunkt beschreibt die Ausrichtung der Handfläche, aus der der neue Rotationswinkel abgeleitet wird. Um abrupte Änderungen und ein sichtbares Springen der Region zu vermeiden, wird der neue Winkel nicht unmittelbar übernommen. Stattdessen fließen 20 % der berechneten Winkeländerung in die nächste Region ein. Zusätzlich wird die Änderung pro Aktualisierung auf 0,08 Radiant, entsprechend etwa 4,6° begrenzt. Der resultierende Winkel wird anschließend auf den Bereich von $-\pi$ bis $\pi$ normiert.

Abschließend wird die Region entlang ihrer lokalen vertikalen Achse leicht in Richtung der Finger verschoben. Die längere Seite der Begrenzungsbox wird mit dem Faktor 2,0 skaliert und sowohl für die Breite als auch für die Höhe verwendet. Dadurch entsteht erneut eine quadratische Region, die neben der Handfläche auch die Finger vollständig erfassen soll. Aus Mittelpunkt, Seitenlänge und Rotation werden die vier Eckpunkte der nächsten Region berechnet.

Die aktualisierte Region wird im folgenden Kamerabild erneut zur Erzeugung des 224x224 Pixel großen Modelleingangs verwendet. Erst wenn die Landmark-Erkennung über mehrere Durchläufe keine gültige Ausgabe mehr liefert, wird das Tracking zurückgesetzt und erneut die Handdetektion ausgeführt.
#### 7.8.5 Vorverarbeitung des Klassifizierungsmodells

Das Klassifikationsmodell verarbeitet nicht das Kamerabild selbst, sondern die vom Handlandmark-Modell bestimmten Koordinaten. Der Modelleingang umfasst insgesamt 88 Merkmale und ist in zwei Bereiche mit jeweils 44 Merkmalen für die linke und rechte Hand unterteilt. Zu Beginn der Vorverarbeitung werden beide Bereiche mit den für eine fehlende Hand vorgesehenen Werten initialisiert. Anschließend wird anhand des vom Landmark-Modell ausgegebenen Handedness-Wertes bestimmt, welchem Bereich die erkannte Hand zugeordnet wird. Werte ab einem Schwellwert von 0,5 werden der rechten Hand und kleinere Werte der linken Hand zugeordnet. Für jede Hand werden die x- und y-Koordinaten der 21 Landmarks verwendet. Die z-Koordinaten werden bei der Zeichenklassifikation nicht berücksichtigt. Vor der weiteren Verarbeitung werden die horizontalen Koordinaten gespiegelt. Diese Transformation stellt die Koordinatenorientierung her, die auch bei der Erzeugung der Trainingsdaten verwendet wurde. Werte außerhalb des normierten Bereichs werden zuvor auf den Bereich von 0 bis 1 begrenzt. Um den Einfluss der Position der Hand innerhalb des Bildausschnitts zu verringern, werden die Landmark-Koordinaten relativ zum Handgelenk angegeben. Hierzu wird die Position des ersten Landmarks, das dem Handgelenk entspricht, von allen Landmark-Koordinaten abgezogen:
$$x_i^{\mathrm{rel}}=x_i-x_0,\qquad y_i^{\mathrm{rel}}=y_i-y_0$$
Dadurch beschreiben die 42 resultierenden Merkmale überwiegend die geometrische Anordnung der Handpunkte zueinander. Zusätzlich werden die absoluten x- und y-Koordinaten des Handgelenks als zwei weitere Merkmale übernommen. Für jede Hand entstehen somit 44 Werte:

-  42 handgelenkrelative Koordinaten der 21 Landmarks
-  zwei absolute Koordinaten des Handgelenks

Die normierten Koordinaten werden zunächst in vorzeichenbehaftete 16-Bit-Ganzzahlen überführt. Der normierte Wertebereich wird dafür mit dem Maximalwert 32767 skaliert. Anschließend werden die insgesamt 88 Zwischenwerte entsprechend den Quantisierungsparametern des Klassifikationsmodells in 8-Bit-Werte umgerechnet: 
$$
q
=
\frac{v}{32767 \cdot s}
+
z
$$
Dabei bezeichnet $v$  das 16-Bit-Merkmal, $s=0,007842$ den Quantisierungsfaktor und $z = 127$ den Nullpunkt. Das Ergebnis wird auf den gültigen Bereich von 0 bis 255 begrenzt und als `uint8_t` in den Eingabepuffer des Klassifikationsmodells geschrieben.
#### 7.8.6 Nachverarbeitung des Klassifikationsmodells

Das Klassifikationsmodell gibt für jede unterstützte Zeichenklasse einen quantisierten Ausgabewert zurück. Dieser beschreibt die relative Bewertung der jeweiligen Klasse durch das Modell. Da die Werte als `uint8_t` vorliegen, können sie ohne vorherige Dequantisierung miteinander verglichen werden. Zur Bestimmung des Klassifikationsergebnisses werden sämtliche Ausgabewerte durchlaufen. Der Index des größten Wertes wird als vorhergesagte Klasse ausgewählt. 

Nach Abschluss der Suche wird der ermittelte Klassenindex über das Array `ai_labels` dem entsprechenden Fingeralphabetzeichen zugeordnet. Das zurückgegebene Ergebnis enthält somit den Klassenindex, den quantisierten Ausgabewert und die zugehörige Klassenbezeichnung. 

### 7.9 Visualisierung und Benutzerausgabe

#### 7.9.1 Darstellung der Erkennungsergebnisse

#### 7.9.2 Benutzeroberfläche und Bedienung


### 7.10 Optimierungen
