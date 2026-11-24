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
 *      DirectDraw acceleration.
 *
 *      By Stefan Schimanski.
 *
 *      See readme.txt for copyright information.
 */

#include "wddraw.h"



/* software version pointers */
static void (*_orig_draw_sprite) (BITMAP * bmp, BITMAP * sprite, int x, int y);
static void (*_orig_masked_blit) (BITMAP * source, BITMAP * dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);



/* ddraw_blit_to_self:
 *  Accelerated vram -> vram blitting routine.
 */
static void ddraw_blit_to_self(BITMAP * source, BITMAP * dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height)
{
   int dest_locked;
   int src_locked;

   RECT src_rect =
   {source_x + source->x_ofs,
    source_y + source->y_ofs,
    source_x + source->x_ofs + width,
    source_y + source->y_ofs + height};

   _enter_gfx_critical();
   gfx_directx_unlock_internal(dest, &dest_locked);
   gfx_directx_unlock_internal(source, &src_locked);

   IDirectDrawSurface_BltFast(BMP_EXTRA(dest)->surf,
			      dest_x + dest->x_ofs, dest_y + dest->y_ofs,
		     BMP_EXTRA(source)->surf, &src_rect, DDBLTFAST_WAIT);

   gfx_directx_lock_internal(source, src_locked);
   gfx_directx_lock_internal(dest, dest_locked);
   _exit_gfx_critical();
}



/* ddraw_masked_blit:
 *  Accelerated masked blitting routine.
 */
static void ddraw_masked_blit(BITMAP * source, BITMAP * dest,
int source_x, int source_y, int dest_x, int dest_y, int width, int height)
{
   RECT dest_rect =
   {dest_x + dest->x_ofs,
    dest_y + dest->y_ofs,
    dest_x + dest->x_ofs + width,
    dest_y + dest->y_ofs + height};

   RECT source_rect =
   {source_x + source->x_ofs,
    source_y + source->y_ofs,
    source_x + source->x_ofs + width,
    source_y + source->y_ofs + height};

   HRESULT hr;
   int dest_locked;
   int src_locked;
   BMP_EXTRA_INFO *dest_extra = BMP_EXTRA(dest);
   BMP_EXTRA_INFO *source_extra = BMP_EXTRA(source);
   DDCOLORKEY src_key =
   {source->vtable->mask_color,
    source->vtable->mask_color};

   if (source->vtable == &_screen_vtable) {
      _enter_gfx_critical();
      gfx_directx_unlock_internal(dest, &dest_locked);
      gfx_directx_unlock_internal(source, &src_locked);

      IDirectDrawSurface_SetColorKey(source_extra->surf,
				     DDCKEY_SRCBLT, &src_key);

      hr = IDirectDrawSurface_Blt(dest_extra->surf, &dest_rect,
				  source_extra->surf, &source_rect,
				  DDBLT_KEYSRC | DDBLT_WAIT, NULL);

      gfx_directx_lock_internal(source, src_locked);
      gfx_directx_lock_internal(dest, dest_locked);
      _exit_gfx_critical();

      if (FAILED(hr))
	 TRACE("Blt failed (%x)\n", hr);
   }
   else {
      /* have to use the original software version */
      _orig_masked_blit(source, dest, source_x, source_y, dest_x, dest_y, width, height);
   }
}



/* ddraw_draw_sprite:
 *  Accelerated sprite drawing routine.
 */
