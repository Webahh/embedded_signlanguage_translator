## 4. Methodik und Entwicklungsprozess

### 4.1 Vorgehensweise bei der Literatur- und Technologierecherche

Die Literatur- und Technologierecherche dient dazu, die fachlichen Grundlagen der Arbeit zu erarbeiten, bestehende Lösungsansätze zu untersuchen und geeignete Technologien für die Umsetzung des Systems auszuwählen. Da es sich bei der vorliegenden Arbeit um eine Entwicklungsarbeit handelt, wurde keine systematische Literaturstudie mit dem Anspruch auf eine vollständige Erfassung aller Veröffentlichungen durchgeführt. Stattdessen erfolgte eine zielgerichtete und entwicklungsbegleitende Recherche, die sich an den jeweils zu lösenden fachlichen und technischen Fragestellungen orientierte.

Die wissenschaftliche Literaturrecherche umfasste insbesondere die Themengebiete Fingeralphabet- und Handzeichenerkennung, kamerabasierte Handdetektion, Handlandmark-Erkennung, Klassifikation mittels neuronaler Netze, Datenaugmentation, Modellquantisierung und Embedded AI. Für die Suche wurden zunächst Mind-Maps und Wortwolken mit den genannten Begriffen erstellt und in verschiedenen Kombinationen in wissenschaftliche Datenbanken (u. a. Google Scholar und BibDiscover) eingegeben und recherchiert. Zu den verwendeten Suchbegriffen gehörten unter anderem:

- `hand gesture classification`   
- `Embedded AI / AI on the Edge`
- `Quantisierung von neuralen Netzen`
- `Tensorflow / Tensorflow Lite`
- `Konvertierung von Modellen (TFLite)`
- `Fingeralphabet`
- `sign language recognition`
- `Gebärdensprache`
- `Modeltraining / Modelquantization parameters`

Bei der Auswahl der Quellen wurden deren fachliche Relevanz, Aktualität und Nachvollziehbarkeit berücksichtigt. Bevorzugt wurden Fachartikel und wissenschaftliche Ausarbeitungen. Für grundlegende Informationen zum deutschen Fingeralphabet wurden Veröffentlichungen und Informationsangebote von Verbänden und Organisationen herangezogen. Die ausgewählten Quellen wurden mithilfe der Literaturverwaltungssoftware Zotero erfasst und verwaltet.

Die Technologierecherche konzentrierte sich überwiegend auf die verwendete Plattform, das STM32N6570 Discovery Kit, sowie auf die für die Implementierung erforderlichen Hard- und Softwarekomponenten. Als Quellen dienten insbesondere die Referenz- und Benutzerhandbücher, Datenblätter, Anwendungshinweise und Beispielprojekte des Herstellers STMicroelectronics. Ergänzend wurden technische Informationen zu den verwendeten Peripherien und Schnittstellen recherchiert. Dazu gehörten unter anderem die Digital Camera Memory Interface Pixel Pipeline (DCMIPP), das Camera Serial Interface (CSI), der LCD-TFT Display Controller (LTDC) und das Extended Serial Peripheral Interface (XSPI).

Die Recherche wurde parallel zum Entwicklungsprozess fortgeführt. Technische Fragestellungen, die während der Implementierung oder Integration auftraten, führten zu einer gezielten Vertiefung einzelner Themenbereiche. Die daraus gewonnenen Erkenntnisse flossen unmittelbar in die Auswahl und Anpassung der Modelle, die Entwicklung der Verarbeitungspipeline sowie die hardwarenahe Umsetzung auf der Zielplattform ein.

Ergänzend wurde ChatGPT als unterstützendes KI-Werkzeug eingesetzt. Die Nutzung umfasste hauptsächlich die Überarbeitung eigener Textentwürfe. Die Unterstützung bei technischen Fragestellungen und Fehleranalysen während der Implementierung wurden nicht ungeprüft übernommen, sondern anhand technischer Dokumentation, Beispielanwendungen und eigener Untersuchungen überprüft. Die Auswahl und Bewertung der Quellen, die technischen  Entscheidungen sowie die abschließende Ausarbeitung erfolgten durch die Verfasser.  

### 4.2 Iterativer Entwicklungs- und Integrationsprozess

Für die Entwicklung des Gesamtsystems wurde ein iteratives und prototypisches Vorgehensmodell gewählt. Ziel jeder Iteration war es, einen funktionsfähigen Zwischenstand zu erreichen, der den vorherigen Prototyp um neue oder verbesserte Funktionen erweitert. Auf diese Weise konnte das System schrittweise aufgebaut und bereits während der Entwicklung praktisch erprobt werden. Auftretende Probleme und neue Erkenntnisse konnten dadurch frühzeitig in die weitere Planung einbezogen werden.

