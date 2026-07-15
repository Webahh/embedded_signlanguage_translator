
## 1. Einleitung 

Das deutsche Fingeralphabet ermöglicht es, einzelne Buchstaben durch festgelegte Handformen darzustellen. Es wird unter anderem verwendet, um Namen, Fachbegriffe oder Wörter zu bachstabieren, für die keine gebräuchliche Gebärde vorhanden ist. Für Personen, die das Fingeralphabet erlernen, ist eine unmittelbare Rückmeldung zur ausgeführten Handform hilfreich. Ein kamerabasiertes Trainingssystem kann die gezeigten Zeichen automatisch erkennen und dadurch das selbstständige Üben unterstützen.

Im Rahmen dieser Arbeit wird die technische Grundlage für einen solchen Fingeralphabet-Trainer entwickelt. Das System erfasst die Hand über eine Kamera, bestimmt deren Position und charakteristische Landmarks und klassifiziert anschließend das dargestellte Zeichen. Die gesamte Verarbeitung soll lokal auf einem eingebetteten System erfolgen. Als Zielplattform wird ein STM32N6570-Mikrocontroller verwendet, der neben den üblichen Mikrocontroller-Funktionen über einen integrierten Beschleuniger für neuronale Netze verfügt.

...

### 1.1 Problemstellung

Die Ausführung einer vollständigen kamerabasierten Erkennungskette auf einem Mikrocontroller ist aufgrund der beschränkten Ressourcen anspruchsvoll Neben den neuronalen Netzen müssen auch die Kameraschnittstelle, die Bildspeicherung, die Vor- und Nachverarbeitung der Daten, die Ablaufsteuerung sowie die grafische Ausgabe berücksichtigt werden. Werden einzelne Verarbeitungsschritte blockierend oder ineffizient umgesetzt, können hohe Latenzen, eine geringe Bildrate oder instabile Erkennungsergebnisse entstehen.

...

Die zentrale Problemstellung dieser Arbeit besteht darin, eine mehrstufige Verarbeitungskette zur Erkennung des deutschen Fingeralphabets so auf einem ressourcenbeschränkten eingebetteten  System umzusetzen, dass eine hinreichen genaue, stabile und echtzeitfähige Verarbeitung erreicht wird.

### 1.2 Motivation und Zielsetzung



### 1.3 Abgrenzung der Arbeit



### 1.4 Aufbau der Arbeit

Nach der Einleitung werden in Kapitel 2 die theoretischen und technischen Grundlagen erläutert. Dazu gehören eingebettete Systeme, das deutsche Fingeralphabet, grundlegende Verfahren des maschinellen Lernens, neuronale Netze zur Klassifikation, Handdetektion und Handlandmarken sowie Datenaugmentation, Modellquantisierung und Edge AI.

Kapitel 3 stellt den Stand der Technik dar. Es werden bestehende Ansätze zur Erkennung von Handzeichen und Fingeralphabeten sowie aktuelle Embedded-AI-Lösungen betrachtet. Anschließend wird die vorliegende Arbeit gegenüber den bestehenden Ansätzen eingeordnet und abgegrenzt.

In Kapitel 4 werden die verwendete Methodik und der Entwicklungsprozess beschrieben. Dazu gehören das Vorgehen bei der Literatur- und Technologierecherche, das gewählte Vorgehensmodell, der iterative Integrationsprozess, die Modellentwicklung sowie das Verifikations- und Evaluationskonzept. Kapitel 5 leitet aus dem Anwendungsszenario die funktionalen und nichtfunktionalen Anforderungen, die technischen Randbedingungen und die Abnahmekriterien ab.

Der Systementwurf wird in Kapitel 6 vorgestellt. Dabei werden das System- und Hardwarekonzept, die KI-Verarbeitungskette sowie der Speicher- und Datenfluss vom Kamerabild bis zum Erkennungsergebnis beschrieben. Kapitel 7 behandelt die konkrete Entwicklung und Implementierung. Hierzu gehören die Aufbereitung des Datensatzes, die Augmentationspipeline, das Training und die Quantisierung des Klassifikationsmodells, die Implementierung der Kamerapipeline, die Handflächen- und Landmark-Erkennung, die Bestimmung der Region of Interest, die Verarbeitung der Landmark-Daten sowie die Integration der neuronalen Netze, die Ablaufsteuerung und die Visualisierung.

Kapitel 8 beschreibt die im Entwicklungsverlauf vorgenommenen Optimierungen und Anpassungen. Im Mittelpunkt stehen die Inferenzpipeline, die Speicher- und Cache-Nutzung, die Stabilität der Landmarken, die Behandlung unterschiedlicher Handrotationen sowie die Reduzierung von Latenzen und blockierenden Abläufen. Anschließend wird das entwickelte System in Kapitel 9 anhand der definierten Kriterien evaluiert und validiert.

Die Ergebnisse der Evaluation werden in Kapitel 10 interpretiert und diskutiert. Dabei werden das Gesamtsystem, seine Grenzen und der technische sowie praktische Beitrag der Arbeit betrachtet. Kapitel 11 fasst die wesentlichen Ergebnisse zusammen, bewertet die Erreichung der Zielsetzung und gibt einen Ausblick auf mögliche Weiterentwicklungen.