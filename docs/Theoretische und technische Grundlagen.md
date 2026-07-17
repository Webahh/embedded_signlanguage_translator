## 2 Theoretische und technische Grundlagen

Dieses Kapitel vermittelt die für das Verständnis der Arbeit erforderlichen Grundlagen. Es beginnt mit den zentralen Konzepten eingebetteter Systeme und der Bare-Metal-Programmierung.  Folgen wird das deutsche Einhand-Fingeralphabet als zu erkennendes Zeichensystem eingeführt. Abschließend werden die Grundlagen des maschinellen Lernens, der Handdetektion, der Modellquantisierung sowie der Edge AI erwähnt.

---

### 2.1 Eingebettete Systeme

#### 2.1.1 Begriffsbildung und Merkmale

Eingebettete Systeme (engl. *embedded systems*) sind Computersysteme, die als Bestandteil eines übergeordneten Gesamtsystems in eine technische Umgebung eingebettet sind (Marwedel, 2021). Im Gegensatz zu universellen Rechnern (Personal Computer, Server) erfüllen sie eine spezifische, fest definierte Aufgabe und sind darauf optimiert. Typische Merkmale sind (Wolf, 2012):

| Aspekt                 | Embedded Spezifisch                                                                                                                                                                                                                                                                |
| ---------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Ressourcenbeschränkung | Eingebettete Systeme verfügen über begrenzten Hauptspeicher (RAM), begrenzten Programmspeicher (Flash) und eine eingeschränkte Rechenleistung im Vergleich zu General-Purpose-Computern                                                                                            |
| Echtzeitfähigkeit      | Viele Anwendungen erfordern, dass Daten innerhalb definierter Zeitgrenzen verarbeitet werden. Man unterscheidet zwischen *weicher* Echtzeit (eine Überschreitung der Frist ist unerwünscht, aber tolerierbar) und *harter* Echtzeit (eine Überschreitung führt zum Systemversagen) |
| Hohe Zuverlässigkeit   | Da eingebettete Systeme häufig in sicherheitskritischen oder industriellen Umgebungen eingesetzt werden, muss ein definiertes Fehlverhalten über lange Zeiträume vermieden werden                                                                                                  |
| Energieeffizienz       | Viele Systeme arbeiten mit begrenzter Energieversorgung (Batterien, Netzteil), weshalb der Energieverbrauch eine zentrale Designrandbedingung darstellt                                                                                                                            |
| Spezialisierung        | Im Unterschied zu universellen Rechnern ist die Funktionalität eines eingebetteten Systems auf einen konkreten Anwendungsfall beschränkt                                                                                                                                           |

#### 2.1.2 Mikrocontroller als Plattform

Als dominierende Hardwareplattform eingebetteter Systeme dienen Mikrocontroller (MCU, engl. *Microcontroller Unit*). Ein Mikrocontroller integriert auf einem einzigen Halbleiterchip alle für einen vollständigen Computer wesentlichen Komponenten (STM32N6x7 - Documentation - STMicroelectronics, n.d.):

- **CPU-Kern:** Recheneinheit mit Instruktionssatz (z.\,B. Arm Cortex-M, RISC-V). Die Taktfrequenz bestimmt die Verarbeitungsgeschwindigkeit und reicht von wenigen MHz bis zu mehrerenhundert MHz
- **Programmspeicher (Flash):** Nichtflüchtiger Speicher für den Programmcode und konstante Daten. Die Größe variiert von einigen Kilobyte bis zu mehreren Megabyte.
- **Arbeitsspeicher (SRAM):** Flüchtiger Speicher für Laufzeitdaten, Variablen und Stack. Typisch sind wenige Kilobyte bis einigenhundert Kilobyte
- **Taktgeber:** Interner Oszillator (HSI, engl. *High-Speed Internal*) oder externer Quarz (HSE, engl. *High-Speed External*) als Zeitbasis für die CPU und Peripherie
- **Peripherie-Einheiten:** Hardwaremodule für Ein-/Ausgabe, Kommunikation und Zeitsteuerung (siehe Abschnitt 2.1.3)

#### 2.1.3 Peripherieschnittstellen

Mikrocontroller stellen verschiedene Hardware-Schnittstellen bereit, um mit der Umwelt zu interagieren. Die wesentlichen Peripherietypen sind (STM32N6x7 - Documentation - STMicroelectronics, n.d.):

**General Purpose Input/Output (GPIO):**
GPIO-Pins sind die einfachste Form der digitalen Ein-/Ausgabe. Jeder Pin kann konfiguriert werden als Eingang (Lesen eines logischen Pegels: 0 oder 1) oder als Ausgang (Setzen eines Pegels). GPIO-Pins werden zum Ansteuern von LEDs, Auslesen von Tasten oder als Steuerleitungen für andere Peripherie module verwendet.

**Universal Asynchronous Receiver/Transmitter (UART):**
Serielles Kommunikationsprotokoll zur asynchronen Datenübertragung. Es wird für Debug-Ausgaben, die Kommunikation mit Host-PCs oder serielle Sensoren eingesetzt. Ein UART-Kanal verwendet zwei Leitungen: TX (Senden) und RX (Empfangen).

