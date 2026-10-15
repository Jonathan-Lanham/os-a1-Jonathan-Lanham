#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"

static char line[512];
static char storage[MAXARG][512];
static char *words[MAXARG];

int
run_command(char *extra[], int extra_count, char *argv[], int start)
{
  char *args[MAXARG + 1];
  int count = 0;

  for (int i = start; argv[i] != 0; i++)
    args[count++] = argv[i];

  if (count + extra_count > MAXARG) {
    fprintf(2, "xargs: too many arguments\n");
    return 1;
  }

  for (int i = 0; i < extra_count; i++)
    args[count++] = extra[i];

  args[count] = 0;

  int pid = fork();
  if (pid < 0) {
    fprintf(2, "xargs: fork failed\n");
    return 1;
  }

  if (pid == 0) {
    exec(args[0], args);
    fprintf(2, "xargs: exec failed\n");
    exit(1);
  }

  int status;
  if (wait(&status) < 0 || status != 0)
    return 1;

  return 0;
}

int
run_line(char *input, char *argv[], int start)
{
  char *extra[MAXARG];
  int count = 0;
  char *p = input;

  while (*p != '\0') {
    while (*p == ' ' || *p == '\t')
      p++;

    if (*p == '\0')
      break;

    if (count >= MAXARG) {
      fprintf(2, "xargs: too many arguments\n");
      return 1;
    }

    extra[count++] = p;

    while (*p != '\0' && *p != ' ' && *p != '\t')
      p++;

    if (*p != '\0')
      *p++ = '\0';
  }

  if (count == 0)
    return 0;

  return run_command(extra, count, argv, start);
}

int
main(int argc, char *argv[])
{
  int start = 1;
  int limit = 0;
  int failed = 0;

  if (argc > 1 && strcmp(argv[1], "-n") == 0) {
    if (argc < 4) {
      fprintf(2, "usage: xargs [-n N] cmd [args...]\n");
      exit(1);
    }

    for (int i = 0; argv[2][i] != '\0'; i++) {
      if (argv[2][i] < '0' || argv[2][i] > '9') {
        fprintf(2, "usage: xargs [-n N] cmd [args...]\n");
        exit(1);
      }
    }

    limit = atoi(argv[2]);
    if (limit < 1) {
      fprintf(2, "usage: xargs [-n N] cmd [args...]\n");
      exit(1);
    }

    start = 3;
  }

  if (start >= argc) {
    fprintf(2, "usage: xargs [-n N] cmd [args...]\n");
    exit(1);
  }

  int base_count = argc - start;
  if (base_count > MAXARG) {
    fprintf(2, "xargs: too many arguments\n");
    exit(1);
  }

  int batch_limit = limit;
  if (limit > 0 && batch_limit > MAXARG - base_count)
    batch_limit = MAXARG - base_count;

  if (limit > 0 && batch_limit < 1) {
    fprintf(2, "xargs: too many arguments\n");
    exit(1);
  }

  int len = 0;
  int word_count = 0;
  char ch;

  while (read(0, &ch, 1) > 0) {
    if (limit == 0) {
      if (ch == '\n') {
        line[len] = '\0';

        if (run_line(line, argv, start))
          failed = 1;

        len = 0;
      } else if (len < sizeof(line) - 1) {
        line[len++] = ch;
      } else {
        fprintf(2, "xargs: input line too long\n");
        exit(1);
      }
    } else {
      if (ch == ' ' || ch == '\t' || ch == '\n') {
        if (len > 0) {
          line[len] = '\0';

          if (word_count >= MAXARG) {
            fprintf(2, "xargs: too many arguments\n");
            exit(1);
          }

          strcpy(storage[word_count], line);
          words[word_count] = storage[word_count];
          word_count++;
          len = 0;

          if (word_count == batch_limit) {
            if (run_command(words, word_count, argv, start))
              failed = 1;

            word_count = 0;
          }
        }
      } else if (len < sizeof(line) - 1) {
        line[len++] = ch;
      } else {
        fprintf(2, "xargs: input word too long\n");
        exit(1);
      }
    }
  }

  if (limit == 0) {
    if (len > 0) {
      line[len] = '\0';
      if (run_line(line, argv, start))
        failed = 1;
    }
  } else {
    if (len > 0) {
      line[len] = '\0';
      strcpy(storage[word_count], line);
      words[word_count] = storage[word_count];
      word_count++;
    }

    if (word_count > 0) {
      if (run_command(words, word_count, argv, start))
        failed = 1;
    }
  }

  exit(failed ? 1 : 0);
}