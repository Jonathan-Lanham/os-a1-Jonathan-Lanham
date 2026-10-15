#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "user/user.h"

char*
fmtname(char *path)
{
  static char buf[DIRSIZ + 1];
  char *p;

  for(p = path + strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  if(strlen(p) >= DIRSIZ)
    return p;

  memmove(buf, p, strlen(p));
  buf[strlen(p)] = 0;
  return buf;
}

int
match(char *pattern, char *name)
{
  if(*pattern == '\0')
    return *name == '\0';

  if(*pattern == '*'){
    return match(pattern + 1, name) ||
           (*name && match(pattern, name + 1));
  }

  if(*name && *pattern == *name)
    return match(pattern + 1, name + 1);

  return 0;
}

void
find(char *path, char *pattern, int type)
{
  int fd;
  struct dirent de;
  struct stat st;
  char buf[512], *p;

  if((fd = open(path, 0)) < 0){
    fprintf(2, "find: cannot open %s\n", path);
    exit(1);
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    exit(1);
  }

  if(st.type != T_DIR){
    if((type == 0 ||
    (type == T_FILE && (st.type == T_FILE || st.type == T_DEVICE)) ||
    (type == T_DIR && st.type == T_DIR)) &&
   match(pattern, fmtname(path)))
      printf("%s\n", path);
    close(fd);
    return;
  }

  if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
    fprintf(2, "find: path too long\n");
    close(fd);
    exit(1);
  }

  strcpy(buf, path);
  p = buf + strlen(buf);
  *p++ = '/';

  while(read(fd, &de, sizeof(de)) == sizeof(de)){
    if(de.inum == 0)
      continue;

    if(strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
      continue;

    memmove(p, de.name, DIRSIZ);
    p[DIRSIZ] = 0;

    if(stat(buf, &st) < 0)
      continue;

    if((type == 0 ||
    (type == T_FILE && (st.type == T_FILE || st.type == T_DEVICE)) ||
    (type == T_DIR && st.type == T_DIR)) &&
   match(pattern, fmtname(buf)))
      printf("%s\n", buf);

    if(st.type == T_DIR)
      find(buf, pattern, type);
  }

  close(fd);
}

int
main(int argc, char *argv[])
{
  int type = 0;

  if(argc != 3 && argc != 5){
    fprintf(2, "usage: find <dir> <pattern> [-type f|d]\n");
    exit(1);
  }

  if(argc == 5){
    if(strcmp(argv[3], "-type") != 0){
      fprintf(2, "usage: find <dir> <pattern> [-type f|d]\n");
      exit(1);
    }

    if(argv[4][0] == 'f' && argv[4][1] == '\0')
      type = T_FILE;
    else if(argv[4][0] == 'd' && argv[4][1] == '\0')
      type = T_DIR;
    else{
      fprintf(2, "usage: find <dir> <pattern> [-type f|d]\n");
      exit(1);
    }
  }

  find(argv[1], argv[2], type);
  exit(0);
}