**Serial Peripheral Interface (SPI):**
Synchrones, vollduplexes Kommunikationsprotokoll mit Master-Slave-Architektur. Es bietet höhere Datenraten als UART und wird bevorzugt für externe Speicher (Flash, PSRAM), Display-Controller oder Sensoren verwendet. Ein SPI-Bus besteht typischerweise aus den Leitungen MOSI, MISO, SCK und einem Chip-Select (CS).

**Inter-Integrated Circuit (I²C):**
Zweidrahtiges, synchrones Kommunikationsprotokoll (SDA, SCL) mit Adressierung. Es ermöglicht den Anschluss mehrerer Slaves über denselben Bus und wird für Sensoren, Echtzeituhren (RTC) und kleinere EEPROMs verwendet.

**Timer:**
Hardware-Timer zählen Taktimpulse und können interrupts auslösen, wenn ein Zählerwert erreicht wird. Sie dienen als Zeitbasis für periodische Aufgaben, zur Erzeugung von PWM-Signalen (Pulsweitenmodulation) und zur Messung von Signalen (Input Capture). In Echtzeitsystemen stellen Timer den Taktgeber für den Scheduler dar.

**Interrupts:**
Interrupts ermöglichen es der CPU, auf asynchrone Ereignisse (z.\,B. Eingangsimpuls, abgeschlossene Datenübertragung, Timer-Überlauf) zeitnah zu reagieren, ohne den Ereigniszeitpunkt aktiv abfragen zu müssen. Der Interrupt-Controller (NVIC, engl. *Nested Vectored Interrupt Controller* bei Arm Cortex-M) verwaltet Prioritäten und erlaubt Verschachtelung (Nested Interrupts). Nach dem Speichern des laufenden Kontexts (Registersatz) springt die CPU über die Vektortabelle auf die zugehörige Interrupt-Service-Routine (ISR) (Yiu, 2013).

**Direct Memory Access (DMA):**
DMA-Einheiten ermöglichen Datenübertragungen zwischen Peripherie und Speicher ohne CPU-Beteiligung. Die CPU gibt lediglich den Startbefehl und kann während der Übertragung andere Aufgaben verarbeiten. DMA ist besonders für große Datenströme (z.\,B. Kamerabilder) relevant, da es die CPU erheblich entlastet.

**DMA2D (ChromART):**
Eine spezielle DMA-Einheit für 2D-Blit-Operationen: Kopieren, Füllen und Alpha-Blending von Bilddaten. Im Kontext dieses Projekts wird DMA2D für die Skalierung und den Transfer von Kamerabildern in die Eingabepuffer der neuronalen Netze genutzt, ohne die CPU zu blockieren.

#### 2.1.4 Speicherarchitektur

Die Speicherarchitektur eingebetteter Systeme weicht wesentlich von klassischen Rechnern ab. Die wesentlichen Speichertypen sind (Wolf, 2012; Yiu, 2013):

- **Flash-Speicher (On-Chip):** Nichtflüchtig, integriert auf dem MCU-Chip. Dient als Programmspeicher (Code) und für konstante Daten (Konstanten-Tabelle, lookup tables). Schreibzugriffe sind langsam und erfordern einen Löschzyklus.
- **SRAM (On-Chip):** Flüchtig, sehr schneller Zugriff (im Takt der CPU). Dient als Hauptspeicher für Variablen, Heap und Stack. Auf Cortex-M85-Systemen ist SRAM in mehrere Banken aufgeteilt (AXISRAM), um parallele Zugriffe durch CPU, DMA und NPU zu ermöglichen.
- **PSRAM (extern):** Pseudo-Static RAM, angeschlossen über OctoSPI oder Quad-SPI. Bietet größere Speicherkapazitäten als On-Chip-SRAM bei moderater Zugriffszeit. Im STM32N6570-System wird PSRAM für Zwischenspeicherung von Bilddaten genutzt.
- **NOR-Flash (extern):** Nichtflüchtiger Speicher mit seitenweisem Zugriff, angeschlossen über OctoSPI. Im vorliegenden Projekt werden die vorkompilierten Modellbinaries (Palm Detection, Hand Landmark, Fingeralphabet) in externem NOR-Flash bei festen Adressen gespeichert.

Die Speicherhierarchie bestimmt maßgeblich die Systemleistung. Cortex-M85-Kerne verfügen über ein Cachesystem (I-Cache, D-Cache), das den Zugriff auf langsame externe Speicher beschleunigt. Da Peripherie-Einheiten (LTDC, DMA2D) jedoch häufig direkt auf den Speicher zugreifen, muss die Cache-Kohärenz manuell verwaltet werden (Flush/Invalidate) (STMicroelectronics, n.d.).

#### 2.1.5 Clock-Systeme

Das Clock-System bestimmt die Taktfrequenz aller Komponenten und ist eine fundamentale Voraussetzung für den Betrieb des Mikrocontrollers (STM32N6x7 - Documentation - STMicroelectronics, n.d.; Yiu, 2013)

