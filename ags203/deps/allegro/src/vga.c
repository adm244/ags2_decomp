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
 *      Video driver for VGA mode 13h (320x200x256).
 *
 *      320x100 mode contributed by Salvador Eduardo Tropea.
 *
 *      80x80 mode contributed by Pedro Cardoso.
 *
 *      See readme.txt for copyright information.
 */


#include <stdlib.h>
#include <string.h>

#ifdef DJGPP
#include <dpmi.h>
#include <go32.h>
#include <sys/segments.h>
#endif

#include "internal.h"


static BITMAP *vga_init(int w, int h, int v_w, int v_h, int color_depth);
static int vga_scroll(int x, int y);



GFX_DRIVER gfx_vga = 
{
   GFX_VGA,
   "VGA", 
   "Standard VGA", 
   vga_init,
   NULL,
   NULL,          /* can't hardware scroll in 13h */
   _vga_vsync,
   _vga_set_pallete_range,
   NULL, NULL,
   NULL, NULL, NULL, NULL,
   NULL, NULL, NULL, NULL,
   NULL,
   320, 200,
   TRUE,          /* no need for bank switches */
   0, 0,
   0x10000,
   0
};



/* vga_init:
 *  Selects mode 13h and creates a screen bitmap for it.
 */
static BITMAP *vga_init(int w, int h, int v_w, int v_h, int color_depth)
{
   BITMAP *b;
   int c;

   #ifdef DJGPP
      __dpmi_regs r;
   #endif

   /* check it is a valid resolution */
   if (color_depth != 8) {
      strcpy(allegro_error, get_config_text("VGA only supports 8 bit color"));
      return NULL;
   }

   if ((w == 320) && (h == 200) && (v_w <= 320) && (v_h <= 200)) {
      /* standard mode 13h */
      b = _make_bitmap(320, 200, 0xA0000, &gfx_vga, 8, 320);
      if (!b)
	 return NULL;

      r.x.ax = 0x13;
      __dpmi_int(0x10, &r);

      gfx_vga.scroll = NULL;
      gfx_vga.w = 320;
      gfx_vga.h = 200;
   }
   else if ((w == 320) && (h == 100) && (v_w <= 320) && (v_h <= 200)) {
      /* tweaked 320x100 mode */
      b = _make_bitmap(320, 200, 0xA0000, &gfx_vga, 8, 320);
      if (!b)
	 return NULL;

      r.x.ax = 0x13;
      __dpmi_int(0x10, &r);

      outportb(0x3D4, 9);
      outportb(0x3D5, inportb(0x3D5) | 0x80);

      gfx_vga.scroll = vga_scroll;
      gfx_vga.w = 320;
      gfx_vga.h = 100;
   }
   else if ((w == 160) && (h == 120) && (v_w <= 160) && (v_h <= 409)) {
      /* tweaked 160x120 mode */
      b = _make_bitmap(160, 409, 0xA0000, &gfx_vga, 8, 160);
      if (!b)
	 return NULL;

      r.x.ax = 0x0D;
      __dpmi_int(0x10, &r);

      outportb(0x3D4, 0x11);
      outportb(0x3D5, inportb(0x3D5)&0x7F);
      outportb(0x3D4, 0x04);
      outportb(0x3D5, inportb(0x3D5)+1);
      outportb(0x3D4, 0x11);
      outportb(0x3D5, inportb(0x3D5)|0x80);

      outportb(0x3C2, (inportb(0x3CC)&~0xC0)|0x80);

      outportb(0x3D4, 0x11);
      outportb(0x3D5, inportb(0x3D5)&0x7F);

      outportb(0x3D4, 0x06);
      outportb(0x3D5, 0x0B);

      outportb(0x3D4, 0x07);
      outportb(0x3D5, 0x3E);

      outportb(0x3D4, 0x10);
      outportb(0x3D5, 0xEA);

      outportb(0x3D4, 0x11);
      outportb(0x3D5, 0x8C);

      outportb(0x3D4, 0x12);
      outportb(0x3D5, 0xDF);

      outportb(0x3D4, 0x15);
      outportb(0x3D5, 0xE7);

      outportb(0x3D4, 0x16);
      outportb(0x3D5, 0x04);

      outportb(0x3D4, 0x11);
      outportb(0x3D5, inportb(0x3D5)|0x80);

      outportb(0x3CE, 0x05);
      outportb(0x3CF, (inportb(0x3CF)&0x60)|0x40);

      inportb(0x3DA);
      outportb(0x3C0, 0x30);
      outportb(0x3C0, inportb(0x3C1)|0x40);

      for (c=0; c<16; c++) {
	 outportb(0x3C0, c);
	 outportb(0x3C0, c);
      }
      outportb(0x3C0, 0x20);

      outportb(0x3C8, 0x00);

      outportb(0x3C4, 0x04);
      outportb(0x3C5, (inportb(0x3C5)&0xF7)|8);
      outportb(0x3D4, 0x14);
      outportb(0x3D5, (inportb(0x3D5)&~0x40)|64);
      outportb(0x3D4, 0x017);
      outportb(0x3D5, (inportb(0x3D5)&~0x40)|64);

      outportb(0x3D4, 0x09);
      outportb(0x3D5, (inportb(0x3D5)&0x60)|3);

      gfx_vga.scroll = vga_scroll;
      gfx_vga.w = 160;
      gfx_vga.h = 120;
   }
   else if ((w == 80) && (h == 80) && (v_w <= 80) && (v_h <= 819)) {
      /* tweaked 80x80 mode */
      b = _make_bitmap(80, 819, 0xA0000, &gfx_vga, 8, 80);
      if (!b)
	 return NULL;

      r.x.ax = 0x13;
      __dpmi_int(0x10, &r);

     outportw(0x3C4, 0x0604);
     outportw(0x3C4, 0x0F02);

     outportw(0x3D4, 0x0014);
     outportw(0x3D4, 0xE317);
     outportw(0x3D4, 0xE317);
     outportw(0x3D4, 0x0409);

     gfx_vga.scroll = vga_scroll;
     gfx_vga.w = 80;
     gfx_vga.h = 80;
   }
   else {
      strcpy(allegro_error, get_config_text("Not a valid VGA resolution"));
      return NULL;
   }

   return b;
}



/* vga_scroll:
 *  Hardware scrolling routine for standard VGA modes.
 */
static int vga_scroll(int x, int y)
{
   long a = (x + (y * VIRTUAL_W));

   if (gfx_vga.w > 80)
      a /= 4;

   DISABLE();

   _vsync_out_h();

   /* write to VGA address registers */
   _write_vga_register(_crtc, 0x0D, a & 0xFF);
   _write_vga_register(_crtc, 0x0C, (a>>8) & 0xFF);

   ENABLE();

   _vsync_in();

   return 0;
}

