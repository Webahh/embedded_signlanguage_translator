from dataclasses import dataclass
import cv2 as cv


class ModeState:
    PALM = 0
    HAND = 1
    SIGN = 2


MODE_LABELS = ["Palm", "Hand", "Sign"]

@dataclass
class Config:
    """Values the to tweak at runtime"""

    # Display info (fed by the pipeline each frame)
    palm_count: str = "0"
    status_text: str = ""

    # Processing mode
    mode: int = ModeState.PALM

    # Feature visibility toggles
    show_palm: bool = True
    show_landmarks: bool = True
    show_sign: bool = False

    # Adjustable detection parameters
    score_threshold: float = 0.5
    iou_threshold: float = 0.4
    handedness_threshold: float = 0.5

    # Lifecycle
    quit_requested: bool = False


# ---------------------------------------------------------------------------
# Layout constants
# ---------------------------------------------------------------------------

_PANEL_W = 300
_MARGIN = 30
_ROW_H = 32
_BTN_H = 36
_SLIDER_H = 10
_SLIDER_HANDLE_R = 8

# Section spacing helpers
_HEADER_H = 20
_HEADER_GAP = 4
_SECTION_GAP = 30
_PARAM_SLIDER_W_OFFSET = 110  # reserved for slider value text


# ---------------------------------------------------------------------------
# Pure renderer
# ---------------------------------------------------------------------------


