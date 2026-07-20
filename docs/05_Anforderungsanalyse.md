
## 5 Anforderungsanalyse

Die Anforderungsanalyse bildet die Grundlage für den Entwurf, die Implementierung und die anschließende Bewertung des zu entwickelnden Systems. Die festgelegten Anforderungen beeinflussen den Aufbau des Systems und bestimmen, welche Funktionen und Eigenschaften bei der Entwicklung besonders berücksichtigt werden müssen (Kleuker, 2025, S. 53ff).

Zur Ermittlung und Beschreibung der Anforderungen werden zunächst das vorgesehene Anwendungsszenario und die daraus abgeleiteten Anwendungsfälle betrachtet. Darauf aufbauend werden die funktionalen und nichtfunktionalen Anforderungen sowie die durch die Zielplattform vorgegebenen Randbedingungen festgelegt. Abschließend werden überprüfbare Abnahmekriterien definiert, die im weiteren Verlauf der Arbeit als Grundlage für die Verifikation und Evaluation des Gesamtsystems dienen.

### 5.1 Anwendungsszenario

Das entwickelte System ist als prototypischer Fingeralphabet-Trainer vorgesehen. Es soll Personen beim selbständigen Üben ausgewählter statischer Handzeichen des deutschen Fingeralphabets unterstützen. Die nutzende Person positioniert hierzu eine Hand im Erfassungsbereich der Kamera und führt ein Zeichen aus. Das Kamerabild wird vom System aufgenommen und lokal verarbeitet.

Die Verarbeitung erfolgt in mehreren aufeinanderfolgenden Schritten. Zunächst wird die Hand im Kamerabild lokalisiert. Anschließend werden charakteristische Handlandmarks bestimmt und für die Klassifikation aufbereitet. Auf Grundlage dieser Merkmale ordnet das Klassifikationsmodell die dargestellte Handform einer der unterstützten Zeichenklassen zu. Das ermittelte Ergebnis wird über das Display des Systems ausgegeben und ermöglicht eine unmittelbare Rückmeldung zur dargestellten Handform. Das System ist für die Verwendung durch jeweils eine Person und die Erfassung einer einzelnen Hand vorgesehen. Die Handzeichen werden nacheinander und innerhalb eines geeigneten Abstands zur Kamera dargestellt. Die Verarbeitung setzt ausreichende Lichtverhältnisse und eine weitgehend freie Sicht auf die Hand voraus. Verdeckte Hände, mehrere gleichzeitig dargestellte Hände sowie dynamische Bewegungsabläufe gehören nicht zum vorgesehenen Anwendungsszenario.

Sämtliche Verarbeitungsschritte werden auf einem eingebetteten System ausgeführt. Eine Verbindung zu einem externen Rechner oder einem Cloud-Dienst ist für den Betrieb des fertigen Prototyps nicht vorgesehen.

![[Use_Case.png | center | 2560]]
Abbildung x: Use-Case Diagramm Fingeralphabet Trainer

Abbildung X stellt das Anwendungsszenario des entwickelten Fingeralphabet-Trainers als Use-Case-Diagramm dar. Die übende Person startet das System und zeigt ein Handzeichen im Erfassungsbereich der Kamera. Der Anwendungsfall „Handzeichen erkennen“ umfasst die Erfassung des Kamerabildes, die Lokalisierung der Hand, die Bestimmung der Handlandmarks und die Klassifikation des dargestellten Zeichens. Das ermittelte Erkennungsergebnis wird anschließend auf dem Display ausgegeben.

### 5.2 Anforderungen & Randbedingungen

Aus dem zuvor festgelegten Anwendungsszenario und dem zugehörigen Use-Case-Diagramm lassen sich die folgenden funktionalen, nichtfunktionalen und technischen Anforderungen, bzw. Randbedingungen ableiten.

### 5.2.1 Funktionale Anforderungen

