/*********************************************************************
*                    SEGGER Microcontroller GmbH                     *
*                        The Embedded Experts                        *
**********************************************************************
-------------------------- END-OF-HEADER -----------------------------

File    : main.c
Purpose : Doom64KB entry point for nRF52840 GPS tracker.
*/

#include <stdint.h>
#include "semihost.h"

/* ------------------------------------------------------------------ */
/* Register access                                                    */
/* ------------------------------------------------------------------ */

#define P0_OUTSET (*(volatile uint32_t *)0x50000508)
#define P0_OUTCLR (*(volatile uint32_t *)0x5000050C)
#define P0_DIRSET (*(volatile uint32_t *)0x50000518)
#define P0_DIRCLR (*(volatile uint32_t *)0x5000051C)

#define P1_OUTSET (*(volatile uint32_t *)0x50000808)
#define P1_OUTCLR (*(volatile uint32_t *)0x5000080C)
#define P1_DIRSET (*(volatile uint32_t *)0x50000818)
#define P1_DIRCLR (*(volatile uint32_t *)0x5000081C)
#define P1_IN     (*(volatile uint32_t *)0x50000810)

/* ------------------------------------------------------------------ */
/* Doom entry point                                                   */
/* ------------------------------------------------------------------ */

extern void D_DoomMain(int argc, const char * const *argv);

int main(void)
{
    /* ---- 1. Power latch: MUST be first. Holds PMIC power on. ---- */
    P0_DIRSET = 0x0000003C;
    P0_OUTSET = 0x0000003C;

    /* ---- 2. Early log so we know we got here ---- */
    semihost_write0("\n=== Doom64KB on nRF52840 ===\n");
    semihost_write0("Power latch engaged (P0.02-P0.05 high)\n");

    /* ---- 3. Hand off to Doom ---- */
    semihost_write0("Calling D_DoomMain...\n");
    D_DoomMain(0, 0);
    semihost_write0("D_DoomMain returned - should never happen\n");

    for (;;) { }
}