#ifndef _SYS_MMAN_H
#define _SYS_MMAN_H

#define PROT_READ   0x04
#define PROT_WRITE  0x02
#define PROT_EXEC   0x01

#define MAP_SHARED  0x02
#define MAP_PRIVATE 0x01

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
int shm_open(const char *path, int oflag, mode_t mode);
int shm_unlink(const char *path);

#endif
