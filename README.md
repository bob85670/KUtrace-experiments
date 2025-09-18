# KUtrace-experiments

Make a trace
From the github KUtrace code,
$ cd postproc
$ ./kutrace_control

At the prompt, type "goipc"
Then enter.
and at the next prompt, type "stop"
Then enter

go       starts a trace
goipc    starts a trace that also includes instructions-per-cycle for each timespan
gollc    starts a trace that also includes last-level cache misses for each timespan
goipcllc does both (ipc and llc are in alphabetic order here)
stop     stops a trace and writes the raw trace binary to a disk file

This will write a raw trace file with a name like
  ku_20240709_152922_dclab-2_11686.trace

$ $ chmod +x "the postproc3.sh file"
$ ./postproc3.sh ku_20240709_152922_dclab-2_11686.trace "Caption here"

This will produce
  ku_20240709_152922_dclab-2_11686.json and
  ku_20240709_152922_dclab-2_11686.html
