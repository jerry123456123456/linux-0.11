#ifndef LIB_SYSCALL_H
#define LIB_SYSCALL_H

#include "core/syscall.h"
#include "os_cfg.h"
#include <stddef.h>
#include<sys/stat.h>

typedef struct _syscall_args_t {
    int id;
    int arg0;
    int arg1;
    int arg2;
    int arg3;
}syscall_args_t;

int msleep (int ms);
int fork(void);
int getpid(void);
int yield (void);
int execve(const char *name, char * const *argv, char * const *env);
int print_msg(char * fmt, int arg);

int open(const char *name, int flags, ...);
int read(int file, char *ptr, int len);
int write(int file, char *ptr, int len);
int close(int file);
int lseek(int file, int ptr, int dir);
int ioctl(int file,int cmd,int arg0,int arg1);
int unlink(const char *path);

int isatty(int file);
int fstat(int file, struct stat *st);
void * sbrk(ptrdiff_t incr);
void _exit(int status);
int wait(int *status);
int dup(int file);

struct dirent {     //一个目录中的单个目录项，包含了目录项的相关信息
   int index;         // 在目录中的偏移
   int type;            // 文件或目录的类型
   char name [255];       // 目录或目录的名称
   int size;            // 文件大小
};

typedef struct _DIR {       //表示一个目录流（directory stream），可以看作是一个指向目录中特定位置的指针
    int index;               // 当前遍历的索引
    struct dirent dirent;
}DIR;

DIR * opendir(const char * name);
struct dirent* readdir(DIR* dir);
int closedir(DIR *dir);

#endif //LIB_SYSCALL_H
