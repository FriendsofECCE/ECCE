"""Local GROMACS engine for atomipy (detect gmx, generate .mdp, run grompp/mdrun).

This subpackage is engine-only; structure/topology come from the usual atomipy
writers. Entry points:

    from atomipy.gromacs import detect_gmx, stage_run_dir, run_stage, run_pipeline
    from atomipy.gromacs import mdp, build_defines
"""

from .mdp import mdp, build_defines
from .runner import (
    detect_gmx, stage_run_dir, stage_minff, run_stage, run_pipeline, run_local_gmx,
    trjconv_to_pdb, trjconv, energy_timeseries, write_freeze_ndx,
)

__all__ = [
    "mdp", "build_defines", "detect_gmx", "stage_run_dir", "stage_minff",
    "run_stage", "run_pipeline", "run_local_gmx", "trjconv_to_pdb", "trjconv",
    "energy_timeseries", "write_freeze_ndx",
]
