#ifndef SEMIHOST_H
#define SEMIHOST_H

#include <stdint.h>

/*
 * ARM semihosting over SWD.
 *
 * Uses bkpt 0xAB. Requires OpenOCD with `arm semihosting enable`.
 *
 * SYS_WRITE0 = 0x04  — print null-terminated string from r1
 * SYS_EXIT   = 0x18  — halt target with exit code in r1
 */

static inline void semihost_write0(const char *s)
{
    register uint32_t r0 __asm__("r0") = 0x04;
    register const char *r1 __asm__("r1") = s;
    __asm__ volatile("bkpt 0xAB" : "+r"(r0) : "r"(r1) : "memory");
}

static inline void semihost_writec(char c)
{
    char buf[2] = { c, 0 };
    semihost_write0(buf);
}

static inline void semihost_exit(uint32_t code)
{
    register uint32_t r0 __asm__("r0") = 0x18;
    register uint32_t r1 __asm__("r1") = code;
    __asm__ volatile("bkpt 0xAB" : "+r"(r0) : "r"(r1) : "memory");
    for (;;) { }
}

/* Print a 32-bit value as 0xXXXXXXXX */
static inline void semihost_hex32(uint32_t v)
{
    char buf[11] = "0x00000000";
    const char *h = "0123456789ABCDEF";
    for (int i = 0; i < 8; i++) {
        buf[2 + i] = h[(v >> (28 - i * 4)) & 0xF];
    }
    semihost_write0(buf);
    semihost_writec('\n');
}

#endif