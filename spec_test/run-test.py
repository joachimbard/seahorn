import os
import subprocess
import sys
import glob
import shutil
import argparse

step = "large"
incremental = "true"
repair = True

timecmd = "/usr/bin/time"
timeout = 12*60 # minutes
delim = " & "
tmpdir = "tmp"
texfilename = "table.tex"
iterations = 1

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


def main():
#    global parser
    parser = argparse.ArgumentParser()
    parser.add_argument('--server', dest='server', default=False, action='store_true',
            help='Should be set when running on the server to move the generated files to ' +
            'permanent storage')
    parser.add_argument('-d', '--dirs', dest='testdirs', nargs='*',
            help='Analyze all benchmarks in the given directories')
    parser.add_argument('benchmarks', nargs='*', help='Analyze these testcases')
    parser.add_argument('-p', '--placement', dest='placements', nargs='+', required=True,
            choices=['before-memory', 'after-branch', 'every-inst'], help='Location of possible ' +
            'fences')
    parser.add_argument('-c', '--choice', dest='choices', nargs = '+', required=True,
            choices=['early', 'late', 'opt'], help='Strategy to choose fences')
    args = parser.parse_args()

    if args.testdirs != None:
        sys.exit(f'NOT IMPLEMENTD: Don\'t use neither "-d" nor "--dirs" ({args.testdirs})')
    for benchmark in args.benchmarks:
        if not benchmark.endswith('.ll'):
            print('Skipping', benchmark, 'because it does not end with ".ll"', file=sys.stderr)
            continue
        for placement in args.placements:
            for choice in args.choices:
                (num_fences, runtime, maxRSS) = run_single_test(benchmark, placement, choice)
                if num_fences < 0:
                    print(benchmark + ": an error occured!", file=sys.stderr)
                else:
                    print(benchmark + ":", num_fences, "fences inserted,", runtime_prefix +
                            runtime + "s,", maxRSS_prefix + maxRSS + "KiB")

    if args.server:
        # copy generated files in tmp to JOBDATADIR
        SLURM_JOB_ID = os.environ.get('SLURM_JOB_ID')
        JOBTMPDIR = f'/tmp/job-{SLURM_JOB_ID}'
        for file in glob.glob(tmpdir + '/*'):
            shutil.copy(file, JOBDATADIR)


if __name__ == '__main__':
    main()


#if sys.argv[1] == "--all":
#    texfile = open("{}/{}".format(tmpdir, texfilename), "w")
#    print("\\begin{tabular}{l", end="", file=texfile)
#    for _ in test_placements:
#        print("|cc", end="", file=texfile)
#    print("}\n\\toprule", file=texfile)
#    print("\\textbf{Benchmark}", end="", file=texfile)
#    for placement in test_placements:
#        print("", "\multicolumn{2}{c}{\\textbf{" + placement + "}}", sep=delim,
#                end="", file=texfile)
#    print("\\\\", file=texfile)
#    for _ in test_placements:
#        print("", "fences", "time", sep=delim, end="", file=texfile)
#    print("\\\\", file=texfile)
#    for d in testdirs:
#        print("\\midrule", file=texfile)
#        for test in sorted(glob.glob(d + "/*.ll")):
#            for i in range(iterations):
#                print(os.path.basename(test).replace("_", "\\_"), end="", file=texfile)
#                for placement in test_placements:
#                    (num_fences, runtime, maxRSS) = run_single_test(test, placement, choice)
#                    if num_fences < 0:
#                        num_fences = "---"
#                    # TODO add maxRSS to table
#                    print("", num_fences, runtime, sep=delim,
#                            end="", file=texfile)
#                print("\\\\", file=texfile)
#    print("\\bottomrule\n\\end{tabular}", file=texfile)
