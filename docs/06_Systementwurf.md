
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

Ausgangspunkt der Verarbeitungskette ist das von der Kamera aufgenommene Bild mit einer Auflösung von 2592 × 1940 Pixeln. Die DCMIPP stellt den Kameradatenstrom parallel über zwei getrennte Verarbeitungspfade bereit. Pipe 1 erzeugt ein Bild mit einer Auflösung von 800 × 480 Pixeln, das für die Displayausgabe und als Ausgangsbild der Handlandmark-Erkennung verwendet wird. Pipe 2 erzeugt parallel dazu ein Bild mit einer Auflösung von 192 × 192 Pixeln für die Handdetektion.

Das Handdetektionsmodell verarbeitet das von Pipe 2 bereitgestellte Bild und ermittelt die Position einer vorhandenen Hand. Im Rahmen der Nachverarbeitung wird aus dem Modellergebnis eine initiale Region of Interest bestimmt. Da die Handdetektion und die Landmark-Erkennung unterschiedliche Bildpfade verwenden, wird die Region of Interest anschließend in das Koordinatensystem des von Pipe 1 erzeugten Bildes übertragen.

Auf Grundlage der transformierten Region of Interest wird aus dem Bild von Pipe 1 ein Ausschnitt mit einer Auflösung von 224 × 224 Pixeln erzeugt. Dieser Ausschnitt wird für das Handlandmark-Modell aufbereitet und anschließend an das Modell übergeben. Das Handlandmark-Modell bestimmt die Positionen der charakteristischen Landmark-Punkte innerhalb der Handregion.

Nach einer erfolgreichen Landmark-Erkennung wird die Region of Interest anhand der Modellausgabe aktualisiert. Für die Verarbeitung des folgenden Kamerabildes wird dadurch nicht erneut die gesamte Handdetektion ausgeführt. Stattdessen wird aus dem nächsten Bild von Pipe 1 unmittelbar eine neue Landmark-Eingabe auf Grundlage der aktualisierten Region erzeugt. Dieser Rückkopplungspfad bildet das Landmark-Tracking.

Kann das Handlandmark-Modell über mehrere aufeinanderfolgende Bilder keine gültige Hand mehr bestimmen, wird das Tracking beendet. Das System wechselt daraufhin zurück zur Handdetektion, um erneut eine initiale Region of Interest im vollständigen Bild zu bestimmen. Die Handdetektion und das Landmark-Tracking bilden somit zwei aufeinander abgestimmte Zustände der Verarbeitungskette.

Für die anschließende Zeichenklassifikation werden ausschließlich die ermittelten Landmark-Koordinaten verwendet. Vor der Übergabe an das Klassifikationsmodell werden diese in eine einheitliche Darstellung überführt und an das erforderliche Eingabeformat angepasst. Dazu gehören die Berücksichtigung der erkannten Händigkeit, die Normalisierung der Koordinaten und deren Überführung in das vom quantisierten Klassifikationsmodell verwendete Zahlenformat. Die konkrete Berechnung dieser Schritte wird im Implementierungskapitel beschrieben.

Das Klassifikationsmodell ordnet die aufbereiteten Landmark-Koordinaten einer der für den Prototyp festgelegten Zeichenklassen zu. Zusätzlich wird die Klasse NONE berücksichtigt, wenn keine der unterstützten Handformen vorliegt. Das ermittelte Klassifikationsergebnis wird anschließend an die grafische Ausgabe übergeben und auf dem Display dargestellt.
### 6.4 Speicher- und Datenflusskonzept

Die im Gesamtsystem anfallenden Daten unterscheiden sich deutlich hinsichtlich ihres Umfangs, ihrer Lebensdauer und ihrer Verwendung. Aus diesem Grund werden die Kamerabilder, Modellgewichte, Modellein- und -ausgaben sowie Steuerungsdaten auf unterschiedliche Speicherbereiche der Zielplattform verteilt. Abbildung X zeigt die vorgesehene Zuordnung der Daten zu den verfügbaren Speichern sowie den Datenfluss zwischen den beteiligten Hardware- und Softwarekomponenten.

