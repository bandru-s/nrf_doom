#include <stdint.h>
#include "semihost.h"

void semihost_write0(const char *s)
{
    
    if (!(*(volatile uint32_t *)0xE000EDF0 & 1u)) return;
    register uint32_t r0 __asm__("r0") = 0x04;
    register const char *r1 __asm__("r1") = s;
    __asm__ volatile("bkpt 0xAB" : "+r"(r0) : "r"(r1) : "memory");
}