| ID    | Funktionale Anforderung                                                                                                                  |
| ----- | ---------------------------------------------------------------------------------------------------------------------------------------- |
| FA_01 | Das System muss über die angeschlossene Kamera laufend Bilddaten erhalten können.                                                        |
| FA_02 | Das System muss eine einzelne Hand im Kamerabild lokalisieren können.                                                                    |
| FA_03 | Aus einer lokalisierten Hand muss eine Region of Interest gebildet und fortlaufend aktualisiert werden können.                           |
| FA_04 | Das System muss innerhalb der Region of Interest die charakteristischen Handlandmarks bestimmen können.                                  |
| FA_05 | Die Handlandmarks müssen für die Fingeralphabet-Klassifizierung vorbereitet werden können.                                               |
| FA_06 | Das System muss anhand der aufbereiteten Handlandmarks das statische Handzeichen einer Klassifizierung ausgeben können.                  |
| FA_07 | Das Klassifizierungs- bzw. Inferenzergebnis muss auf dem Display ausgegeben werden können.                                               |
| FA_08 | Das System muss die Erkennung, bzw. Inferenz wiederholt ausführen und Änderungen von Handzeichen erkennen können.                        |
| FA_09 | Das System muss mit Situationen umgehen können, in denen keine Hand oder kein eindeutig klassifizierbares Handzeichen erkannt wird.      |
| FA_10 | Das System muss die für die Bewertung relevanten Ergebnisse und Laufzeitinformationen erfassen oder zur Auswertung bereitstellen können. |
Tabelle x: Funktionale Anforderungen

### 5.2.2 Nichtfunktionale Anforderungen

| ID     | Nichtfunktionale Anforderungen                                                                                                                                                             |
| ------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| NFA_01 | Das System muss unter den festgelegten Bedingungen eine Erkennungsgenauigkeit von mindestens 80% erreichen                                                                                 |
| NFA_02 | Das System muss eine Bild- bzw. Verarbeitungsrate von mindestens 15 Bildern pro Sekunde erreichen.                                                                                         |
| NFA_03 | Die Zeitspanne von der Bereitstellung eines Kamerabildes bis zur Ausgabe des zugehörigen Klassifikationsergebnisses darf 80 ms nicht überschreiten.                                        |
| NFA_04 | Das System muss mindestens 20 Minuten kontinuierlich arbeiten können, ohne abzustürzen oder einen Neustart zu erfordern.                                                                   |
| NFA_05 | Das System muss die Hand unter den festgelegten Testbedingungen auch bei Positionsänderungen und veränderten Lichtverhältnissen ausreichend zuverlässig lokalisieren und verfolgen können. |
| NFA_06 | Das Erkennungsergebnis muss auf dem Display eindeutig und für die nutzende Person verständlich dargestellt werden.                                                                         |
| NFA_07 | Der Quellcode soll modular aufgebaut und ausreichend dokumentiert sein, sodass einzelne Komponenten unabhängig angepasst und getestet werden können.                                       |
| NFA_08 | Der Speicherbedarf der Modelle und Laufzeitdaten darf die hierfür vorgesehenen internen und externen Speicherbereiche nicht überschreiten.                                                 |
Tabelle x: Nichtfunktionale Anforderungen

### 5.2.3 Technische Anforderungen

