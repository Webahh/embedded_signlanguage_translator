
## 6 Systementwurf

Auf Grundlage der zuvor definierten Anforderungen wird in diesem Kapitel der Entwurf des Gesamtsystems beschrieben. Zunächst wird ein übergeordnetes Systemkonzept vorgestellt, das die zentralen Komponenten und deren Zusammenwirken darstellt. Anschließend werden die Funktionen den verfügbaren Hardwarekomponenten der Zielplattform zugeordnet. Abschließend wird die mehrstufige KI-Verarbeitungskette von der Bilderfassung bis zur Klassifikation des dargestellten Fingeralphabetzeichens beschrieben.

Der Systementwurf legt damit die grundlegende Architektur und die Schnittstellen zwischen den einzelnen Komponenten fest. Die konkrete technische Umsetzung, darunter die Konfiguration der Peripherie, die Speicherverwaltung sowie die Vor- und Nachverarbeitung der Modelldaten, wird im nachfolgenden Implementierungskapitel behandelt.
### 6.1 Systemkonzept

Ausgehend von den zuvor definierten Anforderungen wurde ein Systemkonzept entwickelt, das die funktionalen Komponenten des Gesamtsystems und den zwischen ihnen stattfindenden Datenfluss beschreibt. Das Konzept stellt zunächst eine weitgehend von der konkreten Zielplattform unabhängige Betrachtung des Systems dar. Dadurch können die erforderlichen Verarbeitungsschritte festgelegt werden, bevor diese im Hardware- und Softwareentwurf konkreten Komponenten der Zielplattform zugeordnet werden.

Das in Abbildung X dargestellte Blockdiagramm veranschaulicht den Datenfluss von der Aufnahme eines Kamerabildes bis zur Ausgabe des erkannten Fingeralphabetzeichens auf dem Display.

![[Systemkonzept_erweitert.png]]
Abbildung X: Blockdiagramm des Systemkonzepts

Die Kamera erfasst kontinuierlich Bilddaten und überträgt diese an das System. Die aufgenommenen Bilder werden zunächst in einem Bildpuffer zwischengespeichert. Das Bild wird für die Palmdetection skaliert und anschließend der Verarbeitungskette zur Verfügung gestellt. Innerhalb der Verarbeitungskette kommen drei aufeinander aufbauende neuronale Netze zum Einsatz.

Im ersten Verarbeitungsschritt wird mithilfe des Handdetektionsmodells geprüft, ob sich eine Hand im Kamerabild befindet. Für eine erkannte Hand werden deren Position und räumliche Ausdehnung bestimmt. Aus diesen Informationen wird eine Region of Interest gebildet, welche den für die weiteren Verarbeitungsschritte relevanten Bildbereich enthält. Die Region of Interest wird entsprechend der erkannten Handposition aktualisiert und für die nachfolgende Landmark-Erkennung aufbereitet. 

Das zweite Modell bestimmt innerhalb der Region of Interest die charakteristischen Handlandmarks. Die ermittelten Koordinaten werden anschließend nachverarbeitet und in eine für das Klassifikationsmodell geeignete Form überführt. Hierzu gehören insbesondere die Anpassung des Datenformats sowie die für die Klassifikation erforderliche Normalisierung der Landmark-Koordinaten. Außerdem übernimmt das Modell das Tracking und aktualisiert die Region of Interest, sobald sich die Hand bewegt. Sobald die Hand verloren geht, übernimmt wieder das erste Modell und erzeugt eine neue Region of Interest.

Das dritte Modell klassifiziert die aufbereiteten Handlandmarks und ordnet die dargestellte Handform einer der unterstützten Zeichenklassen zu. Das Klassifikationsergebnis wird gemeinsam mit dem Kamerabild auf dem Display ausgegeben. Wird keine Hand erkannt oder erreicht die Klassifikation nicht die erforderliche Konfidenz, wird kein gültiges Zeichen ausgegeben.

Die Verarbeitungskette wird zyklisch ausgeführt, sodass Änderungen der Handposition und des dargestellten Handzeichens fortlaufend berücksichtigt werden können. Eine übergeordnete Ablaufsteuerung koordiniert dabei die Bildaufnahme, die Ausführung der Modelle, die Vor- und Nachverarbeitung sowie die Aktualisierung der grafischen Ausgabe.
### 6.2 Hardwarekonzept

Das Hardwarekonzept überführt die im Systemkonzept festgelegten Funktionen auf die Komponenten der eingebetteten Zielplattform. Als zentrale Verarbeitungseinheit wird das STM32N6570 Discovery Kit verwendet. Neben dem Mikrocontroller umfasst der Hardwareaufbau das Kameramodul MB1854B mit dem Bildsensor IMX335, das Displaymodul MB1860B sowie die auf dem Board vorhandenen externen Speicher.

