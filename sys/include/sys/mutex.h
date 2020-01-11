#ifndef RTL_MUTEX_H
#define RTL_MUTEX_H

typedef unsigned char spinlock_t;

void spinlock_lock(spinlock_t volatile *lock);
void spinlock_unlock(spinlock_t volatile *lock);

#endif