| ID    | Technische Anforderungen                                                                                                                                                                                                                               |
| ----- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| TA_01 | Als Zielplattform muss das STM32N6570 Discovery Kit verwendet werden                                                                                                                                                                                   |
| TA_02 | Das System muss die vollständige Verarbeitung vom Kamerabild bis zur Ausgabe des Klassifikationsergebnisses lokal auf der eingebetteten Zielplattform ausführen können.                                                                                |
| TA_03 | Die Kamerabilder müssen über die an das Board angeschlossene Kamera erfasst und innerhalb der eingebetteten Plattform verarbeitet werden. Dabei handelt es sich um das IMX-335 Sensormodul.                                                            |
| TA_04 | Die Ausgabe des Kamerabildes und des Erkennungsergebnisses muss über das angeschlossene Display erfolgen. Dabei handelt es sich um das MB1860B.                                                                                                        |
| TA_05 | Die Kamerapipeline wird mit den vom Board bereitgestellten Schnittstellen implementiert. Das sind unter anderem Camera Serial Interface (CSI), Digital Camera Interface Pipe Processing (DCMIPP), LCD-TFT Display Controller (LTDC).                   |
| TA_06 | Bildsignalanpassungen wie z.B. (Gammakorrektur, Blacklevelkorrektur, RGB-Conversion) müssen embedded mittels DCMIPP erfolgen.                                                                                                                          |
| TA_07 | Hardwarezugriffe sollen in abgegrenzten Treiber- und Schnittstellenmodulen gekapselt werden. Soweit für die erforderliche Kontrolle und Ausführungsgeschwindigkeit notwendig, sollen Low-Layer-Treiber oder direkte Registerzugriffe verwendet werden. |
| TA_08 | Die verwendeten neuronalen Netze müssen in das INT8-Zahlenformat quantisiert und mit ST Edge AI Core für die Zielplattform konvertiert werden.                                                                                                         |
| TA_09 | Die von der Hardware unterstützten Operationen der neuronalen Netze müssen auf dem integrierten ST Neural-ART Accelerator ausgeführt werden. Nicht unterstützte Operationen dürfen auf dem Hauptprozessor ausgeführt werden.                           |
| TA_10 | Die Gewichte der neuronalen Netze müssen im externen Octo-SPI-NOR-Flash gespeichert werden.                                                                                                                                                            |
| TA_11 | Die Bilddaten und die hierfür erforderlichen Puffer müssen in der externen Hexadeca-SPI-PSRAM abgelegt werden.                                                                                                                                         |
| TA_12 | Der reguläre Betrieb des Prototyps muss ohne aktive Verbindung zu einem externen Rechner oder einem Cloud-Dienst möglich sein.                                                                                                                         |
Tabelle x: Technische Anforderungen

### 5.2.4 Randbedingungen

| ID    | Randbedingungen                                                                                                                                                                                  |
| ----- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| RB_01 | Als Zielplattform steht ausschließlich das STM32N6570 Discovery Kit mit dem Mikrocontroller STM32N657X0H3Q zur Verfügung.                                                                        |
| RB_02 | Der Mikrocontroller verfügt nur über begrenzten internen Arbeits- und Programmspeicher. Größere Datenbestände müssen daher in den externen Speichern abgelegt werden.                            |
| RB_03 | Als externer nichtflüchtiger Speicher steht ein 1-Gbit-Octo-SPI-NOR-Flash zur Verfügung. Zur Zwischenspeicherung größerer Datenmengen steht eine 256-Mbit-Hexadeca-SPI-PSRAM zur Verfügung.      |
| RB_04 | Zur Beschleunigung der neuronalen Netze steht der integrierte ST Neural-ART Accelerator zur Verfügung. Dessen Nutzung ist auf die unterstützten Operationen und Datenformate beschränkt.         |
| RB_05 | Die Konfiguration und Konvertierung der neuronalen Netze erfolgt mit den Werkzeugen der ST Edge AI Suite beziehungsweise mit ST Edge AI Core.                                                    |
| RB_06 | Für die Handlokalisierung und Handlandmark-Erkennung werden bereits bestehende Modelle verwendet. Lediglich das nachgelagerte Klassifikationsmodell wird im Rahmen dieser Arbeit neu entwickelt. |
| RB_07 | Modellgewichte müssen vor Inbetriebnahme manuell oder per Skript auf den NOR-Flash geladen werden.                                                                                               |
Tabelle x: Randbedingungen

### 5.3 Abnahmekriterien

Die Abnahmekriterien werden aus den zuvor festgelegten Anforderungen abgeleitet. Sie legen fest, anhand welcher Prüfungen und Grenzwerte beurteilt wird, ob das entwickelte Gesamtsystem die jeweiligen Anforderungen erfüllt. Inhaltlich zusammengehörige Anforderungen können dabei gemeinsam überprüft werden.