Die Kameradaten werden über die CSI-Schnittstelle empfangen und mithilfe der Digital Camera Interface Pixel Pipeline verarbeitet. Die DCMIPP übernimmt hardwarenahe Verarbeitungsschritte wie die Anpassung des Bildformats, die Skalierung und die Aufbereitung der Kamerabilder für die weitere Verarbeitung. Die erzeugten Bilddaten und die erforderlichen Bildpuffer werden aufgrund ihres Umfangs in der externen PSRAM abgelegt.

Die Gewichte der neuronalen Netze werden im externen NOR-Flash gespeichert. Für die Ausführung der unterstützten Modelloperationen wird der integrierte ST Neural-ART Accelerator verwendet. Der Hauptprozessor übernimmt insbesondere die Ablaufsteuerung, die Vor- und Nachverarbeitung der Modelldaten sowie die Konfiguration und Koordination der eingesetzten Peripheriekomponenten.

Die grafische Ausgabe erfolgt über den LCD-TFT Display Controller. Dieser liest die darzustellenden Bild- und Ergebnisdaten aus den zugeordneten Speicherbereichen und überträgt sie an das angeschlossene Display. Dadurch können das aktuelle Kamerabild und das ermittelte Klassifikationsergebnis gemeinsam dargestellt werden.

### 6.3 KI-Verarbeitungskette

Die KI-Verarbeitungskette besteht aus drei aufeinander aufbauenden Modellen zur Handdetektion, Handlandmark-Erkennung und Klassifikation des dargestellten Fingeralphabetzeichens. Abbildung X zeigt die einzelnen Verarbeitungsscgitte sowie die beiden durch die DCMIPP bereitgestellten Bildpfade.

![[ML_sequence_v3.png]]
Abbildung X: KI-Verarbeitungskette zur Erkennung der Fingeralphabetzeichen

Ausgangspunkt der Verarbeitungskette ist das von der Kamera aufgenommene Bild mit einer Auflösung von 2592 × 1944 Pixeln. Die DCMIPP stellt den Kameradatenstrom parallel über zwei getrennte Verarbeitungspfade bereit. Pipe 1 erzeugt ein Bild mit einer Auflösung von 800 × 480 Pixeln, das für die Displayausgabe und als Ausgangsbild der Handlandmark-Erkennung verwendet wird. Pipe 2 erzeugt parallel dazu ein Bild mit einer Auflösung von 192 × 192 Pixeln für die Handdetektion.

Das Handdetektionsmodell verarbeitet das von Pipe 2 bereitgestellte Bild und ermittelt die Position einer vorhandenen Hand. Im Rahmen der Nachverarbeitung wird aus dem Modellergebnis eine initiale Region of Interest bestimmt. Da die Handdetektion und die Landmark-Erkennung unterschiedliche Bildpfade verwenden, wird die Region of Interest anschließend in das Koordinatensystem des von Pipe 1 erzeugten Bildes übertragen.

Auf Grundlage der transformierten Region of Interest wird aus dem Bild von Pipe 1 ein Ausschnitt mit einer Auflösung von 224 × 224 Pixeln erzeugt. Dieser Ausschnitt wird für das Handlandmark-Modell aufbereitet und anschließend an das Modell übergeben. Das Handlandmark-Modell bestimmt die Positionen der charakteristischen Landmark-Punkte innerhalb der Handregion.

Nach einer erfolgreichen Landmark-Erkennung wird die Region of Interest anhand der Modellausgabe aktualisiert. Für die Verarbeitung des folgenden Kamerabildes wird dadurch nicht erneut die gesamte Handdetektion ausgeführt. Stattdessen wird aus dem nächsten Bild von Pipe 1 unmittelbar eine neue Landmark-Eingabe auf Grundlage der aktualisierten Region erzeugt. Dieser Rückkopplungspfad bildet das Landmark-Tracking.

Kann das Handlandmark-Modell über mehrere aufeinanderfolgende Bilder keine gültige Hand mehr bestimmen, wird das Tracking beendet. Das System wechselt daraufhin zurück zur Handdetektion, um erneut eine initiale Region of Interest im vollständigen Bild zu bestimmen. Die Handdetektion und das Landmark-Tracking bilden somit zwei aufeinander abgestimmte Zustände der Verarbeitungskette.

Für die anschließende Zeichenklassifikation werden ausschließlich die ermittelten Landmark-Koordinaten verwendet. Vor der Übergabe an das Klassifikationsmodell werden diese in eine einheitliche Darstellung überführt und an das erforderliche Eingabeformat angepasst. Dazu gehören die Berücksichtigung der erkannten Händigkeit, die Normalisierung der Koordinaten und deren Überführung in das vom quantisierten Klassifikationsmodell verwendete Zahlenformat. Die konkrete Berechnung dieser Schritte wird im Implementierungskapitel beschrieben.

Das Klassifikationsmodell ordnet die aufbereiteten Landmark-Koordinaten einer der für den Prototyp festgelegten Zeichenklassen zu. Zusätzlich wird die Klasse NONE berücksichtigt, wenn keine der unterstützten Handformen vorliegt. Das ermittelte Klassifikationsergebnis wird anschließend an die grafische Ausgabe übergeben und auf dem Display dargestellt.
### 6.4 Speicher- und Datenflusskonzept

