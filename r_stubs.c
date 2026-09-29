/*
 * r_stubs.c — temporary placeholders for rendering functions
 *
 * WARNING: R_PointInSubsector and R_PointToAngle3 are NOT correct here.
 * They let the linker succeed so we can flash and see the boot log,
 * but the game will behave incorrectly until real implementations are
 * provided (pull r_main.c from upstream and prune the drawing parts).
 *
 * R_ResetPlanes and P_UpdateAnimatedFlat are pure-visual and can stay
 * as no-ops permanently for a headless build.
 */

#include <stdint.h>
#include "doomtype.h"
#include "doomdef.h"
#include "r_defs.h"
#include "m_fixed.h"

extern subsector_t __far *_g_subsectors;

subsector_t *R_PointInSubsector(fixed_t x, fixed_t y)
{
    (void)x;
    (void)y;
    return _g_subsectors;   /* wrong, but a valid pointer */
}

angle_t R_PointToAngle3(fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2)
{
    (void)x1;
    (void)y1;
    (void)x2;
    (void)y2;
    return 0;
}

void R_ResetPlanes(void) {}
void P_UpdateAnimatedFlat(void) {}