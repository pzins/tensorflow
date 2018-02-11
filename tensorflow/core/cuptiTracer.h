#undef TRACEPOINT_PROVIDER
#define TRACEPOINT_PROVIDER cuptiTracer

#undef TRACEPOINT_INCLUDE
#define TRACEPOINT_INCLUDE "./cuptiTracer.h"

#if !defined(_cuptiTracer_H) || defined(TRACEPOINT_HEADER_MULTI_READ)
#define _cuptiTracer_H

#include <lttng/tracepoint.h>

TRACEPOINT_EVENT(
    cuptiTracer,
    runtime_api_entry,
    TP_ARGS(
        uint64_t, timestamp,
        int, my_integer_arg
    ),
    TP_FIELDS(
        ctf_integer(uint64_t, timestamp, timestamp)
        ctf_integer(int, is_ready, my_integer_arg)
    )
)
TRACEPOINT_EVENT(
    cuptiTracer,
    runtime_api_exit,
    TP_ARGS(
        uint64_t, timestamp,
        int, my_integer_arg
    ),
    TP_FIELDS(
        ctf_integer(uint64_t, timestamp, timestamp)
        ctf_integer(int, is_ready, my_integer_arg)

    )
)

TRACEPOINT_EVENT(
    cuptiTracer,
    driver_api_entry,
    TP_ARGS(
        uint64_t, timestamp,
        int, my_integer_arg
    ),
    TP_FIELDS(
        ctf_integer(uint64_t, timestamp, timestamp)
        ctf_integer(int, is_ready, my_integer_arg)
    )
)
TRACEPOINT_EVENT(
    cuptiTracer,
    driver_api_exit,
    TP_ARGS(
        uint64_t, timestamp,
        int, my_integer_arg
    ),
    TP_FIELDS(
        ctf_integer(uint64_t, timestamp, timestamp)
        ctf_integer(int, is_ready, my_integer_arg)

    )
)

TRACEPOINT_EVENT(
    cuptiTracer,
    kernel_begin,
    TP_ARGS(
        const char*, my_string_arg,
        uint64_t, timestamp

    ),
    TP_FIELDS(
        ctf_string(name, my_string_arg)
        ctf_integer(uint64_t, timestamp, timestamp)

    )
)

TRACEPOINT_EVENT(
    cuptiTracer,
    kernel_end,
    TP_ARGS(
        const char*, my_string_arg,
        uint64_t, timestamp

    ),
    TP_FIELDS(
        ctf_string(name, my_string_arg)
        ctf_integer(uint64_t, timestamp, timestamp)

    )
)

TRACEPOINT_EVENT(
    cuptiTracer,
    memcpy_begin,
    TP_ARGS(
        const char*, my_string_arg,
        const char*, my_string_arg2,
        uint64_t, timestamp

    ),
    TP_FIELDS(
        ctf_string(name, my_string_arg)
        ctf_string(details, my_string_arg2)
        ctf_integer(uint64_t, timestamp, timestamp)

    )
)
TRACEPOINT_EVENT(
    cuptiTracer,
    memcpy_end,
    TP_ARGS(
        const char*, my_string_arg,
        const char*, my_string_arg2,
        uint64_t, timestamp

    ),
    TP_FIELDS(
        ctf_string(name, my_string_arg)
        ctf_string(details, my_string_arg2)
        ctf_integer(uint64_t, timestamp, timestamp)

    )
)

#endif

#include <lttng/tracepoint-event.h>
