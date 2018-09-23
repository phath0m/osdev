#include <rtl/mutex.h>

void
spinlock_lock(spinlock_t volatile *lock)
{
    while (*lock) {

    }
    
    __sync_lock_test_and_set(lock, 1);
}

void
spinlock_unlock(spinlock_t volatile *lock)
{
    __sync_lock_release(lock, 0);
}
