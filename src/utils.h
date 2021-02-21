#ifndef __utils_inc
#define __utils_inc

#include <stdio.h>

#define EPRINTF(T, M, ...)                                                                                             \
  do                                                                                                                   \
  {                                                                                                                    \
    fprintf(stderr, "[%s:%d] " T ": " M "\n", __FILE__, __LINE__, ##__VA_ARGS__);                                      \
  } while (0)

#define ASSERTF(E, ...)                                                                                                \
  do                                                                                                                   \
  {                                                                                                                    \
    if (!E)                                                                                                            \
    {                                                                                                                  \
      EPRINTF("ERROR", ##__VA_ARGS__);                                                                                 \
      exit(1);                                                                                                         \
    }                                                                                                                  \
  } while (0)

#define MUST_INIT(E, M) ASSERTF(E, "Couldn't initialize %s.", M)

// clang-format off
#define UNUSED(x) (void)(x)
#define UNUSED2(x1,x2) UNUSED(x1); UNUSED(x2)
#define UNUSED3(x1, x2, x3) UNUSED2(x1, x2); UNUSED(x3)
#define UNUSED4(x1, x2, x3, x4) UNUSED3(x1, x2, x3); UNUSED(x4)
#define UNUSED5(x1, x2, x3, x4, x5) UNUSED4(x1, x2, x3, x4); UNUSED(x5)
#define UNUSED6(x1, x2, x3, x4, x5, x6) UNUSED5(x1, x2, x3, x4, x5); UNUSED(x6)
// clang-format on

#endif
