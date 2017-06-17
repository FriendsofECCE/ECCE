#
# README for the format of the .Q files:
#
# First We List the Classes - Queue Names
# The order they are listed does not currently guarantee that
# the queues will be listed in this order in the GUI.
Queues:			No_Class 

###############################################################################
# For Each Queue We Specify It's Attributes
# All values are optional except maxProcessors.
# As of 11/18/98, the following are not used (in any meaningful way):
#  description, administrator, maxSlots, priority, nice
###############################################################################
<class name> class
<class name>|maxSlots:       <unsigned int>
<class name>|priority:	     <string>
<class name>|nice:           <signed int>

<class name>|maxJobCPU:      <unsigned int> # seconds or hh:mm:ss
<class name>|maxProcessCPU:  <unsigned int> # seconds or hh:mm:ss
<class name>|runLimit:       <unsigned int> # minutes or hh:mm:ss
<class name>|minProcessors:  <unsigned int>
<class name>|maxProcessors:  <unsigned int>

<class name>|fileLimit:      <unsigned int> # KB
<class name>|coreLimit:      <unsigned int> # KB
<class name>|memLimit:       <unsigned int> # MB
<class name>|dataLimit:      <unsigned int> # MB
<class name>|stackLimit:     <unsigned int> # MB


###############################################################################
# Example:
###############################################################################
No_Class|maxSlots:		4
No_Class|priority:		30~
No_Class|nice:			0
No_Class|runLimit:		72:05:00
No_Class|minProcessors:		1
No_Class|maxProcessors:		82

