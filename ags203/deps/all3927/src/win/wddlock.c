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
 *      DirectDraw bitmap locking.
 *
 *      By Stefan Schimanski.
 *
 *      See readme.txt for copyright information.
 */

#include "wddraw.h"



static BOOL _handle_disp_switch = FALSE;



/* gfx_directx_lock_surface:
 */
int gfx_directx_lock_surface(LPDIRECTDRAWSURFACE surf, void **data, int *pitch)
{
   HRESULT hr;
   DDSURFACEDESC surf_desc;

   surf_desc.dwSize = sizeof(surf_desc);
   surf_desc.dwFlags = 0;

   if (surf) {
      hr = IDirectDrawSurface_Lock(surf, NULL, &surf_desc,
			    DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL);
      if (FAILED(hr))
	 return -1;

      *data = surf_desc.lpSurface;
      *pitch = surf_desc.lPitch;

      return 0;
   }
   else
      return -1;
}



/* gfx_directx_unlock_surface:
 */
int gfx_directx_unlock_surface(LPDIRECTDRAWSURFACE surf, void *data)
{
   HRESULT hr;

   if (surf) {
      hr = IDirectDrawSurface_Unlock(surf, data);
      if (FAILED(hr))
	 return -1;

      return 0;
   }
   else
      return -1;
}



/* gfx_directx_switch_out:
 */
static void gfx_directx_switch_out(void)
{ 
   /* fixup bitmaps for background execution */ 
   if (_handle_disp_switch)
   {
      _handle_disp_switch = FALSE;

      if (get_display_switch_mode()==SWITCH_BACKGROUND)
      {
	 /* todo */
      }
   }

   _exit_gfx_critical();
   thread_switch_out();
   _enter_gfx_critical();
}



/* gfx_switch_out:
 */
void gfx_switch_out(void)
{
   _handle_disp_switch = FALSE;
}



/* gfx_switch_in:
 */
void gfx_switch_in(void)
{
   _handle_disp_switch = TRUE;
}



/* gfx_directx_lock:
 *  locks the surface and prepares the lines array of the bitmap
 */
void gfx_directx_lock(struct BITMAP *bmp)
{
   LPDIRECTDRAWSURFACE surf;
   BMP_EXTRA_INFO *bmp_extra;
   BMP_EXTRA_INFO *item;
   BITMAP *parent;
   HRESULT hr;
   DDSURFACEDESC surf_desc;
   static int in_callback = 0;
   int pitch;
   char *data;
   int y, h;

   /* lock bitmap for other threads */
   _enter_gfx_critical();

   /* handle display switch */
   if (!app_foreground)
      gfx_directx_switch_out();

   if (bmp->id & BMP_ID_SUB) {
      parent = (struct BITMAP *)bmp->extra;
      bmp->id |= BMP_ID_LOCKED;

      gfx_directx_lock(parent);

      pitch = (long)parent->line[1] - (long)parent->line[0];
      data = parent->line[0] + pitch * bmp->y_ofs + bmp->x_ofs * ((_color_depth+7) >> 3);

      /* prepare line array */
      h = bmp->h;

      if (data != bmp->line[0]) {
	 for (y = 0; y < h; y++) {
	    bmp->line[y] = data;
	    data += pitch;
	 }
      }
   }
   else {
      bmp_extra = BMP_EXTRA(bmp);
      bmp_extra->locked++;
      if (bmp_extra->locked == 1) {
	 bmp->id |= BMP_ID_LOCKED;
	 bmp_extra->flags &= ~BMP_FLAG_LOST;

	 /* try to lock surface */
	 surf = bmp_extra->surf;

	 surf_desc.dwSize = sizeof(surf_desc);
	 surf_desc.dwFlags = 0;

	 hr = IDirectDrawSurface_Lock(surf, NULL, &surf_desc,
		 DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL); 

	 if (FAILED(hr)) {
	    /* lost bitmap, try to restore all surfaces */
	    item = directx_bmp_list;
	    while (item) {
	       /* if restoration fails, stop restoring */
	       if (FAILED(IDirectDrawSurface_Restore(item->surf)))
		  break;
	       item = item->next;
	    }

	    /* if at least one surface was restored, call lost bitmap callback */
	    /*if (item != directx_bmp_list && in_callback == 0) {
	       in_callback = 1;
	       sys_directx_switch_in_callback();
	       in_callback = 0;
	    }*/

	    /* try again to lock */
	    surf_desc.dwSize = sizeof(surf_desc);
	    surf_desc.dwFlags = 0;

	    hr = IDirectDrawSurface_Lock(surf, NULL, &surf_desc,
		    DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL); 
	    if (FAILED(hr)) {
	       /* lock failed, use pseudo surface memory */
	       bmp_extra->flags |= BMP_FLAG_LOST;
	       data = pseudo_surf_mem;
	       pitch = 0;
	    } else
	    {
	       data = surf_desc.lpSurface;
	       pitch = surf_desc.lPitch;
	    }
	 } else
	 {
	    data = surf_desc.lpSurface;
	    pitch = surf_desc.lPitch;
	 }

	 /* prepare line array */
	 h = bmp->h;

	 if (data != bmp->line[0]) {
	    for (y = 0; y < h; y++) {
	       bmp->line[y] = data;
	       data += pitch;
	    }
	 }
      }
   }
}



