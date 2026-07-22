
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
### 7.2 Augmentationspipeline


### 7.3 Entwicklung und Training des Klassifikationsmodells



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