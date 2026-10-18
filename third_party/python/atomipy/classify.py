"""
atomipy.classify — public access to the system-component classifier.

`classify_atom(atom, mineral_types=None)` returns one of 'water', 'ion',
'mineral', or 'organic' for a single atomipy atom dict. The implementation
lives in atomipy.composition; this module re-exports it as a stable, lightweight
import location (`from atomipy.classify import classify_atom`).
"""

from .composition import classify_atom, system_component_flags

__all__ = ["classify_atom", "system_component_flags"]
