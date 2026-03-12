#ifndef SEPI_SYNC_H
#define SEPI_SYNC_H

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#include <tracy/tracy.h>

#include <sepi/base.h>

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
// why use an array of [1] item?
// 1) id field can be used a pointer if the bracket is not used
// 2) you can increase the capacity of id by just adding items to it
// credits go to rad-debugger source code where i got this idea from!

typedef Nothing SyncThreadFn(RawPtr* ptr);
typedef SyncThreadFn* SyncThreadFnPtr;

typedef struct SyncThread SyncThread;
struct SyncThread {
  U64 id[1];
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

static SyncThread sync_thread_start(SyncThreadFnPtr fnptr, RawPtr optdata);
static Bool sync_thread_await(SyncThread thread);

static SyncLock sync_lock_create(Nothing);
static SyncLock sync_lock_destroy(SyncLock lockt);
static SyncLock sync_lock_acquire(SyncLock lockt);
static SyncLock sync_lock_release(SyncLock lockt);

static SyncRWLock sync_rw_lock_create(Nothing);
static SyncRWLock sync_rw_lock_destroy(SyncRWLock rw_lock);
static SyncRWLock sync_rw_lock_acquire_for_reading(SyncRWLock rw_lock);
static SyncRWLock sync_rw_lock_acquire_for_writing(SyncRWLock rw_lock);
static SyncRWLock sync_rw_lock_release(SyncRWLock rw_lock);

static SyncTokens sync_tokens_create(U32 initial_count, U32 max_count);
static SyncTokens sync_tokens_destroy(SyncTokens tokens);
static SyncTokens sync_tokens_acquire(SyncTokens tokens);
static SyncTokens sync_tokens_release(SyncTokens tokens);

static SyncSignal sync_signal_create(Nothing);
static SyncSignal sync_signal_destroy(SyncSignal signal);
static SyncSignal sync_signal_listen(SyncSignal signal,
                                          SyncLock lock);
static SyncSignal sync_signal_listen_for(SyncSignal signal,
                                              SyncLock lock,
                                              Duration duration);
static SyncSignal sync_signal_rw_lock_listen(SyncSignal signal,
                                                     SyncRWLock rw_lock);
static SyncSignal sync_signal_rw_lock_listen_for(
    SyncSignal signal,
    SyncRWLock rw_lock,
    Duration duration);
static SyncSignal sync_signal_notify_one(SyncSignal signal);
static SyncSignal sync_signal_notify_all(SyncSignal signal);

static SyncCheckpoint sync_checkpoint_create(Nothing);
static SyncCheckpoint sync_checkpoint_destroy(SyncCheckpoint checkpointt);
static SyncCheckpoint sync_checkpoint_await(SyncCheckpoint checkpointt);

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_SYNC_IMPLEMENTATION

mount_slave_profiling_context();

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_SYNC_IMPLEMENTATION */
#endif /* SEPI_SYNC_H */
