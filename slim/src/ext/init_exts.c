#include "lmntal.h"

void init_integer(void);
void init_nlmem(void);
void init_atomic(void);
void init_io(void);

void init_builtin_extensions(void)
{
  init_integer();
  init_nlmem();
  init_atomic();
  init_io();
}