| ID    | Zugeordnete Anforderungen | Abnahmekriterium                                                                                                                                         | Prüfverfahren                                                                                                                         |
| ----- | ------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------- |
| AK_01 | FA_01                     | Das System erfasst fortlaufend gültige Kamerabilder.                                                                                                     | Visuelle Überprüfung des Displays.                                                                                                    |
| AK_02 | FA_02 bis FA_04           | Eine Hand wird lokalisiert, eine Region of Interest wird gebildet und die Handlandmarks werden bestimmt.                                                 | Prüfung anhand ausgewählter Bilder und Visualisierung der Region of Interest, bzw. der Landmarks.                                     |
| AK_03 | FA_05, FA_06, NFA_01      | Das Klassifikationsmodell erreicht im Anwendungsfall eine Gesamtgenauigkeit von mindestens 80 %.                                                         | Auswertung der Modellvorhersagen und Erstellung einer Konfusionsmatrix.                                                               |
| AK_04 | FA_07, NFA_06             | Das erkannte Zeichen wird eindeutig und lesbar auf dem Display dargestellt.                                                                              | Visuelle Prüfung der Anzeige für alle unterstützen Zeichenklassen.                                                                    |
| AK_05 | FA_08                     | Mehrere nacheinander dargestellte Handzeichen werden ohne Neustart des Systems erkannt und die Anzeige wird entsprechend aktualisiert.                   | Durchführung einer festgelegten Zeichenfolge und Vergleich von dargestelltem und ausgegebenem Zeichen.                                |
| AK_06 | FA_09                     | Wird keine Hand oder kein ausreichend eindeutiges Handzeichen erkannt, gibt das System kein gültiges Klassifikationsergebnis aus.                        | Tests ohne Hand sowie mit nicht unterstützten oder uneindeutigen Handformen.                                                          |
| AK_07 | FA_10                     | Erkennungs-, Laufzeit- und Speicherinformationen können für die Evaluation ausgelesen werden.                                                            | Überprüfung der Messwerte über die Debug-Schnittstelle.                                                                               |
| AK_08 | NFA_02                    | Das Gesamtsystem verarbeitet mindestens 15 vollständige Kamerabilder pro Sekunde.                                                                        | Zählung der vollständig verarbeiteten Bilder während eines festgelegten Messzeitraums und Berechnung der mittleren Verarbeitungsrate. |
| AK_09 | NFA_03                    | Die Zeitspanne vom Beginn der Verarbeitung eines Kamerabildes bis zur Bereitstellung des zugehörigen Klassifikationsergebnisses beträgt höchstens 80 ms. | Zeitmessung an festgelegten Punkten der Verarbeitungskette mithilfe von Zeitstempeln oder Hardware-Timern.                            |
| AK_10 | NFA_04                    | Das System arbeitet mindestens 20 Minuten ohne Absturz.                                                                                                  | Kontinuierlicher Dauerlauftest.                                                                                                       |
| AK_11 | NFA_05                    | Die Hand wird unter den festgelegten Bewegungs- und Beleuchtungsszenarien in mindestens 70 % der ausgewerteten Bildern erfolgreich lokalisiert.          | Durchführung und Auswertung definierter Testszenarien.                                                                                |
| AK_12 | TA_05, TA_06              | Die Kameradaten werden über die festgelegten Schnittstellen verarbeitet. Signalanpassungen erfolgen embedded.                                            | Überprüfung der Peripheriekonfiguration und verwendeten Registereinstellungen.                                                        |
| AK_13 | TA_10, TA_11              | Die Modellgewichte sind im externen NOR-Flash abgelegt.<br>Die Bilddaten und Bildpuffer sind im externen PSRAM abgelegt.                                 | Überprüfung der Speicheradressen mittels Linker-Map-Datei und Memorybrowser.                                                          |
| AK_14 | TA_01, TA_12              | Das Gesamtsystem wird auf dem STM32N6570 Discovery Kit ausgeführt. Die Rechenschritte werden ausschließlich auf diesem Board durchgeführt.               | Durchführung sämtlicher Funktions- und Leistungstests auf der Zielplattform als abschließender Test.                                  |
| AK_15 | NFA_07, TA_07             | Der Quellcode ist überwiegend modular aufgebaut und ohne HAL implementiert.                                                                              | Durchführung von Code-Review.                                                                                                         |
| AK_16 | TA_08                     | Die verwendeten Modelle sind quantisiert und für den Gebrauch mit der NPU optimiert.                                                                     | Überprüfung der Modelle und des Quellcodes.                                                                                           |

Tabelle x: Abnahmekriterien.