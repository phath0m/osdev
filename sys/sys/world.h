#ifndef _SYS_WORLD_H
#define _SYS_WORLD_H
#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>


#define WORLD_CREATE        0x00    /* creates a world */
#define WORLD_ADJUST        0x01    /* adjust flags */

/* world flags */
#define WF_NONE             0x00
#define WF_ISOLATE_PROC     0x01    /* Should this world be able to interact with processes outside itself? */ 
#define WF_REMAP_GIDS       0x02    /* Should remap group IDs? */
#define WF_REMAP_UIDS       0x04    /* Should remap user IDs? */
#define WF_REMAP_PIDS       0x08    /* Should remap pids? */
#define WF_KILL_CHILDREN    0x10    /* Should kill children when initial process dies */

#ifdef __KERNEL__

#include <ds/list.h>

struct world {
    pid_t           world_id;
    uid_t           effective_uid;
    gid_t           effective_gid;
    int             flags;
    struct list     members;
    struct proc *   leader;
};

int             kern_worldctl(struct proc *, int, int);
struct world *  world_new(struct proc *, int);

#define WORLD_CAN_SEE_PROC(w, proc) (((w) == (proc)->world) || ((w)->flags & WF_ISOLATE_PROC) == 0)

#endif

int             worldctl(int, int);

#ifdef __cplusplus
}
#endif
#endif
