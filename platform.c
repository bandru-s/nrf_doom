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

#define ZONE_SIZE (160 * 1024)
static uint8_t zone_heap[ZONE_SIZE];

uint8_t __far *I_ZoneBase(uint32_t *heapSize)
{
    *heapSize = ZONE_SIZE;
    return zone_heap;
}

/* ------------------------------------------------------------------ */
/* Timer — TIMER0 at 1 kHz                                            */
/* ------------------------------------------------------------------ */

#define TIMER0_BASE 0x40008000UL

#define TIMER0_TASKS_START     (*(volatile uint32_t *)(TIMER0_BASE + 0x000))
#define TIMER0_TASKS_STOP      (*(volatile uint32_t *)(TIMER0_BASE + 0x004))
#define TIMER0_TASKS_CLEAR     (*(volatile uint32_t *)(TIMER0_BASE + 0x00C))
#define TIMER0_EVENTS_COMPARE0 (*(volatile uint32_t *)(TIMER0_BASE + 0x140))
#define TIMER0_INTENSET        (*(volatile uint32_t *)(TIMER0_BASE + 0x304))
#define TIMER0_MODE            (*(volatile uint32_t *)(TIMER0_BASE + 0x504))
#define TIMER0_BITMODE         (*(volatile uint32_t *)(TIMER0_BASE + 0x508))
#define TIMER0_PRESCALER       (*(volatile uint32_t *)(TIMER0_BASE + 0x510))
#define TIMER0_CC0             (*(volatile uint32_t *)(TIMER0_BASE + 0x540))

/* NVIC Set-Enable Register 0 */
#define NVIC_ISER0  (*(volatile uint32_t *)0xE000E100)

volatile uint32_t g_ms_ticks = 0;
volatile uint32_t dbg_status[8] = {0};

void TIMER0_IRQHandler(void)
{
    if (TIMER0_EVENTS_COMPARE0)
    {
        TIMER0_EVENTS_COMPARE0 = 0;
        g_ms_ticks++;
    }
}

void I_InitTimer(void)
{
    semihost_write0("  TIMER0: configuring\n");

    TIMER0_TASKS_STOP = 1;
    TIMER0_TASKS_CLEAR = 1;
    TIMER0_MODE = 0;                 /* Timer mode */
    TIMER0_BITMODE = 0;              /* 16-bit (max 0xFFFF) */
    TIMER0_PRESCALER = 4;            /* 16 MHz / 2^4 = 1 MHz */
    TIMER0_CC0 = 1000;               /* 1 MHz / 1000 = 1 kHz = 1 ms */

    TIMER0_TASKS_START = 1;
    semihost_write0("  TIMER0: started\n");
}

int32_t I_GetTime(void)
{
    extern void DMX_Update(void);
    
    static uint32_t last_counter = 0;
    static uint32_t accumulated_ms = 0;

    /* Capture current TIMER0 counter into CC[1] */
    *(volatile uint32_t *)(TIMER0_BASE + 0x044) = 1;   /* TASKS_CAPTURE[1] */
    uint32_t counter = *(volatile uint32_t *)(TIMER0_BASE + 0x544);   /* CC[1] */

    /* 24-bit counter, wraps every ~16.7 seconds at 1 MHz */
    uint32_t delta = (counter - last_counter) & 0x00FFFFFF;
    last_counter = counter;

    /* 1 MHz counter, so 1000 counts = 1 ms */
    accumulated_ms += delta / 1000;
    DMX_Update();
    return (int32_t)((accumulated_ms * TICRATE) / 1000);
}

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
/* Audio — PWM playback via P0.19                                     */
/* ------------------------------------------------------------------ */

extern const uint8_t  doom_sfx_data[];
extern const uint32_t doom_sfx_offsets[];
extern const uint32_t doom_sfx_lengths[];

#define PWM0_BASE 0x4001C000UL
#define PWM0(off) (*(volatile uint32_t *)(PWM0_BASE + (off)))

#define AUDIO_PIN       19          /* P0.19 */
#define PWM_TOP         242         /* COUNTERTOP */
#define PWM_REFRESH     11          /* 16MHz/242/12 ? 5512 Hz */
#define PWM_HALF        256         /* samples per half-buffer */
#define PWM_ACTIVE_HIGH 0x8000u