class Dashboard:
    """Draws the side-panel UI and interprets mouse events
    """

    def __init__(self, config: Config | None = None):
        self.config = config or Config()
        self._panel_x = 0
        self._panel_y = 0
        self._dragging: str | None = None

    # ------------------------------------------------------------------
    # Mouse handling
    # ------------------------------------------------------------------

    def setup(self, window_name: str):
        cv.setMouseCallback(window_name, self._mouse_callback)

    def _mouse_callback(self, event, x, y, flags, param):
        if event == cv.EVENT_LBUTTONDOWN:
            self._handle_click(x, y)
        elif event == cv.EVENT_LBUTTONUP:
            self._dragging = None
        elif event == cv.EVENT_MOUSEMOVE and self._dragging is not None:
            self._handle_drag(x, y)

    def _handle_click(self, x, y):
        if x < self._panel_x:
            return
        rel_x = x - self._panel_x
        rel_y = y - self._panel_y
        sec = self._section_offsets()
        sw = _PANEL_W - 2 * _MARGIN

        # Mode split button
        if sec["mode"] <= rel_y < sec["mode"] + _BTN_H:
            seg_w = sw // 3
            idx = (rel_x - _MARGIN) // seg_w
            if 0 <= idx < 3:
                self.config.mode = idx
            return

        # Show-feature toggles
        for i in range(3):
            ty = sec["show"] + i * (_BTN_H + 4)
            if ty <= rel_y < ty + _BTN_H:
                if i == 0:
                    self.config.show_palm = not self.config.show_palm
                elif i == 1:
                    self.config.show_landmarks = not self.config.show_landmarks
                elif i == 2:
                    self.config.show_sign = not self.config.show_sign
                return

        # Parameter sliders
        slider_w = sw - _PARAM_SLIDER_W_OFFSET
        param_attrs = ["score_threshold", "iou_threshold", "handedness_threshold"]
        for i, attr in enumerate(param_attrs):
            sy = sec["params"] + i * (_ROW_H + 4)
            if sy <= rel_y < sy + _ROW_H:
                sx = _MARGIN + 80
                if sx <= rel_x < sx + slider_w:
                    fraction = max(0.0, min(1.0, (rel_x - sx) / slider_w))
                    setattr(self.config, attr, round(fraction, 2))
                    self._dragging = attr
                return

        # Quit button
        if sec["quit"] <= rel_y < sec["quit"] + _BTN_H:
            self.config.quit_requested = True

    def _handle_drag(self, x, y):
        if self._dragging not in ("score_threshold", "iou_threshold", "handedness_threshold"):
            return
        rel_x = x - self._panel_x
        sw = _PANEL_W - 2 * _MARGIN
        slider_w = sw - _PARAM_SLIDER_W_OFFSET
        sx = _MARGIN + 80
        fraction = max(0.0, min(1.0, (rel_x - sx) / slider_w))
        setattr(self.config, self._dragging, round(fraction, 2))

    # ------------------------------------------------------------------
    # Layout
    # ------------------------------------------------------------------

    @staticmethod
    def _section_offsets():
        y = _MARGIN

        mode_header = y
        mode_btn = mode_header + _HEADER_H + _HEADER_GAP

        show_header = mode_btn + _BTN_H + _SECTION_GAP
        show_btns = show_header + _HEADER_H + _HEADER_GAP

        params_header = show_btns + 3 * (_BTN_H + 4) + _SECTION_GAP
        params_rows = params_header + _HEADER_H + _HEADER_GAP

        quit_btn = params_rows + 3 * (_ROW_H + 4) + 4

        return {
            "mode_header": mode_header,
            "mode": mode_btn,
            "show_header": show_header,
            "show": show_btns,
            "params_header": params_header,
            "params": params_rows,
            "quit": quit_btn,
        }

    # ------------------------------------------------------------------
    # Rendering helpers
    # ------------------------------------------------------------------

    @staticmethod
    def _text(frame, text, pos, font_scale, color, thickness=2):
        cv.putText(frame, text, pos, cv.FONT_HERSHEY_SIMPLEX, font_scale, color, thickness)

    def _panel_bg(self, frame, px, py, h):
        overlay = frame[py:py + h, px:px + _PANEL_W].copy()
        cv.rectangle(overlay, (0, 0), (_PANEL_W, h), (20, 20, 20), -1)
        cv.addWeighted(overlay, 0.85, frame[py:py + h, px:px + _PANEL_W], 0.15, 0,
                       frame[py:py + h, px:px + _PANEL_W])
        cv.rectangle(frame, (px, py), (px + _PANEL_W, h), (100, 100, 100), 2)

    def _draw_info_overlay(self, frame):
        lines = [
            f"Palms: {self.config.palm_count}",
            f"Status: {self.config.status_text}",
        ]
        for i, line in enumerate(lines):
            y_pos = 30 + i * 30
            self._text(frame, line, (20, y_pos), 0.65, (0, 255, 255))

    def _draw_mode_section(self, frame, px, py, sec):
        sw = _PANEL_W - 2 * _MARGIN
        bw = sw // 3
        for i in range(3):
            bx = px + _MARGIN + i * bw
            by = py + sec["mode"]
            color = (60, 120, 200) if self.config.mode == i else (60, 60, 60)
            cv.rectangle(frame, (bx, by), (bx + bw, py + sec["mode"] + _BTN_H), color, -1)
            cv.rectangle(frame, (bx, by), (bx + bw, py + sec["mode"] + _BTN_H), (150, 150, 150), 1)
            ts = cv.getTextSize(MODE_LABELS[i], cv.FONT_HERSHEY_SIMPLEX, 0.5, 2)[0]
            tx = bx + (bw - ts[0]) // 2
            ty = by + (_BTN_H + ts[1]) // 2
            self._text(frame, MODE_LABELS[i], (tx, ty), 0.5, (255, 255, 255))

    def _draw_show_section(self, frame, px, py, sec):
        sw = _PANEL_W - 2 * _MARGIN
        toggles = [
            ("Palm Boxes", self.config.show_palm),
            ("Landmarks", self.config.show_landmarks),
            ("Sign Lang", self.config.show_sign),
        ]
        for i, (label, active) in enumerate(toggles):
            ty = py + sec["show"] + i * (_BTN_H + 4)
            bg = (40, 80, 40) if active else (50, 50, 50)
            cv.rectangle(frame, (px + _MARGIN, ty), (px + _MARGIN + sw, ty + _BTN_H), bg, -1)
            cv.rectangle(frame, (px + _MARGIN, ty), (px + _MARGIN + sw, ty + _BTN_H), (120, 120, 120), 1)
            self._text(frame, f"{'[x]' if active else '[ ]'} {label}",
                       (px + _MARGIN + 8, ty + (_BTN_H + 14) // 2), 0.5, (255, 255, 255))

    def _draw_params_section(self, frame, px, py, sec):
        sw = _PANEL_W - 2 * _MARGIN
        slider_w = sw - _PARAM_SLIDER_W_OFFSET

        params = [
            ("Score", self.config.score_threshold),
            ("IOU", self.config.iou_threshold),
            ("Hand", self.config.handedness_threshold),
        ]
        for i, (label, val) in enumerate(params):
            sy = py + sec["params"] + i * (_ROW_H + 4)
            self._text(frame, label, (px + _MARGIN, sy + 14), 0.45, (180, 180, 180))

            sx = px + _MARGIN + 80
            cv.rectangle(frame, (sx, sy + 8), (sx + slider_w, sy + 8 + _SLIDER_H), (80, 80, 80), -1)
            cv.rectangle(frame, (sx, sy + 8), (sx + slider_w, sy + 8 + _SLIDER_H), (120, 120, 120), 1)

            hx = int(sx + val * slider_w)
            cv.circle(frame, (hx, sy + 8 + _SLIDER_H // 2), _SLIDER_HANDLE_R, (0, 150, 255), -1)
            cv.circle(frame, (hx, sy + 8 + _SLIDER_H // 2), _SLIDER_HANDLE_R, (200, 200, 200), 1)

            self._text(frame, f"{val:.2f}", (sx + slider_w + 6, sy + 14), 0.4, (0, 200, 255))

    def _draw_quit_section(self, frame, px, py, sec):
        sw = _PANEL_W - 2 * _MARGIN
        qy = py + sec["quit"]
        cv.rectangle(frame, (px + _MARGIN, qy), (px + _MARGIN + sw, qy + _BTN_H), (40, 40, 120), -1)
        cv.rectangle(frame, (px + _MARGIN, qy), (px + _MARGIN + sw, qy + _BTN_H), (150, 80, 80), 2)
        self._text(frame, "QUIT", (px + _MARGIN + sw // 2 - 20, qy + (_BTN_H + 14) // 2),
                   0.55, (255, 200, 200))

    # ------------------------------------------------------------------
    # Main entry point
    # ------------------------------------------------------------------

    def draw(self, frame):
        h, w = frame.shape[:2]
        self._panel_x = w - _PANEL_W
        self._panel_y = 0
        px, py = self._panel_x, self._panel_y
        sec = self._section_offsets()

        self._panel_bg(frame, px, py, h)
        self._draw_info_overlay(frame)
        self._text(frame, "MODE", (px + _MARGIN, py + sec["mode_header"]), 0.55, (200, 200, 200))
        self._draw_mode_section(frame, px, py, sec)
        self._text(frame, "SHOW FEATURES", (px + _MARGIN, py + sec["show_header"]), 0.55, (200, 200, 200))
        self._draw_show_section(frame, px, py, sec)
        self._text(frame, "PARAMETERS", (px + _MARGIN, py + sec["params_header"]), 0.55, (200, 200, 200))
        self._draw_params_section(frame, px, py, sec)
        self._draw_quit_section(frame, px, py, sec)

        return frame
