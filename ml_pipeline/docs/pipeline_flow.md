# Pipeline Flow

```mermaid
flowchart TD
    Start(["Frame Loop (every frame)"]) --> Capture["Capture frame"]
    Capture --> TrackLoop{"Any active ROIs?"}

    TrackLoop -->|Yes| LM["Run landmark model on ROI"]
    TrackLoop -->|No| DetectFull

    LM --> Confidence{"Confidence\nabove threshold?"}
    Confidence -->|Yes| Decode["Convert landmarks from\ncrop to image coordinates"]
    Decode --> Update["Compute new bounding box\nand update ROI"]
    Update --> MoreTracks{"More active ROIs?"}
    MoreTracks -->|Yes| LM

    Confidence -->|No| Drop["Mark ROI as inactive"]
    Drop --> MoreTracks

    MoreTracks -->|No| CheckFull{"All ROIs active\nand none lost this frame?"}
    CheckFull -->|Yes| ReturnTracked["Return boxes from ROIs\nSkip full detection"]
    ReturnTracked --> Start

    CheckFull -->|No| DetectFull["Run detection model\non full frame"]
    DetectFull --> Filter["Remove detections that overlap\nalready tracked objects"]
    Filter --> Count{"How many new detections?"}

    Count -->|0| ReturnActive["Return current boxes"]
    ReturnActive --> Start

    Count -->|1| CreateROI["Create ROI from detection\nassign to free slot"]
    CreateROI --> LM1["Run landmark model on new ROI"]
    LM1 --> Update1["Update ROI from\nlandmark positions"]
    Update1 --> Return1["Return detected object"]
    Return1 --> Start

    Count -->|≥2| CreateROI2["Create ROI for each detection\nfill all free slots"]
    CreateROI2 --> LM2["Run landmark model\non each ROI"]
    LM2 --> Update2["Update each ROI\nfrom landmarks"]
    Update2 --> Return2["Return all detected objects"]
    Return2 --> Start

    style Start stroke:#66a
    style ReturnTracked stroke:#292
    style ReturnActive stroke:#a22
    style Return1 stroke:#292
    style Return2 stroke:#292
```

## Walkthrough

### 1. Processing Existing ROIs
For every active ROI slot, run the landmark model on the current crop:
- **Confidence above threshold** → convert landmarks to image coordinates, compute new bounding box, update ROI for next frame
- **Confidence too low** → mark slot free, trigger full detection on this frame

### 2. Skip Full Detection?
If **all** slots have active ROIs and none were lost this frame → return the boxes directly from the ROIs. Otherwise fall through to full-frame detection.

### 3. Full Detection + Branching
| Detections | What happens |
|------------|-------------|
| 0 | Return whatever is still active (likely nothing) |
| 1 | Create ROI from detection → fill one free slot → run landmark → object detected |
| ≥2 | Create ROI for each detection → fill all free slots → run landmark on each → all objects detected |

Detections that overlap an object already followed by an ROI are removed to avoid duplicate ROIs.

## Object Movement Behaviour

| Scenario | Result |
|----------|--------|
| Object moves within ROI | Landmarks converted to image coords → ROI re-centred each frame → ROI follows object |
| Object leaves ROI (confidence drops) | Slot freed → full detection runs same frame → new ROI created if re-detected |
| New object enters frame | Free slot exists → full detection picks it up → ROI created |
