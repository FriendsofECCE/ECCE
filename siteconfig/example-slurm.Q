# Example queue definition file, for a Slurm cluster.
#
# NOT loaded by ECCE: nothing in the "Queues" registry points at it. It is
# here to be copied, because the only other examples of this format are
# chinook.Q and franklin.Q -- PNNL and NERSC machines decommissioned years
# ago, whose values (4800 processors, Moab) are not a useful starting point.
#
# To use it for your own cluster:
#
#   1. Register the machine first, in Machine Registration. A queue entry
#      naming a machine that is not registered is a FATAL error, not a
#      warning.
#   2. Copy this file to ~/.ECCE/<machine>.Q and edit the values.
#   3. Add the machine to ~/.ECCE/Queues:
#
#        Queues: <machine>
#
#        <machine>|queueMgrName:   Slurm
#        <machine>|prefFile:       <machine>.Q
#
#      Your entries are ADDED to the site ones, so the machines configured
#      site-wide stay available.
#
# Anything Slurm needs beyond these four numbers -- --account, --qos,
# --constraint, --gres=gpu:N, module loads -- is not configured here. It
# goes in the submit directive block in ~/.ECCE/CONFIG.<machine>; see
# siteconfig/submit.site for the template and GETTING_STARTED.md for the
# variable list.

# Every queue you want to appear in the launcher, space separated.
Queues:    normal debug

# Processor counts bound the launcher's own controls, so keep them honest.
normal|minProcessors:         1
normal|maxProcessors:         256
# Wall clock limit, in MINUTES. 1440 = 24 hours.
normal|runLimit:              1440
# Memory limit in MB; 0 means "no limit".
normal|memLimit:              0

debug|minProcessors:          1
debug|maxProcessors:          16
debug|runLimit:               30
debug|memLimit:               0