- **Oszillatoren:** Der interne Hochgeschwindigkeitsoszillator (HSI) bietet eine integrierte, quarzfreie Takquelle (typisch 16-64 MHz). Externe Quarze (HSE) liefern eine höhere Genauigkeit und Stabilität.
- **PLL (Phase-Locked Loop):** Ein PLL multiplier die Taktfrequenz der Basisoszillatoren auf höhere Werte. Das STM32N6570-System verwendet mehrere PLLs: PLL1 für den CPU-Takt (800 MHz), PLL2 für den NPU (1000 MHz), PLL3 für den NPU-Speicher (900 MHz) und PLL4 für Peripherietakte.
- **Takthierarchie:** Der PLL-Ausgang wird über Teiler (Prescaler) auf verschiedene Busdomänen verteilt: AHB (High-Speed Bus), APB1/APB2 (Peripheral Buses). Peripherie-Einheiten erhalten ihren Takt von diesen Bussen.

#### 2.1.6 Bare-Metal-Programmierung

**Definition und Abgrenzung:**
Als Bare-Metal-Programmierung bezeichnet man die Softwareentwicklung auf einem Mikrocontroller ohne Einsatz eines Betriebssystems (OS) oder Echtzeitbetriebssystems (RTOS) (Mikrocontroller, n.d.). Der Programmcode hat direkten Zugriff auf die Hardwareregister, und die Ausführungsreihenfolge wird vollständig durch den eigenen Code bestimmt. Dies steht im Gegensatz zu OS-basierter Programmierung, bei der ein Betriebssystem (z.\,B. FreeRTOS, Zephyr) die Ressourcenverwaltung, Scheduling und Synchonisierung übernimmt.

**Startup und Systeminitialisierung:**
Nach dem Einschalten oder Reset beginnt die CPU an einer durch den Vektor-Tabelle definierten Adresse (Reset-Handler). Der Startup-Code führt folgende Schritte aus: (1) Kopieren der Initialisierungsdaten aus Flash nach SRAM (.data-Sektion), (2) Löschen der .bss-Sektion (uninitialisierte globale Variablen), (3) Konfiguration des Stack-Pointers, (4) Aufruf der main-Funktion. Der Vektor-Tabelle enthält außerdem Adressen aller Interrupt-Handler (Exceptions)

**Register-Level-Programmierung:**
Im Bare-Metal-Ansatz werden Peripherie-Einheiten über Memory-Mapped I/O-Register konfiguriert. Jedes Register hat eine fest definierte Adresse im Speicherbereich. Die Konfiguration erfolgt durch Setzen oder Löschen einzelner Bits mit Bitmasken. Beispielsweise wird ein GPIO-Pin als Ausgang konfiguriert, indem im Mode-Register die entsprechenden Bits gesetzt werden. Der Vorteil gegenüber einer Hardware-Abstraktionsschicht (HAL) liegt in der vollen Kontrolle über die Ausführungszeit und den Speicherverbrauch.

**Vorteile und Trade-Offs:**
Bare-Metal bietet minimale Latenz, minimalen Speicherverbrauch und deterministisches Verhalten. Die Nachteile liegen in der höheren Entwicklungskomplexität: Es gibt kein Thread-Management, keine Synchronisationsprimitive (Mutexe, Semaphore) und keine automatische Ressourcenverwaltung. Für ressourcenbeschränkte Systeme mit klar definiertem Ablauf überwiegen jedoch die Vorteile (Mikrocontroller, n.d.).

#### 2.1.7 Hardware-Beschleunigung

Neben der CPU stehen dedizierte Hardware-Einheiten zur Verfügung, die bestimmte Operationen mit hoher Effizienz ausführen:

- **DMA:** Entlastet die CPU bei Datenübertragungen (siehe Abschnitt 2.1.3).
- **DMA2D:** Übernimmt 2D-Bildoperationen (Kopieren, Skalieren, Füllen) ohne CPU-Beteiligung.
- **NPU (Neural Processing Unit):** Dedizierter Beschleuniger für neuronale Netze. Der NPU führt Matrix-Multiplikationen und Aktivierungsfunktionen mit hoher Parallelität aus und erreicht dabei deutlich höhere Energieeffizienz als die allgemeine CPU. Im STM32N6570-DK arbeitet der NPU mit einer Taktfrequenz von 1000 MHz und nativer INT8-Arithmetik (siehe Abschnitt 2.7). (STMicroelectronics, n.d.)

#### 2.1.8 STM32N6570-DK als Zielplattform

Das vorliegende Projekt nutzt den STM32N6570 Discovery Kit als Zielplattform. Die wesentlichen technischen Daten sind:

