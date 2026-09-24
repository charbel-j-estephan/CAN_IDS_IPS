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


def load_model(path):
    import joblib
    bundle = joblib.load(path)
    return bundle["model"], bundle["class_names"]
