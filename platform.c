/*
 * platform.c — nRF52840 platform layer for Doom64KB headless port
 */

#include <stdint.h>
#include <stdarg.h>
#include "doomdef.h"
#include "doomtype.h"
#include "i_system.h"
#include "i_sound.h"
#include "semihost.h"
#include "sounds.h" 
/* ------------------------------------------------------------------ */
/* Zone heap                                                          */
/* ------------------------------------------------------------------ */

#define ZONE_SIZE (96 * 1024)
static uint8_t zone_heap[ZONE_SIZE];

uint8_t __far *I_ZoneBase(uint32_t *heapSize)
{
    *heapSize = ZONE_SIZE;
    return zone_heap;
}

/* ------------------------------------------------------------------ */
/* Timer                                                              */
/* ------------------------------------------------------------------ */

volatile uint32_t g_ms_ticks = 0;

int32_t I_GetTime(void)
{
    return (int32_t)(g_ms_ticks * TICRATE / 1000);
}

void I_InitTimer(void) {}

/* ------------------------------------------------------------------ */
/* Input                                                              */
/* ------------------------------------------------------------------ */

void I_InitKeyboard(void) {}
void I_StartTic(void) {}

/* ------------------------------------------------------------------ */
/* Palette                                                            */
/* ------------------------------------------------------------------ */

void I_SetPalette(int8_t pal) { (void)pal; }
void I_ReloadPalette(void) {}

/* ------------------------------------------------------------------ */
/* Error / quit                                                       */
/* ------------------------------------------------------------------ */

_Noreturn void I_Error(const char *error, ...)
{
    semihost_write0("ERROR: ");
    semihost_write0(error);
    semihost_write0("\n");
    for (;;) { }
}

_Noreturn void I_Quit(void)
{
    semihost_write0("I_Quit called\n");
    for (;;) { }
}

/* ------------------------------------------------------------------ */
/* Audio stubs (until real PWM backend)                               */
/* ------------------------------------------------------------------ */

void DMX_Init(void) {}
void DMX_Init2(void) {}

void DMX_Play(sfxenum_t id) { (void)id; }