| Komponente        | Spezifikation                                                                                      |
| ----------------- | -------------------------------------------------------------------------------------------------- |
| CPU               | Arm Cortex-M85 @ 800 MHz (PLL1, HSI 64 MHz)                                                        |
| NPU               | Neural Processing Unit @ 1000 MHz (PLL2)                                                           |
| NPU-Speicher      | AXISRAM3-6 @ 900 MHz (PLL3), 4 SRAM-Banken                                                         |
| Kamera            | 5 MP Sensor (IMX335)<br>CSI-2 @ 20 MHz<br>DCMIPP (Digital Camera Memory Interface Pixel Processor) |
| Display           | LTDC (LCD-TFT Display Controller) @ 25 MHz                                                         |
| Externer Speicher | PSRAM<br>NOR-Flash via OctoSPI @ 200 MHz                                                           |
| APB-Peripherie    | Alle bei 200 MHz (HCLK = AXI/2)                                                                    |

Der Cortex-M85-Kern ist der leistungsstärkste Armv8.1-M-Prozessor und bietet unter anderem TrustZone-Sicherheit, Helium (M-Profile Vector Extension) und erweiterte Debug-Funktionen. Die Kombination aus leistungsstarker CPU, dediziertem NPU und umfangreicher Peripherie macht die Plattform geeignet für kamerabasierte KI-Anwendungen auf dem Embedded-Gerät. (STMicroelectronics, n.d.)

---

### 2.2 Deutsches Einhand-Fingeralphabet

#### 2.2.1 Gebärdensprache als eigenständige Sprache

Gebärdensprachen sind natürlich entstandene, vollwertige Sprachen, die auf visuell-manuellen Mitteln basieren (Deutscher Gehörlosen-Bund e.V., n.d.). Sie unterscheiden sich von Lautsprachen nicht nur in der Modaliät (Gestik und Mimik statt Laut und Klang), sondern verfügen über eigene grammatische Strukturen, Syntax und Semantik. Gebärdensprachen sind eigenständige Sprachen und keine abgeleiteten oder vereinfachten Formen der jeweiligen Lautsprache.

Die Deutsche Gebärdensprache (DGS) ist die natürlich entstandene Sprache der Gehörlosengemeinschaft in Deutschland. Sie wird vor allem von gehörlosen und schwerhörigen Menschen sowie von deren Angehörigen und Gebärdensprachdolmetschern verwendet. Die DGS unterscheidet sich wesentlich von der deutschen Lautsprache in Grammatik, Satzbau und Ausdruck. Gebärdensprachliche Äußerungen bestehen aus einer Kombination von Handformen, Handbewegungen, Handpositionen im Raum sowie Mimik und Körpersprache. (Deutscher Gehörlosen-Bund e.V., n.d.)

#### 2.2.2 Das Fingeralphabet

Das Fingeralphabet ist ein Teilgebiet der Deutschen Gebärdensprache (AktionMensch e.V., n.d.; Deutscher Gehörlosen-Bund e.V., n.d.). Es dient dem Buchstabieren einzelner Wörter und wird verwendet, wenn für ein Wort keine eigene Gebärde existiert. Etwa bei Eigennamen, Fachbegriffen oder bei der Kommunikation mit hörenden Gesprächspartnern. Im Gegensatz zur vollständigen Gebärdensprache beschränkt sich das Fingeralphabet auf die Darstellung von Handformen ohne Begleitung durch Mimik oder Bewegung im Raum.

#### 2.2.3 Das deutsche Einhand-Fingeralphabet

Das deutsche Einhand-Fingeralphabet umfasst 27 Zeichen (AktionMensch e.V., n.d.):

- **26 Buchstaben:** A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z, Ä, Ö, Ü
- **1 Sonderzeichen:** SCH (als Trigraph dargestellt, d.\,h. ein Zeichen für drei Buchstaben)

![[aktion-mensch-deutsches-fingeralphabet.jpg]]

#### 2.2.4 Handpositionen und Fingerkonfigurationen

Jedes Zeichen des Fingeralphabets wird durch eine spezifische Konfiguration der Hand charakterisiert, die sich aus mehreren Parametern zusammensetzt (AktionMensch e.V., n.d.):

- Beteiligte Finger: Welche Finger sind gestreckt, welche sind gebeugt oder angeballt?
- Fingerkonfiguration: Sind Finger together (zusammengedrückt) oder gespreizt?
- Daumenposition: Liegt der Daumen über den Fingern, neben ihnen, oder ist er abgespreizt?
- Handfläche: Zeigt die Handfläche zum Betrachter (ventral) oder weg (dorsal)?
- Fingerstellung: Zeigen die Finger nach oben, unten oder zur Seite?

Beispielhafte Beschreibung einiger Zeichen:

| Zeichen | Handkonfiguration                                                                       |
| ------- | --------------------------------------------------------------------------------------- |
| **A**   | Faust geschlossen, Daumen liegt seitlich an der Handfläche an                           |
| **B**   | Vier Finger gestreckt und zusammen, Daumen auf die Handfläche geklappt                  |
| **C**   | Alle Finger gebeugt, Hand formt eine C-förmige Öffnung                                  |
| **E**   | Alle Finger auf den Daumen geklappt, Daumen sichtbar unter den Fingerspitzen            |
| **I**   | Nur kleiner Finger gestreckt, übrige Finger geballt                                     |
| **L**   | Daumen und Zeigefinger im 90°-Winkel gestreckt, übrige Finger geballt                   |
| **S**   | Faust geschlossen, Daumen vor den Fingern gekreuzt                                      |
| **Y**   | Daumen und kleiner Finger gestreckt, übrige Finger geballt                              |
| **SCH** | Drei Finger (Zeige-, Mittel-, Ringfinger) gestreckt und leicht gebeugt, Daumen darunter |

