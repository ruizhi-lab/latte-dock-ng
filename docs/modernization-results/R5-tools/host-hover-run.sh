#!/bin/bash
# Host-driven hover sweep (runs on the developer host): the guest pointer moves back and forth over the dock's
# icons through the QEMU tablet for the whole sampling window of one single-start scenario in the guest.
# The window starts about 34 s after the runner is launched (3 s settle, dock start, 30 s warmup), so the sweep
# starts 32 s after the launch and runs 66 s.
# usage: host-hover-run.sh plain|instrumented SCENARIO
set -eu
mode="$1"; scenario="$2"
vm=fedora@192.168.122.128
tools="$(dirname "$0")"
"$tools/host-pointer.sh" 960 540
sleep 1
if [[ "$mode" == instrumented ]]; then
    remote="~/latte-r5/tools/phase4-frames.sh $scenario"
else
    remote="python3 -I ~/latte-r5/tools/r5-scenario.py --scenario $scenario --revision \$(cat ~/latte-r5/src-main-eba91a3e6/SOURCE_REVISION) --visibility 0 --starts 1 --warmup 30 --seconds 60 --out ~/latte-r5/raw"
fi
ssh -o BatchMode=yes "$vm" "systemd-run --user --unit=latte-r5-series --collect --quiet bash -c '$remote'"
sleep 32
python3 -I "$tools/host-sweep.py" 66 1040 990 1390
while ssh -o BatchMode=yes "$vm" 'systemctl --user is-active --quiet latte-r5-series.service'; do sleep 3; done
echo "scenario $scenario finished"
