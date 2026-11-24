/*         ______   ___    ___ 
 *        /\  _  \ /\_ \  /\_ \ 
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      DirectDraw gfx drivers header
 *
 *      By Stefan Schimanski.
 *
 *      See readme.txt for copyright information.
 */

#ifndef WDDRAW_H_INCLUDED
#define WDDRAW_H_INCLUDED

#define DIRECTDRAW_VERSION 0x0300

#include "allegro.h"
#include "allegro/aintern.h"
#include "allegro/aintwin.h"
#include <ddraw.h>

#ifndef ALLEGRO_WINDOWS
#error something is wrong with the makefile
#endif


/* general */
#define _enter_gfx_critical() EnterCriticalSection(&gfx_crit_sect);
#define _exit_gfx_critical() LeaveCriticalSection(&gfx_crit_sect);

AL_VAR(CRITICAL_SECTION, gfx_crit_sect);
AL_VAR(char *, pseudo_surf_mem);

typedef struct BMP_EXTRA_INFO {
   LPDIRECTDRAWSURFACE surf;
   struct BMP_EXTRA_INFO *next;
   struct BMP_EXTRA_INFO *prev;
   int flags;
   int locked;
} BMP_EXTRA_INFO;

#define BMP_EXTRA(bmp) ((struct BMP_EXTRA_INFO *)(bmp->extra))
#define BMP_FLAG_LOST  0x00000004

AL_VAR(LPDIRECTDRAW, directdraw);
AL_VAR(LPDIRECTDRAWSURFACE, dd_prim_surface);
AL_VAR(LPDIRECTDRAWPALETTE, dd_palette);
AL_VAR(LPDIRECTDRAWCLIPPER, dd_clipper);
AL_VAR(DDCAPS, dd_caps);
AL_VAR(struct BITMAP *, dd_frontbuffer);


/* vtable routines */
AL_FUNC(void, gfx_directx_exit, (struct BITMAP *b));
AL_FUNC(void, gfx_directx_sync, (void));
AL_FUNC(void, gfx_directx_set_palette, (struct RGB *p, int from, int to, int vsync));
AL_FUNC(int, gfx_directx_poll_scroll, (void));
AL_FUNC(void, gfx_directx_created_sub_bitmap, (struct BITMAP *bmp, struct BITMAP *parent));
AL_FUNC(struct BITMAP *, gfx_directx_create_video_bitmap, (int width, int height));
AL_FUNC(void, gfx_directx_destroy_video_bitmap, (struct BITMAP *bitmap));
AL_FUNC(int, gfx_directx_show_video_bitmap, (struct BITMAP *bitmap));
AL_FUNC(int, gfx_directx_request_video_bitmap, (struct BITMAP *bitmap));
AL_FUNC(struct BITMAP *, gfx_directx_create_system_bitmap, (int width, int height));
AL_FUNC(void, gfx_directx_destroy_system_bitmap, (struct BITMAP *bitmap));


/* driver initialisation and shutdown */
AL_FUNC(int, init_directx, (void));
AL_FUNC(int, set_video_mode, (int w, int h, int v_w, int v_h, int color_depth));
AL_FUNC(int, create_palette, (LPDIRECTDRAWSURFACE surf));
AL_FUNC(int, create_primary, (int w, int h, int color_depth));
AL_FUNC(int, create_clipper, (HWND hwnd));
AL_FUNC(int, setup_driver, (GFX_DRIVER * drv, int w, int h, int color_depth));
AL_FUNC(int, finalize_directx_init, (void));
AL_FUNC(int, gfx_directx_wnd_exit, (void));
AL_FUNC(void, gfx_directx_exit, (struct BITMAP *b));
AL_FUNC(int, enable_acceleration, (GFX_DRIVER * drv));


/* bitmap locking */
AL_FUNC(void, gfx_directx_lock, (struct BITMAP *bmp));
AL_FUNC(void, gfx_directx_unlock, (struct BITMAP *bmp));
AL_FUNC(void, gfx_directx_lock_internal, (BITMAP * bmp, int temp));
AL_FUNC(void, gfx_directx_unlock_internal, (BITMAP * bmp, int *temp));
AL_FUNC(void, gfx_directx_write_bank, (void));
AL_FUNC(void, gfx_directx_unwrite_bank, (void));


/* bitmap creation */
AL_FUNC(LPDIRECTDRAWSURFACE, gfx_directx_create_surface, (int w, int h, int color_depth,
   int video, int primary, int overlay));
AL_FUNC(BITMAP *, make_directx_bitmap, (LPDIRECTDRAWSURFACE surf, int w, int h, int color_depth, int id));


/* video bitmap list */
AL_FUNC(void, register_directx_bitmap, (struct BITMAP *bmp));
AL_FUNC(void, unregister_directx_bitmap, (struct BITMAP *bmp));
AL_FUNC(void, unregister_all_directx_bitmaps, (void));

AL_VAR(BMP_EXTRA_INFO *, directx_bmp_list);


/* overlay */
AL_FUNC(void, hide_overlay, (void));

AL_VAR(LPDIRECTDRAWSURFACE, overlay_surface);
AL_VAR(BOOL, overlay_visible);

#endif