Zu Beginn einer Iteration wurden die nächsten Entwicklungsschritte gemeinsam festgelegt und auf konkrete Aufgaben verteilt. Da das Projekt als Gruppenarbeit durchgeführt wurde, erfolgte eine personenspezifische Aufteilung in die bereits in Kapitel 1.3.2 genannten Aufgabenbereiche. Während des Entwicklungsprozesses fanden regelmäßige Absprachen zum aktuellen Stand, zu aufgetretenen Problemen und zum weiteren Vorgehen statt. Dabei wurden Ergebnisse der vorherigen Iteration bewertet und die Ziele der nächsten Iteration festgelegt. Falls sich eine Lösung als ungeeignet erwies oder neue technische Einschränkungen erkannt wurden, wurde das weitere Vorgehen entsprechend angepasst.

Zur Versionsverwaltung des Quellcodes wurde Git eingesetzt. Die gemeinsame Entwicklung erfolgte über ein zentrales Repository. Neue Funktionen und umfangreiche Änderungen wurden überwiegend in separaten Feature-Branches entwickelt. Dadurch konnten Änderungen zunächst unabhängig vom stabilen Entwicklungsstand umgesetzt und getestet werden. Gleichzeitig erleichterte die Trennung der Entwicklungsstände die Zuordnung und Überprüfung einzelner Änderungen. Nach erfolgreicher Überprüfung wurden die Branches in den gemeinsamen Hauptzweig integriert. Insbesondere bei großen Änderungen und vor der Zusammenführung der Branches wurden gegenseitige Code-Reviews durchgeführt. Diese dienten dazu, Fehler zu erkennen, die Verständlichkeit des Quellcodes zu verbessern und mögliche Auswirkungen auf andere Komponenten zu berücksichtigen. Festgestellte Probleme wurden vor oder während der Integration behoben. 

Durch die Kombination aus iterativer Prototypentwicklung, klarer Aufgabenverteilung, regelmäßiger Abstimmung und gemeinsamer Versionsverwaltung konnte der Entwicklungsfortschritt fortlaufend nachvollzogen werden. Gleichzeitig blieb das Vorgehen flexibel genug, um auf technische Schwierigkeiten und neue Erkenntnisse während der Entwicklung reagieren zu können.

### 4.3 Vorgehen bei der Modellentwicklung

Die Entwicklung des Klassifikationsmodells erfolgte in mehreren aufeinander aufbauenden Schritten. Zunächst wurden die zu erkennenden Handzeichen festgelegt und ein eigener Datensatz erstellt. Die aufgenommenen Daten wurden anschließend aufbereitet und in Trainings-, Validierungs- und Testdaten unterteilt.

Auf Grundlage des Datensatzes wurden verschiedene Modellvarianten entwickelt und trainiert. Dabei wurden sowohl die Modellarchitektur als auch die Trainingsparameter schrittweise angepasst. Zur Erweiterung des Datensatzes kamen unterschiedliche Augmentationsverfahren zum Einsatz. Diese sollten die Variabilität der Trainingsdaten erhöhen und das Modell gegenüber unterschiedlichen Ausführungen der Handzeichen robuster machen.

Das Modell mit der besten Erkennungsgenauigkeit wurde ausgewählt und abschließend für die eingebettete Plattform verfügbar gemacht und integriert. Dabei wurde geprüft, ob und in welchem Umfang sich die Erkennungsleistung durch die Anpassungen verändert hatte.

### 4.4 Verifikation- und Evaluationskonzept

Die Verifikation und Evaluation des entwickelten Systems erfolgte entwicklungsbegleitend und abschließend anhand der zuvor festgelegten Anforderungen. Dabei wurde zwischen der Überprüfung der korrekten Umsetzung einzelner Komponenten und der Bewertung des Gesamtsystems unterschieden. Auf diese Weise sollte sowohl die technische Funktionsfähigkeit als auch die Eignung des entstandenen Prototyps untersucht werden. 

Die Verifikation wurde wie der Entwicklungsprozess iterativ durchgeführt. Neu entwickelte Funktionen mussten zunächst unabhängig getestet werden, bevor sie in den gemeinsamen Entwicklungsstand übernommen wurden. Nach der Integration wurde kontrolliert, ob die hinzugefügte Funktion wie vorgesehen arbeitete und bereits vorhandene Funktionen weiterhin verfügbar waren. Dieses Vorgehen erleichterte es, auftretende Fehler einzudämmen, zuzuordnen und entsprechend anzupassen.

Die Evaluation des Gesamtsystems wurde anhand der funktionalen und nichtfunktionalen Anforderungen, sowie der daraus abgeleiteten Abnahmekriterien vorgenommen. Die konkreten Versuchsbedingungen, Bewertungsparameter und Ergebnisse werden im Evaluationskapitel beschrieben. Auf Grundlage dieser Ergebnisse wird beurteilt, inwieweit die Zielsetzung der Arbeit erreicht wurde und welche Einschränkungen beim entwickelten Prototyp bestehen.