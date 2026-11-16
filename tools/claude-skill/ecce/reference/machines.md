# Machines, codes and queues

The full text moved into AGENTS.md, section "Compute machines and HPC"
(installed as `<ECCE_HOME>/doc/AGENTS.md`, in the source tree
`docs/AGENTS.md`). It covers registering a machine and reaching an HPC
cluster through it, how ECCE logs in, what an agent can and cannot do on the
cluster (no command yet puts a file on a machine, #1002), setting up each code
on a machine (Quantum ESPRESSO pseudopotentials, ORCA, Gaussian, GROMACS,
CP2K), checking a machine, the settings files in `~/.ECCE`, and lessons from
real use (Slurm header, time and memory limits, Kebnekaise, CP2K versions).
