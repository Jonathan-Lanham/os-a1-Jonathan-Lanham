#include "kernel/types.h"
#include "kernel/sysinfo.h"
#include "kernel/fcntl.h"
#include "user/user.h"

static void
check(int condition, char *message)
{
  if(!condition){
    fprintf(2, "FAIL: %s\n", message);
    exit(1);
  }
}

static struct sysinfo
getinfo(void)
{
  struct sysinfo info;
  check(sysinfo(&info) == 0, "sysinfo call");
  return info;
}

int
main(void)
{
  struct sysinfo before, after;

  // Test 1: Basic sysinfo
  before = getinfo();
  printf("PASS: basic sysinfo\n");

  // Test 2: Invalid pointer
  check(sysinfo((struct sysinfo *)-1) == -1,
        "invalid pointer");
  printf("PASS: invalid pointer\n");

  // Test 3: Memory allocation
  before = getinfo();

  char *mem = sbrk(4096);
  check(mem != SBRK_ERROR, "sbrk allocation");

  after = getinfo();
  check(after.freemem < before.freemem,
        "free memory must decrease");

  check(sbrk(-4096) != SBRK_ERROR,
        "sbrk release");

  struct sysinfo released = getinfo();
  check(released.freemem > after.freemem,
        "free memory must increase after release");

  printf("PASS: memory allocation and release\n");

  // Test 4: Open and close
  before = getinfo();

  int fd = open("README", O_RDONLY);
  check(fd >= 0, "open README");

  after = getinfo();
  check(after.nopenfile == before.nopenfile + 1,
        "open file count");

  check(close(fd) == 0, "close README");

  after = getinfo();
  check(after.nopenfile == before.nopenfile,
        "close file count");

  printf("PASS: open and close counts\n");

  // Test 5: Pipe allocation
  before = getinfo();

  int fds[2];
  check(pipe(fds) == 0, "pipe creation");

  after = getinfo();
  check(after.nopenfile == before.nopenfile + 2,
        "pipe must allocate two file entries");

  close(fds[0]);
  close(fds[1]);

  after = getinfo();
  check(after.nopenfile == before.nopenfile,
        "pipe entries must be released");

  printf("PASS: pipe and close counts\n");

  // Test 6: Fork and wait
  int ready[2], done[2];

  check(pipe(ready) == 0, "ready pipe");
  check(pipe(done) == 0, "done pipe");

  before = getinfo();

  int pid = fork();
  check(pid >= 0, "fork");

  if(pid == 0){
    char c;

    close(ready[0]);
    close(done[1]);

    check(write(ready[1], "x", 1) == 1,
          "child ready signal");
    check(read(done[0], &c, 1) == 1,
          "child release signal");

    close(ready[1]);
    close(done[0]);
    exit(0);
  }

  close(ready[1]);
  close(done[0]);

  char c;
  check(read(ready[0], &c, 1) == 1,
        "parent ready signal");

  after = getinfo();
  check(after.nproc == before.nproc + 1,
        "process count after fork");

  check(write(done[1], "x", 1) == 1,
        "parent release signal");

  int status;
  check(wait(&status) == pid, "wait");
  check(status == 0, "child exit status");

  close(ready[0]);
  close(done[1]);

  after = getinfo();
  check(after.nproc == before.nproc,
        "process count after wait");

  printf("PASS: fork and wait counts\n");

  printf("ALL SYSINFO TESTS PASSED\n");
  exit(0);
}