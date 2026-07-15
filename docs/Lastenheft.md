# Lastenheft

Ausgangslage
- STM32N6570DK mit Display, Kamera, NPU (800 MHz CPU, 1000 MHz NPU)
- Zwei vortrainierte MediaPipe TFLite-Modelle: Palm Detection (192x192) und Hand Landmark (224x224)
- Custom Sign-Language-Classifier (88-Input MLP, 26 Klassen: A-Y + NONE + SCH) wurde trainiert und INT8-quantifiziert
- Embedded-Seite ist aktuell nicht vorhanden

Zielsetzung
- Baremetal Treiber und Business Logic auf dem Mikrocontroller implementieren
- Python-Pipeline (Palm -> Hand -> Sign) mit Steuerung fertigstellen
- Bestehende Modelle in Python laufen lassen, quantifizieren und auf µC deployen
- Sign-Language-Modell trainieren, quantifizieren und auf µC deployen

Funktionale Anforderungen
- Kamera-Eingabe (2880x1620, DCMIPP/CSI)
- Palm Detection auf skaliertem Frame
- Hand Landmark Detection auf ROI-Crops (max. 2 Hände)
- Multi-Hand-Tracking mit ROI-Propagation über Frames (optional)
- Sign-Language-Klassifikation (26 Klassen)
- Ergebnis-Anzeige auf Display (LTDC)
- NPU-beschleunigte Inferenz der drei Modelle
- Datensatz für Sign Language Modell erstellen
- Daten-Augmentierungspipeline (Mirror, Translate, Zoom, Jitter, Drop Frames)

Nicht-Funktionale Anforderungen
- Echtzeitfähigkeit (Frame-Rate ≥ 15 FPS)
- INT8-Quantisierung aller Modelle für NPU
- Speichereffizienz (Modellgröße, RAM-Verbrauch)
- Robustheit gegenüber Kamerarauschen

Rahmenbedingungen
- STM32N6570DK Hardware (NPU, CSI, DCMIPP, LTDC)
- Baremetal-Programmierung (kein RTOS und HAL)
- Python 3.x mit TensorFlow/Keras, OpenCV, MediaPipe
- TFLite-Modellformat
- ST Edge AI Toolchain für NPU-Deployment

Abnahmekriterien
- Python-Pipeline erkennt Buchstaben zuverlässig anhand von Kamera-Input
- Modelle laufen auf dem µC über NPU
- Display zeigt Erkennungsergebnis in Echtzeit an
- INT8-Quantisierung eingehalten

**Ausformuliert ->**
## Ausgangslage

Das Projekt Eingebetteter Gebärdensprach-Übersetzer wird mit dem Ziel verfolgt, eine Echtzeit-Erkennung von Fingeralphabet-Gebärden auf einem ressourcenbeschränkten Mikrocontroller zu ermöglichen. Als Hardware-Plattform steht der STM32N6570DK zur Verfügung, der über eine Kamera-Schnittstelle (DCMIPP/CSI), ein Display (LTDC) sowie einen Neural Processing Unit (NPU) verfügt. Die NPU ist speziell für die beschleunigte Inferenz von CNN-Modellen auf ressourcenlimitierten Geräten ausgelegt. Auf der Seite des maschinellen Lernens stehen zwei vortrainierte TFLite-Modelle aus der MediaPipe-Bibliothek zur Verfügung. Ein Palm-Detection-Modell zur Erkennung von Handflächen im Gesamtbild und ein Hand-Landmark-Modell zur Erkennung von Hand-Landmarks in einem zugeschnittenen Bild.

## Zielsetzung

Das übergeordnete Ziel des Projekts besteht darin, einen funktionsfähigen, eingebetteten Gebärdensprach-Übersetzer zu entwickeln, der auf dem STM32N6570DK in Echtzeit Fingeralphabet-Gebärden erkennt und das Ergebnis auf dem Display ausgibt. Konkret sollen die folgenden Teilziele erreicht werden. Erstens sollen Baremetal-Treiber sowie Business-Logic auf dem Mikrocontroller implementiert werden. Dies schließlich Kamera-Ansteuerung, Display-Ausgabe und NPU-Inferenz ein. Zweitens soll die Python-Pipeline mit den drei Modellschritten Palm-Detection, Hand-Landmark und Sign-Language-Klassifikation vollständig mit Steuerungslogik entwickelt werden. Drittens sollen die bestehenden vortrainierten Modelle in Python getestet, quantifiziert und auf den Mikrocontroller deployt werden. Viertens soll das eigens zu trainierende Sign-Language-Modell auf dem Mikrocontroller betrieben werden. Um dies zu bewerkstelligen benötigt es einen zu erstellenden Datensatz mithilfe das Modell erlernt wird. Anschließend folgt die Quantisierung und deploment der modelle.

