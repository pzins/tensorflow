#undef TRACEPOINT_PROVIDER
#define TRACEPOINT_PROVIDER tensorflowTracer

#undef TRACEPOINT_INCLUDE
#define TRACEPOINT_INCLUDE "./tensorflowTracer.h"

#if !defined(_TENSORFLOWTRACER_H) || defined(TRACEPOINT_HEADER_MULTI_READ)
#define _TENSORFLOWTRACER_H

#include <lttng/tracepoint.h>

TRACEPOINT_EVENT(
    tensorflowTracer,
    process_entry,
    TP_ARGS(
        const char*, name_arg,
        uint64_t, schedule_arg
    ),
    TP_FIELDS(
        ctf_string(name, name_arg)
        ctf_integer(uint64_t, schedule, schedule_arg)
    )
)

TRACEPOINT_EVENT(
    tensorflowTracer,
    process_exit,
    TP_ARGS(
        const char*, name_arg,
        uint64_t, schedule_arg
    ),
    TP_FIELDS(
        ctf_string(name, name_arg)
        ctf_integer(uint64_t, schedule, schedule_arg)
    )
)


TRACEPOINT_EVENT(
    tensorflowTracer,
    inline_ready_entry,
    TP_ARGS(
        const char*, name_arg
    ),
    TP_FIELDS(
        ctf_string(name, name_arg)
    )
)
TRACEPOINT_EVENT(
    tensorflowTracer,
    inline_ready_exit,
    TP_ARGS(
        const char*, name_arg
    ),
    TP_FIELDS(
        ctf_string(name, name_arg)
    )
)

TRACEPOINT_EVENT(
    tensorflowTracer,
    push_succ_entry,
    TP_ARGS(
        const char*, name_arg
    ),
    TP_FIELDS(
        ctf_string(name, name_arg)
    )
)
TRACEPOINT_EVENT(
    tensorflowTracer,
    push_succ_exit,
    TP_ARGS(
        const char*, name_arg,
        int, is_ready_arg
    ),
    TP_FIELDS(
        ctf_string(name, name_arg)
        ctf_integer(int, is_ready, is_ready_arg)
    )
)


TRACEPOINT_EVENT(
    tensorflowTracer,
    session_start,
    TP_ARGS(
        const char*, name_arg,
        int, count_arg
    ),
    TP_FIELDS(
        ctf_string(name, name_arg)
        ctf_integer(int, count, count_arg)
    )
)
TRACEPOINT_EVENT(
    tensorflowTracer,
    session_end,
    TP_ARGS(
        const char*, name_arg,
        int, count_arg
    ),
    TP_FIELDS(
        ctf_string(name, name_arg)
        ctf_integer(int, count, count_arg)
    )
)

TRACEPOINT_EVENT(
    tensorflowTracer,
    operation_start,
    TP_ARGS(
        const char*, placement_arg,
        const char*, name_arg
    ),
    TP_FIELDS(
        ctf_string(placement, placement_arg)
        ctf_string(name, name_arg)
    )
)
TRACEPOINT_EVENT(
    tensorflowTracer,
    operation_end,
    TP_ARGS(
        const char*, placement_arg,
        const char*, name_arg
    ),
    TP_FIELDS(
        ctf_string(placement, placement_arg)
        ctf_string(name, name_arg)
    )
)

TRACEPOINT_EVENT(
    tensorflowTracer,
    async_operation_start,
    TP_ARGS(
        const char*, placement_arg,
        const char*, name_arg
    ),
    TP_FIELDS(
        ctf_string(placement, placement_arg)
        ctf_string(name, name_arg)
    )
)
TRACEPOINT_EVENT(
    tensorflowTracer,
    async_operation_end,
    TP_ARGS(
        const char*, placement_arg,
        const char*, name_arg
    ),
    TP_FIELDS(
        ctf_string(placement, placement_arg)
        ctf_string(name, name_arg)
    )
)

TRACEPOINT_EVENT(
    tensorflowTracer,
    allocate_chunk_entry,
    TP_ARGS(
        const char*, name_arg
    ),
    TP_FIELDS(
        ctf_string(name, name_arg)
    )
)
TRACEPOINT_EVENT(
    tensorflowTracer,
    allocate_chunk_exit,
    TP_ARGS(
        const char*, name_arg
    ),
    TP_FIELDS(
        ctf_string(name, name_arg)
    )
)

TRACEPOINT_EVENT(
    tensorflowTracer,
    allocate_raw_internal_entry,
    TP_ARGS(
        const char*, name_arg,
        const char*, ptr_arg,
        int, num_bytes_arg,
        int, rounded_bytes_arg,
        int, bin_num_arg
    ),
    TP_FIELDS(
        ctf_string(name, name_arg)
        ctf_string(ptr, ptr_arg)
        ctf_integer(int, num_bytes, num_bytes_arg)
        ctf_integer(int, rounded_bytes, rounded_bytes_arg)
        ctf_integer(int, bin_num, bin_num_arg)


    )
)
TRACEPOINT_EVENT(
    tensorflowTracer,
    allocate_raw_internal_exit,
    TP_ARGS(
        const char*, name_arg,
        const char*, ptr_arg,
        int, num_bytes_arg,
        int, rounded_bytes_arg,
        int, bin_num_arg,
        int, need_extend_arg,
        int, success_arg
    ),
    TP_FIELDS(
        ctf_string(name, name_arg)
        ctf_string(ptr, ptr_arg)
        ctf_integer(int, num_bytes, num_bytes_arg)
        ctf_integer(int, rounded_bytes, rounded_bytes_arg)
        ctf_integer(int, bin_num, bin_num_arg)
        ctf_integer(int, need_extend, need_extend_arg)
        ctf_integer(int, success, success_arg)

    )
)


