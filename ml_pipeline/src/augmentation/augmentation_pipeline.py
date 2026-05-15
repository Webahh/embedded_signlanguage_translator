from src.gesture_generation.gesture import Gesture


class AugmentationPipeline:
    """
    Generates a pipeline that takes a Gesture and calls a number of augmentation functions on it.
    This pipeline will return the result of the Augmentation
    """

    def __init__(self):
        self.__pipeline = []

    def add(self, name: str, func, **kwargs):
        """Adds a step to the pipeline, that is applied for every gesture"""

        def wrapper(gesture: Gesture):
            return [(g, name) for g in func(gesture, **kwargs)]

        self.__pipeline.append(wrapper)

    def augment(self, gesture: Gesture) -> list[tuple[Gesture, str]]:
        """Returns list of (augmented_gesture, augmentation_name)"""
        results = [(gesture, "orig")]

        for func in self.__pipeline:
            new_results = []

            for g, label in results:
                augmented = func(g)
                for aug_g, aug_label in augmented:
                    combined_label = (
                        f"{label}+{aug_label}" if label != "orig" else aug_label
                    )
                    new_results.append((aug_g, combined_label))

            results.extend(new_results)

        return results
