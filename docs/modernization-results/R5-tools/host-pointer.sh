#!/bin/bash
# Place the Fedora guest's pointer with the QEMU usb-tablet (absolute axes 0..32767), from the host.
# usage: host-pointer.sh PIXEL_X PIXEL_Y   (the guest screen is 1920x1080)
# The tablet only reports a changed position. If something else moved the pointer since the
# last placement, a repeated target would produce no event, so nudge by one pixel first.
# Verify the result with kwin-probe.sh (cursor=X,Y); this script cannot read it back.
set -eu
send() {
    local x=$(( $1 * 32767 / 1919 )) y=$(( $2 * 32767 / 1079 ))
    virsh -c qemu:///system qemu-monitor-command fedora \
      "{\"execute\":\"input-send-event\",\"arguments\":{\"events\":[{\"type\":\"abs\",\"data\":{\"axis\":\"x\",\"value\":$x}},{\"type\":\"abs\",\"data\":{\"axis\":\"y\",\"value\":$y}}]}}" >/dev/null
}
send $(( $1 + 3 )) $(( $2 + 3 ))
sleep 0.3
send "$1" "$2"