// int64 num_allocs;        // Number of allocations.
// int64 bytes_in_use;      // Number of bytes in use.
// int64 max_bytes_in_use;  // The maximum bytes in use.
// int64 max_alloc_size;    // The max single allocation seen.

TRACEPOINT_EVENT(
    tensorflowTracer,
    find_chunk_ptr,
    TP_ARGS(
        const char*, name_arg,
        const char*, allocator_name_arg,
        uint64_t, num_allocs_arg,
        uint64_t, bytes_in_use_arg,
        uint64_t, max_bytes_in_use_arg,
        uint64_t, max_alloc_size_arg
    ),
    TP_FIELDS(
        ctf_string(name, name_arg)
        ctf_string(allocator_name, allocator_name_arg)
        ctf_integer(uint64_t, num_allocs, num_allocs_arg)
        ctf_integer(uint64_t, bytes_in_use, bytes_in_use_arg)
        ctf_integer(uint64_t, max_bytes_in_use, max_bytes_in_use_arg)
        ctf_integer(uint64_t, max_alloc_size, max_alloc_size_arg)
    )
)


TRACEPOINT_EVENT(
    tensorflowTracer,
    deallocate_raw_internal_entry,
    TP_ARGS(
        const char*, name_arg,
        const char*, ptr_arg,
        int, num_bytes_arg
    ),
    TP_FIELDS(
        ctf_string(name, name_arg)
        ctf_string(ptr, ptr_arg)
        ctf_integer(int, num_bytes, num_bytes_arg)
    )
)

TRACEPOINT_EVENT(
    tensorflowTracer,
    deallocate_raw_internal_exit,
    TP_ARGS(
        const char*, name_arg,
        const char*, ptr_arg,
        int, num_bytes_arg
    ),
    TP_FIELDS(
        ctf_string(name, name_arg)
        ctf_string(ptr, ptr_arg)
        ctf_integer(int, num_bytes, num_bytes_arg)
    )
)
TRACEPOINT_EVENT(
    tensorflowTracer,
    do_create_entry,
    TP_ARGS(
        const char*, name_arg,
        const char*, container_arg
    ),
    TP_FIELDS(
        ctf_string(name, name_arg)
        ctf_string(container, container_arg)
    )
)

TRACEPOINT_EVENT(
    tensorflowTracer,
    do_create_exit,
    TP_ARGS(
        const char*, name_arg,
        const char*, container_arg,
        int, success_arg
    ),
    TP_FIELDS(
        ctf_string(name, name_arg)
        ctf_string(container, container_arg)
        ctf_integer(int, success, success_arg)
    )
)
TRACEPOINT_EVENT(
    tensorflowTracer,
    cleanup_entry,
    TP_ARGS(
        const char*, container_arg
    ),
    TP_FIELDS(
        ctf_string(name, container_arg)
    )
)
TRACEPOINT_EVENT(
    tensorflowTracer,
    cleanup_exit,
    TP_ARGS(
        const char*, container_arg
    ),
    TP_FIELDS(
        ctf_string(name, container_arg)
    )
)





TRACEPOINT_EVENT(
    tensorflowTracer,
    gpu_bfc_alloc_entry,
    TP_ARGS(
        const char*, name_arg,
        uint64_t, num_bytes_arg,
        int, alignment_arg
    ),
    TP_FIELDS(
        ctf_string(name, name_arg)
        ctf_integer(uint64_t, num_bytes, num_bytes_arg)
        ctf_integer(int, alignment, alignment_arg)
    )
)
TRACEPOINT_EVENT(
    tensorflowTracer,
    gpu_bfc_alloc_exit,
    TP_ARGS(
        const char*, name_arg,
        uint64_t, num_bytes_arg,
        int, alignment_arg
    ),
    TP_FIELDS(
        ctf_string(name, name_arg)
        ctf_integer(uint64_t, num_bytes, num_bytes_arg)
        ctf_integer(int, alignment, alignment_arg)
    )
)

TRACEPOINT_EVENT(
    tensorflowTracer,
    gpu_bfc_free_entry,
    TP_ARGS(
        const char*, name_arg,
        uint64_t, num_bytes_arg
    ),
    TP_FIELDS(
        ctf_string(name, name_arg)
        ctf_integer(int64_t, num_bytes, num_bytes_arg)
    )
)
TRACEPOINT_EVENT(
    tensorflowTracer,
    gpu_bfc_free_exit,
    TP_ARGS(
        const char*, name_arg,
        uint64_t, num_bytes_arg
    ),
    TP_FIELDS(
        ctf_string(name, name_arg)
        ctf_integer(int64_t, num_bytes, num_bytes_arg)
    )
)




#endif

#include <lttng/tracepoint-event.h>
