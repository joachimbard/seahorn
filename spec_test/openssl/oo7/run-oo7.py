import os
import subprocess
import sys

timecmd = "/usr/bin/time"
timeout = 2*60 # minutes
#delim = " & "
#tmpdir = "tmp"
#texfilename = "table.tex"
iterations = 1

runtime_prefix = "runtime: "
maxRSS_prefix = "maxRSS: "
swapped_prefix = "swapped out: "

def run_single_test(file):
    print("run oo7 on", file)

    swap = subprocess.run(["swapon", "--show"], check=True, capture_output=True, text=True)
    if swap.stdout != "":
        print("\033[31mSwap enabled. Turn it off (sudo swapoff -va)!\033[0m", file=sys.stderr)

#    cmd = [timecmd, "-f", "{}%U + %S =? %e\n{}%M".format(runtime_prefix, maxRSS_prefix),
#           'bash -c "(bap {} --recipe=check && python2 ~/PhD/oo7/tool/incidents_profile.py incidents {}.asm)"'.format(file, file)
#          ]
    cmd = '{0} -f "{2}%U + %S =? %e\n{3}%M\n{4}%W" bash -c "(bap {1} --recipe=check && python2 ~/PhD/oo7/tool/incidents_profile.py incidents {1}.asm)"'.format(timecmd, file, runtime_prefix, maxRSS_prefix, swapped_prefix)

    try:
#        p = subprocess.run(cmd, timeout=60*timeout, check=True, capture_output=True, text=True)
        p = subprocess.run(cmd, shell=True, timeout=60*timeout, check=True, capture_output=False, text=True)
    except subprocess.TimeoutExpired as e:
        print("Timeout ({}min) expired for {}!".format(timeout, file), file=sys.stderr)
        err_str = e.stderr.decode()
        print(err_str, file=sys.stderr)
        # kill all subprocesses
        subprocess.run(["pkill", "bap"])
        return ("---$\dagger$", "---")
    except Exception as e:
        err_str = e.stderr
        print(err_str, file=sys.stderr)
        raise e

    subprocess.run("mv incidents incidents-{}.txt".format(file), shell=True, check=True, text=True)
    subprocess.run("mv incidents_profile.txt incidents_profile-{}.txt".format(file), shell=True, check=True, text=True)

#    for line in p.stderr.splitlines():
#        if line.startswith(runtime_prefix):
#            runtime = line[len(runtime_prefix):]
#        if line.startswith(maxRSS_prefix):
#            maxRSS = line[len(maxRSS_prefix):]
#
#    return (runtime, maxRSS)

    return ("", "")


if len(sys.argv) < 2:
    print(sys.argv[0])
    sys.exit("Script expects testfile")
file = sys.argv[1]
asmfile = file + ".asm"

if not os.access(file, os.X_OK):
    exit("file {} does not exist or is not executable".format(file))
if not os.access(asmfile, os.R_OK):
    exit("file {} does not exist or is not readable".format(asmfile))

(runtime, maxRSS) = run_single_test(file)