---

### 2.3 Grundlagen des maschinellen Lernens

#### 2.3.1 Begriffsbildung

Maschinelles Lernen (ML, engl. *machine learning*) ist ein Teilgebiet der künstlichen Intelligenz, das Systeme befähigt, aus Daten zu lernen und Vorhersagen oder Entscheidungen zu treffen, ohne explizit programmiert zu werden (Pattern Recognition and Machine Learning, n.d.). Anstelle fester Regeln wird dem Algorithmus ein Trainingsdataset zur Verfügung gestellt, aus dem er statistische Muster und Zusammenhänge ableitet.

#### 2.3.2 Supervised Learning

Im "überwachten Lernen" (supervised learning) wird dem Modell ein Datensatz bestehend aus Eingabedaten (Features) und zugehörigen korrekten Ausgaben (Labels) präsentiert (Hastie et al., 2009; Pattern Recognition and Machine Learning, n.d.). Das Modell lernt, eine Funktion $f: X \rightarrow Y$ zu approximieren, die Eingaben auf die zugehörigen Ausgaben abbildet.

#### 2.3.3 Klassifikation

Klassifikation ist ein zentrales Teilgebiet des maschinellen Lernens, bei dem die Aufgabe besteht, eine Eingabe einer von diskreten Klassenzugehörigkeiten zuzuordnen. Das Modell produziert für jede Klasse einen Wahrscheinlichkeitswert, und die Klasse mit der höchsten Wahrscheinlichkeit wird als Vorhersage ausgegeben. Typischerweise werden die Klassen innerhalb des Datensatzes mit Hilfe des One-Hot-Formats dargestellt (Hastie et al., 2009)

##### Sparse Categorical Crossentropy

Das One-Hot-Format verbraucht bei Hunderten von Klassen viel Speicherplatz. Sparse categorical Crossentropy löst dieses Problem indem der Datensatz einen einzigen Integer-Wert annimmt und die mathematisch äquivalente Kreuzentropie-Berechnung Speicher effizient im Hintergrund durchführt. (Sparse Categorical Crossentropy vs. Categorical Crossentropy, 18:23:14+00:00)

| Kategorie | Index | One-Hot-Vektor   |
| --------- | ----- | ---------------- |
| NONE      | 0     | \[1, 0, 0, 0, 0] |
| A         | 1     | \[0, 1, 0, 0, 0] |
| B         | 2     | \[0, 0, 1, 0, 0] |
| C         | 3     | \[0, 0, 0, 1, 0] |
| D         | 4     | \[0, 0, 0, 0, 1] |

#### 2.3.4 Metriken

Zur Bewertung eines Klassifikationsmodells werden verschiedene Metriken herangezogen (Hastie et al., 2009):

- Accuracy (Genauigkeit): Anteil der korrekten Vorhersagen an allen Vorhersagen. Für ausgewogene Datensätze aussagekräftig, bei unausgewogenen Klassen jedoch irreführend.
- Confusion Matrix: Tabelle, die die tatsächlichen Klassen den vorhergesagten Klassen gegenüberstellt. Sie macht sichtbar, welche Zeichen miteinander verwechselt werden.

|              | Predicted 0    | Predicted 1    |
| ------------ | -------------- | -------------- |
| **Actual 0** | True Negative  | False Positive |
| **Actual 1** | False Negative | True Positive  |

#### 2.3.5 Overfitting und Gegenmaßnahmen

Overfitting liegt vor, wenn ein Modell die Trainingsdaten zu gut lernt und dabei spezifische Muster, Rauschen und Ausreißer mitinterpretiert, diese allerdings nicht für die Allgemeingültigkeit relevant sind (Pattern Recognition and Machine Learning, n.d.). Das Modell performt auf Trainingsdaten exzellent (Hohe Genauigkeit), auf unbekannten Daten jedoch schlecht (Geringe Genauigkeit).

Gegenmaßnahmen bezüglich Overfitting umfassen:
- Regularisierung: Bestrafung großer Gewichtswerte, um die Modellkomplexität zu begrenzen.
- Dropout: Zufälliges Deaktivieren eines Teils der Neuronen während des Trainings, um Abhängigkeiten zwischen Neuronen zu reduzieren.
- Datenaugmentation: Künstliche Vermehrung der Trainingsdaten durch Transformationen (siehe Abschnitt 2.6).
- Validierung: Aufteilen des Datensatzes in Trainings-, Validierungs- und Testset, um die Generalisierungsfähigkeit zu überwachen.

---

### 2.4 Neuronale Netze zur Klassifikation

#### 2.4.1 Künstliche Neuronale Netze

