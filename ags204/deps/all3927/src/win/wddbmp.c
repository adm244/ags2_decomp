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
 *      DirectDraw bitmap creation.
 *
 *      By Stefan Schimanski.
 *
 *      See readme.txt for copyright information.
 */

#include "wddraw.h"



/* gfx_directx_create_surface: 
 */ 
LPDIRECTDRAWSURFACE gfx_directx_create_surface(int w, int h, int color_depth,
   int video, int primary, int overlay)
{
   DDSURFACEDESC surf_desc;
   LPDIRECTDRAWSURFACE surf;
   HRESULT hr;

   /* describe surface characteristics */
   surf_desc.dwSize = sizeof(surf_desc);
   surf_desc.dwFlags = DDSD_CAPS;
   surf_desc.ddsCaps.dwCaps = 0;

   if (video || primary) {
      surf_desc.ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;

      if (primary) {
	 surf_desc.ddsCaps.dwCaps |= DDSCAPS_PRIMARYSURFACE;
      }
      else if (overlay) {
	 surf_desc.ddsCaps.dwCaps |= DDSCAPS_OVERLAY;

	 surf_desc.dwFlags |= DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
	 surf_desc.dwHeight = h;
	 surf_desc.dwWidth = w;

	 surf_desc.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	 surf_desc.ddpfPixelFormat.dwFlags = DDPF_RGB;
	 surf_desc.ddpfPixelFormat.dwRGBBitCount = color_depth;
	 surf_desc.ddpfPixelFormat.dwRBitMask = 0xf800;
	 surf_desc.ddpfPixelFormat.dwGBitMask = 0x07e0;
	 surf_desc.ddpfPixelFormat.dwBBitMask = 0x001f;
	 surf_desc.ddpfPixelFormat.dwRGBAlphaBitMask = 0x8000;
      }
      else {
	 /*surf_desc.ddsCaps.dwCaps |= DDSCAPS_BACKBUFFER; */
	 surf_desc.dwFlags |= DDSD_HEIGHT | DDSD_WIDTH;
	 surf_desc.dwHeight = h;
	 surf_desc.dwWidth = w;
      }
   }
   else {
      surf_desc.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
   }

   /* create the surface with this properties */
   hr = IDirectDraw_CreateSurface(directdraw, &surf_desc, &surf, NULL);
   if (FAILED(hr))
      return NULL;

   return surf;
}



/* gfx_directx_destroy_surf:
 */
static void gfx_directx_destroy_surf(LPDIRECTDRAWSURFACE surf)
{
   if (surf) {
      IDirectDrawSurface_Release(surf);
   }
}



/* _make_video_bitmap:
 */
BITMAP *_make_video_bitmap(int w, int h, unsigned long addr, struct GFX_VTABLE *vtable, int color_depth, int bpl)
{
   int i, size;
   BITMAP *b;

   if (!vtable)
      return NULL;

   size = sizeof(BITMAP) + sizeof(char *) * h;

   b = (BITMAP *) malloc(size);
   if (!b)
      return NULL;

   _gfx_bank = realloc(_gfx_bank, h * sizeof(int));
   if (!_gfx_bank) {
      free(b);
      return NULL;
   }

   LOCK_DATA(b, size);
   LOCK_DATA(_gfx_bank, h * sizeof(int));

   b->w = b->cr = w;
   b->h = b->cb = h;
   b->clip = TRUE;
   b->cl = b->ct = 0;
   b->vtable = vtable;
   b->write_bank = b->read_bank = _stub_bank_switch;
   b->dat = NULL;
   b->id = BMP_ID_VIDEO;
   b->extra = NULL;
   b->x_ofs = 0;
   b->y_ofs = 0;
   b->seg = _video_ds();

   _last_bank_1 = _last_bank_2 = -1;

   b->line[0] = (char *)addr;
   _gfx_bank[0] = 0;

   for (i = 1; i < h; i++) {
      b->line[i] = b->line[i - 1] + bpl;
      _gfx_bank[i] = 0;
   }

   return b;
}



/* make_directx_bitmap:
 *  connects DirectDraw surface with Allegro bitmap object
 */
struct BITMAP *make_directx_bitmap(LPDIRECTDRAWSURFACE surf, int w, int h, int color_depth, int id)
{
   struct BITMAP *bmp;
   struct BMP_EXTRA_INFO *bmp_extra;

   /* create Allegro bitmap */
   bmp = _make_video_bitmap(w, h, (long)pseudo_surf_mem, &_screen_vtable, color_depth, 0);
   bmp->id = id;
   bmp->extra = malloc(sizeof(struct BMP_EXTRA_INFO));
   bmp->write_bank = gfx_directx_write_bank;
   bmp->read_bank = gfx_directx_write_bank;

   /* setup surface info structure to store additional information */
   bmp_extra = BMP_EXTRA(bmp);
   bmp_extra->surf = surf;
   bmp_extra->next = NULL;
   bmp_extra->prev = NULL;
   bmp_extra->flags = 0;
   bmp_extra->locked = 0;

   register_directx_bitmap(bmp);

   return bmp;
}



/* release_directx_bitmap:
 *  release DirectDraw surface and destroy surface info structure
 */
static void release_directx_bitmap(struct BITMAP *bmp)
{
   struct BMP_EXTRA_INFO *bmp_extra;
   if (bmp) {
      if (bmp->extra) {
	 bmp_extra = BMP_EXTRA(bmp);
	 unregister_directx_bitmap(bmp);
	 free(bmp_extra);
      }
   }
}



/* gfx_directx_created_sub_bitmap:
 */
void gfx_directx_created_sub_bitmap(struct BITMAP *bmp, struct BITMAP *parent)
{
   bmp->extra = parent;
}



