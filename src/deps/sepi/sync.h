#ifndef SEPI_SYNC_H
#define SEPI_SYNC_H

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#if defined(OS_LINUX)
#include <pthread.h>
#include <time.h>
#elif defined(OS_MACOS)
#include <pthread.h>
#include <time.h>
#elif defined(OS_WINDOWS)
#include <windows.h>
#endif

#include <tracy/tracy.h>

#include <sepi/base.h>
#include <sepi/context.h>

/* ===================================================== */
/*                  FORWARD DECLERATION                  */
/* ===================================================== */

/* ===================================================== */
/*                       CONSTANTS                       */
/* ===================================================== */

/* ===================================================== */
/*                         TYPES                         */
/* ===================================================== */

// NOTE:
// user has to define a custom enum in the target app defining the thread tags,
// however we use this type in the api

// NOTE:
// why use an array of [1] item?
// 1) id field can be used a pointer if the bracket is not used
// 2) you can increase the capacity of id by just adding items to it
// credits go to rad-debugger source code where i got this idea from!

typedef RawPtr SyncThreadFn(RawPtr ptr);
typedef SyncThreadFn* SyncThreadFnPtr;

typedef struct SyncThread SyncThread;
struct SyncThread {
  U64 id[1];
  SyncThreadFnPtr fnptr;
  RawPtr argptr;
  ContextID context_id;
};

// mutex
typedef struct SyncLock SyncLock;
struct SyncLock {
  U64 id[1];
};

// read-write-mutex
typedef struct SyncRWLock SyncRWLock;
struct SyncRWLock {
  U64 id[1];
};

// semaphore
typedef struct SyncTokens SyncTokens;
struct SyncTokens {
  U64 id[1];
};

// condition
typedef struct SyncSignal SyncSignal;
struct SyncSignal {
  U64 id[1];
};

// barrier
typedef struct SyncCheckpoint SyncCheckpoint;
struct SyncCheckpoint {
  U64 id[1];
};

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

SyncThread* sync_thread_start(SyncThreadFn fnptr, RawPtr argptr, ContextID tag);
Nothing sync_thread_await(SyncThread* thread);

SyncLock sync_lock_create(Nothing);
Nothing sync_lock_destroy(SyncLock lock);
Nothing sync_lock_acquire(SyncLock lock);
Nothing sync_lock_acquire_for(SyncLock lock, Duration duration);
Nothing sync_lock_release(SyncLock lock);

SyncRWLock sync_rwlock_create(Nothing);
Nothing sync_rwlock_destroy(SyncRWLock rwlock);
Nothing sync_rwlock_acquire_for_reading(SyncRWLock rwlock);
Nothing sync_rwlock_acquire_for_writing(SyncRWLock rwlock);
Nothing sync_rwlock_release(SyncRWLock rwlock);

SyncTokens sync_tokens_create(U32 initial_count, U32 max_count);
SyncTokens sync_tokens_destroy(SyncTokens tokens);
SyncTokens sync_tokens_acquire(SyncTokens tokens);
SyncTokens sync_tokens_release(SyncTokens tokens);

SyncSignal sync_signal_create(Nothing);
SyncSignal sync_signal_destroy(SyncSignal signal);
SyncSignal sync_signal_listen(SyncSignal signal, SyncLock lock);
SyncSignal sync_signal_listen_for(SyncSignal signal,
                                  SyncLock lock,
                                  Duration duration);
SyncSignal sync_signal_rw_lock_listen(SyncSignal signal, SyncRWLock rwlock);
SyncSignal sync_signal_rw_lock_listen_for(SyncSignal signal,
                                          SyncRWLock rwlock,
                                          Duration duration);
SyncSignal sync_signal_notify_one(SyncSignal signal);
SyncSignal sync_signal_notify_all(SyncSignal signal);

SyncCheckpoint sync_checkpoint_create(Nothing);
SyncCheckpoint sync_checkpoint_destroy(SyncCheckpoint checkpointt);
SyncCheckpoint sync_checkpoint_await(SyncCheckpoint checkpointt);

/* ===================================================== */
/*                         MACROS                        */
/* ===================================================== */

#define with_lock(lock) \
  defer(sync_lock_acquire((lock)), sync_lock_release((lock)))

#define with_read_lock(lock) \
  defer(sync_rwlock_acquire_for_reading((lock)), sync_rwlock_release((lock)))

#define with_write_lock(lock) \
  defer(sync_rwlock_acquire_for_writing((lock)), sync_rwlock_release((lock)))

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_SYNC_IMPLEMENTATION

mount_slave_profiling_context();

/* ----------------------------------------------------- */

local RawPtr
sync_thread_start_wrapper(RawPtr arg) {
  start_profiling();

  assert(arg != 0);

  SyncThread* thread = (SyncThread*)arg;
  assert(thread->fnptr != 0);

  context_init(thread->context_id);
  RawPtr result = thread->fnptr(thread->argptr);
  context_deinit();

  end_profiling();

  return result;
}

/* ----------------------------------------------------- */

