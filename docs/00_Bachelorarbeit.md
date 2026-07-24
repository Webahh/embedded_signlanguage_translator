
## Titelbild hier einfügen??

## Abstract hier einfügen??

## Unterschriften hier einfügen??

## Gliederung

1. [[01_Einleitung | Einleitung]]
   - 1.1 Problemstellung
   - 1.2 Motivation und Zielsetzung
   - 1.3 Abgrenzung des Untersuchungsgegenstandes
	   - 1.3.1 Inhaltliche Abgrenzung
	   - 1.3.2 Personenspezifische Abgrenzung
   - 1.4 Aufbau der Arbeit 
1. [[02_Theoretische und technische Grundlagen| Theoretische und technische Grundlagen]]
   - 2.1 Eingebettete Systeme
   - 2.2 Deutsches Fingeralphabet
   - 2.3 Grundlagen des maschinellen Lernens
   - 2.4 Neuronale Netze zur Klassifikation
   - 2.5 Handdetection und Handlandmarks
   - 2.6 Datenaugmentation und Modellquantisierung
   - 2.7 Edge AI und neuronale Beschleuniger
1. [[03_Stand_der_Technik | Stand der Technik]]
   - 3.1 Bestehende Ansätze zur Erkennung von Handzeichen
   - 3.2 Kamerabasierte Fingeralphabet- und Gebärdenerkennung
   - 3.3 Aktuelle Embedded-AI-Lösungen
   - 3.4 Einordnung der Arbeit und Abgrenzung der eigenen Arbeit 
1. [[04_Methodik_und_Entwicklungsprozess | Methodik und Entwicklungsprozess]]
   - 4.1 Vorgehensweise bei der Literatur- und Technologierecherche
   - 4.2 Iterativer Entwicklungs- und Integrationsprozess
   - 4.3 Vorgehen bei der Modellentwicklung
   - 4.4 Verifikations- und Evaluationskonzept
1. [[05_Anforderungsanalyse | Anforderungsanalyse]]
   - 5.1 Anwendungsszenario
   - 5.2 Anforderungen und Randbedingungen
	   - 5.2.1 Funktionale Anforderungen
	   - 5.2.2 Nichtfunktionale Anforderungen
	   - 5.2.3 Technische Anforderungen
	   - 5.2.4 Randbedingungen
   - 5.3 Abnahmekriterien
1. [[06_Systementwurf | Systementwurf]]
   - 6.1 Systemkonzept
   - 6.2 Hardwarekonzept
   - 6.3 KI-Verarbeitungskette
   - 6.4 Speicher- und Datenflusskonzept
   - 6.5 Ablauf vom Kamerabild bis zum Erkennungsergebnis
1. [[07_Entwicklung_und_Implementierung | Entwicklung und Implementierung]]
   - 7.1 Aufbau und Aufbereitung des Datensatzes
	   - 7.1.1 Rohvideos
	   - 7.1.2 Frame-Extraktion und Handdetektion
	   - 7.1.3 Augmentation
	   - 7.1.4 Normalisierung und Interpolation
	   - 7.1.5 Datenstrukturen und Speicherung
   - 7.2 Entwicklung und Training des Klassifikationsmodells
	   - 7.2.1 Problemdefinition
	   - 7.2.2 Datenanalyse
	   - 7.2.3 Modellauswahl
	   - 7.2.4 Modellarchitektur
	   - 7.2.5 Verlustfunktion und Optimierung
	   - 7.2.6 Trainingprozess
   - 7.3 Quantisierung und Konvertierung des Modells
   - 7.4 Softwaregrundstruktur und hardwarenahe Basistreiber
	   - 7.4.1 Aufbauder Embedded-Software
	   - 7.4.2 Zentrale Konfiguration und Systeminitialisierung
	   - 7.4.3 Registerbasierte Treiberentwicklung
	   - 7.4.4 Grundlegende Systemtreiber
   - 7.5 Kamera - Display Pipeline
	   - 7.5.1 Initialisierung des Kamerasensors
	   - 7.5.2 Übertragung der Kamerdaten über CSI & DCMIPP
	   - 7.5.3 Verwaltung der Bildpuffer
	   - 7.5.4 LTDC-Konfiguration und Verwaltung der Displayebenen
   - 7.6 Ablaufsteuerung und Software Scheduler
   - 7.7 Neural Processing Unit und AI-Interface
	   - 7.7.1 Initialisierung des Neural-ART Accelerators
	   - 7.7.2 Einheitliche KI-Schnittstelle
   - 7.8 Vor- und Nachverarbeitungsschritte
	   - 7.8.1 DCMIPP-gestützte Bildvorverarbeitung
	   - 7.8.2 Nachverarbeitung der Handdetektion
	   - 7.8.3 Vorberarbeitung der Landmark-Erkennung
	   - 7.8.4 Nachverarbeitung der Landmark-Erkennung
	   - 7.8.5 Vorberarbeitung des Klassifizierungsmodells
	   - 7.8.6 Nachverarbeitung des Klassifikationsmodells
   - 7.9 Visualisierung und Benutzerausgabe
	   - 7.9.1 Darstellung der Erkennungsergebnisse
	   - 7.9.2 Benutzeroberfläche und Bedienung
   - 7.10 Optimierungen
1. [[08_Evaluation_und_Validierung | Evaluation und Validierung]]
   - 9.1  Versuchsaufbau und Testbedingungen
   - 9.2 Erkennungsgenauigkeit
   - 9.3 Konfusionsmatrix und klassenbezogene Ergebnisse 
   - 9.4 Einfluss von Beleuchtung und Hintergrund
   - 9.5 Inferenzlatenz und Ende-zu-Ende-Latenz
   - 9.6 Bildrate und Datendurchsatz
   - 9.7 Speicher- und Ressourcenverbrauch
   - 9.8 Abgleich mit den definierten Anforderungen
1. [[09_Diskussion | Diskussion]]
   - 10.1 Interpretation der Ergebnisse
   - 10.2 Bewertung des entwickelten Gesamtsystems 
   - 10.3 Grenzen und bekannte Einschränkungen
   - 10.4 Technischer und praktischer Beitrag der Arbeit
1. [[10_Fazit_und_Ausblick | Fazit und Ausblick]]
   - 11.1 Zusammenfassung und Zielerreichung
   - 11.2 Ausblick
1. [[11_Literaturverzeichnis | Literaturverzeichnis]]


## Abbildungsverzeichnis
## Tabellenverzeichnis
## Abkürzungsverzeichnis

AI / KI Artifical Intelligence / Künstliche Intelligenz
CPU Central Processing Unit
FPU Floating Point Processing Unit
NPU Neural Processing Unit
GPIO General Purpose Input/Output
UART / USART Universal Synchronous/Asynchronous Receiver/Transmitter
I²C / I2C Inter-Integrated Circuit
DMA Direct Memory Access
MCU Microcontroller
RAM Random-Access-Memory
SRAM Static Random-Access-Memory
PSRAM Pseudo-Static Random-Access-Memory
NOR-Flash Not OR-Flash