Die im Gesamtsystem anfallenden Daten unterscheiden sich deutlich hinsichtlich ihres Umfangs, ihrer Lebensdauer und ihrer Verwendung. Aus diesem Grund werden die Kamerabilder, Modellgewichte, Modellein- und -ausgaben sowie Steuerungsdaten auf unterschiedliche Speicherbereiche der Zielplattform verteilt. Abbildung X zeigt die vorgesehene Zuordnung der Daten zu den verfügbaren Speichern.

![[qC App Data Flow Diagram.drawio.png]]
Abbildung X: Speicher- und Datenflusskonzept des Gesamtsystem

Die großformatigen Kamera- und Displaydaten werden in der externen PSRAM gespeichert. Dazu gehören die Hintergrundbildpuffer für das von Pipe 1 erzeugte Displaybild, die Vordergrundpuffer für die grafische Benutzeroberfläche sowie die beiden Eingabebildpuffer der Handdetektion. Die Verwendung der externen PSRAM ist erforderlich, da die Bilddaten einen erheblichen Speicherbedarf besitzen und den intern verfügbaren Arbeitsspeicher stark beanspruchen würden.

Die Gewichte der drei neuronalen Netze werden im externen NOR-Flash abgelegt. Dies betrifft das Modell zur Handdetektion, das Handlandmark-Modell und das Klassifikationsmodell. Da die Modellgewichte während des Betriebs nicht verändert werden, können sie dauerhaft im nichtflüchtigen Speicher verbleiben. Für jedes neuronale Netz werden zusätzliche Arbeitsspeicherbereiche benötigt. Dazu gehören jeweils ein Eingabepuffer, die während der Inferenz benötigten Aktivierungspuffer und ein Ausgabepuffer. Diese Daten werden in den für den Neural-ART Accelerator erreichbaren Speicherbereichen abgelegt. Die Puffer werden nur während der Modellausführung benötigt und enthalten keine dauerhaft zu speichernden Daten.

Kleinere Steuerungs-, Status- und Ergebnisdaten werden im internen SRAM des Mikrocontrollers gespeichert. Dazu zählen unter anderem das aktuelle Klassifikationsergebnis, Zustandsinformationen der Verarbeitungskette und die von der Ablaufsteuerung verwendeten Verwaltungsdaten. Die grundlegende Speicherzuordnung ist in Tabelle X zusammengefasst.

Der Bilddatenfluss beginnt an der DCMIPP. Die von Pipe 1 erzeugten Bilddaten werden in den Hintergrundbildpuffern der Displayausgabe in der externen PSRAM abgelegt. In diese Bildpuffer zeichnet der Hauptprozessor zusätzlich die Region of Interest und die ermittelten Handlandmarks ein. Der LTDC liest die zusammengesetzten Bilddaten anschließend über Layer 1 aus und gibt sie auf dem Display aus. Für weitere Elemente der grafischen Benutzeroberfläche steht ein separater Vordergrundpuffer zur Verfügung, der dem zweiten Layer des LTDC zugeordnet ist. Dieser enthält beispielsweise Bedienelemente, Statusinformationen und das Klassifikationsergebnis. Der LTDC kombiniert den Hintergrund aus Layer 1 mit dem teilweise transparenten Vordergrund aus Layer 2 zur endgültigen Displayausgabe.

Die von Pipe 2 erzeugten Bilddaten werden in den beiden Eingabebildpuffern der Handdetektion gespeichert und von dort für das Handdetektionsmodell bereitgestellt. Die daraus ermittelte Region of Interest legt fest, welcher Bereich des Displaybildes für das Handlandmark-Modell aufbereitet wird. Die Ausgabedaten des Landmark-Modells bilden nach ihrer Aufbereitung die Eingabedaten des Klassifikationsmodells. Das abschließend ermittelte Klassifikationsergebnis wird im internen SRAM zwischengespeichert und von der Benutzeroberfläche für die grafische Ausgabe verwendet. Die Modellgewichte verbleiben während dieses Datenflusses im externen NOR-Flash, während die jeweiligen Eingabe-, Aktivierungs- und Ausgabedaten in den NPU-erreichbaren Arbeitsbereichen liegen.

| Speicherbereich                               | Gespeicherte Daten                                                                   |
| --------------------------------------------- | ------------------------------------------------------------------------------------ |
| Externer PSRAM                                | Displaypuffer, Eingabebilder der Handdetektion                                       |
| Externer NOR-Flash                            | Modellgewichte aller Modelle                                                         |
| NPU-erreichbarer Arbeitsspeicher<br>(AXI-RAM) | Eingabe-, Aktivierungs- und Ausgabepuffer der neuronalen Netze                       |
| Interner SRAM                                 | Steuerungsdaten, Zustandsinformationen, Landmark-Daten und Klassifikationsergebnisse |
Tabelle X: Zuordnung der Systemdaten zu den Speicherbereichen
### 6.5 Ablauf vom Kamerabild bis zum Erkennungsergebnis

Hier UML ablaufdiagramm einfügen