SyncThread*
sync_thread_start(SyncThreadFn fnptr, RawPtr argptr, ContextID context_id) {
  start_profiling();

  assert(fnptr != 0);

  SyncThread* thread = arena_push(context_arena(), sizeof(SyncThread),
                                  alignof(SyncThread), TRUE);

  thread->fnptr = fnptr;
  thread->argptr = argptr;
  thread->context_id = context_id;

#if defined(OS_LINUX) || defined(OS_MACOS)
  pthread_t* _thread =
      arena_push(context_arena(), sizeof(pthread_t), alignof(pthread_t), TRUE);
  runtime_assert(0 ==
                 pthread_create(_thread, 0, sync_thread_start_wrapper, thread));
  thread->id[0] = (U64)_thread;
#elif defined(OS_WINDOWS)
  runtime_assert(0 != CreateThread(0, 0, sync_thread_start_wrapper, thread, 0, thread->id);
#endif

  end_profiling();
  return thread;
}

/* ----------------------------------------------------- */

Nothing
sync_thread_await(SyncThread* thread) {
  start_profiling();

  assert(thread != 0);

#if defined(OS_LINUX) || defined(OS_MACOS)
  pthread_join(*(pthread_t*)thread->id[0], 0);
#elif defined(OS_WINDOWS)
  WaitForSingleObject(thread->id, INFINITE);
  CloseHandle(thread->id);
#endif

  end_profiling();
}

/* ----------------------------------------------------- */

SyncLock
sync_lock_create(Nothing) {
  start_profiling();

  SyncLock lock;

#if defined(OS_LINUX) || defined(OS_MACOS)
  pthread_mutex_t* _mutex = arena_push(context_arena(), sizeof(pthread_mutex_t),
                                       alignof(pthread_mutex_t), TRUE);
  runtime_assert(0 == pthread_mutex_init(_mutex, 0));
  lock.id[0] = (U64)_mutex;
#elif defined(OS_WINDOWS)
  not_implemented();
#endif

  end_profiling();
  return lock;
}

/* ----------------------------------------------------- */

Nothing
sync_lock_destroy(SyncLock lock) {
  start_profiling();

  assert(0 != lock.id[0]);

#if defined(OS_LINUX) || defined(OS_MACOS)
  runtime_assert(0 == pthread_mutex_destroy((pthread_mutex_t*)lock.id[0]));
#elif defined(OS_WINDOWS)
  not_implemented();
#endif

  lock.id[0] = 0;
  end_profiling();
}

/* ----------------------------------------------------- */

Nothing
sync_lock_acquire(SyncLock lock) {
  start_profiling();

#if defined(OS_LINUX) || defined(OS_MACOS)
  runtime_assert(0 == pthread_mutex_lock((pthread_mutex_t*)lock.id[0]));
#elif defined(OS_WINDOWS)
  not_implemented();
#endif

  end_profiling();
}

/* ----------------------------------------------------- */

Nothing
sync_lock_acquire_for(SyncLock lock, Duration seconds) {
  start_profiling();

#if defined(OS_LINUX)
  struct timespec _seconds = {.tv_sec = seconds, .tv_nsec = 0};
  runtime_assert(
      0 == pthread_mutex_timedlock((pthread_mutex_t*)lock.id[0], &_seconds));
#elif defined(OS_MACOS)
  // NOTE:
  // unfortunately macos posix implementation does not support pthread_mutex_timedlock
  not_implemented();
#elif defined(OS_WINDOWS)
  not_implemented();
#endif

  end_profiling();
}

/* ----------------------------------------------------- */

Nothing
sync_lock_release(SyncLock lock) {
  start_profiling();

#if defined(OS_LINUX) || defined(OS_MACOS)
  runtime_assert(0 == pthread_mutex_unlock((pthread_mutex_t*)lock.id[0]));
#elif defined(OS_WINDOWS)
  not_implemented();
#endif

  end_profiling();
}

/* ----------------------------------------------------- */

SyncRWLock
sync_rwlock_create(Nothing) {
  start_profiling();

  SyncRWLock rwlock;

#if defined(OS_LINUX) || defined(OS_MACOS)
  pthread_rwlock_t* _rwlock =
      arena_push(context_arena(), sizeof(pthread_rwlock_t),
                 alignof(pthread_rwlock_t), TRUE);
  runtime_assert(0 == pthread_rwlock_init(_rwlock, 0));
  rwlock.id[0] = (U64)_rwlock;
#elif defined(OS_WINDOWS)
  not_implemented();
#endif

  end_profiling();
  return rwlock;
}

/* ----------------------------------------------------- */

Nothing
sync_rwlock_destroy(SyncRWLock rwlock) {
  start_profiling();

  assert(0 != rwlock.id[0]);

#if defined(OS_LINUX) || defined(OS_MACOS)
  runtime_assert(0 == pthread_rwlock_destroy((pthread_rwlock_t*)rwlock.id[0]));
#elif defined(OS_WINDOWS)
  not_implemented();
#endif

  rwlock.id[0] = 0;
  end_profiling();
}

/* ----------------------------------------------------- */

Nothing
sync_rwlock_acquire_for_reading(SyncRWLock rwlock) {
  start_profiling();

#if defined(OS_LINUX) || defined(OS_MACOS)
  runtime_assert(0 == pthread_rwlock_rdlock((pthread_rwlock_t*)rwlock.id[0]));
#elif defined(OS_WINDOWS)
  not_implemented();
#endif

  end_profiling();
}

/* ----------------------------------------------------- */

Nothing
sync_rwlock_acquire_for_writing(SyncRWLock rwlock) {
  start_profiling();

#if defined(OS_LINUX) || defined(OS_MACOS)
  runtime_assert(0 == pthread_rwlock_wrlock((pthread_rwlock_t*)rwlock.id[0]));
#elif defined(OS_WINDOWS)
  not_implemented();
#endif

  end_profiling();
}

/* ----------------------------------------------------- */

Nothing
sync_rwlock_release(SyncRWLock rwlock) {
  start_profiling();

#if defined(OS_LINUX) || defined(OS_MACOS)
  runtime_assert(0 == pthread_rwlock_unlock((pthread_rwlock_t*)rwlock.id[0]));
#elif defined(OS_WINDOWS)
  not_implemented();
#endif

  end_profiling();
}

/* ----------------------------------------------------- */

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_SYNC_IMPLEMENTATION */
#endif /* SEPI_SYNC_H */
