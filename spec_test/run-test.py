import os
import subprocess
import sys
import glob

step = "large"
incremental = "true"
choice = "opt"
repair = True

timecmd = "/usr/bin/time"
timeout = 4*60 # minutes
delim = " & "
tmpdir = "tmp"
texfilename = "table.tex"
#testdirs = ["Kocher"]
testdirs = ["openssl"]
#testdirs = ["hacl-star"]
#testdirs = ["tmp/testdir"]
#testdirs = ["Kocher", "openssl", "hacl-star"]
iterations = 1
#test_placements = ["before-memory"]
test_placements = ["after-branch", "before-memory"]
#placements = ["every-inst", "after-branch", "before-memory"]

runtime_prefix = "runtime: "
maxRSS_prefix = "maxRSS: "
swapped_prefix = "swapped out: "

def run_single_test(llfile, placement, choice):
    basename = os.path.basename(llfile[:-len(".ll")])
    outfile = "{}/{}_{}_{}".format(tmpdir, basename, placement, choice)

    print("run on", llfile, "with fences at", placement, "and", choice)

    swap = subprocess.run(["swapon", "--show"], check=True, capture_output=True, text=True)
    if swap.stdout != "":
        print("\033[31mSwap enabled. Turn it off (sudo swapoff -va)!\033[0m", file=sys.stderr)

    repairfile = "--ofixed={}_fixed.ll".format(outfile) if repair else ""
    cmd = [timecmd, "-f", "{}%U + %S =? %e\n{}%M\n{}%W".format(runtime_prefix, maxRSS_prefix, swapped_prefix),
            "../build/run/bin/sea", "horn", "--solve",
            "--dsa=sea-cs",
#            "--ztrace=spacer",
#            "--bv-chc",
            "-o={}.smt2".format(outfile),
            "--oll={}.ll".format(outfile),
            repairfile,
            "--step={}".format(step), "--horn-answer",
            "--horn-tail-simplifier-pve=false", "--horn-subsumption=false",
            "--horn-inline-all",
            "--speculative-exe",
            "--insert-fences",
            "--fence-placement={}".format(placement),
            "--fence-choice={}".format(choice)]

    cmd.append("--horn-incremental-cover={}".format(incremental))
    cmd.append(llfile)

    try:
        p = subprocess.run(cmd, timeout=60*timeout, check=True, capture_output=True, text=True)
#        p = subprocess.run(cmd, timeout=60*timeout, check=True, capture_output=False, text=True)
    except subprocess.TimeoutExpired as e:
        print("Timeout ({}min) expired for {}!".format(timeout, llfile), file=sys.stderr)
        print("Timeout ({}min) expired!".format(timeout), file=open(outfile + ".err", "w"))
        out_str = e.stdout.decode()
        err_str = e.stderr.decode()
        print(out_str, file=open(outfile + ".out", "w"))
        print(err_str, file=sys.stderr)
        print(err_str, file=open(outfile + ".err", "w"))
        # kill the seahorn subprocess
        subprocess.run(["pkill", "seahorn"])
        return (-1, "---$\dagger$", "---")
    except Exception as e:
        out_str = e.stdout
        err_str = e.stderr
        print(out_str, file=open(outfile + ".out", "w"))
        print(err_str, file=sys.stderr)
        print(err_str, file=open(outfile + ".err", "w"))
        raise e

    print(p.stdout, file=open(outfile + ".out", "w"))

    # TODO: check stderr for errors
    print(p.stderr, file=sys.stderr)
    print(p.stderr, file=open(outfile + ".err", "w"))

    secure = False
    num_fences = 0

    for line in p.stdout.splitlines():
        if line.startswith("insert fence"):
            print("    " + line)
            num_fences += 1
        if line == "unsat":
            secure = True
            break

    for line in p.stderr.splitlines():
        if line.startswith(runtime_prefix):
            runtime = line[len(runtime_prefix):]
        if line.startswith(maxRSS_prefix):
            maxRSS = line[len(maxRSS_prefix):]
        if line.startswith("Program not secure"):
            print("  " + line, file=sys.stderr)
            return (-1, "---", "---")

    if not secure:
        print("Program still not secure", file=sys.stderr)
        return (-1, "---", "---")
    return (num_fences, runtime, maxRSS)


if sys.argv[1] == "--all":
    texfile = open("{}/{}".format(tmpdir, texfilename), "w")
    print("\\begin{tabular}{l", end="", file=texfile)
    for _ in test_placements:
        print("|cc", end="", file=texfile)
    print("}\n\\toprule", file=texfile)
    print("\\textbf{Benchmark}", end="", file=texfile)
    for placement in test_placements:
        print("", "\multicolumn{2}{c}{\\textbf{" + placement + "}}", sep=delim,
                end="", file=texfile)
    print("\\\\", file=texfile)
    for _ in test_placements:
        print("", "fences", "time", sep=delim, end="", file=texfile)
    print("\\\\", file=texfile)
    for d in testdirs:
        print("\\midrule", file=texfile)
        for test in sorted(glob.glob(d + "/*.ll")):
            for i in range(iterations):
                print(os.path.basename(test).replace("_", "\\_"), end="", file=texfile)
                for placement in test_placements:
                    (num_fences, runtime, maxRSS) = run_single_test(test, placement, choice)
                    if num_fences < 0:
                        num_fences = "---"
                    # TODO add maxRSS to table
                    print("", num_fences, runtime, sep=delim,
                            end="", file=texfile)
                print("\\\\", file=texfile)
    print("\\bottomrule\n\\end{tabular}", file=texfile)
else:
    if len(sys.argv) < 4:
        print(sys.argv[0])
        sys.exit("Script expects testfile, fence placement, and fence choice as arguments")
    llfile = sys.argv[1]
    placement = sys.argv[2]
    choice = sys.argv[3]

    if not llfile.endswith(".ll"):
        sys.exit("Input file must end with '.ll'")

#    if len(sys.argv) > 4 and sys.argv[4] == "--in-place-training":
#        cmd.append(sys.argv[4])
#        print("use in-place training")

    (num_fences, runtime, maxRSS) = run_single_test(llfile, placement, choice)
    if num_fences < 0:
        print(llfile + ": an error occured!", file=sys.stderr)
    else:
        print(llfile + ":", num_fences, "fences inserted,", runtime_prefix + runtime + "s,", maxRSS_prefix + maxRSS + "KiB")
