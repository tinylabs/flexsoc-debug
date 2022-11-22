/**
 *  Flexsoc logging library
 *
 *  All rights reserved.
 *  Tiny Labs Inc
 *  2020
 */

#ifndef LOG_H
#define LOG_H

#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>

#define LOG_FATAL  -2  // Exit if fatal
#define LOG_ERR    -1  // Always print errors
#define LOG_SILENT  0  // Mostly silent (apart from errors)
#define LOG_NORMAL  1  // Normal printouts
#define LOG_DEBUG   2  // Add debug info
#define LOG_TRACE   3  // Trace at transaction layer
#define LOG_REG     4  // Trace at register layer
#define LOG_TRANS   5  // Trace transport layer

// Init logging
void log_init (int8_t log_level);

// Get log level
int8_t log_level (void);

// Log message
void log (int8_t lvl, const char *fmt, ...);
void log_nonl (int8_t lvl, const char *fmt, ...);
void vlog (int8_t lvl, const char *fmt, va_list ap);

// Dumping data
void log_dump_word (int8_t lvl, uint8_t indent, const uint32_t *data, size_t len);
void log_dump_half (int8_t lvl, uint8_t indent, const uint16_t *data, size_t len);
void log_dump_byte (int8_t lvl, uint8_t indent, const uint8_t *data, size_t len);

#endif /* LOG_H */
