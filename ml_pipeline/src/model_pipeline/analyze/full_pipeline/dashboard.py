from dataclasses import dataclass
import cv2 as cv
import numpy as np


class ModeState:
    PALM = 0
    HAND = 1
    SIGN = 2


MODE_LABELS = ["Palm", "Hand", "Sign"]


@dataclass
class Config:
    """Values the to tweak at runtime"""

    palm_count: str = "0"
    status_text: str = ""

    mode: int = ModeState.PALM

    show_palm: bool = True
    show_landmarks: bool = True
    show_sign: bool = True

    score_threshold: float = 0.5
    iou_threshold: float = 0.4
    handedness_threshold: float = 0.5

    quit_requested: bool = False    


_PANEL_W = 300
_MARGIN = 30
_ROW_H = 32
_BTN_H = 36
_SLIDER_H = 10
_SLIDER_HANDLE_R = 8
_HEADER_H = 20
_HEADER_GAP = 4
_SECTION_GAP = 30
_PARAM_SLIDER_W_OFFSET = 110


class Dashboard:
    """Draws the side-panel UI and interprets mouse events"""

    def __init__(self, config: Config | None = None):
        self.config = config or Config()
        self._dragging: str | None = None
        self._x_offset: int = 0

    # ------------------------------------------------------------------
    # Mouse handling
    # ------------------------------------------------------------------

    def setup(self, window_name: str):
        cv.setMouseCallback(window_name, self._mouse_callback)

    def _mouse_callback(self, event, x, y, flags, param):
        x -= self._x_offset
        if x < 0:
            return
        if event == cv.EVENT_LBUTTONDOWN:
            self._handle_click(x, y)
        elif event == cv.EVENT_LBUTTONUP:
            self._dragging = None
        elif event == cv.EVENT_MOUSEMOVE and self._dragging is not None:
            self._handle_drag(x, y)

    def _handle_click(self, x, y):
        sec = self._section_offsets()
        sw = _PANEL_W - 2 * _MARGIN

        if sec["mode"] <= y < sec["mode"] + _BTN_H:
            seg_w = sw // 3
            idx = (x - _MARGIN) // seg_w
            if 0 <= idx < 3:
                self.config.mode = idx
            return

        for i in range(3):
            ty = sec["show"] + i * (_BTN_H + 4)
            if ty <= y < ty + _BTN_H:
                if i == 0:
                    self.config.show_palm = not self.config.show_palm
                elif i == 1:
                    self.config.show_landmarks = not self.config.show_landmarks
                elif i == 2:
                    self.config.show_sign = not self.config.show_sign
                return

        slider_w = sw - _PARAM_SLIDER_W_OFFSET
        param_attrs = ["score_threshold", "iou_threshold", "handedness_threshold"]
        for i, attr in enumerate(param_attrs):
            sy = sec["params"] + i * (_ROW_H + 4)
            if sy <= y < sy + _ROW_H:
                sx = _MARGIN + 80
                if sx <= x < sx + slider_w:
                    fraction = max(0.0, min(1.0, (x - sx) / slider_w))
                    setattr(self.config, attr, round(fraction, 2))
                    self._dragging = attr
                return

        if sec["quit"] <= y < sec["quit"] + _BTN_H:
            self.config.quit_requested = True

    def _handle_drag(self, x, y):
        if self._dragging not in ("score_threshold", "iou_threshold", "handedness_threshold"):
            return
        sw = _PANEL_W - 2 * _MARGIN
        slider_w = sw - _PARAM_SLIDER_W_OFFSET
        sx = _MARGIN + 80
        fraction = max(0.0, min(1.0, (x - sx) / slider_w))
        setattr(self.config, self._dragging, round(fraction, 2))

    # ------------------------------------------------------------------
    # Layout
    # ------------------------------------------------------------------

    @staticmethod
    def _section_offsets():
        y = _MARGIN
        mode_btn = y + _HEADER_H + _HEADER_GAP
        show_btns = mode_btn + _BTN_H + _SECTION_GAP + _HEADER_H + _HEADER_GAP
        params_rows = show_btns + 3 * (_BTN_H + 4) + _SECTION_GAP + _HEADER_H + _HEADER_GAP
        quit_btn = params_rows + 3 * (_ROW_H + 4) + 4
        return {
            "mode_header": y,
            "mode": y + _HEADER_H + _HEADER_GAP,
            "show_header": mode_btn + _BTN_H + _SECTION_GAP,
            "show": show_btns,
            "params_header": show_btns + 3 * (_BTN_H + 4) + _SECTION_GAP,
            "params": params_rows,
            "quit": quit_btn,
        }

    # ------------------------------------------------------------------
    # Rendering helpers
    # ------------------------------------------------------------------

    @staticmethod
    def _text(canvas, text, pos, font_scale, color, thickness=2):
        cv.putText(canvas, text, pos, cv.FONT_HERSHEY_SIMPLEX, font_scale, color, thickness)

    @staticmethod
    def _outline_text(canvas, text, pos, font_scale, color, outline_color = (0, 0, 0), thickness=2):
        cv.putText(canvas, text, pos, cv.FONT_HERSHEY_SIMPLEX, font_scale, outline_color, thickness + 1)
        cv.putText(canvas, text, pos, cv.FONT_HERSHEY_SIMPLEX, font_scale, color, thickness)

    # ------------------------------------------------------------------
    # Camera overlay (top-left info)
    # ------------------------------------------------------------------

    def draw_info(self, frame):
        lines = [
            f"Palms: {self.config.palm_count}",
            f"Status: {self.config.status_text}",
        ]
        for i, line in enumerate(lines):
            self._outline_text(frame, line, (20, 30 + i * 30), 0.65, (0, 255, 255))

    # ------------------------------------------------------------------
    # Dedicated dashboard window
    # ------------------------------------------------------------------

    def render(self, height: int) -> np.ndarray:
        canvas = np.full((height, _PANEL_W, 3), 20, dtype=np.uint8)
        cv.rectangle(canvas, (0, 0), (_PANEL_W - 1, height - 1), (100, 100, 100), 2)

        sec = self._section_offsets()
        sw = _PANEL_W - 2 * _MARGIN

        # MODE
        self._text(canvas, "MODE", (_MARGIN, sec["mode_header"]), 0.55, (200, 200, 200))
        bw = sw // 3
        for i in range(3):
            bx = _MARGIN + i * bw
            by = sec["mode"]
            color = (60, 120, 200) if self.config.mode == i else (60, 60, 60)
            cv.rectangle(canvas, (bx, by), (bx + bw, by + _BTN_H), color, -1)
            cv.rectangle(canvas, (bx, by), (bx + bw, by + _BTN_H), (150, 150, 150), 1)
            ts = cv.getTextSize(MODE_LABELS[i], cv.FONT_HERSHEY_SIMPLEX, 0.5, 2)[0]
            self._text(canvas, MODE_LABELS[i],
                       (bx + (bw - ts[0]) // 2, by + (_BTN_H + ts[1]) // 2),
                       0.5, (255, 255, 255))

        # SHOW FEATURES
        self._text(canvas, "SHOW FEATURES", (_MARGIN, sec["show_header"]), 0.55, (200, 200, 200))
        toggles = [
            ("Palm Boxes", self.config.show_palm),
            ("Landmarks", self.config.show_landmarks),
            ("Sign Lang", self.config.show_sign),
        ]
        for i, (label, active) in enumerate(toggles):
            ty = sec["show"] + i * (_BTN_H + 4)
            bg = (40, 80, 40) if active else (50, 50, 50)
            cv.rectangle(canvas, (_MARGIN, ty), (_MARGIN + sw, ty + _BTN_H), bg, -1)
            cv.rectangle(canvas, (_MARGIN, ty), (_MARGIN + sw, ty + _BTN_H), (120, 120, 120), 1)
            self._text(canvas, f"{'[x]' if active else '[ ]'} {label}",
                       (_MARGIN + 8, ty + (_BTN_H + 14) // 2), 0.5, (255, 255, 255))

        # PARAMETERS
        self._text(canvas, "PARAMETERS", (_MARGIN, sec["params_header"]), 0.55, (200, 200, 200))
        slider_w = sw - _PARAM_SLIDER_W_OFFSET
        params = [
            ("Score", self.config.score_threshold),
            ("IOU", self.config.iou_threshold),
            ("Hand", self.config.handedness_threshold),
        ]
        for i, (label, val) in enumerate(params):
            sy = sec["params"] + i * (_ROW_H + 4)
            self._text(canvas, label, (_MARGIN, sy + 14), 0.45, (180, 180, 180))
            sx = _MARGIN + 80
            cv.rectangle(canvas, (sx, sy + 8), (sx + slider_w, sy + 8 + _SLIDER_H), (80, 80, 80), -1)
            cv.rectangle(canvas, (sx, sy + 8), (sx + slider_w, sy + 8 + _SLIDER_H), (120, 120, 120), 1)
            hx = int(sx + val * slider_w)
            cv.circle(canvas, (hx, sy + 8 + _SLIDER_H // 2), _SLIDER_HANDLE_R, (0, 150, 255), -1)
            cv.circle(canvas, (hx, sy + 8 + _SLIDER_H // 2), _SLIDER_HANDLE_R, (200, 200, 200), 1)
            self._text(canvas, f"{val:.2f}", (sx + slider_w + 6, sy + 14), 0.4, (0, 200, 255))

        # QUIT
        qy = sec["quit"]
        cv.rectangle(canvas, (_MARGIN, qy), (_MARGIN + sw, qy + _BTN_H), (40, 40, 120), -1)
        cv.rectangle(canvas, (_MARGIN, qy), (_MARGIN + sw, qy + _BTN_H), (150, 80, 80), 2)
        self._text(canvas, "QUIT", (_MARGIN + sw // 2 - 20, qy + (_BTN_H + 14) // 2),
                   0.55, (255, 200, 200))

        return canvas
