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




#endif

#include <lttng/tracepoint-event.h>
