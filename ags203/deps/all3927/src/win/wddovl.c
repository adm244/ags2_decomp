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
 *      DirectDraw overlay gfx driver.
 *
 *      By Stefan Schimanski.
 *
 *      See readme.txt for copyright information.
 */

#include "wddraw.h"



static struct BITMAP *init_directx_ovl(int w, int h, int v_w, int v_h, int color_depth);



GFX_DRIVER gfx_directx_ovl =
{
   GFX_DIRECTX_OVL,
   empty_string,
   empty_string,
   "DirectDraw overlay",
   init_directx_ovl,
   gfx_directx_exit,
   NULL,			// AL_METHOD(int, scroll, (int x, int y));   
   gfx_directx_sync,
   gfx_directx_set_palette,
   NULL,			// AL_METHOD(int, request_scroll, (int x, int y));
   NULL,			// gfx_directx_poll_scroll,
   NULL,			// AL_METHOD(void, enable_triple_buffer, (void));
   gfx_directx_create_video_bitmap,
   gfx_directx_destroy_video_bitmap,
   NULL,			// gfx_directx_show_video_bitmap,
   NULL,			// gfx_directx_request_video_bitmap,
   gfx_directx_create_system_bitmap,
   gfx_directx_destroy_system_bitmap,
   NULL,			// AL_METHOD(int, set_mouse_sprite, (struct BITMAP *sprite, int xfocus, int yfocus));
   NULL,			// AL_METHOD(int, show_mouse, (struct BITMAP *bmp, int x, int y));
   NULL,			// AL_METHOD(void, hide_mouse, (void));
   NULL,			// AL_METHOD(void, move_mouse, (int x, int y));
   NULL,			// AL_METHOD(void, drawing_mode, (void));
   0, 0,			// int w, h;                     /* physical (not virtual!) screen size */
   TRUE,			// int linear;                   /* true if video memory is linear */
   0,				// long bank_size;               /* bank size, in bytes */
   0,				// long bank_gran;               /* bank granularity, in bytes */
   0,				// long vid_mem;                 /* video memory size, in bytes */
   0,				// long vid_phys_base;           /* physical address of video memory */
};



LPDIRECTDRAWSURFACE overlay_surface = NULL;
BOOL overlay_visible = FALSE;


 
/* create_overlay:
 */
static int create_overlay(int w, int h, int color_depth)
{
   /* create primary surface */
   overlay_surface = gfx_directx_create_surface(w, h, color_depth, 1, 0, 1);
   if (!overlay_surface) {
      TRACE("Can't create overlay surface.\n");
      return -1;
   }

   return 0;
}



/* show_overlay:
 */
static int show_overlay(int x, int y, int w, int h)
{
   HRESULT hr;
   DDCOLORKEY key;
   RECT dest_rect =
   {x, y, x + w, y + h};

   TRACE("show_overlay(%d, %d, %d, %d)\n", x, y, w, h);

   overlay_visible = FALSE;

   /* dest color keying */
   key.dwColorSpaceLowValue = (wnd_back_color & 0xffff);
   key.dwColorSpaceHighValue = (wnd_back_color >> 16);

   hr = IDirectDrawSurface_SetColorKey(dd_prim_surface, DDCKEY_DESTOVERLAY, &key);
   if (FAILED(hr)) {
      TRACE("Can't set overlay dest color key\n");
      return -1;
   }

   /* update overlay */
   TRACE("Updating overlay (key=0x%x)\n", wnd_back_color);
   hr = IDirectDrawSurface_UpdateOverlay(overlay_surface, NULL,
					 dd_prim_surface, &dest_rect,
				     DDOVER_SHOW | DDOVER_KEYDEST, NULL);
   if (FAILED(hr)) {
      TRACE("Can't display overlay (%x)\n", hr);
      return -1;
   }

   overlay_visible = TRUE;

   return 0;
}



/* hide_overlay:
 */
void hide_overlay(void)
{
   IDirectDrawSurface_UpdateOverlay(dd_prim_surface, NULL,
      overlay_surface, NULL, DDOVER_HIDE, NULL);

   overlay_visible = FALSE;
}



/* update_overlay:
 *  moves and resizes overlay to fit into the window's client area
 */
static int update_overlay()
{
   RECT pos;
   RECT size =
   {0, 0, 100, 100};

   AdjustWindowRect(&size, GetWindowLong(allegro_wnd, GWL_STYLE), FALSE);
   GetWindowRect(allegro_wnd, &pos);

   return show_overlay(pos.left - size.left, pos.top - size.top,
		       SCREEN_W, SCREEN_H);
}



/* wnd_set_windowed_coop:
 */
static int wnd_set_windowed_coop(void)
{
   HRESULT hr;
   
   hr = IDirectDraw_SetCooperativeLevel(directdraw, allegro_wnd, DDSCL_NORMAL);
   if (FAILED(hr)) {
      TRACE("SetCooperative level = %s (%x), hwnd = %x\n", win_err_str(hr), hr, allegro_wnd);
      return -1;
   }

   return 0;
}



/* handle_window_size:
 *  updates overlay if window is moved or resized
 */
void handle_window_size(int x, int y, int w, int h)
{
   if (overlay_visible)
      show_overlay(x, y, w, h);
}



/* gfx_directx_ovl:
 */
static struct BITMAP *init_directx_ovl(int w, int h, int v_w, int v_h, int color_depth)
{
   RECT win_size;

   /* overlay would allow scrolling on some cards, but isn't implemented yet */
   if ((v_w != w && v_w != 0) || (v_h != h && v_h != 0))
      return NULL;

   _enter_critical();

   /* init DirectX */
   if (init_directx() != 0)
      goto Error;
   if (wnd_call_proc(wnd_set_windowed_coop) != 0)
      goto Error;
   if (finalize_directx_init() != 0)
      goto Error;
   if (finalize_directx_init() != 0)
      goto Error;
   if ((dd_caps.dwCaps & DDCAPS_OVERLAY) == 0)
      goto Error;

   /* adjust window */
   wnd_paint_back = TRUE;
   win_size.left = 50;
   win_size.right = 50 + w;
   win_size.top = 50;
   win_size.bottom = 50 + h;

   AdjustWindowRect(&win_size, GetWindowLong(allegro_wnd, GWL_STYLE), FALSE);
   MoveWindow(allegro_wnd, win_size.left, win_size.top,
   win_size.right - win_size.left, win_size.bottom - win_size.top, TRUE);

   /* create surfaces */
   if (create_primary(w, h, color_depth) != 0)
      goto Error;
   if (create_overlay(w, h, color_depth) != 0)
      goto Error;
   if (color_depth == 8)
      if (create_palette(overlay_surface) != 0)
	 goto Error;
   if (update_overlay() != 0)
      goto Error;

   /* setup Allegro gfx driver */
   if (setup_driver(&gfx_directx_ovl, w, h, color_depth) != 0)
      goto Error;
   dd_frontbuffer = make_directx_bitmap(overlay_surface, w, h, color_depth, BMP_ID_VIDEO);
   enable_acceleration(&gfx_directx_ovl);

   _exit_critical();

   return dd_frontbuffer;

 Error:
   _exit_critical();

   /* release the DirectDraw object */
   gfx_directx_exit(NULL);

   return NULL;
}
