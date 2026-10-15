#include "kernel/types.h"
#include "kernel/uproc.h"
#include "kernel/param.h"
#include "user/user.h"

int
main(void)
{
  struct uproc procs[NPROC];
  int n = getprocs(procs, NPROC);

  if (n < 0) {
    fprintf(2, "ps: getprocs failed\n");
    exit(1);
  }

  printf("PID\tPPID\tSTATE\tSIZE\tNAME\n");

  for (int i = 0; i < n; i++) {
    char *state;

    switch (procs[i].state) {
    case 1:
      state = "used";
      break;
    case 2:
      state = "sleeping";
      break;
    case 3:
      state = "runnable";
      break;
    case 4:
      state = "running";
      break;
    case 5:
      state = "zombie";
      break;
    default:
      state = "unused";
      break;
    }

    printf("%d\t%d\t%s\t%lu\t%s\n", procs[i].pid, procs[i].ppid, state,
           procs[i].sz, procs[i].name);
  }

  exit(0);
}