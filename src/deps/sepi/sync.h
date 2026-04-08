#ifndef SEPI_SYNC_H
#define SEPI_SYNC_H

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#if defined(OS_LINUX)
#include <pthread.h>
#elif defined(OS_MACOS)
#include <pthread.h>
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

fn SyncThread* sync_thread_start(SyncThreadFn fnptr,
                              RawPtr argptr,
                              ContextID tag);
fn Nothing sync_thread_await(SyncThread* thread);

fn SyncLock sync_lock_create(Nothing);
fn SyncLock sync_lock_destroy(SyncLock lockt);
fn SyncLock sync_lock_acquire(SyncLock lockt);
fn SyncLock sync_lock_release(SyncLock lockt);

fn SyncRWLock sync_rw_lock_create(Nothing);
fn SyncRWLock sync_rw_lock_destroy(SyncRWLock rw_lock);
fn SyncRWLock sync_rw_lock_acquire_for_reading(SyncRWLock rw_lock);
fn SyncRWLock sync_rw_lock_acquire_for_writing(SyncRWLock rw_lock);
fn SyncRWLock sync_rw_lock_release(SyncRWLock rw_lock);

fn SyncTokens sync_tokens_create(U32 initial_count, U32 max_count);
fn SyncTokens sync_tokens_destroy(SyncTokens tokens);
fn SyncTokens sync_tokens_acquire(SyncTokens tokens);
fn SyncTokens sync_tokens_release(SyncTokens tokens);

fn SyncSignal sync_signal_create(Nothing);
fn SyncSignal sync_signal_destroy(SyncSignal signal);
fn SyncSignal sync_signal_listen(SyncSignal signal, SyncLock lock);
fn SyncSignal sync_signal_listen_for(SyncSignal signal,
                                  SyncLock lock,
                                  Duration duration);
fn SyncSignal sync_signal_rw_lock_listen(SyncSignal signal, SyncRWLock rw_lock);
fn SyncSignal sync_signal_rw_lock_listen_for(SyncSignal signal,
                                          SyncRWLock rw_lock,
                                          Duration duration);
fn SyncSignal sync_signal_notify_one(SyncSignal signal);
fn SyncSignal sync_signal_notify_all(SyncSignal signal);

fn SyncCheckpoint sync_checkpoint_create(Nothing);
fn SyncCheckpoint sync_checkpoint_destroy(SyncCheckpoint checkpointt);
fn SyncCheckpoint sync_checkpoint_await(SyncCheckpoint checkpointt);

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_SYNC_IMPLEMENTATION

mount_slave_profiling_context();

local fn RawPtr sync_thread_start_wrapper(RawPtr arg) {
  start_profiling(1);

  assert(arg != 0);

  SyncThread* thread = (SyncThread*)arg;
  assert(thread->fnptr != 0);

  context_init(thread->context_id);
  RawPtr result = thread->fnptr(thread->argptr);
  context_deinit();

  end_profiling();

  return result;
}

fn SyncThread*
sync_thread_start(SyncThreadFn fnptr, RawPtr argptr, ContextID context_id) {
  start_profiling(1);

  assert(fnptr != 0);

  SyncThread* thread = arena_push(context_arena(), sizeof(SyncThread),
                                  alignof(SyncThread), TRUE);
  thread->fnptr = fnptr;
  thread->argptr = argptr;
  thread->context_id = context_id;

#if defined(OS_LINUX) || defined(OS_MACOS)
  pthread_t* _thread =
      arena_push(context_arena(), sizeof(pthread_t), alignof(pthread_t), TRUE);
  runtime_assert(0 == pthread_create(_thread, 0, sync_thread_start_wrapper, thread));
  thread->id[0] = (U64)_thread;
#elif defined(OS_WINDOWS)
  runtime_assert(0 != CreateThread(0, 0, sync_thread_start_wrapper, thread, 0, thread->id);
#endif

  end_profiling();
  return thread;
}

fn Nothing
sync_thread_await(SyncThread* thread) {
  start_profiling(1);

  assert(thread != 0);

#if defined(OS_LINUX) || defined(OS_MACOS)
  pthread_join(*(pthread_t*)thread->id[0], 0);
#elif defined(OS_WINDOWS)
  WaitForSingleObject(thread.id, INFINITE);
  CloseHandle(thread.id);
#endif

  end_profiling();
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_SYNC_IMPLEMENTATION */
#endif /* SEPI_SYNC_H */
