/**
 *  flexsoc logging
 *
 *  All rights reserved.
 *  Tiny Labs Inc
 *  2020
 */
#include "log.h"
#include <stdio.h>
#include <stdlib.h>

static int8_t _log_level = LOG_NORMAL;

void log_init (int8_t lvl)
{
    _log_level = lvl;
}

int8_t log_level (void)
{
    return _log_level;
}

void vlog (int8_t lvl, const char *fmt, va_list ap)
{
    if (lvl <= _log_level)
        vprintf (fmt, ap);
}

void log (int8_t lvl, const char *fmt, ...)
{
    va_list va;
    va_start (va, fmt);
    vlog (lvl, fmt, va);
    if (lvl <= _log_level)
        printf ("\n");
    va_end (va);
    if (lvl <= LOG_FATAL)
        exit (-1);
}

void log_nonl (int8_t lvl, const char *fmt, ...)
{
    va_list va;
    va_start (va, fmt);
    vlog (lvl, fmt, va);
    va_end (va);
    if (lvl <= LOG_FATAL)
        exit (-1);
}

void log_dump_word (int8_t lvl, uint8_t indent, const uint32_t *data, size_t len)
{
    if (lvl <= _log_level) {
        printf ("%.*s", indent, "                  ");
        for (int i = 0; i < len; i++) {
            if (i & (i % 4) == 0) {
                printf ("%.*s", indent, "                  ");
                printf ("\n");
            }
            printf ("%08X ", data[i]);
        }
        printf ("\n");
    }   
}

void log_dump_half (int8_t lvl, uint8_t indent, const uint16_t *data, size_t len)
{
    if (lvl <= _log_level) {
        printf ("%.*s", indent, "                  ");
        for (int i = 0; i < len; i++) {
            if (i & (i % 8) == 0) {
                printf ("%.*s", indent, "                  ");
                printf ("\n");
            }
            printf ("%04X ", data[i]);
        }
        printf ("\n");
    }   
}

void log_dump_byte (int8_t lvl, uint8_t indent, const uint8_t *data, size_t len)
{
    if (lvl <= _log_level) {
        printf ("%.*s", indent, "                  ");
        for (int i = 0; i < len; i++) {
            if (i & (i % 16) == 0) {
                printf ("%.*s", indent, "                  ");
                printf ("\n");
            }
            printf ("%02X ", data[i]);
        }
        printf ("\n");
    }    
}
