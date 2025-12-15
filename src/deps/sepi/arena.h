#ifndef SEPI_ARENA_H
#define SEPI_ARENA_H

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#include "../tracy/tracy.h"

#include "base.h"
#include "platform.h"

/* ===================================================== */
/*                       CONSTANTS                       */
/* ===================================================== */

#define ARENA_DEFAULT_RESERVE_SIZE MB(64)
#define ARENA_DEFAULT_COMMIT_SIZE MB(64)

/* ===================================================== */
/*                         TYPES                         */
/* ===================================================== */

typedef struct ArenaParams ArenaParams;
struct ArenaParams {
  U64 requested_reserve_size;
  U64 requested_commit_size;
  CStr caller_file_name;
  U32 caller_file_line;
};

typedef struct Arena Arena;
struct Arena {
  Arena* previous_block;
  Arena* current_block;
  Arena* free_last;
  U64 requested_commit_size;
  U64 committed_size;
  U64 requested_reserve_size;
  U64 reserved_size;
  U64 base_position;
  U64 offset;
  CStr caller_file_name;
  U32 caller_file_line;
};

typedef struct ArenaScratch ArenaScratch;
struct ArenaScratch {
  Arena* arena;
  U64 offset;
};

/* ===================================================== */
/*                          API                          */
/* ===================================================== */

Arena* arena_create_(ArenaParams* ap);
Nothing arena_destroy(Arena* a);
RawPtr arena_push(Arena*, U64 size, U64 align, Bool with_zero);
Nothing arena_pop(Arena*, U64 amount);
Nothing arena_pop_to(Arena* a, U64 position);
Nothing arena_clear(Arena*);
U64 arena_get_position(Arena* a);
ArenaScratch arena_scratch_begin(Arena* a);
Nothing arena_scratch_end(ArenaScratch s);

#define arena_create(...) arena_create_(&(ArenaParams){.requested_reserve_size = ARENA_DEFAULT_RESERVE_SIZE, .requested_commit_size = ARENA_DEFAULT_COMMIT_SIZE, .caller_file_name = __FILE__, .caller_file_line = __LINE__, __VA_ARGS__})
#define arena_push_array_no_zero_aligned(arena, type, count, alignment) (type *)arena_push((arena), sizeof(type) * (count), (alignment), (TRUE))
#define arena_push_array_aligned(arena, type, count, alignment) (type *)arena_push((arena), sizeof(type) * (count), (alignment), (FALSE))
#define arena_push_array_no_zero(arena, type, count) arena_push_array_no_zero_aligned(arena, type, count, Max(8, AlignOf(type)))
#define arena_push_array(arena, type, count) arena_push_array_aligned(arena, type, count, Max(8, AlignOf(type)))

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_ARENA_IMPLEMENTATION

extern TracyCZoneCtx trcyctx;

Arena*
arena_create_(ArenaParams* ap) {
  TracyCZoneN(trcyctx, "arena_create_", 1);

  U64 const large_page_size = platform_get_large_page_size();

  /* TODO: this uses large pages size by default */
  U64 requested_reserve_size = AlignUp(ap->requested_reserve_size,
                                       large_page_size);
  U64 requested_commit_size = AlignUp(ap->requested_commit_size, large_page_size);

  RawPtr base = platform_reserve_large_pages(requested_reserve_size);
  platform_commit_large_pages(base, requested_commit_size);

  if(!base) {
    Abort("failed to allocate memory to arena allocator");
  }

  Sz header_size = sizeof(Arena);
  Arena* a = (Arena*)base;

  a->current_block = a;
  a->free_last = 0;

  a->requested_reserve_size = ap->requested_reserve_size;
  a->reserved_size = requested_reserve_size;

  a->requested_commit_size = ap->requested_commit_size;
  a->committed_size = requested_commit_size;

  a->base_position = 0;
  a->offset = header_size;

  a->caller_file_name = ap->caller_file_name;
  a->caller_file_line = ap->caller_file_line;

  AsanPoisonMemoryRegion(base, requested_commit_size);
  AsanUnpoisonMemoryRegion(base, header_size);

  TracyCZoneEnd(trcyctx);
  return a;
}

Nothing
arena_destroy(Arena* a) {
  TracyCZoneN(trcyctx, "arena_destroy", 1);

  for(Arena* it = a->current_block, *previous_block = 0; it != 0;
      it = previous_block) {
    previous_block = it->previous_block;
    platform_release(it, it->reserved_size);
  }

  TracyCZoneEnd(trcyctx);
}