![[qC App Data Flow Diagram.drawio.png]]
Abbildung X: Speicher- und Datenflusskonzept des Gesamtsystem

Die großformatigen Kamera- und Displaydaten werden im externen PSRAM gespeichert. Für das von Pipe 1 erzeugte Displaybild stehen vier Hintergrundbildpuffer zur Verfügung. Die mehreren Puffer ermöglichen es, die Bilderfassung, die KI-Verarbeitung, das Einzeichnen der Erkennungsergebnisse und die Displayausgabe auf voneinander getrennten Bildständen durchzuführen. Die Hintergrundbildpuffer werden durch den LTDC als Layer 1 ausgegeben. Zusätzlich greift der Hauptprozessor auf diese Puffer zu, um den Bildausschnitt für das Handlandmark-Modell zu erzeugen und die Region of Interest sowie die Handlandmarks direkt in das Displaybild einzuzeichnen.

Für die grafische Benutzeroberfläche stehen zwei separate Vordergrundpuffer im PSRAM zur Verfügung. Sie werden dem zweiten Layer des LTDC zugeordnet und enthalten unter anderem Bedienelemente, Statusinformationen und die Ausgabe des erkannten Fingeralphabetzeichens. Die doppelte Pufferung ermöglicht es, einen neuen Inhalt vorzubereiten, während der andere Puffer durch den LTDC ausgegeben wird. Der LTDC kombiniert den Hintergrund aus Layer 1 mit dem teilweise transparenten Vordergrund aus Layer 2 zur endgültigen Displayausgabe.

Pipe 2 erzeugt die auf 192 × 192 Pixel verkleinerten Eingabebilder für die Handdetektion. Diese werden abwechselnd in zwei weiteren Puffern in der externen PSRAM gespeichert. Während ein Puffer durch die DCMIPP beschrieben wird, kann der zuvor fertiggestellte Puffer für die Handdetektion bereitgestellt werden.

Die Gewichte der drei neuronalen Netze werden im externen NOR-Flash abgelegt. Für die Handdetektion, die Handlandmark-Erkennung und die Zeichenklassifikation sind jeweils getrennte Modelldaten vorgesehen. Da die Modellgewichte während des regulären Betriebs nicht verändert werden, können sie dauerhaft im nichtflüchtigen Speicher verbleiben.

Für die Ausführung der neuronalen Netze werden NPU-erreichbare Arbeitsspeicherbereiche verwendet. Jedes Modell benötigt einen Eingabepuffer, Aktivierungspuffer für die während der Inferenz entstehenden Zwischenergebnisse und einen Ausgabepuffer. Diese Daten werden nur für die Modellausführung benötigt und daher nicht dauerhaft gespeichert.

Die Modellausgaben werden durch den Hauptprozessor nachverarbeitet. Das Ergebnis der Handdetektion wird zur Bestimmung der initialen Region of Interest verwendet. Mithilfe dieser Region wird aus einem Hintergrundbildpuffer ein Ausschnitt für das Handlandmark-Modell erzeugt und in dessen Eingabepuffer übertragen. Die Ausgaben des Landmark-Modells werden zur Aktualisierung der Region of Interest, zur Darstellung der Landmark-Punkte und als Grundlage für die Zeichenklassifikation verwendet.

Nach der Aufbereitung werden die Landmark-Daten in den Eingabepuffer des Klassifikationsmodells übertragen. Dessen Ausgabe wird durch den Hauptprozessor in ein darstellbares Klassifikationsergebnis überführt und im dafür vorgesehenen Bereich im PSRAM abgelegt. Die Benutzeroberfläche greift auf diese Daten zu und übernimmt das Ergebnis in den Vordergrundpuffer.

Kleinere Steuerungs- und Zustandsdaten verbleiben im internen SRAM des Mikrocontrollers. Dazu gehören unter anderem der aktuelle Zustand der KI-Verarbeitungskette, die Region of Interest, die Landmark-Koordinaten, Zählerstände und Verwaltungsinformationen der Ablaufsteuerung.