Ein künstliches neuronales Netz (KNN) ist ein mathematisches Modell, das lose von biologischen neuronalen Netzen inspiriert ist (Kim, 2016; Raschka et al., 2022). Es besteht aus einer Menge von künstlichen Neuronen (auch *Einheiten* oder *Knoten* genannt), die in Schichten (engl. *layers*) organisiert sind. Jedes Neuron berechnet eine gewichtete Summe seiner Eingaben, addiert einen Bias-Wert und wendet eine Aktivierungsfunktion an:

![[Neuron.drawio.png]]
$$y = f\left(\sum_{i=1}^{n} w_i \cdot x_i + b\right)$$
wobei $x_i$ die Eingaben, $w_i$ die Gewichte, $b$ der Bias und $f$ die Aktivierungsfunktion sind.
\[Mathematisch modelliertes Neuron (Gurney, 2018) | Eigene Darstellung]

#### 2.4.2 Aktivierungsfunktionen

Aktivierungsfunktionen fügen Nichtlinearität in das Netz ein und ermöglichen es, komplexe Muster zu erlernen. Die wesentlichen im Projekt verwendeten Funktionen sind (Kim, 2016):

- ReLU (Rectified Linear Unit): $f(x) = \max(0, x)$. Einfach zu berechnen, gradientenfreundlich und in versteckten Schichten Standard. Negative Eingaben werden auf 0 gesetzt.
- Softmax: Wandelt einen Vektor von Rohergebnissen (logits) in eine Wahrscheinlichkeitsverteilung um. Für die Ausgabeschicht eines Klassifikators geeignet, da die Summe aller Ausgabewerte 1 ergibt und jeder Wert zwischen 0 und 1 liegt.

#### 2.4.3 Architektur: Multi-Layer Perceptron (MLP)

Ein Multi-Layer Perceptron (MLP) ist ein Neuronales Netz mit mindestens einer versteckten Schicht (Kim, 2016). Die Daten durchlaufen das Netz in eine Richtung, von der Eingabeschicht über die versteckten Schichten zur Ausgabeschicht, ohne Rückkopplungen. 

![[ML - Schichten.drawio.png]]
\[(Kim, 2016) | Eigene Darstellung]
#### 2.4.4 Backpropagation und Gradient Descent

Das Training neuronaler Netze erfolgt mittels *Backpropagation* (Rückpropagierung des Fehlers) in Kombination mit einem Optimierungsalgorithmus wie *Stochastic Gradient Descent* (SGD) oder Varianten davon (Adam, RMSprop) (Kim, 2016; Raschka et al., 2022):

1. Forward Pass: Die Eingabedaten durchlaufen das Netz, und die Vorhersage wird berechnet.
2. Loss-Berechnung: Die Abweichung zwischen Vorhersage und tatsächlichem Label wird mittels einer Loss-Funktion (z.\,B. Categorical Cross-Entropy für Multi-Class-Klassifikation) quantifiziert.
3. Backward Pass: Die Ableitungen des Loss nach den Gewichten werden über die Schichten zurückgerechnet (Kettenregel der Differentiation).
4. Gewichts-Update: Die Gewichte werden in Richtung des negativen Gradienten angepasst, um den Loss zu minimieren.

Dieser Zyklus wird über mehrere Epochen (Durchläufe durch den gesamten Datensatz) wiederholt, bis das Modell eine zufriedenstellende Güte erreicht.

---

### 2.5 Handdetektion und Handlandmarks

#### 2.5.1 Zweistufige Erkennungspipeline

Die Erkennung von Handzeichen in Kamerabildern erfordert eine mehrstufige Verarbeitung. Der gängigste Ansatz, der auch im vorliegenden Projekt verwendet wird, basiert auf einer zweistufigen Pipeline (Lugaresi et al., n.d.; Zhang & Notni, 2025)

1. Stufe 1 - Palm Detection (Handflächen-Erkennung): Im gesamten Kamerabild wird die Position einer oder mehrerer Hände lokalisiert. Das Ergebnis ist eine Bounding Box (Umrahmung) um die erkannte Handfläche.
2. Stufe 2 - Hand Landmark-Erkennung: Innerhalb der erkannten Handfläche werden 21 charakteristische Landmark-Punkte bestimmt, die die Position der Finger-Gelenke und Fingerspitzen beschreiben.

Diese Zweiteilung ist aus Effizienzgründen sinnvoll: Die Palm Detection arbeitet auf einem downgesampelten Bild (192×192 Pixel), während die Landmark-Erkennung auf dem zugeschnittenen Hand-Ausschnitt (224×224 Pixel) arbeitet. Dadurch wird die Rechenlast erheblich reduziert.

#### 2.5.2 Palm Detection

Die Palm Detection identifiziert die Position der Handfläche im Kamerabild (Lugaresi et al., n.d.). Das Modell (basierend auf MediaPipe) nimmt ein RGB-Bild der Größe 192×192 Pixel als Eingabe und erzeugt als Ausgabe eine Menge von Kandidaten. Jeder Kandidat besteht aus:

- Score: Ein Vertrauenswert (0 bis 1), der angibt, wie wahrscheinlich es sich um eine Handfläche handelt.
- Regression: Vier Werte, die die Bounding Box (x, y, Breite, Höhe) der Handfläche definieren.

Da das Modell pro Bild 2016 Kandidaten ausgibt, wird eine Non-Maximum Suppression (NMS) durchgeführt: Überlappende Bounding Boxes werden zusammengefasst, und nur der Kandidat mit dem höchsten Score in jedem Cluster wird beibehalten. Als Schwellenwerte dienen ein Confidence-Threshold von 0,3 und ein IoU-Threshold (Intersection over Union) von 0,4.

#### 2.5.3 Handlandmarks

Die Handlandmark-Erkennung nimmt den durch die Palm Detection definierten Bildausschnitt (ROI, engl. *Region of Interest*) und bestimmt darin 21 Landmark-Punkte (Zhang & Notni, 2025). Jeder Punkt wird durch seine relative Position (x, y) innerhalb der ROI beschrieben. Die 21 Landmarks entsprechen den anatomisch relevanten Punkten der Hand:

| Index | Beschreibung                             |
| ----- | ---------------------------------------- |
| 0     | Handgelenk (Wrist)                       |
| 1-4   | Daumen: MCP, IP, Tip (3 Punkte)          |
| 5-8   | Zeigefinger: PIP, DIP, Tip (3 Punkte)    |
| 9-12  | Mittelfinger: PIP, DIP, Tip (3 Punkte)   |
| 13-16 | Ringfinger: PIP, DIP, Tip (3 Punkte)     |
| 17-20 | Kleiner Finger: PIP, DIP, Tip (3 Punkte) |
\[MCP = Metacarpophalangeal Joint, IP = Interphalangeal Joint, PIP = Proximal Interphalangeal Joint, DIP = Distal Interphalangeal Joint, Tip = Fingerspitze]

![[Mediapipe - Hand Points.png]]
\[Abbildung x: Beispiel Hand Landmark Punkte| Eigene Darstellung]

Zusätzlich gibt das Modell einen Handedness-Score aus, der angibt, ob es sich um eine linke oder rechte Hand handelt.
#### 2.5.4 ROI

Eine ROI (Region of Interrest) ist ein rotiertes Rechteck im Pixel Raum, der genau definiert, welchen Bildausschnitt ein Modell fokussieren oder als Eingabe für ML Modelle dienen soll.
Dies kann verwendet werden um Modelle selektiv/gezielt auszuführen.

```
┌───────────────────────────────────┐
│         Original Frame            │
│      ┌────────────────────┐       │
│      │   ROI (rotated)    │       │
│      │   ┌──────────┐     │       │
│      │   │ landmark │     │       │
│      │   │  crop    │     │       │
│      │   └──────────┘     │       │
│      │                    │       │
│      └────────────────────┘       │
└───────────────────────────────────┘
```
\[ROI - Visuelle Repräsentation| Eigene Darstellung] NOTE: Ersetze durch draw.io darstellung

##### 2.5.4.1 Lifecycle

Der Lifecycle einer ROI ist in Abbildung X dargestellt. Eine ROI entsteht durch die Ausgabe des Detector-Modells, das eine Bounding Box sowie Schlüsselpunkte liefert. Aus diesen werden Position, Orientierung und größe der initialen ROI berechnet. in den folgenden Frames wird innerhalb der ROI das Landmark-Modell ausgeführt. bei ausreichender Konfidenz werden Landmarken in Bildkoordinaten zurückgerechnet und daraus eine aktualisierte ROI bestimmt. Dieser Vorgang wiederhohlt sich für jeden Frame. Fällt die Konfidenz unter den definierten Schwellwert, wird die ROI verworfen und der Lifecycle endet.

---

### 2.6 Datenaugmentation und Modellquantisierung

#### 2.6.1 Datenaugmentation

**Definition und Ziel:**
Datenaugmentation bezeichnet die künstliche Vermehrung eines Trainingsdatensatzes durch Transformationen der vorhandenen Daten (Pattern Recognition and Machine Learning, n.d.). Der Zweck besteht darin, die Robustheit und Generalisierungsfähigkeit des Modells zu erhöhen, ohne additional Daten erfassen zu müssen. Für eingebettete Systeme mit begrenztem Speicher ist eine effiziente Datennutzung besonders relevant.

**Transformationsverfahren:**
Im vorliegenden Projekt werden folgende Augmentationen eingesetzt:

- Spiegelung (Horizontal Flip): Die Hand wird horizontal gespiegelt. Dies simulierte Unterschiede zwischen linker und rechter Hand sowie Variationen in der Handhaltung.
- Zufällige Translation: Die Landmarks werden um einen zufälligen Betrag in x- und y-Richtung verschoben, um unterschiedliche Handpositionen im Kamerabild zu simulieren.
- Zufälliger Zoom: Die Landmarks werden um einen Faktor zwischen 0,5× und 1,5× skaliert, um unterschiedliche Handgrößen und Abstände zur Kamera abzubilden.
- Jitter: Zufälliges Rauschen wird zu den Koordinaten hinzugefügt, um natürliche Schwankungen in der Landmark-Erkennung zu simulieren.

