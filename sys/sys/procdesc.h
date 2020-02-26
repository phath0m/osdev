#ifndef _SYS_PROCDESC_H
#define _SYS_PROCDESC_H

#include <sys/file.h>

struct file * proc_getfile(int fildes);
int proc_getfildes();
int proc_newfildes(struct file *file);
int proc_dup(int oldfd);
int proc_dup2(int oldfd, int newfd);
int proc_fcntl(int fd, int cmd, void *arg);

#endif
