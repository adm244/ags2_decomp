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
 *      Video driver using SVGAlib.
 *
 *      By Stefan T. Boettner.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/aintern.h"
#include "allegro/aintunix.h"


#ifdef ALLEGRO_LINUX_SVGALIB

#include <vga.h>



static BITMAP *svga_init(int w, int h, int v_w, int v_h, int color_depth);
static void svga_exit(BITMAP *b);
static int  svga_scroll(int x, int y);
static void svga_vsync(void);
static void svga_set_palette(RGB *p, int from, int to, int vsync);



GFX_DRIVER gfx_svgalib = 
{
   GFX_SVGALIB,
   empty_string,
   empty_string,
   "SVGAlib", 
   svga_init,
   svga_exit,
   svga_scroll,
   svga_vsync,
   svga_set_palette,
   NULL, NULL, NULL,             /* no triple buffering */
   NULL, NULL, NULL, NULL,       /* no video bitmaps */
   NULL, NULL,                   /* no system bitmaps */
   NULL, NULL, NULL, NULL,       /* no hardware cursor */
   NULL,                         /* no drawing mode hook */
   NULL, NULL,
   0, 0,
   TRUE,
   0, 0, 0, 0
};



static unsigned int display_start_mask=0, scanline_width, bytesperpixel;



/* svga_init:
 *  Sets a graphics mode.
 */
static BITMAP *svga_init(int w, int h, int v_w, int v_h, int color_depth)
{
   static int first_init=1;
   int i, vidmem, width;
   vga_modeinfo *info;
   BITMAP *bmp;

   if (!__al_linux_have_ioperms) {
      ustrcpy(allegro_error, get_config_text("This driver needs root privileges"));
      return 0;
   }

   if (first_init) {
      vga_init();
      first_init = 0;
   }

   vga_setmode(TEXT);   /* makes the mode switch more stable */

   for (i=0; i<=vga_lastmodenumber(); i++) {
      info = vga_getmodeinfo(i);

      if ((info->width == w) &&
	  (info->height == h) &&
	  (info->maxlogicalwidth >= v_w*info->bytesperpixel) &&
	  (info->bytesperpixel*8 == color_depth) && 
	  (info->flags & CAPABLE_LINEAR)) {

	 if (vga_setmode(i))
	    continue;

	 vidmem = vga_setlinearaddressing();

	 if (vidmem < 0) {
	    ustrcpy(allegro_error, get_config_text("Cannot enable linear addressing"));
	    return NULL;
	 }

	 if (v_w) {
	    width = v_w;
	    scanline_width = width * info->bytesperpixel;
	    vga_setlogicalwidth(scanline_width);
	 }
	 else {
	    width = info->linewidth / info->bytesperpixel;
	    scanline_width = info->linewidth;
	 }

	 bmp = _make_bitmap(width, info->maxpixels/width,
			    (unsigned long)vga_getgraphmem(),
			    &gfx_svgalib, color_depth, info->linewidth);

	 gfx_svgalib.w = vga_getxdim();
	 gfx_svgalib.h = vga_getydim();
	 gfx_svgalib.vid_mem = vidmem;
	 gfx_svgalib.vid_phys_base = (unsigned long)vga_getgraphmem();

	 display_start_mask = info->startaddressrange;
	 bytesperpixel = info->bytesperpixel;

	 switch (color_depth) {

	    case 15:
	       _rgb_r_shift_15 = 10;
	       _rgb_g_shift_15 = 5;
	       _rgb_b_shift_15 = 0;
	       break;

	    case 16:
	       _rgb_r_shift_16 = 11;
	       _rgb_g_shift_16 = 5;
	       _rgb_b_shift_16 = 0;
	       break;

	    case 24:
	       _rgb_r_shift_24 = 16;
	       _rgb_g_shift_24 = 8;
	       _rgb_b_shift_24 = 0;
	       break;

	    case 32:
	       _rgb_r_shift_32 = 16;
	       _rgb_g_shift_32 = 8;
	       _rgb_b_shift_32 = 0;
	       break;
	 }

	 return bmp;
      }
   }

   ustrcpy(allegro_error, get_config_text("Resolution not supported"));
   return NULL;
}



/* svga_exit:
 *  Unsets the video mode.
 */
static void svga_exit(BITMAP *b)
{
   vga_setmode (TEXT);
}



/* svga_scroll:
 *  Hardware scrolling routine.
 */
static int svga_scroll(int x, int y)
{
   vga_setdisplaystart((x*bytesperpixel + y*scanline_width) & display_start_mask);

   return 0;
}



/* svga_vsync:
 *  Waits for a retrace.
 */
static void svga_vsync()
{
   vga_waitretrace();
}



/* svga_set_palette:
 *  Sets the palette.
 */
static void svga_set_palette(RGB *p, int from, int to, int vsync)
{
   int i;

   if (vsync)
      vga_waitretrace();

   for (i=from; i<=to; i++)
      vga_setpalette(i, p[i-from].r, p[i-from].g, p[i-from].b);
}



#endif      /* ifdef ALLEGRO_LINUX_SVGALIB */
