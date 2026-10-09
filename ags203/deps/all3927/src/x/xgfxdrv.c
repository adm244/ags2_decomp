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
 *      Video driver for X-Windows.
 *
 *      By Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/aintunix.h"
#include "xwin.h"



static BITMAP *_xwin_gfx_init(int w, int h, int vw, int vh, int color_depth);
static void _xwin_gfx_exit(BITMAP *bmp);
static void _xwin_gfx_vsync(void);


GFX_DRIVER gfx_xwin =
{
   GFX_XWINDOWS,
   empty_string,
   empty_string,
   "X-Windows graphics",
   _xwin_gfx_init,
   _xwin_gfx_exit,
   _xwin_scroll_screen,
   _xwin_gfx_vsync,
   _xwin_set_palette_range,
   NULL, NULL, NULL,
   NULL, NULL, NULL, NULL,
   NULL, NULL,
   NULL, NULL, NULL, NULL,
   NULL,
   NULL, NULL,
   320, 200,
   TRUE,
   0, 0,
   0x10000,
   0
};



/* list the available drivers */
_DRIVER_INFO _xwin_gfx_driver_list[] =
{
   {  GFX_XWINDOWS, &gfx_xwin, TRUE  },
   {  0,            NULL,      0     }
};



/* _xwin_gfx_init:
 *  Creates screen bitmap.
 */
static BITMAP *_xwin_gfx_init(int w, int h, int vw, int vh, int color_depth)
{
   return _xwin_create_screen(&gfx_xwin, w, h, vw, vh, color_depth);
}



/* _xwin_gfx_exit:
 *  Shuts down the X-Windows driver.
 */
static void _xwin_gfx_exit(BITMAP *bmp)
{
   _xwin_destroy_screen();
}



/* _xwin_gfx_vsync:
 *  Wait for vertical retrace.
 */
static void _xwin_gfx_vsync(void)
{
}