static void ddraw_draw_sprite(BITMAP * bmp, BITMAP * sprite, int x, int y)
{
   int sx, sy, w, h;

   if (sprite->vtable == &_screen_vtable) {
      sx = sprite->x_ofs;
      sy = sprite->y_ofs;
      w = sprite->w;
      h = sprite->h;

      if (bmp->clip) {
	 if (x < bmp->cl) {
	    sx += bmp->cl - x;
	    w -= bmp->cl - x;
	    x = bmp->cl;
	 }

	 if (y < bmp->ct) {
	    sy += bmp->ct - y;
	    h -= bmp->ct - y;
	    y = bmp->ct;
	 }

	 if (x + w > bmp->cr)
	    w = bmp->cr - x;

	 if (w <= 0)
	    return;

	 if (y + h > bmp->cb)
	    h = bmp->cb - y;

	 if (h <= 0)
	    return;
      }

      ddraw_masked_blit(sprite, bmp, sx, sy, x, y, w, h);
   }
   else {
      /* have to use the original software version */
      _orig_draw_sprite(bmp, sprite, x, y);
   }
}



/* ddraw_clear_to_color:
 *  Accelerated screen clear routine.
 */
static void ddraw_clear_to_color(BITMAP * bitmap, int color)
{
   RECT dest_rect =
   {bitmap->cl + bitmap->x_ofs,
    bitmap->ct + bitmap->y_ofs,
    bitmap->cr - bitmap->cl,
    bitmap->cb - bitmap->ct};
   HRESULT hr;
   DDBLTFX blt_fx;
   int dest_locked;
   BMP_EXTRA_INFO *dest_extra = BMP_EXTRA(bitmap);

   /* set fill color */
   blt_fx.dwSize = sizeof(blt_fx);
   blt_fx.dwDDFX = 0;
   blt_fx.dwFillColor = color;

   _enter_gfx_critical();
   gfx_directx_unlock_internal(bitmap, &dest_locked);

   hr = IDirectDrawSurface_Blt(BMP_EXTRA(bitmap)->surf, &dest_rect, NULL, NULL,
			       DDBLT_COLORFILL | DDBLT_WAIT, &blt_fx);

   gfx_directx_lock_internal(bitmap, dest_locked);
   _exit_gfx_critical();

   if (FAILED(hr))
      TRACE("Blt failed (%x)\n", hr);
}



/* enable_acceleration:
 *  checks graphic driver for capabilities to accelerate Allegro
 */
int enable_acceleration(GFX_DRIVER * drv)
{
   HRESULT hr;

   /* safe pointer to software versions */
   _orig_draw_sprite = _screen_vtable.draw_sprite;
   _orig_masked_blit = _screen_vtable.masked_blit;

   /* accelerated video to video blits? */
   if (dd_caps.dwCaps & DDCAPS_BLT) {
      _screen_vtable.blit_to_self = ddraw_blit_to_self;
      _screen_vtable.blit_to_self_forward = ddraw_blit_to_self;
      _screen_vtable.blit_to_self_backward = ddraw_blit_to_self;

      gfx_capabilities |= GFX_HW_VRAM_BLIT;
   }

   /* accelerated color fills? */
   if (dd_caps.dwCaps & DDCAPS_BLTCOLORFILL) {
      _screen_vtable.clear_to_color = ddraw_clear_to_color;

      gfx_capabilities |= GFX_HW_FILL | GFX_HW_FILL_XOR;
   }

   /* accelerated color key blits? */
   if ((dd_caps.dwCaps & DDCAPS_COLORKEY) &&
       (dd_caps.dwCKeyCaps & DDCKEYCAPS_SRCBLT)) {
      _screen_vtable.masked_blit = ddraw_masked_blit;
      _screen_vtable.draw_sprite = ddraw_draw_sprite;
      _screen_vtable.draw_256_sprite = ddraw_draw_sprite;

      gfx_capabilities |= GFX_HW_VRAM_BLIT_MASKED;
   }

   /* triple buffering? */
   hr = IDirectDrawSurface_GetFlipStatus(BMP_EXTRA(dd_frontbuffer)->surf, DDGFS_ISFLIPDONE);
   if (hr == DDERR_WASSTILLDRAWING || hr == DD_OK) {
      drv->poll_scroll = gfx_directx_poll_scroll;
      gfx_capabilities |= GFX_CAN_TRIPLE_BUFFER;
   }
   
   return 0;
}
