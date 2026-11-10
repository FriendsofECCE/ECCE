"""Common COSMO solvent choices shared by the theory dialogs and writers."""
import os


def solvent_choices():
    with open(os.path.join(os.path.dirname(__file__), "cosmo_solvents.dat")) as stream:
        return [line.strip() for line in stream
                if line.strip() and not line.startswith("#")]
