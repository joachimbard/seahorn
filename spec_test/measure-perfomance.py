import sys
import os
import subprocess
import argparse
import time

timecmd = '/usr/bin/time'
timeout = 30 # seconds
cooloff = 60 # seconds
delim = ','
tmpdir = 'tmp'
results_table_filename = 'perfomance.csv'
default_iterations = 1

seed_filename = "seeds.txt"
runtime_prefix = "  runtime:"
clock_diff_prefix = "  clock diff:"

standard_benchmarks = [
        'openssl/fixed/openssl-aes_cbc_encrypt_ct',
        'openssl/fixed/openssl-aes_cbc_encrypt_non-ct',
        'hacl-star/fixed/Hacl_Chacha20_encrypt',
        'hacl-star/fixed/Hacl_Poly1305_32_mac'
        ]

all_testcase_suffixes = [
        '_non-fixed',
        '_after-branch_opt_fixed',
        '_before-memory_opt_fixed'
        ]


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

        print(p.stderr, end="", file=sys.stderr)
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

def run_benchmark(benchmark, iterations, seeds, result_table):
    # check for 'lfence' in non-fixed version
    non_fixed = f'{benchmark}_non-fixed'
    disassemble = ['objdump', '-D', non_fixed]
    p = subprocess.run(disassemble, capture_output=True, text=True)
    if 'lfence' in p.stdout:
        exit(f'Error: found "lfence" in {non_fixed} (which should not contain one)')

    for _ in range(iterations):
        print(benchmark.split('/')[-1], end='', file=result_table)
        for suffix in all_testcase_suffixes:
            if cooloff > 0:
                time.sleep(cooloff)
            (runtime, clock_diff) = run_single_testcase(benchmark + suffix, seeds)
            if runtime < 0 or clock_diff < 0:
#                print('Error: negative runtime for', testcase, file=sys.stderr)
                exit('Error: negative runtime for' + testcase)
#            print('', f'{runtime:.2f}', f'{clock_diff:.2f}', sep=delim, end='', file=result_table)
            print('', f'{clock_diff:.2f}', sep=delim, end='', file=result_table)
        print(file=result_table)



def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--all', dest='all', default=False, action='store_true',
            help='Run all standard benchmarks additionally to the explicit given ones')
    parser.add_argument('benchmarks', nargs='*', help='Analyze all testcases of these benchmarks')
    parser.add_argument('--single', dest='single', help='Run the single testcase')
    parser.add_argument('-i', '--iterations', dest='iterations', default=default_iterations,
            type=int, help='Specify how often a benchmark is tested')
    args = parser.parse_args()

    seeds = get_seeds()
#    print("seeds:", seeds)
    if args.single:
        run_single_testcase(args.single, seeds)
        return

    test_benchmarks = args.benchmarks
    if args.all:
        test_benchmarks.extend(standard_benchmarks)

    with open(f'{tmpdir}/{results_table_filename}', 'w') as result_table:
#        print('Benchmark', 'non-fixed', '', 'after-branch', '', 'before-memory', '',
        print('Benchmark', 'non-fixed', 'after-branch', 'before-memory',
                sep=delim, file=result_table)
        for _ in range(len(all_testcase_suffixes)):
#            print('', 'runtime', 'clock', sep=delim, end='', file=result_table)
            print('', 'clock', sep=delim, end='', file=result_table)
        print(file=result_table)

        for benchmark in test_benchmarks:
            run_benchmark(benchmark, args.iterations, seeds, result_table)


if __name__ == '__main__':
    main()
