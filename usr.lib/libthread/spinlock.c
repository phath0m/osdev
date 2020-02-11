#include "thread.h"

void
thread_spin_lock(thread_spinlock_t volatile *lock)
{
    while (*lock) {

    }
    
    __sync_lock_test_and_set(lock, 1);

}

void
thread_spin_unlock(thread_spinlock_t volatile *lock)
{
    __sync_lock_release(lock, 0);
}
