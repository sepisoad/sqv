#ifndef SEPI_ARENA_H
#define SEPI_ARENA_H

/* ===================================================== */
/*                     DEPENDENCIES                      */
/* ===================================================== */

#include <tracy/tracy.h>

#include <sepi/base.h>
#include <sepi/platform.h>

/* ===================================================== */
/*                       CONSTANTS                       */
/* ===================================================== */

#define ARENA_DEFAULT_RESERVE_SIZE mega_bytes(64)
#define ARENA_DEFAULT_COMMIT_SIZE mega_bytes(64)

/* ===================================================== */
/*                         TYPES                         */
/* ===================================================== */

typedef struct ArenaParams ArenaParams;
struct ArenaParams {
  U64 requested_reserve_size;
  U64 requested_commit_size;
  CZStr caller_file_name;
  U32 caller_file_line;
};

typedef struct Arena Arena;
struct Arena {
  Arena* previous_block;
  Arena* current_block;
  Arena* last_freed_block;
  U64 requested_commit_size;
  U64 committed_size;
  U64 requested_reserve_size;
  U64 reserved_size;
  U64 base_position;
  U64 offset;
  CZStr caller_file_name;
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

Arena* arena_create_(ArenaParams* arena_params);
Nothing arena_destroy(Arena* arena);
RawPtr arena_push(Arena* arena,
                  U64 size_to_allocate,
                  U64 memory_alignment,
                  Bool with_zero);
Nothing arena_pop(Arena* arena, U64 amount);
Nothing arena_pop_to(Arena* arena, U64 position);
Nothing arena_clear(Arena* arena);
U64 arena_get_position(Arena* arena);
ArenaScratch arena_scratch_begin(Arena* arena);
Nothing arena_scratch_end(ArenaScratch arena_scratch);

#define arena_create(...)                                                  \
  arena_create_(                                                           \
      &(ArenaParams){.requested_reserve_size = ARENA_DEFAULT_RESERVE_SIZE, \
                     .requested_commit_size = ARENA_DEFAULT_COMMIT_SIZE,   \
                     .caller_file_name = __FILE__,                         \
                     .caller_file_line = __LINE__,                         \
                     __VA_ARGS__})
#define arena_push_array_0_init_aligned(arena, type, count, alignment) \
  (type*)arena_push((arena), sizeof(type) * (count), (alignment), (TRUE))
#define arena_push_array_aligned(arena, type, count, alignment) \
  (type*)arena_push((arena), sizeof(type) * (count), (alignment), (FALSE))
#define arena_push_array_0_init(arena, type, count) \
  arena_push_array_0_init_aligned(arena, type, count, max(8, alignof(type)))
#define arena_push_array(arena, type, count) \
  arena_push_array_aligned(arena, type, count, max(8, alignof(type)))

/* ===================================================== */
/*                    IMPLEMENTATION                     */
/* ===================================================== */

#ifdef SEPI_ARENA_IMPLEMENTATION

mount_slave_profiling_context();

/* ----------------------------------------------------- */

Arena*
arena_create_(ArenaParams* ap) {
  start_profiling(1);

  U64 platform_large_page_size = platform_get_large_page_size();
  if (platform_large_page_size == 0) {
    platform_large_page_size = (U64)platform_get_page_size();  // fallback
  }

  /* TODO: this uses large pages size by default */
  U64 requested_reserve_size =
      align_up(ap->requested_reserve_size, platform_large_page_size);
  U64 requested_commit_size =
      align_up(ap->requested_commit_size, platform_large_page_size);

  RawPtr raw_base_pointer =
      platform_reserve_large_pages(requested_reserve_size);
  platform_commit_large_pages(raw_base_pointer, requested_commit_size);

  if (!raw_base_pointer) {
    abort("failed to allocate memory to arena allocator");
  }

  Sz arena_header_size = sizeof(Arena);
  Arena* arena = (Arena*)raw_base_pointer;

  arena->current_block = arena;
  arena->last_freed_block = 0;

  arena->requested_reserve_size = ap->requested_reserve_size;
  arena->reserved_size = requested_reserve_size;

  arena->requested_commit_size = ap->requested_commit_size;
  arena->committed_size = requested_commit_size;

  arena->base_position = 0;
  arena->offset = arena_header_size;

  arena->caller_file_name = ap->caller_file_name;
  arena->caller_file_line = ap->caller_file_line;

  asan_poison_memory_region(raw_base_pointer, requested_commit_size);
  asan_unpoison_memory_region(raw_base_pointer, arena_header_size);

  end_profiling();
  return arena;
}

/* ----------------------------------------------------- */

Nothing
arena_destroy(Arena* arena) {
  start_profiling(1);

  for (Arena *iterator = arena->current_block, *previous_block = 0;
       iterator != 0; iterator = previous_block) {
    previous_block = iterator->previous_block;
    platform_release(iterator, iterator->reserved_size);
  }

  end_profiling();
}

/* ----------------------------------------------------- */

RawPtr
arena_push(Arena* arena,
           U64 size_to_allocate,
           U64 memory_alignment,
           Bool with_zero) {
  start_profiling(1);

  Arena* current_block = arena->current_block;
  U64 aligned_memory_offset = align_up(current_block->offset, memory_alignment);
  U64 aligned_memory_size_to_allocate =
      aligned_memory_offset + size_to_allocate;

  if (current_block->reserved_size < aligned_memory_size_to_allocate) {
    Arena* new_arena_block = 0;
    Arena* previous_arena_block;

    for (new_arena_block = arena->last_freed_block, previous_arena_block = 0;
         new_arena_block != 0; previous_arena_block = new_arena_block,
        new_arena_block = new_arena_block->previous_block) {
      if (new_arena_block->reserved_size >=
          align_up(new_arena_block->offset, memory_alignment) +
              size_to_allocate) {
        if (previous_arena_block) {
          previous_arena_block->previous_block =
              new_arena_block->previous_block;
        } else {
          arena->last_freed_block = new_arena_block->previous_block;
        }
        break;
      }
    }

    if (new_arena_block == 0) {
      Sz arena_header_size = sizeof(Arena);
      U64 requested_reserve_size = current_block->requested_reserve_size;
      U64 requested_commit_size = current_block->requested_commit_size;
      if (size_to_allocate + arena_header_size > requested_reserve_size) {
        requested_reserve_size =
            align_up(size_to_allocate + arena_header_size, memory_alignment);
        requested_commit_size =
            align_up(size_to_allocate + arena_header_size, memory_alignment);
      }
      new_arena_block = arena_create_(&(ArenaParams){
          .requested_reserve_size = requested_reserve_size,
          .requested_commit_size = requested_commit_size,
          .caller_file_name = (CZStr)current_block->caller_file_name,
          .caller_file_line = current_block->caller_file_line});
    }

    new_arena_block->base_position =
        current_block->base_position + current_block->reserved_size;
    new_arena_block->previous_block = arena->current_block;
    arena->current_block = new_arena_block;
    current_block = new_arena_block;
    aligned_memory_offset = align_up(current_block->offset, memory_alignment);
    aligned_memory_size_to_allocate = aligned_memory_offset + size_to_allocate;
  }

  U64 size_to_zero = 0;
  if (with_zero) {
    size_to_zero =
        min(current_block->committed_size, aligned_memory_size_to_allocate) -
        aligned_memory_offset;
  }

  if (current_block->committed_size < aligned_memory_size_to_allocate) {
    U64 new_commit_size = aligned_memory_size_to_allocate +
                          current_block->requested_commit_size - 1;
    new_commit_size -= new_commit_size % current_block->requested_commit_size;
    U64 clamped_commit_size =
        max(new_commit_size, current_block->reserved_size);
    U64 needed_commit_size =
        clamped_commit_size - current_block->committed_size;
    U8* committed_size_ptr = (U8*)current_block + current_block->committed_size;
    platform_commit_large_pages(committed_size_ptr, needed_commit_size);
    current_block->committed_size = clamped_commit_size;
  }

  RawPtr result = 0;
  if (current_block->committed_size >= aligned_memory_size_to_allocate) {
    result = (U8*)current_block + aligned_memory_offset;
    current_block->offset = aligned_memory_size_to_allocate;
    asan_unpoison_memory_region(result, size_to_allocate);
    if (size_to_zero != 0) {
      zero_memory(result, size_to_zero);
    }
  }

  if (result == 0) {
    abort("failed to allocate memory from arena allocator");
  }

  end_profiling();
  return result;
}

/* ----------------------------------------------------- */

Nothing
arena_pop(Arena* a, U64 amount) {
  start_profiling(1);

  U64 old_position = arena_get_position(a);
  U64 new_position = old_position;
  if (amount < old_position) {
    new_position = old_position - amount;
  }

  arena_pop_to(a, new_position);

  end_profiling();
}

/* ----------------------------------------------------- */

Nothing
arena_pop_to(Arena* a, U64 position) {
  start_profiling(1);

  Sz arena_header_size = sizeof(Arena);
  U64 normilized_position = max(arena_header_size, position);
  Arena* current_block = a->current_block;

  for (Arena* previous_block = 0;
       current_block->base_position >= normilized_position;
       current_block = previous_block) {
    previous_block = current_block->previous_block;
    current_block->offset = arena_header_size;
    current_block->previous_block = a->last_freed_block;
    a->last_freed_block = current_block;
    asan_poison_memory_region((U8*)current_block + arena_header_size,
                              current_block->reserved_size - arena_header_size);
  }

  a->current_block = current_block;
  U64 new_offset = normilized_position - current_block->base_position;
  runtime_assert(new_offset <= current_block->offset);
  asan_poison_memory_region((U8*)current_block + new_offset,
                            (current_block->offset - new_offset));
  current_block->offset = new_offset;

  end_profiling();
}

/* ----------------------------------------------------- */

Nothing
arena_clear(Arena* a) {
  start_profiling(1);

  arena_pop_to(a, 0);

  end_profiling();
}

/* ----------------------------------------------------- */

U64
arena_get_position(Arena* a) {
  start_profiling(1);

  Arena* current_block = a->current_block;
  U64 position = current_block->base_position + current_block->offset;

  end_profiling();
  return position;
}

ArenaScratch
arena_scratch_begin(Arena* a) {
  start_profiling(1);

  U64 position = arena_get_position(a);

  end_profiling();
  return (ArenaScratch){a, position};
}

Nothing
arena_scratch_end(ArenaScratch s) {
  start_profiling(1);

  arena_pop_to(s.arena, s.offset);

  end_profiling();
}

/* ===================================================== */
/*                          END                          */
/* ===================================================== */

#endif /* SEPI_ARENA_IMPLEMENTATION */
#endif /* SEPI_ARENA_H */