## Funktionale Anforderungen

Die Pipeline muss eine Kamera-Eingabe mit einer Auflösung von 5MP über die DCMIPP/CSI-Schnittstelle verarbeiten können. Für jede eingelesene Bildfolge muss eine Palm-Detection auf dem skalierten Frame (192x192 Pixel) durchgeführt werden. Die resultierenden Positionen von bis zu zwei Händen zu ermitteln. Anschließend muss für jede erkannte Hand ein zugeschnittener ROI-Bereich extrahiert und das Hand-Landmark-Modell auf diesem Bereich ausgeführt werden, um 21 Hand-Landmarks zu bestimmen. Das Multi-Hand-Tracking muss über mehrere Bildfolgen hinweg funktionieren, indem die ROI über die Landmark-Ergebnisse aktualisiert und für die nächste Folge vorhergesagt wird. Für jede erkannte Handkonfiguration muss der Sign-Language-Klassifikator die 88-Eingabewerte (2 Hände × 22 Gelenke × 2 Koordinaten) verarbeiten und eine der 26 Klassen (A–Y, NONE, SCH) mit Konfidenzwert ausgeben. Die Erkennungsergebnisse müssen auf dem Display (LTDC) angezeigt werden. Die Daten-Augmentierungspipeline muss die folgenden Transformationen unterstützen: Spiegelung, zufällige Translation, Skalierung, Jitter-Rauschen und Frame-Dropping.

## Nicht-Funktionale Anforderungen

Das System muss in der Lage sein, die Bildverarbeitung in Echtzeit durchzuführen, wobei eine Mindestbildrate von 15 FPS angestrebt wird. Alle drei Modelle müssen als INT8-quantifizierte TFLite-Modelle vorliegen, um auf der NPU des STM32N6570DK betrieben zu werden. Der Speicherverbrauch muss innerhalb der verfügbaren Ressourcen des Mikrocontrollers liegen, einschließlich des internen SRAMs und des externen PSRAMs. Das System muss robust gegenüber normalen Handschwankungen, Kamerarauschen und leicht veränderten Beleuchtungsverhältnissen sein. Die Wrist-relative Normalisierung der Landmarks muss positionsinvariant funktionieren, sodass dieselbe Geste an verschiedenen Bildschirmpositionen dasselbe Ergebnis liefert.

## Rahmenbedingungen

Die Hardware-Plattform ist der STM32N6570DK mit einem 800 MHz CPU-Kern, einem 1000 MHz NPU, einer Kamera-Schnittstelle (DCMIPP/CSI) und einem Display-Controller (LTDC). Die Programmierung der Embedded-Seite erfolgt baremetal ohne Betriebssystem. Die Python-Pipeline wird mit Python 3.x implementiert und verwendet die Bibliotheken TensorFlow/Keras für Modelltraining und -inferenz, OpenCV für Bildverarbeitung, MediaPipe für die ursprüngliche Datenerfassung sowie NumPy für numerische Berechnungen. Das Modellformat ist TFLite. Für das Deployment auf den Mikrocontroller wird die ST Edge AI Toolchain verwendet. Die Codestandards für die Embedded-Entwicklung folgen den definierten Code Conventions (Doxygen-Dokumentation, Modul-Präfixe, Tab-Indenzierung). Die Modellparameter und Konfigurationswerte sind zentral in der Datei config.py hinterlegt.

## Abnahmekriterien

Die Python-Pipeline muss in der Lage sein, anhand von Kamera-Input die Fingeralphabet-Buchstaben A–Y (ohne J und Z) sowie die Sonderklassen NONE und SCH zuverlässig zu erkennen. Die vortrainierten Modelle (Palm-Detection und Hand-Landmark) müssen in Python korrekt funktionieren und die ROI-basierte Tracking-Pipeline muss über mehrere Bildfolgen stabil arbeiten. Das eigens trainierte Sign-Language-Modell muss eine hinreichende Erkennungsgenauigkeit aufweisen und nach der INT8-Quantisierung eine akzeptable Genauigkeitsabweichung gegenüber dem FP32-Modell zeigen. Die drei Modelle müssen als INT8-quantifizierte TFLite-Dateien vorliegen und auf dem NPU des STM32N6570DK lauffähig sein. Die Display-Ausgabe muss das aktuelle Erkennungsergebnis mit zugehörigem Konfidenzwert in Echtzeit anzeigen. Der Gesamtverbrauch an RAM und Flash muss innerhalb der verfügbaren Ressourcen des Mikrocontrollers liegen