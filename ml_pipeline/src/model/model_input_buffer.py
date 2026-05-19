from src.model.model_input import ModelInput
from src.model.model import Model
import numpy as np
import atomics
import threading
import time


class AtomicAltar:
    """A thread save method to send a single piece of data between threads."""

    def __init__(self):
        """Initializes an empty atomic altar"""
        self._gate = atomics.atomic(width=4, atype=atomics.UINT)
        self._gate.store(0)
        self._altar = None

    def destroy(self):
        """Destroys the atomic altar"""
        self._gate.store(39)

    def is_destroyed(self):
        """Checks whether or not the altar is destroyed"""
        return self._gate.load() == 39

    def offering_available(self):
        """Checks whether or not there is an offering available to consume"""
        return self._gate.load() == 1

    def is_empty(self):
        """Checks whether or not the altar is empty"""
        return self._gate.load() == 0

    def offer(self, v):
        """Puts 'v' on the altar. Blocks until the altar is empty, before offering 'v'"""
        while self._gate.load() == 1:
            time.sleep(0.0001)

        self._altar = v
        self._gate.store(1)

    def consume(self):
        """Takes the offered value from the altar, blocking if no offering is available"""
        while self._gate.load() == 0:
            time.sleep(0.0001)

        val = self._altar
        self._altar = None
        self._gate.store(0)

        return val


class ModelInputBuffer:
    def __init__(
        self,
        model: Model,
        callback: callable,
    ):
        self._buffer = np.zeros(shape=2 * 22 * 2, dtype=np.int16)
        self._model = model
        self._altar = AtomicAltar()

        def infer_worker(altar: AtomicAltar):
            while True:
                if altar.is_destroyed():
                    break

                buffer = altar.consume()

                if buffer is None:
                    continue

                result = self._model.infer(buffer)
                callback(result)

        threading.Thread(target=lambda: infer_worker(self._altar), daemon=True).start()

    def push_all(self, inputs: [ModelInput]):
        for inp in inputs:
            self.push(inp)

    def push(self, input: ModelInput):
        old_buffer = self._buffer
        self._buffer = input.flattened()

        # Check if the model needs a new buffer
        if self._altar.is_empty():
            self._altar.offer(old_buffer)

    def destroy(self):
        self._altar.destroy()
