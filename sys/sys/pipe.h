#ifndef _SYS_PIPE_H
#define _SYS_PIPE_H

#include <sys/file.h>

void            create_pipe(struct file **files);
struct file *   fifo_to_file(struct vnode *parent, mode_t mode);

#endif

