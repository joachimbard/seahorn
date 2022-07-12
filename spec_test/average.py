import sys
#import time

delim = " & "

if len(sys.argv) < 3:
    sys.exit("input and output file exptected")
intable = open(sys.argv[1], "r")
outtable = open(sys.argv[2], "w")
prev_testcase = ""
fences_every_inst = 0
fences_branch = 0
fences_mem = 0
sum_branch = 0
sum_every_inst = 0
sum_mem = 0

for l in intable:
    cols = l.split(delim)
    testcase = cols[0]
    if testcase.endswith(".ll"):
        if testcase != prev_testcase:
            # new testcase
            if prev_testcase != "":
                print(prev_testcase,
                        fences_every_inst, "{:.2f}".format(sum_every_inst / iterations),
                        fences_branch, "{:.2f}".format(sum_branch / iterations),
                        fences_mem, "{:.2f}".format(sum_mem / iterations),
                        sep=delim, end="\\\\\n", file=outtable)
            prev_testcase = testcase
            iterations = 1
            fences_every_inst = cols[1]
            sum_every_inst = float(cols[2])
            fences_branch = cols[3]
            sum_branch = float(cols[4])
            fences_mem = cols[5]
            sum_mem = float(cols[6][:-3])
        else:
            # same testcase as before
            iterations += 1
            if fences_every_inst != cols[1] or fences_branch != cols[3] or fences_mem != cols[5]:
                print("number of fences differ for", testcase + ": expected",
                        fences_every_inst, fences_branch, fences_mem, "but got",
                        cols[1], cols[3], cols[3], file=sys.stderr)
            sum_every_inst += float(cols[2])
            sum_branch += float(cols[4])
            sum_mem += float(cols[6][:-3])
    else:
        if prev_testcase != "":
            print(prev_testcase,
                    fences_every_inst, "{:.2f}".format(sum_every_inst, iterations),
                    fences_branch, "{:.2f}".format(sum_branch / iterations),
                    fences_mem, "{:.2f}".format(sum_mem / iterations),
                    sep=delim, end="\\\\\n", file=outtable)
            prev_testcase = ""
        print(l, end="", file=outtable)