| Speicherbereich                               | Gespeicherte Daten                                                                   |
| --------------------------------------------- | ------------------------------------------------------------------------------------ |
| Externer PSRAM                                | Displaypuffer, Eingabebilder der Handdetektion                                       |
| Externer NOR-Flash                            | Modellgewichte aller Modelle                                                         |
| NPU-erreichbarer Arbeitsspeicher<br>(AXI-RAM) | Eingabe-, Aktivierungs- und Ausgabepuffer der neuronalen Netze                       |
| Interner SRAM                                 | Steuerungsdaten, Zustandsinformationen, Landmark-Daten und Klassifikationsergebnisse |
Tabelle X: Zuordnung der Systemdaten zu den Speicherbereichen

### 6.5 Ablauf vom Kamerabild bis zum Erkennungsergebnis

In den vorherigen Abschnitten wurden zunächst die Komponenten des Gesamtsystems, deren Zuordnung zur Zielplattform sowie die KI-Verarbeitungskette und der zwischen den Komponenten stattfindende Datenfluss beschrieben. Darauf aufbauend stellt dieser Abschnitt den zyklischen Ablauf und die dabei auftretenden Entscheidungen von der Erfassung eines Kamerabildes bis zur Ausgabe des Klassifikationsergebnisses dar. Der Ablauf berücksichtigt insbesondere den Wechsel zwischen der erstmaligen Handdetektion und dem anschließenden Landmark-Tracking.

![[Aktivitätsdiagramm_ablauf.png]]
Abbildung X: UML Aktivitätsdiagramm - Ablauf vom Kamerabild bis zum Erkennungsergebnis

Abbildung X führt die zuvor beschriebenen Komponenten und Verarbeitungsschritte in einem gemeinsamen Ablauf zusammen. Nach der Erfassung wird das Kamerabild durch die DCMIPP verarbeitet und über Pipe 1 für die Display- und Landmark-Verarbeitung sowie über Pipe 2 für die Handdetektion bereitgestellt.

Ist noch kein Landmark-Tracking aktiv, wird zunächst die Handdetektion auf dem von Pipe 2 bereitgestellten Bild ausgeführt. Wird keine Hand erkannt, beginnt die Verarbeitung mit dem nächsten Kamerabild erneut. Bei einer erfolgreichen Erkennung wird eine initiale Region of Interest bestimmt und in das Koordinatensystem von Pipe 1 übertragen. Aus dem dort vorliegenden Displaybild wird anschließend ein 224 × 224 Pixel großer Ausschnitt für das Landmark-Modell erzeugt. Bei aktivem Tracking kann hierfür unmittelbar die anhand des vorherigen Bildes aktualisierte Region verwendet werden.

Nach der Ausführung des Landmark-Modells wird geprüft, ob eine gültige Modellausgabe vorliegt. Ist dies der Fall, wird die Region of Interest für den nächsten Durchlauf aktualisiert, das Tracking aktiviert beziehungsweise fortgeführt und der Verlustzähler zurückgesetzt. Anschließend werden die Landmark-Koordinaten entsprechend der in Abschnitt 6.3 beschriebenen KI-Verarbeitungskette aufbereitet und durch das Klassifikationsmodell einem Fingeralphabetzeichen zugeordnet. Das ermittelte Ergebnis wird nachverarbeitet und über die in Abschnitt 6.4 beschriebenen Anzeige- und Speicherbereiche für die grafische Ausgabe bereitgestellt.

Liegt keine gültige Landmark-Ausgabe vor, wird der Verlustzähler erhöht. Solange der festgelegte Grenzwert nicht erreicht ist, bleibt das Tracking aktiv und die Landmark-Erkennung wird mit dem nächsten Kamerabild erneut ausgeführt. Wird der Grenzwert erreicht, wird das Tracking zurückgesetzt, sodass im folgenden Durchlauf erneut eine Handdetektion auf dem vollständigen Eingabebild erfolgt.

Unabhängig vom jeweiligen Verarbeitungspfad kehrt der Ablauf anschließend zur Erfassung eines neuen Kamerabildes zurück. Dadurch entsteht ein kontinuierlicher Verarbeitungszyklus, der sowohl die erstmalige Lokalisierung einer Hand als auch deren anschließende Verfolgung und die wiederholte Klassifikation dargestellter Fingeralphabetzeichen ermöglicht.