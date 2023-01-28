import sys
import os
import subprocess

timecmd = '/usr/bin/time'
timeout = 30 # seconds
delim = ','
tmpdir = 'tmp'
results_table_filename = 'perfomance.csv'

seed_filename = "seeds.txt"
runtime_prefix = "  runtime:"
clock_diff_prefix = "  clock diff:"

def get_seeds():
    seeds = [];
    with open(seed_filename, "r") as seed_file:
        for line in seed_file:
            if line.startswith("0x"):
                seed = line[:-1] # remove '\n'
                seeds.append(seed)
    return seeds

def run_single_testcase(testcase, seeds):
    if not os.access(testcase, os.X_OK):
        exit("file {} does not exist or is not executable".format(testcase))

    runtime = 0.0
    clock_diff = 0.0
    for seed in seeds:
#        print(seed + ":")
        cmd = [timecmd, "-f", runtime_prefix + "%e",
                "./{}".format(testcase), seed]

        try:
            p = subprocess.run(cmd, timeout=timeout, check=True, capture_output=True, text=True)
        except subprocess.TimeoutExpired:
            print("Timeout ({}s) expired for {}!".format(timeout, testcase), file=sys.stderr)
            return (-1, -1)

#        print(p.stderr, end="", file=sys.stderr)
        if p.stdout != "":
            print(p.stdout, file=sys.stdout)
        for line in p.stderr.splitlines():
            if line.startswith(runtime_prefix):
                runtime += float(line[len(runtime_prefix):])
            if line.startswith(clock_diff_prefix):
                clock_diff += float(line[len(clock_diff_prefix):])

    clock_diff = clock_diff / 1000
    print("overall runtime of {}: {:.2f}".format(testcase, runtime))
    print("overall clock diff of {}: {:.2f}".format(testcase,  clock_diff))
    return (runtime, clock_diff)

def run_benchmark(benchmark, iterations, seeds):
    # check for 'lfence' in non-fixed version
    non_fixed = f'{benchmark}_non-fixed'
    disassemble = ['objdump', '-D', non_fixed]
    p = subprocess.run(disassemble, capture_output=True, text=True)
    if 'lfence' in p.stdout:
        exit(f'Error: found "lfence" in {non_fixed} (which should not contain one)')

    result_table = open(f'{tmpdir}/{results_table_filename}', 'w')
    all_testcases = [
            f'{benchmark}_non-fixed',
            f'{benchmark}_after-branch_opt_fixed',
            f'{benchmark}_before-memory_opt_fixed']

    print('Benchmark', 'non-fixed', '', 'after-branch', '', 'before-memory', '',
            sep=delim, file=result_table)
    for _ in range(len(all_testcases)):
        print('', 'runtime', 'clock diff', sep=delim, end='', file=result_table)
    print(file=result_table)

    for _ in range(iterations):
        print(benchmark.split('/')[-1], end='', file=result_table)
        for testcase in all_testcases:
            (runtime, clock_diff) = run_single_testcase(testcase, seeds)
            if runtime < 0 or clock_diff < 0:
#                print('Error: negative runtime for', testcase, file=sys.stderr)
                exit('Error: negative runtime for' + testcase)
            print('', f'{runtime:.2f}', f'{clock_diff:.2f}', sep=delim, end='', file=result_table)
        print(file=result_table)

    result_table.close()

if (len(sys.argv) < 2):
    sys.exit("Script expects testfile as argument")

seeds = get_seeds()
#print("seeds:", seeds)
# TODO: arg parser
run_benchmark(sys.argv[1], 3, seeds)
