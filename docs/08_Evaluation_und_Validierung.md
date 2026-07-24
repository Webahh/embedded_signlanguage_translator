
## 8. Evaluation und Validierung

### 8.1  Versuchsaufbau und Testbedingungen

### 8.2 Erkennungsgenauigkeit

### 8.3 Konfusionsmatrix und klassenbezogene Ergebnisse 

**Trainingsergebnisse:**

| Metrik                      | Wert    |
| --------------------------- | ------- |
| Trainingsgenauigkeit        | 98,34 % |
| Validierungsgenauigkeit     | 99,21 % |
| Finaler Trainingsverlust    | 0,0754  |
| Finaler Validierungsverlust | 0,0319  |

**Confusionsmatrix**:
![[confusion_matrix.png]]
![[confusion_matrix_normalized.png]]

Das Modell zeigt keine Anzeichen von Overfitting: Der Validierungsverlust sinkt kontinuierlich über alle 20 Epochen.

### 8.4 Einfluss von Beleuchtung und Hintergrund

### 8.5 Inferenzlatenz und Ende-zu-Ende-Latenz

### 8.6 Bildrate und Datendurchsatz

### 8.7 Speicher- und Ressourcenverbrauch

### 8.8 Abgleich mit den definierten Anforderungen