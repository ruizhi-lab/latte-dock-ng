# R5 helper, loaded by gdb: statistical profile of one thread with a single attach (no perf, no root).
#   gdb -p PID -batch -x gdb-sampler.py   (environment: R5_GDB_OUT, R5_GDB_SECONDS, R5_GDB_THREAD)
# Repeats: resume the process, let it run for a random 40-200 ms, interrupt, print the stack of thread
# R5_GDB_THREAD (1 is the main thread). A thread that is busy x percent of the time shows up in about
# x percent of the samples, so the share of busy samples under a function is its share of the CPU time.
import os
import random
import time

import gdb

out = open(os.environ.get("R5_GDB_OUT", "/tmp/r5-gdb-samples.txt"), "w")
seconds = float(os.environ.get("R5_GDB_SECONDS", "120"))
thread_num = int(os.environ.get("R5_GDB_THREAD", "1"))
gdb.execute("set pagination off")
gdb.execute("set confirm off")
deadline = time.time() + seconds
count = 0
while time.time() < deadline:
    gdb.execute("continue&", to_string=True)
    time.sleep(random.uniform(0.04, 0.2))
    gdb.execute("interrupt", to_string=True)
    for _ in range(400):                      # wait up to 2 s for the stop to be reported
        if not any(t.is_running() for t in gdb.selected_inferior().threads()):
            break
        time.sleep(0.005)
    else:
        continue
    count += 1
    out.write(f"=== sample {count} {time.time():.6f}\n")
    try:
        gdb.execute(f"thread {thread_num}", to_string=True)
        out.write(gdb.execute("bt 40", to_string=True))
    except gdb.error as error:
        out.write(f"# error {error}\n")
    out.flush()
out.write(f"=== done {count} samples\n")
out.close()
gdb.execute("detach")
gdb.execute("quit")