RawPtr
arena_push(Arena* a, U64 size, U64 align, Bool with_zero) {
  TracyCZoneN(trcyctx, "arena_push", 1);

  Arena* current_block = a->current_block;
  U64 offset_aligned = AlignUp(current_block->offset, align);
  U64 offset_aligned_sized = offset_aligned + size;

  if(current_block->reserved_size < offset_aligned_sized) {
    Arena* new_block = 0;
    Arena* previous_block;

    for(new_block = a->free_last, previous_block = 0; new_block != 0;
        previous_block = new_block, new_block = new_block->previous_block) {
      if(new_block->reserved_size >= AlignUp(new_block->offset, align) + size) {
        if(previous_block) {
          previous_block->previous_block = new_block->previous_block;
        } else {
          a->free_last = new_block->previous_block;
        }
        break;
      }
    }


    if(new_block == 0) {
      Sz header_size = sizeof(Arena);
      U64 requested_reserve_size = current_block->requested_reserve_size;
      U64 requested_commit_size = current_block->requested_commit_size;
      if(size + header_size > requested_reserve_size) {
        requested_reserve_size = AlignUp(size + header_size, align);
        requested_commit_size = AlignUp(size + header_size, align);
      }
      new_block = arena_create(.requested_reserve_size = requested_reserve_size,
                              .requested_commit_size = requested_commit_size,
                              .caller_file_name = (CStr) current_block->caller_file_name,
                              .caller_file_line = current_block->caller_file_line);
    }

    new_block->base_position = current_block->base_position +
                               current_block->reserved_size;
    new_block->previous_block = a->current_block;
    a->current_block = new_block;
    current_block = new_block;
    offset_aligned = AlignUp(current_block->offset, align);
    offset_aligned_sized = offset_aligned + size;
  }

  U64 size_to_zero = 0;
  if(with_zero) {
    size_to_zero = Min(current_block->committed_size,
                       offset_aligned_sized) - offset_aligned;
  }

  if(current_block->committed_size < offset_aligned_sized) {
    U64 new_commit_size = offset_aligned_sized +
                          current_block->requested_commit_size - 1;
    new_commit_size -= new_commit_size % current_block->requested_commit_size;
    U64 commit_size_clamped = Max(new_commit_size, current_block->reserved_size);
    U64 needed_commit_size = commit_size_clamped - current_block->committed_size;
    U8* committed_size_ptr = (U8*)current_block + current_block->committed_size;
    platform_commit_large_pages(committed_size_ptr, needed_commit_size);
    current_block->committed_size = commit_size_clamped;
  }

  RawPtr result = 0;
  if(current_block->committed_size >= offset_aligned_sized) {
    result = (U8*)current_block + offset_aligned;
    current_block->offset = offset_aligned_sized;
    AsanUnpoisonMemoryRegion(result, size);
    if(size_to_zero != 0) {
      MemZero(result, size_to_zero);
    }
  }

  if(result == 0) {
    Abort("failed to allocate memory from arena allocator");
  }

  TracyCZoneEnd(trcyctx);
  return result;
}

Nothing
arena_pop(Arena* a, U64 amount) {
  TracyCZoneN(trcyctx, "arena_pop", 1);

  U64 old_position = arena_get_position(a);
  U64 new_position = old_position;
  if (amount < old_position) {
    new_position = old_position - amount;
  }

  arena_pop_to(a, new_position);

  TracyCZoneEnd(trcyctx);
}

Nothing
arena_pop_to(Arena* a, U64 position) {
  TracyCZoneN(trcyctx, "arena_pop_to", 1);

  Sz header_size = sizeof(Arena);
  U64 normilized_position = Max(header_size, position);
  Arena* current_block = a->current_block;

  for(Arena* previous_block = 0;
      current_block->base_position >= normilized_position;
      current_block = previous_block
     ) {
    previous_block = current_block->previous_block;
    current_block->offset = header_size;
    current_block->previous_block = a->free_last;
    a->free_last = current_block;
    AsanPoisonMemoryRegion((U8*)current_block + header_size,
                           current_block->reserved_size - header_size);
  }

  a->current_block = current_block;
  U64 new_offset = normilized_position - current_block->base_position;
  AssertAlways(new_offset <= current_block->offset);
  AsanPoisonMemoryRegion((U8*)current_block + new_offset,
                         (current_block->offset - new_offset));
  current_block->offset = new_offset;

  TracyCZoneEnd(trcyctx);
}

Nothing
arena_clear(Arena* a) {
  TracyCZoneN(trcyctx, "arena_clear", 1);

  arena_pop_to(a, 0);

  TracyCZoneEnd(trcyctx);
}

U64
arena_get_position(Arena* a) {
  TracyCZoneN(trcyctx, "arena_get_position", 1);

  Arena* current_block = a->current_block;
  U64 position = current_block->base_position + current_block->offset;

  TracyCZoneEnd(trcyctx);
  return position;
}

ArenaScratch
arena_scratch_begin(Arena* a) {
  TracyCZoneN(trcyctx, "arena_scratch_begin", 1);

  U64 position = arena_get_position(a);

  TracyCZoneEnd(trcyctx);
  return (ArenaScratch) {
    a, position
  };
}

Nothing
arena_scratch_end(ArenaScratch s) {
  TracyCZoneN(trcyctx, "arena_scratch_end", 1);

  arena_pop_to(s.arena, s.offset);

  TracyCZoneEnd(trcyctx);
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_ARENA_IMPLEMENTATION */
#endif /* SEPI_ARENA_H */

