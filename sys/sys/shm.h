#ifndef _SYS_SHM_H
#define _SYS_SHM_H

int shm_open(struct proc *proc, struct file **result, const char *name, int oflag, mode_t mode);
int shm_unlink(struct proc *proc, const char *name);

#endif