**Implementierung:**
Die Augmentationspipeline (`AugmentationPipeline`) wendet diese Transformationen sequenziell auf die extrahierten Landmarks an und erzeugt pro Original-Datensatz mehrere Variationen. Dadurch wird der effektive Datensatz um einen Faktor von typisch 5-10× vermehrt.

#### 2.6.2 Modellquantisierung

**Warum Quantisierung?**
Neuronale Netze verwenden bei Training und Inferenz üblicherweise Fließkommazahlen (Float32, 4 Byte pro Wert). Auf ressourcenbeschränkten embedded Plattformen ist dies sowohl speichermäßig als auch rechnerisch ineffizient (Jacob et al., 2018). Die Quantisierung reduziert die Genauigkeit der Gewichte und Aktivierungen auf Ganzzahlen (typisch INT8, 1 Byte pro Wert).

**Float32 vs. INT8:**

| Eigenschaft | Float32 | INT8 |
|---|---|---|
| Speicher pro Gewicht | 4 Byte | 1 Byte |
| Speicherersparnis | - | 75 % |
| Rechengeschwindigkeit | Normativ | Deutlich schneller (NPU-nativ) |
| Genauigkeit | Hoch | Leicht reduziert (akzeptabel) |

**Scale und Zero-Point:**
Die Quantisierung bildet den Float32-Wertebereich auf einen Integer-Bereich ab. Für die affine Quantisierung gilt (Post-Training Quantization | TensorFlow Model Optimization, n.d.):

$$q = \text{round}\left(\frac{r}{s}\right) + z$$

wobei $r$ den realen Float-Wert, $s$ den Skalierungsfaktor, $z$ den Zero-Point und $q$ den quantisierten Integer-Wert darstellt. Die Rückrechnung erfolgt über:

$$r = s \cdot (q - z)$$

**Post-Training-Quantisierung (PTQ):**
Bei der Post-Training-Quantisierung wird ein bereits trainiertes Float32-Modell in ein INT8-Modell konvertiert, ohne erneutes Training (Post-Training Quantization | TensorFlow Model Optimization, n.d.). Dafür wird ein Representative Dataset (Repräsentatives Datenset) verwendet. Es handelt sich um eine Stichprobe der Trainingsdaten, anhand derer die Aktivierungsbereiche der Schichten analysiert und die optimalen Skalierungsfaktoren bestimmt werden.

**Genauigkeitsverluste:**
Die Reduktion von Float32 auf INT8 kann zu einem leichten Rückgang der Modellgenauigkeit führen. In der Praxis ist dieser Verlust bei geeigneter Kalibrierung (Representative Dataset) jedoch gering (typisch < 2 % Accuracy-Verlust) und für die meisten Anwendungsfälle akzeptabel.

---

### 2.7 Edge AI und neuronale Beschleuniger

#### 2.7.1 Edge AI

Edge AI bezeichnet die Ausführung von KI-Inferenzen direkt auf dem Endgerät (engl. *edge device*) anstelle einer Übertragung der Daten an einen Cloud-Server (Jain, 2023). Die wesentlichen Vorteile gegenüber Cloud-basierten Ansätzen sind:

- Geringe Latenz: Die Verarbeitung erfolgt lokal, ohne Netzwerkübertragungszeit. Für Echtzeitanwendungen wie die Gesture-Erkennung ist dies essenziell.
- Datenschutz: Biometrische Daten (Kamerabilder der Hand) verlassen das Gerät nicht.
- Offline-Fähigkeit: Das System funktioniert ohne Internetverbindung.
- Reduzierte Bandbreite: Es müssen keine Bilder übertragen werden.

Die Haupt-Herausforderung liegt in der Ressourcenbeschränkung: Speicher, Rechenleistung und Energieverbrauch sind auf embedded Plattformen stark limitiert. Ein KI-Modell, das auf einem Server mit Gigabytes an Speicher und leistungsstarken GPUs trainiert wurde, muss auf dem Mikrocontroller mit wenigen Kilobytes an SRAM und einem dedizierten Beschleuniger betrieben werden (Abadade et al., 2023).

#### 2.7.2 Neuronale Beschleuniger (NPU)

Eine Neural Processing Unit (NPU) ist eine dedizierte Hardware-Einheit, die speziell für die Ausführung neuronaler Netze optimiert ist (Passold & da Silva, 2025; STMicroelectronics, n.d.). Im Gegensatz zur allgemeinen CPU, die nur einen Bruchteil ihrer Rechenleistung für Matrix-Operationen nutzt, führt die NPU Matrix-Multiplikationen und Vektoroperationen mit hoher Parallelität aus.

Die NPU arbeitet mit einer eigenen Speicherhierarchie: Die AXISRAM-Banken dienen als Puffer für Gewichte und Aktivierungen, sodass die NPU unabhängig von der CPU auf Daten zugreifen kann. Die Modellbinaries werden aus dem externen NOR-Flash geladen und beim Systemstart in den AXISRAM kopiert.