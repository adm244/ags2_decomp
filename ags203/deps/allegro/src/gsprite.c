/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *      By Shawn Hargreaves
 *      shawn@talula.demon.co.uk
 *      http://www.talula.demon.co.uk/allegro/
 *
 *      Gouraud shaded sprite renderer, by Patrick Hogan.
 *
 *      See readme.txt for copyright information.
 */


#include <stdlib.h>

#ifdef DJGPP
#include <sys/farptr.h>
#endif

#include "allegro.h"



/* draw_gouraud_sprite:
 *  Draws a lit or tinted sprite, interpolating the four corner colors
 *  over the surface of the image.
 */
void draw_gouraud_sprite(BITMAP * bmp, BITMAP * sprite, int x, int y,
			  int c1, int c2, int c3, int c4)
{
   fixed mc1, mc2, mh;
   fixed lc, rc, hc;
   int x1 = x;
   int y1 = y;
   int x2 = x + sprite->w;
   int y2 = y + sprite->h;
   int i, j;
   int pixel;
   unsigned long addr;

   _farsetsel(bmp->seg);

   /* set up vertical gradients for left and right sides */
   mc1 = itofix(c4 - c1) / sprite->h;
   mc2 = itofix(c3 - c2) / sprite->h;
   lc = itofix(c1);
   rc = itofix(c2);

   /* check clipping */
   if (bmp->clip) {
      if (y1 < bmp->ct) {
	 lc += mc1 * (bmp->ct - y1);
	 rc += mc2 * (bmp->ct - y1);
	 y1 = bmp->ct;
      }
      y2 = MIN(y2, bmp->cb);
      x1 = MAX(x1, bmp->cl);
      x2 = MIN(x2, bmp->cr);
   }

   for (j=y1; j<y2; j++) {
      /* set up horizontal gradient for line */
      mh = (rc - lc) / sprite->w;
      hc = lc;

      /* more clip checking */
      if ((bmp->clip) && (x < bmp->cl))
	 hc += mh * (bmp->cl - x);

      if (is_planar_bitmap(bmp)) {
	 /* modex version */
	 addr = ((unsigned long)bmp->line[j]<<2) + x1;
	 for (i=x1; i<x2; i++) {
	    if (sprite->line[j-y][i-x]) {
	       outportw(0x3C4, (0x100<<(i&3))|2);
	       pixel = color_map->data[fixtoi(hc)][sprite->line[j-y][i-x]];
	       _farnspokeb(addr>>2, pixel);
	    }
	    hc += mh;
	    addr++;
	 }
      }
      else {
	 /* draw routines for all linear modes */
	 switch (bitmap_color_depth(bmp)) {

	    case 8:
	    default:
	       addr = bmp_write_line(bmp, j) + x1;
	       for (i=x1; i<x2; i++) {
		  if (sprite->line[j-y][i-x]) {
		     pixel = color_map->data[fixtoi(hc)][sprite->line[j-y][i-x]];
		     _farnspokeb(addr, pixel);
		  }
		  hc += mh;
		  addr++;
	       }
	       break;

	 #ifdef ALLEGRO_COLOR16

	    case 15:
	    case 16:
	       addr = bmp_write_line(bmp, j) + x1*sizeof(short);
	       for (i=x1; i<x2; i++) {
		  pixel = ((unsigned short *)sprite->line[j-y])[i-x];
		  if (pixel != bmp->vtable->mask_color) {
		     if (bitmap_color_depth(bmp) == 16)
			pixel = _blender_map16->blend[fixtoi(hc)](pixel, _blender_col_16);
		     else
			pixel = _blender_map15->blend[fixtoi(hc)](pixel, _blender_col_15);
		     _farnspokew(addr, pixel);
		  }
		  hc += mh;
		  addr += sizeof(short);
	       }
	       break;

	 #endif

	 #ifdef ALLEGRO_COLOR24

	    case 24:
	       addr = bmp_write_line(bmp, j) + x1*3;
	       for (i=x1; i<x2; i++) {
		  pixel = *((unsigned long *)(((unsigned char *)sprite->line[j-y]) + (i-x)*3));
		  pixel &= 0xFFFFFF;
		  if (pixel != bmp->vtable->mask_color) {
		     pixel = _blender_map24->blend[fixtoi(hc)](pixel, _blender_col_24);
		     _farnspokew(addr, pixel);
		     _farnspokeb(addr+2, (pixel>>16)&0xFF);
		  }
		  hc += mh;
		  addr += 3;
	       }
	       break;

	 #endif

	 #ifdef ALLEGRO_COLOR32

	    case 32:
	       addr = bmp_write_line(bmp, j) + x1*sizeof(long);
	       for (i=x1; i<x2; i++) {
		  pixel = ((unsigned long *)sprite->line[j-y])[i-x];
		  if (pixel != bmp->vtable->mask_color) {
		     pixel = _blender_map24->blend[fixtoi(hc)](pixel, _blender_col_32);
		     _farnspokel(addr, pixel);
		  }
		  hc += mh;
		  addr += sizeof(long);
	       }
	       break;

	 #endif
	 }
      }

      lc += mc1;
      rc += mc2;
   }
}

