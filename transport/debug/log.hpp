/**
 * \file log.hpp
 * \author ichramm
 *
 * Created on
 */
#ifndef debug_log_hpp__
#define debug_log_hpp__

namespace et {
namespace debug {
namespace masks {

enum value {
    tcp_trace = (1 << 0),
    udp_trace = (1 << 1)
};

}
}
}

#ifndef __TRACE_MASK
    #define __TRACE_MASK ((unsigned int)0xFFFFFFFF)
#endif


/* disable warnings */
#if defined(__clang__)
 #pragma GCC diagnostic push
 #pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif // clanf

#define __TRACE(mask, fmt, ...) do { \
    if (mask & __TRACE_MASK) \
        fprintf(stderr, "%s:%d %s - " fmt "\n", __FILE__, __LINE__, __FUNCTION__ , ##__VA_ARGS__); \
} while(false)

#define __DUMP_BUFFER(dest, title, inbuff, inlen) do { \
    char *buff = (char*)&inbuff[0]; \
    size_t len = inlen * sizeof(inbuff[0]); \
    fprintf(dest, "%s {\n", title); \
    size_t buff_pos = 0; \
    while (buff_pos < len) { \
        size_t line_pos = 0; \
        for ( ; line_pos < 16 && buff_pos + line_pos < len; ++line_pos) { \
            fprintf(dest, "%02X ", (uint8_t)buff[buff_pos + line_pos]); \
        } \
        for (size_t i = line_pos; i < 16; ++i) { \
            fprintf(dest, "-- ");\
        }\
        for (size_t i = 0; i < line_pos; ++i) { \
            char c = buff[buff_pos + i]; \
            if (c < ' ' || c > '~') c = '.'; \
            fprintf(dest, "%c", c); \
        } \
        buff_pos += line_pos; \
        fprintf(dest, "\n"); \
    } \
    fprintf(dest, "}\n"); \
} while(false)


#if defined(__clang__)
/* restore warnings */
#pragma GCC diagnostic pop
#endif


#endif // debug_log_hpp__
