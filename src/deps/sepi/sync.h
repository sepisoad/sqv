#ifndef SEPI_SYNC_H
#define SEPI_SYNC_H

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#if defined(OS_LINUX)
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#elif defined(OS_MACOS)
#include <pthread.h>
#include <dispatch/dispatch.h>
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
Nothing sync_thread_sleep(Duration duration);

// NOTE: A.K.A Mutex
SyncLock sync_lock_create(Nothing);
Nothing sync_lock_destroy(SyncLock lock);
Nothing sync_lock_acquire(SyncLock lock);
// Nothing sync_lock_acquire_for(SyncLock lock, Duration duration);
Nothing sync_lock_release(SyncLock lock);

SyncRWLock sync_rwlock_create(Nothing);
Nothing sync_rwlock_destroy(SyncRWLock rwlock);
Nothing sync_rwlock_acquire_for_reading(SyncRWLock rwlock);
Nothing sync_rwlock_acquire_for_writing(SyncRWLock rwlock);
Nothing sync_rwlock_release(SyncRWLock rwlock);

// NOTE: A.K.A Semaphores
// TODO: tokens are not really tested!
SyncTokens sync_tokens_create(U32 initial_count);
Nothing sync_tokens_destroy(SyncTokens tokens);
Nothing sync_tokens_acquire(SyncTokens tokens);
Nothing sync_tokens_release(SyncTokens tokens);

// NOTE: A.K.A Conditions
SyncSignal sync_signal_create(Nothing);
Nothing sync_signal_destroy(SyncSignal signal);
Nothing sync_signal_listen(SyncSignal signal, SyncLock lock);
Nothing sync_signal_listen_for(SyncSignal signal,
                               SyncLock lock,
                               Duration duration);
Nothing sync_signal_rw_lock_listen(SyncSignal signal, SyncRWLock rwlock);
Nothing sync_signal_rw_lock_listen_for(SyncSignal signal,
                                       SyncRWLock rwlock,
                                       Duration duration);
Nothing sync_signal_notify_one(SyncSignal signal);
Nothing sync_signal_notify_all(SyncSignal signal);

// NOTE: A.K.A Barrier
SyncCheckpoint sync_checkpoint_create(Nothing);
Nothing sync_checkpoint_destroy(SyncCheckpoint checkpointt);
Nothing sync_checkpoint_await(SyncCheckpoint checkpointt);

/* ===================================================== */
/*                         MACROS                        */
/* ===================================================== */

#define with_lock(lock) \
  defer(sync_lock_acquire((lock)), sync_lock_release((lock)))

#define with_read_lock(lock) \
  defer(sync_rwlock_acquire_for_reading((lock)), sync_rwlock_release((lock)))

#define with_write_lock(lock) \
  defer(sync_rwlock_acquire_for_writing((lock)), sync_rwlock_release((lock)))

#define with_tokens(tokens) \
  defer(sync_tokens_acquire((tokens)), sync_tokens_release((tokens)))

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

Nothing
sync_thread_sleep(Duration duration) {
  start_profiling();

#if defined(OS_LINUX) || defined(OS_MACOS)
  struct timespec ts;
  ts.tv_sec = duration / 1000000000L;
  ts.tv_nsec = duration % 1000000000L;
  nanosleep(&ts, NULL);
#elif defined(OS_WINDOWS)
  Sleep((unsigned long)(duration / 1000000L));
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

// Nothing
// sync_lock_acquire_for(SyncLock lock, Duration seconds) {
//   start_profiling();

// #if defined(OS_LINUX)
//   struct timespec _seconds = {.tv_sec = seconds, .tv_nsec = 0};
//   runtime_assert(
//       0 == pthread_mutex_timedlock((pthread_mutex_t*)lock.id[0], &_seconds));
// #elif defined(OS_MACOS)
//   // NOTE:
//   // unfortunately macos posix implementation does not support
//   pthread_mutex_timedlock not_implemented();
// #elif defined(OS_WINDOWS)
//   not_implemented();
// #endif

//   end_profiling();
// }

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

SyncTokens
sync_tokens_create(U32 initial_count) {
  start_profiling();

  SyncTokens tokens;

#if defined(OS_LINUX)
  sem_t* _sem =
      arena_push(context_arena(), sizeof(sem_t), alignof(sem_t), TRUE);
  runtime_assert(0 == sem_init(_sem, 0, initial_count));
  tokens.id[0] = (U64)_sem;
#elif defined(OS_MACOS)
  dispatch_semaphore_t* _sem =
      arena_push(context_arena(), sizeof(dispatch_semaphore_t),
                 alignof(dispatch_semaphore_t), TRUE);
  *_sem = dispatch_semaphore_create(initial_count);
  runtime_assert(0 != _sem);
  tokens.id[0] = (U64)_sem;
#elif defined(OS_WINDOWS)
  not_implemented();
#endif

  end_profiling();
  return tokens;
}

/* ----------------------------------------------------- */

Nothing
sync_tokens_destroy(SyncTokens tokens) {
  start_profiling();

  assert(0 != tokens.id[0]);

#if defined(OS_LINUX)
  runtime_assert(0 == sem_destroy((sem_t*)tokens.id[0]));
#elif defined(OS_MACOS)
  // dispatch_release((dispatch_semaphore_t)tokens.id[0]);
#elif defined(OS_WINDOWS)
#endif

  end_profiling();
}

/* ----------------------------------------------------- */

Nothing
sync_tokens_acquire(SyncTokens tokens) {
  start_profiling();

  assert(0 != tokens.id[0]);

#if defined(OS_LINUX)
  while (1) {
    i32 result = sem_wait((sem_t*)tokens.id[0]);
    if (0 == result)
      break;

    if (EAGAIN == result)
      continue;

    break;
  }
#elif defined(OS_MACOS)
  while (1) {
    I32 result = dispatch_semaphore_wait((dispatch_semaphore_t)tokens.id[0], DISPATCH_TIME_NOW);
    if (0 == result)
      break;
  }
#elif defined(OS_WINDOWS)
#endif

  end_profiling();
}

/* ----------------------------------------------------- */

Nothing
sync_tokens_release(SyncTokens tokens) {
  start_profiling();

  assert(0 != tokens.id[0]);

#if defined(OS_LINUX)
  runtime_assert(0 == sem_post((sem_t*)tokens.id[0]));
  while (1) {
    i32 result = sem_wait((sem_t*)tokens.id[0]);
    if (0 == result)
      break;

    if (EAGAIN == result)
      continue;

    break;
  }

#elif defined(OS_MACOS)
  dispatch_semaphore_signal((dispatch_semaphore_t)tokens.id[0]);
#elif defined(OS_WINDOWS)
#endif

  end_profiling();
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_SYNC_IMPLEMENTATION */
#endif /* SEPI_SYNC_H */
