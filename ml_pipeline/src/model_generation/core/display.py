import cv2 as cv
import numpy as np
from PIL import Image, ImageDraw, ImageFont


class ConfidenceDisplay:
    """
    Handles drawing inference results onto video frames.
    """

    def __init__(self):
        try:
            self.font = ImageFont.truetype("arial.ttf", 16)
        except:
            self.font = ImageFont.load_default()

    def draw_confidence_table(self, img, confidences, threshold=0.02):
        """
        Draws a compact confidence overlay in the top-right corner.

        Args:
            img: OpenCV BGR image
            confidences: dict {label: confidence}
            threshold: minimum confidence to display
        """

        if not confidences:
            return img

        # Filter weak predictions
        confidences = {
            k: v for k, v in confidences.items()
            if v >= threshold
        }

        if not confidences:
            return img

        # Sort descending
        confidences = dict(
            sorted(confidences.items(), key=lambda x: x[1], reverse=True)
        )

        pil_img = Image.fromarray(cv.cvtColor(img, cv.COLOR_BGR2RGB))
        draw = ImageDraw.Draw(pil_img)

        padding = 8
        line_height = 24
        bar_width = 90
        bar_height = 10

        rows = list(confidences.items())

        max_label_width = max(
            draw.textbbox((0, 0), label, font=self.font)[2]
            for label, _ in rows
        )

        panel_width = max_label_width + bar_width + 70
        footer_height = 18 if threshold > 0 else 0
        panel_height = line_height * len(rows) + padding * 2 + footer_height

        # Top-right corner
        x0 = img.shape[1] - panel_width - 15
        y0 = 15

        # Background
        overlay = Image.new("RGBA", pil_img.size, (0, 0, 0, 0))
        overlay_draw = ImageDraw.Draw(overlay)

        overlay_draw.rounded_rectangle(
            [x0, y0, x0 + panel_width, y0 + panel_height],
            radius=10,
            fill=(0, 0, 0, 140)
        )

        pil_img = Image.alpha_composite(pil_img.convert("RGBA"), overlay)
        draw = ImageDraw.Draw(pil_img)

        # Rows
        for i, (label, conf) in enumerate(rows):
            y = y0 + padding + i * line_height

            draw.text(
                (x0 + 8, y),
                label,
                font=self.font,
                fill=(255, 255, 255, 255)
            )

            bar_x = x0 + max_label_width + 25
            bar_y = y + 6

            draw.rectangle(
                [bar_x, bar_y, bar_x + bar_width, bar_y + bar_height],
                fill=(70, 70, 70, 200)
            )

            fill_w = int(bar_width * conf)

            color = (
                int((1 - conf) * 255),
                int(conf * 255),
                80,
                255
            )

            draw.rectangle(
                [bar_x, bar_y, bar_x + fill_w, bar_y + bar_height],
                fill=color
            )

            draw.text(
                (bar_x + bar_width + 6, y),
                f"{conf * 100:.2f}%",
                font=self.font,
                fill=(220, 220, 220, 255)
            )

        # Footer
        if threshold > 0:
            note = f"Hidden: values < {threshold * 100:.1f}%"

            text_bbox = draw.textbbox((0, 0), note, font=self.font)
            text_width = text_bbox[2] - text_bbox[0]

            note_x = x0 + (panel_width - text_width) // 2
            note_y = y0 + panel_height - footer_height + 2

            draw.text(
                (note_x, note_y),
                note,
                font=self.font,
                fill=(180, 180, 180, 255)
            )

        return cv.cvtColor(np.array(pil_img.convert("RGB")), cv.COLOR_BGR2RGB)