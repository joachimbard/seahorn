import sys
import os
import subprocess

timecmd = "/usr/bin/time"
timeout = 30 # seconds

def get_seeds():
    seeds = [];
    seed_file = open("seeds.txt", "r")
    for line in seed_file:
        if line.startswith("0x"):
            seed = line[:-1] # remove '\n'
            seeds.append(seed)
    return seeds

def run_single(testcase, seeds):
    if not os.access(testcase, os.X_OK):
        exit("file {} does not exist or is not executable".format(testcase))

    runtime = 0.0
    clock_diff = 0.0
    for seed in seeds:
        cmd = [timecmd, "-f", "runtime:%e",
                "./{}".format(testcase), seed]

        try:
            p = subprocess.run(cmd, timeout=timeout, check=True, capture_output=True, text=True)
        except subprocess.TimeoutExpired:
            print("Timeout ({}s) expired for {}!".format(timeout, testcase), file=sys.stderr)
            return -1

        print(p.stderr, file=sys.stderr)
        print(p.stdout, file=sys.stdout)
        for line in p.stderr.splitlines():
            if line.startswith("runtime:"):
                runtime += float(line[len("runtime:"):])
            if line.startswith("clock diff:"):
                clock_diff += float(line[len("clock diff:"):])

    print("overall runtime of {}: {:.2f}".format(testcase, runtime))
    print("overall clock diff of {}: {:.2f}".format(testcase, clock_diff / 1000))

if (len(sys.argv) < 2):
    sys.exit("Script expects testfile as argument")

seeds = get_seeds()
print("seeds:", seeds)
run_single(sys.argv[1], seeds)
