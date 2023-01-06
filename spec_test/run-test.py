import os
import subprocess
import sys
import glob
import shutil
import argparse
import time

step = "large"
incremental = "true"
speculation_depth = 100
repair = True
debug = False

cooloff = 30 # seconds
timecmd = "/usr/bin/time"
timeout = 0

delim = " & "
tmpdir = "tmp"
texfilename = "table.tex"
iterations = 1

insert_fence_prefix = 'insert fence id '
runtime_prefix = "runtime [s]: "
maxRSS_prefix = "maxRSS [KiB]: "
swapped_prefix = "swapped out: "

def run_single_test(llfile, placement, choice):
    basename = os.path.basename(llfile[:-len(".ll")])
    outfile = "{}/{}_{}_{}".format(tmpdir, basename, placement, choice)

    print("run on", llfile, "with fences at", placement, "and", choice)

    swap = subprocess.run(["swapon", "--show"], check=True, capture_output=True, text=True)
    if swap.stdout != "":
        print("Swap enabled.", file=sys.stderr)

    repairfile = "--ofixed={}_fixed.ll".format(outfile) if repair else ""
    cmd = [timecmd, "-f", "{}%U + %S =? %e\n{}%M\n{}%W".format(runtime_prefix, maxRSS_prefix, swapped_prefix),
            "../build/run/bin/sea", "horn", "--solve",
#            "--dsa=sea-cs",
#            "--ztrace=spacer",
            "-o={}.smt2".format(outfile),
            "--oll={}.ll".format(outfile),
            repairfile,
            "--step={}".format(step), "--horn-answer",
            "--horn-tail-simplifier-pve=false", "--horn-subsumption=false",
            "--horn-inline-all",
            "--speculative-exe",
            '--speculation-depth={}'.format(speculation_depth),
            "--insert-fences",
            "--horn-incremental-cover={}".format(incremental),
            "--fence-placement={}".format(placement),
            "--fence-choice={}".format(choice),
            llfile]
#    if speculation_depth > 0:
#        cmd.append('--bv-chc')

    try:
        p = subprocess.run(cmd, timeout=60*timeout, check=True, capture_output=not debug, text=True)
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
        return (-1, 'timeout', '---')
    except Exception as e:
        out_str = e.stdout
        err_str = e.stderr
        print(out_str, file=open(outfile + ".out", "w"))
        print(err_str, file=sys.stderr)
        print(err_str, file=open(outfile + ".err", "w"))
        raise e

    if debug:
        return (-1, 'Analysis of result is not possible due to debug mode. Check yourself!', '---')

    print(p.stdout, file=open(outfile + ".out", "w"))

    # TODO: check stderr for errors
    print(p.stderr, file=sys.stderr)
    print(p.stderr, file=open(outfile + ".err", "w"))

    secure = False
    inserted_fences = list()

    print(llfile, 'with fences at', placement, 'and', choice, 'choice:')
    for line in p.stdout.splitlines():
        if line.startswith('speculation depth:') or line.startswith('number of') \
                or line.startswith('incremental cover:'):
            print('  ' + line)
        if line.startswith(insert_fence_prefix):
            inserted_fences.append(int(line[len(insert_fence_prefix):]))
        if line == 'unsat':
            secure = True
            print('  inserted fences ({}): {}'.format(len(inserted_fences), inserted_fences))
            break

    for line in p.stderr.splitlines():
        if line.startswith(runtime_prefix):
            runtime = line[len(runtime_prefix):]
            print('  ' + line)
        if line.startswith(maxRSS_prefix):
            maxRSS = float(line[len(maxRSS_prefix):]) / 1024.0
            print('  maxRSS [MiB]:', '{:.2f}'.format(maxRSS))
        if line.startswith('Program not secure'):
            print("  " + line, file=sys.stderr)
            return (-1, 'program not secure', '---')

    if not secure:
        print('Program still not secure', file=sys.stderr)
        return (-1, 'fences still missing', '---')
    return (len(inserted_fences), runtime, maxRSS)


def main():
#    global parser
    parser = argparse.ArgumentParser()
    parser.add_argument('--server', dest='server', default=False, action='store_true',
            help='Should be set when running on the server to move the generated files to ' +
            'permanent storage')
    parser.add_argument('--debug', dest='debug', default=False, action='store_true',
            help='In debug mode stdout and stderr are not captured but instead just printed')
    parser.add_argument('-d', '--dirs', dest='testdirs', nargs='*',
            help='Analyze all benchmarks in the given directories')
    parser.add_argument('benchmarks', nargs='*', help='Analyze these testcases')
    parser.add_argument('-p', '--placement', dest='placements', nargs='+', required=True,
            choices=['before-memory', 'after-branch', 'every-inst'], help='Location of possible ' +
            'fences')
    parser.add_argument('-c', '--choice', dest='choices', nargs = '+', required=True,
            choices=['early', 'late', 'opt'], help='Strategy to choose fences')
    parser.add_argument('-t', '--timeout', dest='timeout', type=int, required=True,
            help='Set the time limit in minutes')
    parser.add_argument('-s', '--speculation-depth', dest='speculation_depth', type=int,
            required=True, help='Set the time limit in minutes')
    args = parser.parse_args()

    if args.testdirs != None:
        sys.exit(f'NOT IMPLEMENTD: Don\'t use neither "-d" nor "--dirs" ({args.testdirs})')
    global timeout
    global speculation_depth
    global debug
    timeout = args.timeout
    speculation_depth = args.speculation_depth
    debug = args.debug
    for benchmark in args.benchmarks:
        if not benchmark.endswith('.ll'):
            print('Skipping', benchmark, 'because it does not end with ".ll"', file=sys.stderr)
            continue
        for placement in args.placements:
            for choice in args.choices:
                if args.server:
                    time.sleep(cooloff)
                (num_fences, runtime, maxRSS) = run_single_test(benchmark, placement, choice)
                if num_fences < 0:
                    print('the following error occured on', benchmark + ':', runtime,
                            file=sys.stderr)

    if args.server:
        # copy generated files to location which is stored onto permanent storage
        # Note: make sure that externally (i.e. by another program) the directory is actually stored
        save_dir = '/tmp/'
        if not os.path.isdir(save_dir):
            print(save_dir, 'not a directory')
        for file in glob.glob(tmpdir + '/*'):
            shutil.copy(file, save_dir)


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