/* gfx_directx_create_video_bitmap:
 */
struct BITMAP *gfx_directx_create_video_bitmap(int width, int height)
{
   LPDIRECTDRAWSURFACE surf;

   /* create DirectDraw surface */
   surf = gfx_directx_create_surface(width, height, _color_depth, 1, 0, 0);
   if (surf == NULL)
      return NULL;

   /* create Allegro bitmap for surface */
   return make_directx_bitmap(surf, width, height, _color_depth, BMP_ID_VIDEO);
}



/* gfx_directx_destroy_video_bitmap:
 */
void gfx_directx_destroy_video_bitmap(struct BITMAP *bitmap)
{
   if (bitmap == screen)
      return;

   if (bitmap == dd_frontbuffer)
      gfx_directx_show_video_bitmap(screen);

   gfx_directx_destroy_surf(BMP_EXTRA(bitmap)->surf);
   release_directx_bitmap(bitmap);
   destroy_bitmap(bitmap);
}



/* gfx_directx_flip_bitmap:
 */
static __inline int gfx_directx_flip_bitmap(struct BITMAP *bitmap, int wait)
{
   LPDIRECTDRAWSURFACE visible;
   LPDIRECTDRAWSURFACE invisible;
   HRESULT hr;
   int failed = 0;

   invisible = BMP_EXTRA(bitmap)->surf;
   visible = BMP_EXTRA(dd_frontbuffer)->surf;

   if (visible != invisible) {
      /* link invisible surface to primary surface */
      hr = IDirectDrawSurface_AddAttachedSurface(visible, invisible);
      if (FAILED(hr)) {
	 TRACE("Can't attach surface (%x)\n", hr);
	 return -1;
      }

      /* flip to invisible surface */
	 /* DDCAPS2_FLIPNOVSYNC support? */
      if (dd_caps.dwCaps2 & DDCAPS2_FLIPNOVSYNC)
	 hr = IDirectDrawSurface_Flip(visible, invisible, (wait?DDFLIP_WAIT:0) | DDFLIP_NOVSYNC);
      else
	 hr = IDirectDrawSurface_Flip(visible, invisible, wait?DDFLIP_WAIT:0);
      if (FAILED(hr)) {
	 TRACE("Can't flip (%x)\n", hr);
	 failed = 1;
      }

      /* remove background surface */
      hr = IDirectDrawSurface_DeleteAttachedSurface(visible, 0, invisible);
      if (FAILED(hr)) {
	 TRACE("Can't detach surface (%x)\n", hr);
	 failed = 1;
      }

      /* exchange surface objects */
      if (!failed) {
	 BMP_EXTRA(bitmap)->surf = visible;
	 BMP_EXTRA(dd_frontbuffer)->surf = invisible;
	 dd_frontbuffer = bitmap;

	 return 0;
      }
      else
	 return -1;
   }

   return 0;
}



/* gfx_directx_show_video_bitmap:
 */
int gfx_directx_show_video_bitmap(struct BITMAP *bitmap)
{
   return gfx_directx_flip_bitmap(bitmap, 1);
}



/* gfx_directx_request_video_bitmap:
 *  same as show_video_bitmap, but without waiting for flip.
 */
int gfx_directx_request_video_bitmap(struct BITMAP *bitmap)
{
   return gfx_directx_flip_bitmap(bitmap, 0);
}



/* gfx_directx_poll_scroll:
 *  checks whether the last flip is finished.
 */
int gfx_directx_poll_scroll(void)
{
   HRESULT hr = IDirectDrawSurface_GetFlipStatus(
      BMP_EXTRA(dd_frontbuffer)->surf, DDGFS_ISFLIPDONE);
   if (FAILED(hr))
      return -1;
   else
      return 0;
}



/* gfx_directx_create_system_bitmap:
 */
struct BITMAP *gfx_directx_create_system_bitmap(int width, int height)
{
   LPDIRECTDRAWSURFACE surf;

   /* create DirectDraw surface */
   surf = gfx_directx_create_surface(width, height, _color_depth, 0, 0, 0);
   if (surf == NULL)
      return NULL;

   /* create Allegro bitmap for surface */
   return make_directx_bitmap(surf, width, height, _color_depth, BMP_ID_SYSTEM);
}



/* gfx_directx_destroy_system_bitmap:
 */
void gfx_directx_destroy_system_bitmap(struct BITMAP *bitmap)
{
   gfx_directx_destroy_surf(BMP_EXTRA(bitmap)->surf);
   release_directx_bitmap(bitmap);
   destroy_bitmap(bitmap);
}



/* win_get_dc:
 *  return HDC for video and system bitmaps
 */
HDC win_get_dc(BITMAP * bmp)
{
   LPDIRECTDRAWSURFACE surf;
   HDC dc;
   HRESULT hr;

   if (bmp) {
      if (bmp->id & (BMP_ID_SYSTEM + BMP_ID_VIDEO)) {
	 surf = BMP_EXTRA(bmp)->surf;
	 hr = IDirectDrawSurface_GetDC(surf, &dc);
	 if (hr == DD_OK)
	    return dc;
      }
   }

   return NULL;
}



/* win_release_dc:
 *  release HDC of video or system bitmaps
 */
void win_release_dc(BITMAP * bmp, HDC dc)
{
   LPDIRECTDRAWSURFACE surf;

   if (bmp) {
      if (bmp->id & (BMP_ID_SYSTEM + BMP_ID_VIDEO)) {
	 surf = BMP_EXTRA(bmp)->surf;
	 IDirectDrawSurface_ReleaseDC(surf, dc);
      }
   }
}