#define P0_OUTCLR (*(volatile uint32_t *)0x5000050C)
#define P0_DIRSET (*(volatile uint32_t *)0x50000518)

static uint16_t pwm_buf[2][PWM_HALF] __attribute__((aligned(4)));

static const uint8_t *active_pcm = 0;
static uint32_t active_len = 0;
static volatile uint32_t active_pos = 0;

static void pwm_fill(int which)
{
    for (int i = 0; i < PWM_HALF; i++) {
        int s = 0;
        if (active_pcm && active_pos < active_len) {
            s = (int)active_pcm[active_pos] - 128;
            active_pos++;
        }
        int duty = (PWM_TOP / 2) + (s / 8);   /* ÷8 attenuation */
        if (duty < 0) duty = 0;
        if (duty > (int)PWM_TOP) duty = (int)PWM_TOP;
        pwm_buf[which][i] = (uint16_t)(PWM_ACTIVE_HIGH | (uint16_t)duty);
    }
}

void DMX_Init(void)
{
    dbg_status[3] = 0xA0;
    semihost_write0("  DMX_Init: starting\n");

    dbg_status[3] = 0xA1;
    P0_OUTCLR = (1UL << AUDIO_PIN);
    P0_DIRSET = (1UL << AUDIO_PIN);

    dbg_status[3] = 0xA2;
    pwm_fill(0);
    pwm_fill(1);

    dbg_status[3] = 0xA3;
    PWM0(0x500) = 0;              /* disable */
    PWM0(0x560) = AUDIO_PIN;      /* PSEL.OUT[0] */
    PWM0(0x564) = 0xFFFFFFFFu;    /* PSEL.OUT[1] disconnected */
    PWM0(0x568) = 0xFFFFFFFFu;
    PWM0(0x56C) = 0xFFFFFFFFu;
    PWM0(0x504) = 0;              /* MODE */
    PWM0(0x50C) = 0;              /* PRESCALER */
    PWM0(0x508) = PWM_TOP;        /* COUNTERTOP */
    PWM0(0x510) = 0;              /* DECODER */
    PWM0(0x514) = 0xFFFF;         /* LOOP */
    PWM0(0x200) = (1u << 2);      /* SHORTS: LOOPSDONE -> SEQSTART[0] */

    dbg_status[3] = 0xA4;
    PWM0(0x520) = (uint32_t)pwm_buf[0];
    PWM0(0x524) = PWM_HALF;
    PWM0(0x528) = PWM_REFRESH;
    PWM0(0x52C) = 0;
    PWM0(0x540) = (uint32_t)pwm_buf[1];
    PWM0(0x544) = PWM_HALF;
    PWM0(0x548) = PWM_REFRESH;
    PWM0(0x54C) = 0;

    dbg_status[3] = 0xA5;
    PWM0(0x108) = 0;
    PWM0(0x10C) = 0;
    PWM0(0x500) = 1;              /* ENABLE */
    dbg_status[3] = 0xA6;
    semihost_write0("  DMX_Init: PWM configured\n");
}

void DMX_Init2(void) { }
void DMX_Shutdown(void) { }

void DMX_Play(sfxenum_t id)
{
    if ((int)id <= 0)
        return;
    if ((int)id >= NUMSFX)
        return;

    uint32_t len = doom_sfx_lengths[id];
    if (len == 0)
        return;

    active_pcm = &doom_sfx_data[doom_sfx_offsets[id]];
    active_len = len;
    active_pos = 0;

    static int pwm_started = 0;
    if (!pwm_started) {
        pwm_started = 1;
        PWM0(0x108) = 0;
        PWM0(0x10C) = 0;
        PWM0(0x500) = 1;    /* ENABLE */
        PWM0(0x008) = 1;  /* TASKS_SEQSTART[0] disabled for now */
        dbg_status[3] = 0xB2;   /* first DMX_Play called */
    }
}

void DMX_Update(void)
{
    static uint32_t update_count = 0;
    update_count++;
    if (PWM0(0x108)) {
        PWM0(0x108) = 0;
        pwm_fill(1);
    }
    if (PWM0(0x10C)) {
        PWM0(0x10C) = 0;
        pwm_fill(0);
    }
}