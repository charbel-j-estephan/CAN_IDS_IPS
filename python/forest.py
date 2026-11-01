"""Shared helpers for the hardware friendly view of a trained forest."""
import numpy as np


def hard_vote_predict(model, X):
    """Majority vote over trees, ties go to the lowest class index.

    scikit learn's predict() averages probabilities (soft vote). The FPGA
    module and emlearn's C code both count one vote per tree, so this is the
    reference both of them must match exactly. Returns class indices into
    model.classes_.
    """
    X = np.asarray(X, dtype=np.float32)
    n_classes = len(model.classes_)
    votes = np.zeros((len(X), n_classes), dtype=np.int32)
    rows = np.arange(len(X))
    for tree in model.estimators_:
        votes[rows, np.argmax(tree.predict_proba(X), axis=1)] += 1
    return np.argmax(votes, axis=1)


FUZZY = 2   # dataset.CLASS_NAMES.index("Fuzzy")


def known_id_rule(pred, can_ids, known_ids):
    """The forest's verdict, except that Normal on an ID the car never sent in
    the training traffic becomes Fuzzy.

    With 9 comparisons per tree a forest cannot single out the car's 38 IDs
    among 2048, so most Fuzzy frames it misses carry an ID the car never uses.
    The STM32 and the FPGA check the ID in a 2048 bit table, known after the
    12th bit of the frame. Other verdicts stay: DoS (ID 0x000, also not a car
    ID) keeps its class."""
    pred = np.array(pred, copy=True)
    pred[(pred == 0) & ~np.isin(can_ids, known_ids)] = FUZZY
    return pred


def load_model(path):
    import joblib
    bundle = joblib.load(path)
    return bundle["model"], bundle["class_names"]
