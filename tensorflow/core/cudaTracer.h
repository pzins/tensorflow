#undef TRACEPOINT_PROVIDER
#define TRACEPOINT_PROVIDER cudaTracer

#undef TRACEPOINT_INCLUDE
#define TRACEPOINT_INCLUDE "./cudaTracer.h"

#if !defined(_cudaTracer_H) || defined(TRACEPOINT_HEADER_MULTI_READ)
#define _cudaTracer_H

#include <lttng/tracepoint.h>

TRACEPOINT_EVENT(
    cudaTracer,
    runtime_api_entry,
    TP_ARGS(
        const char*, cat_arg,
        const char*, name_arg,
        uint64_t, timestamp_arg,
        uint64_t, threadId_arg
    ),
    TP_FIELDS(
        ctf_string(cat, cat_arg)
        ctf_string(name, name_arg)
        ctf_integer(uint64_t, timestamp, timestamp_arg)
        ctf_integer(uint64_t, threadId, threadId_arg)
    )
)
TRACEPOINT_EVENT(
    cudaTracer,
    runtime_api_exit,
    TP_ARGS(
        const char*, cat_arg,
        const char*, name_arg,
        uint64_t, timestamp_arg,
        uint64_t, threadId_arg
    ),
    TP_FIELDS(
        ctf_string(cat, cat_arg)
        ctf_string(name, name_arg)
        ctf_integer(uint64_t, timestamp, timestamp_arg)
        ctf_integer(uint64_t, threadId, threadId_arg)
    )
)

TRACEPOINT_EVENT(
    cudaTracer,
    driver_api_entry,
    TP_ARGS(
        const char*, cat_arg,
        const char*, name_arg,
        uint64_t, timestamp_arg,
        uint64_t, threadId_arg
    ),
    TP_FIELDS(
        ctf_string(cat, cat_arg)
        ctf_string(name, name_arg)
        ctf_integer(uint64_t, timestamp, timestamp_arg)
        ctf_integer(uint64_t, threadId, threadId_arg)
    )
)
TRACEPOINT_EVENT(
    cudaTracer,
    driver_api_exit,
    TP_ARGS(
        const char*, cat_arg,
        const char*, name_arg,
        uint64_t, timestamp_arg,
        uint64_t, threadId_arg
    ),
    TP_FIELDS(
        ctf_string(cat, cat_arg)
        ctf_string(name, name_arg)
        ctf_integer(uint64_t, timestamp, timestamp_arg)
        ctf_integer(uint64_t, threadId, threadId_arg)
    )
)

TRACEPOINT_EVENT(
    cudaTracer,
    kernel_begin,
    TP_ARGS(
        const char*, cat_arg,
        const char*, name_arg,
        uint64_t, timestamp_arg
    ),
    TP_FIELDS(
        ctf_string(cat, cat_arg)
        ctf_string(name, name_arg)
        ctf_integer(uint64_t, timestamp, timestamp_arg)
    )
)

TRACEPOINT_EVENT(
    cudaTracer,
    kernel_end,
    TP_ARGS(
        const char*, cat_arg,
        const char*, name_arg,
        uint64_t, timestamp_arg
    ),
    TP_FIELDS(
        ctf_string(cat, cat_arg)
        ctf_string(name, name_arg)
        ctf_integer(uint64_t, timestamp, timestamp_arg)
    )
)

TRACEPOINT_EVENT(
    cudaTracer,
    kernel_queued,
    TP_ARGS(
        const char*, cat_arg,
        const char*, name_arg,
        uint64_t, timestamp_arg
    ),
    TP_FIELDS(
        ctf_string(cat, cat_arg)
        ctf_string(name, name_arg)
        ctf_integer(uint64_t, timestamp, timestamp_arg)
    )
)

TRACEPOINT_EVENT(
    cudaTracer,
    memcpy_begin,
    TP_ARGS(
        const char*, cat_arg,
        const char*, name_arg,
        const char*, details_arg,
        uint64_t, timestamp_arg
    ),
    TP_FIELDS(
        ctf_string(cat, cat_arg)
        ctf_string(name, name_arg)
        ctf_string(details, details_arg)
        ctf_integer(uint64_t, timestamp, timestamp_arg)
    )
)
TRACEPOINT_EVENT(
    cudaTracer,
    memcpy_end,
    TP_ARGS(
        const char*, cat_arg,
        const char*, name_arg,
        const char*, details_arg,
        uint64_t, timestamp_arg
    ),
    TP_FIELDS(
        ctf_string(cat, cat_arg)
        ctf_string(name, name_arg)
        ctf_string(details, details_arg)
        ctf_integer(uint64_t, timestamp, timestamp_arg)
    )
)

#endif

#include <lttng/tracepoint-event.h>