/* gfx_directx_unlock:
 *  unlocks the surface
 */
void gfx_directx_unlock(struct BITMAP *bmp)
{
   struct BMP_EXTRA_INFO *bmp_extra;

   if (bmp->id & BMP_ID_SUB) {
      gfx_directx_unlock((struct BITMAP *)bmp->extra);
      bmp->id &= ~BMP_ID_LOCKED;
   }
   else {
      bmp_extra = BMP_EXTRA(bmp);
      if (bmp_extra->locked == 1) {
	 /* only unlock if it doesn't use pseudo video memory */
	 if ((bmp_extra->flags & BMP_FLAG_LOST) == 0) { 
	    IDirectDrawSurface_Unlock(bmp_extra->surf, bmp->line[0]);
	 }

	 bmp->id &= ~BMP_ID_LOCKED;
      }

      if (bmp_extra->locked > 0)
	 bmp_extra->locked--;
   }

   /* release bitmap for other threads */
   _exit_gfx_critical();
}



/* gfx_directx_lock_internal:
 */
void gfx_directx_lock_internal(BITMAP * bmp, int temp)
{
   BITMAP *parent;

   if (temp > 0) {
      /* find parent */
      parent = bmp;
      while (parent->id & BMP_ID_SUB)
	 parent = (BITMAP *) parent->extra;

      gfx_directx_lock(bmp);
      BMP_EXTRA(parent)->locked = temp;
   }
}



/* gfx_directx_unlock_internal:
 */
void gfx_directx_unlock_internal(BITMAP * bmp, int *temp)
{
   BITMAP *parent;

   /* find parent */
   parent = bmp;
   while (parent->id & BMP_ID_SUB)
      parent = (BITMAP *) parent->extra;

   *temp = BMP_EXTRA(parent)->locked;
   if (*temp > 0) {
      BMP_EXTRA(parent)->locked = 1;
      gfx_directx_unlock(bmp);
   }
}



/* gfx_directx_bank_switch:
 *  edx = bitmap
 *  eax = line
 */
void gfx_directx_write_bank(void)
{
   _asm
   {
       ; Check whether is is locked already
       test [edx]BITMAP.id, BMP_ID_LOCKED
       jnz Locked

       ; lock the surface once
       pushad
       push edx ; bitmap

       call gfx_directx_lock ; todo: inline manually to avoid this call
       pop edx
       popad

       or [edx]BITMAP.id, BMP_ID_AUTOLOCK

   Locked:
       mov eax, [edx + eax * 4]BITMAP.line
   }
}



/* gfx_directx_unbank_switch:
 *  edx = bmp
 */
void gfx_directx_unwrite_bank(void)
{
   _asm
   {
       ; only unlock if bmp was locked with bank_switch
       test [edx]BITMAP.id, BMP_ID_AUTOLOCK
       jz NoUnlock

       ; unlock surface
       pushad
       push edx
       call gfx_directx_unlock ; todo: inline manually to avoid this call
       pop edx
       popad

       and [edx]BITMAP.id, ~BMP_ID_AUTOLOCK

   NoUnlock:
   }
} 
