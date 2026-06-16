import threading
import time
from dataclasses import dataclass
from typing import Optional

import cv2 as cv


@dataclass
class LatestFrame:
    ok: bool
    frame: Optional[object]


class ThreadedVideoCapture:
    def __init__(self, source):
        self.cap = cv.VideoCapture(source)
        self.lock = threading.Lock()
        self.latest = LatestFrame(False, None)
        self.running = False
        self.thread = None

    def set(self, prop, value):
        self.cap.set(prop, value)

    def isOpened(self):
        return self.cap.isOpened()

    def start(self):
        self.running = True
        self.thread = threading.Thread(target=self._reader, daemon=True)
        self.thread.start()
        return self

    def _reader(self):
        while self.running:
            ok, frame = self.cap.read()
            with self.lock:
                self.latest = LatestFrame(ok, frame)

            if not ok:
                time.sleep(0.01)

    def read(self):
        with self.lock:
            return self.latest.ok, None if self.latest.frame is None else self.latest.frame.copy()

    def release(self):
        self.running = False
        if self.thread is not None:
            self.thread.join(timeout=1.0)
        self.cap.release()
