#include <sys/proc.h>
#include <sys/sysctl.h>
#include <sys/types.h>

int
kern_sysctl(int *name, int namelen, void *oldp, size_t *oldlenp, void *newp, size_t newlen)
{
    switch (name[0]) {
        case KERN_PROC:
            return proc_sysctl(&name[1], namelen - 1, oldp, oldlenp, newp, newlen);
    }

    return -1;
}

int
sysctl(int *name, int namelen, void *oldp, size_t *oldlenp, void *newp, size_t newlen)
{
    switch (name[0]) {
        case CTL_KERN:
            return kern_sysctl(&name[1], namelen - 1, oldp, oldlenp, newp, newlen);    
    }
    
    return -1;
}

