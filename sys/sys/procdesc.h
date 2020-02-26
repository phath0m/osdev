#ifndef _SYS_PROCDESC_H
#define _SYS_PROCDESC_H

#include <sys/file.h>

struct file * procdesc_getfile(int fildes);
int procdesc_getfd();
int procdesc_newfd(struct file *file);
int procdesc_dup(int oldfd);
int procdesc_dup2(int oldfd, int newfd);
int procdesc_fcntl(int fd, int cmd, void *arg);

#endif
