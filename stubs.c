/*
 * stubs.c — Headless no-op implementations for Doom64KB on nRF52840
 *
 * These replace video, text-mode, and automap functions that were
 * removed from the tree. Game logic still calls them via the original
 * code paths, so the linker needs the symbols. All of them do nothing.
 */

#include <stdint.h>
#include "doomtype.h"
#include "doomdef.h"
#include "d_event.h"
#include "d_player.h"
#include "r_data.h"
#include "v_video.h"

/* ------------------------------------------------------------------ */
/* Video — implement the functions that v_video.h declares            */
/* Note: V_DrawNamePatchScaled is a macro in v_video.h that expands   */
/* to V_DrawNumPatchScaled(x, y, W_GetNumForName(name)). Do NOT try   */
/* to define the macro name — only the underlying function.           */
/* ------------------------------------------------------------------ */

void V_DrawBackground(int16_t backgroundnum) { (void)backgroundnum; }
void V_DrawRaw(int16_t num, uint16_t offset) { (void)num; (void)offset; }
void V_DrawRawFullScreen(int16_t num) { (void)num; }

void V_DrawPatchScaled(int16_t x, int16_t y, const patch_t __far* patch)
    { (void)x; (void)y; (void)patch; }
void V_DrawNumPatchScaled(int16_t x, int16_t y, int16_t lump)
    { (void)x; (void)y; (void)lump; }
void V_DrawNumPatchNotScaled(int16_t x, int16_t y, int16_t lump)
    { (void)x; (void)y; (void)lump; }

void V_DrawPatchNotScaled(int16_t x, int16_t y, const patch_t __far* patch)
    { (void)x; (void)y; (void)patch; }

void V_ClearViewWindow(void) {}
void V_InitDrawLine(void) {}
void V_ShutdownDrawLine(void) {}
void V_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color)
    { (void)x0; (void)y0; (void)x1; (void)y1; (void)color; }

/* ------------------------------------------------------------------ */
/* I/O subsystem                                                      */
/* ------------------------------------------------------------------ */

void I_InitGraphics(void) {}
void I_InitKeyboard(void) {}
void I_InitTimer(void) {}
void I_FinishUpdate(void) {}
void I_StartTic(void) {}
void I_SetPalette(int16_t pal) { (void)pal; }
void I_ReloadPalette(void) {}
void I_InitSound2(void) {}

/* ------------------------------------------------------------------ */
/* Rendering subsystem                                                */
/* ------------------------------------------------------------------ */

void R_Init(void) {}
void R_Shutdown(void) {}
void R_RenderPlayerView(player_t *player) { (void)player; }

/* ------------------------------------------------------------------ */
/* Automap                                                            */
/* ------------------------------------------------------------------ */

void AM_Drawer(void) {}
void AM_Ticker(void) {}
void AM_Stop(void) {}
void AM_Start(void) {}
boolean AM_Responder(event_t *ev) { (void)ev; return false; }

/* ------------------------------------------------------------------ */
/* UI drawers (HUD, status bar, intermission, menu, finale)           */
/* ------------------------------------------------------------------ */

void ST_Drawer(void) {}
void ST_doPaletteStuff(void) {}
void HU_Drawer(void) {}
void WI_Drawer(void) {}
void M_Drawer(void) {}
void F_Drawer(void) {}

/* ------------------------------------------------------------------ */
/* Screen wipe                                                        */
/* ------------------------------------------------------------------ */

void D_Wipe(void) {}
void wipe_StartScreen(void) {}
void wipe_EndScreen(void) {}

/* ------------------------------------------------------------------ */
/* Text-mode finale library (f_libt.c)                                */
/* ------------------------------------------------------------------ */

void F_TextWrite(int32_t count) { (void)count; }

int16_t V_NumPatchWidth(int16_t lump) { (void)lump; return 0; }