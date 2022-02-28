// RUN: %sea smt %s --step=small -o %t.sm.smt2
// RUN: %z3 %t.sm.smt2 fp.spacer.order_children=2 2>&1 | OutputCheck %s
//
// RUN: %sea smt %s --step=small --inline -o %t.sm.inline.smt2
// RUN: %z3 %t.sm.inline.smt2 fp.spacer.order_children=2 2>&1 | OutputCheck %s
//
// RUN: %sea smt %s --step=large -o %t.lg.smt2
// RUN: %z3 %t.lg.smt2 fp.spacer.order_children=2 2>&1 | OutputCheck %s
//
// RUN: %sea smt %s --step=large --inline -o %t.lg.inline.smt2
// RUN: %z3 %t.lg.inline.smt2 fp.spacer.order_children=2 2>&1 | OutputCheck %s
//
// CHECK: ^unsat$

#include "seahorn/seasynth.h"

extern int nd1();
extern int nd2();

extern bool infer(int k);
bool PARTIAL_FN inv(int k) { return infer(k); }

// When the loop in this program terminates, k_i = k_0 - n.
// Since k_0 > n, then k_i > 0.
// Therefore, the synthesis assertion is never reached and inv(k) can be set to false.
// Consequently, the program is safe, and correctness relies on the loop invariant.
//
// This test is meant to ensure that a solver handles false invariants.
int main(void)
{
  int n = nd1();
  assume(n > 0);
  int k = nd2();
  assume(k > n);
  int j = 0;

  // After i iterations, i = j <= n, and k_i = k_0 - i >= k_0 - n > 0.
  while (j < n)
  {
    j = j + 1;
    k = k - 1;
  }

  // At this point k_i = k_0 - n > 0.
  if (k < 0)
  {
    k = nd3();
    sassert(inv(k));
  }

  // inv can be set to false so this is safe.
  assume(inv(k));
  sassert(false);
  return 0;
}
