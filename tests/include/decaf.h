
#ifndef DECAF_H
#define DECAF_H

#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Assert that the condition is true.
 *
 * In cases where the symbolic executor determines that the
 * condition could be false, it will produce a test case with
 * concrete inputs which reproduce the failure.
 */
void decaf_assert(bool cond);

/**
 * Assume that a condition is true.
 *
 * This will silently remove any executions in which the condition
 * could evaluate to false.
 */
void decaf_assume(bool cond);

#ifdef __cplusplus
}
#endif

#endif
