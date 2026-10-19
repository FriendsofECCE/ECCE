"""Persist / restore a workflow node's output (atoms + box + topology metadata) so the
web module can re-run a *selection* of nodes without recomputing everything upstream.

Enabled only when the ``ATOMIPY_NODE_CACHE`` environment variable points at a writable
directory (the web build subprocess sets it); otherwise :func:`save_node_state` is a silent
no-op. Freshness is content-addressed: each save records a hash of the node's configuration
and all of its ancestors, and :func:`load_node_state` refuses a cache whose hash no longer
matches — so a changed upstream (or a same-id node from a different workflow) errors clearly
instead of feeding stale data into a partial re-run.
"""
import os
import pickle


class NodeStateList(list):
    """A plain list that can also carry topology metadata (``itp`` / ``_top_path`` /
    ``_defines`` / ``_mol_counts_override``), so a reloaded node output behaves exactly
    like the in-memory one produced during a normal run."""
    pass


def _cache_dir():
    return os.environ.get("ATOMIPY_NODE_CACHE") or None


def save_node_state(node_id, atoms, box, state_hash=None):
    """Pickle a node's output to ``<ATOMIPY_NODE_CACHE>/<node_id>.pkl``.

    No-op (and never raises) if the cache is disabled or the write fails — caching must
    never break a build. Writes atomically via a temp file + ``os.replace``.
    """
    d = _cache_dir()
    if not d:
        return
    try:
        os.makedirs(d, exist_ok=True)
        payload = {
            "atoms": list(atoms) if atoms is not None else [],
            "box": box,
            "hash": state_hash,
            "itp": getattr(atoms, "itp", None),
            "_top_path": getattr(atoms, "_top_path", None),
            "_defines": getattr(atoms, "_defines", None),
            "_mol_counts_override": getattr(atoms, "_mol_counts_override", None),
        }
        tmp = os.path.join(d, f".{node_id}.tmp")
        with open(tmp, "wb") as f:
            pickle.dump(payload, f, protocol=pickle.HIGHEST_PROTOCOL)
        os.replace(tmp, os.path.join(d, f"{node_id}.pkl"))
    except Exception as e:
        print(f"(node-cache: save skipped for {node_id}: {e})")


def load_node_state(node_id, expected_hash=None):
    """Return ``(atoms, box)`` for a previously cached node.

    Raises with an actionable message if the cache is disabled, the entry is missing, or it
    is STALE (its recorded hash != ``expected_hash``, i.e. that node or an ancestor changed
    since it was cached).
    """
    d = _cache_dir()
    if not d:
        raise RuntimeError("Node-state cache is not enabled (ATOMIPY_NODE_CACHE unset).")
    path = os.path.join(d, f"{node_id}.pkl")
    if not os.path.exists(path):
        raise RuntimeError(
            f"No cached output for upstream node '{node_id}'. Run the workflow up to it "
            f"once so its result is cached, then re-run the selection."
        )
    with open(path, "rb") as f:
        payload = pickle.load(f)
    if expected_hash is not None and payload.get("hash") != expected_hash:
        raise RuntimeError(
            f"Cached output for upstream node '{node_id}' is STALE (that node or something "
            f"above it changed since it last ran). Re-run the workflow up to it first."
        )
    atoms = NodeStateList(payload.get("atoms") or [])
    for attr in ("itp", "_top_path", "_defines", "_mol_counts_override"):
        if payload.get(attr) is not None:
            setattr(atoms, attr, payload[attr])
    return atoms, payload.get("box")
