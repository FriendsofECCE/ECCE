# Queue definition for the "dummy" machine -- the one ECCE never contacts
# (issue #94).
#
# These numbers do not describe any real cluster, because ECCE cannot know
# which one you will copy the job to. They exist so the launcher has
# something to bound its processor and time-limit controls with, and so the
# generated submit script carries plausible directives you can edit.
#
# Adjust for your actual machine by copying this to ~/.ECCE/dummy.Q; see
# example-slurm.Q for the format and the per-user override recipe.

Queues:    normal

normal|minProcessors:         1
normal|maxProcessors:         256
# Wall clock limit in MINUTES. 1440 = 24 hours.
normal|runLimit:              1440
# Memory limit in MB; 0 means "no limit".
normal|memLimit:              0
normal|memUnits:              MB
