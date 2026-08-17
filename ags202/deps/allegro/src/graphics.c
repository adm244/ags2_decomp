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
 *      Graphics mode set and bitmap creation routines.
 *
 *      See readme.txt for copyright information.
 */


#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>

#ifdef DJGPP
#include <go32.h>
#include <conio.h>
#include <sys/farptr.h>
#endif

#include "internal.h"



static int gfx_virgin = TRUE;          /* first time we've been called? */
static int a_rez = 3;                  /* original screen mode */
static int a_lines = -1;               /* original screen height */

int _sub_bitmap_id_count = 1;          /* hash value for sub-bitmaps */

int _modex_split_position = 0;         /* has the mode-X screen been split? */

RGB_MAP *rgb_map = NULL;               /* RGB -> pallete entry conversion */

COLOR_MAP *color_map = NULL;           /* translucency/lighting table */

int _color_depth = 8;                  /* how many bits per pixel? */

int _color_conv = COLORCONV_TOTAL;     /* which formats to auto convert? */

int pallete_color[256];                /* pallete -> pixel mapping */

BLENDER_MAP *_blender_map15 = NULL;    /* truecolor pixel blender routines */
BLENDER_MAP *_blender_map16 = NULL;
BLENDER_MAP *_blender_map24 = NULL;

int _blender_col_15 = 0;               /* for truecolor lit sprites */
int _blender_col_16 = 0;
int _blender_col_24 = 0;
int _blender_col_32 = 0;

int _blender_alpha = 0;                /* for truecolor translucent drawing */

int _rgb_r_shift_15 = 10;              /* truecolor pixel format */
int _rgb_g_shift_15 = 5;
int _rgb_b_shift_15 = 0;
int _rgb_r_shift_16 = 11;
int _rgb_g_shift_16 = 5;
int _rgb_b_shift_16 = 0;
int _rgb_r_shift_24 = 16;
int _rgb_g_shift_24 = 8;
int _rgb_b_shift_24 = 0;
int _rgb_r_shift_32 = 16;
int _rgb_g_shift_32 = 8;
int _rgb_b_shift_32 = 0;


/* lookup table for scaling 5 bit colors up to 8 bits */
int _rgb_scale_5[32] =
{
   0, 8, 16, 24, 32, 41, 49, 57,
   65, 74, 82, 90, 98, 106, 115, 123,
   131, 139, 148, 156, 164, 172, 180, 189,
   197, 205, 213, 222, 230, 238, 246, 255
};


/* lookup table for scaling 6 bit colors up to 8 bits */
int _rgb_scale_6[64] =
{
   0, 4, 8, 12, 16, 20, 24, 28,
   32, 36, 40, 44, 48, 52, 56, 60,
   64, 68, 72, 76, 80, 85, 89, 93,
   97, 101, 105, 109, 113, 117, 121, 125,
   129, 133, 137, 141, 145, 149, 153, 157,
   161, 165, 170, 174, 178, 182, 186, 190,
   194, 198, 202, 206, 210, 214, 218, 222,
   226, 230, 234, 238, 242, 246, 250, 255
};


GFX_VTABLE _screen_vtable;             /* accelerated drivers change this */

__dpmi_regs _dpmi_reg;                 /* for calling int 10 bank switch */

int _window_2_offset = 0;              /* windows at different addresses? */

void (*_pm_vesa_switcher)() = NULL;    /* VBE pmode bank switch routine */
void (*_pm_vesa_scroller)() = NULL;    /* VBE pmode scrolling routine */
void (*_pm_vesa_pallete)() = NULL;     /* VBE pmode pallete get/set routine */

int _mmio_segment = 0;                 /* VBE selector to pass in %es */

void *_accel_driver = NULL;            /* the VBE/AF driver */

int _accel_active = FALSE;             /* is the accelerator busy? */

void *_accel_set_bank;
void *_accel_idle;


typedef struct VRAM_BITMAP             /* list of video memory bitmaps */
{
   int x, y, w, h;
   BITMAP *bmp;
   struct VRAM_BITMAP *next;
} VRAM_BITMAP;


static VRAM_BITMAP *vram_bitmap_list = NULL;



/* lock_bitmap:
 *  Locks all the memory used by a bitmap structure.
 */
void lock_bitmap(BITMAP *bmp)
{
   LOCK_DATA(bmp, sizeof(BITMAP) + sizeof(char *) * bmp->h);

   if (bmp->dat)
      LOCK_DATA(bmp->dat, bmp->w * bmp->h);
}



/* shutdown_gfx:
 *  Used by allegro_exit() to return the system to text mode.
 */
static void shutdown_gfx()
{
   #ifdef DJGPP
      /* djgpp shutdown */
      __dpmi_regs r;

      r.x.ax = 0x0F00; 
      __dpmi_int(0x10, &r);

      if (((r.x.ax & 0xFF) != a_rez) || (gfx_driver))
	 set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);

   #else
      /* linux shutdown */

   #endif
}



/* _check_gfx_virginity:
 *  Checks if this is the first call to any graphics related function, and
 *  if so does some once-only initialisation.
 */
void _check_gfx_virginity()
{
   extern void blit_end();
   int c;

   if (gfx_virgin) {
      #ifdef DJGPP
	 /* djgpp initialisation */
	 __dpmi_regs r;
	 struct text_info textinfo;

	 LOCK_VARIABLE(_last_bank_1);
	 LOCK_VARIABLE(_last_bank_2);
	 LOCK_VARIABLE(_gfx_bank);
	 LOCK_FUNCTION(_stub_bank_switch);
	 LOCK_FUNCTION(blit);

	 for (c=0; _vtable_list[c].vtable; c++) {
	    LOCK_DATA(_vtable_list[c].vtable, sizeof(GFX_VTABLE));
	    LOCK_CODE(_vtable_list[c].vtable->draw_sprite, (long)_vtable_list[c].vtable->draw_sprite_end - (long)_vtable_list[c].vtable->draw_sprite);
	    LOCK_CODE(_vtable_list[c].vtable->blit_from_memory, (long)_vtable_list[c].vtable->blit_end - (long)_vtable_list[c].vtable->blit_from_memory);
	 }

	 for (c=0; c<256; c++) {       /* store current color pallete */
	    outportb(0x3C7, c);
	    _current_pallete[c].r = inportb(0x3C9);
	    _current_pallete[c].g = inportb(0x3C9);
	    _current_pallete[c].b = inportb(0x3C9);
	 }

	 r.x.ax = 0x0F00;              /* store current video mode */
	 __dpmi_int(0x10, &r);
	 a_rez = r.x.ax & 0xFF; 

	 if (a_rez > 19)               /* ignore non-standard modes */
	    a_rez = 3;

	 if (a_rez == 3) {             /* store current screen height */
	    gettextinfo(&textinfo); 
	    a_lines = textinfo.screenheight;
	 }
	 else
	    a_lines = -1;

      #else
	 /* linux initialisation */

      #endif

      _add_exit_func(shutdown_gfx);

      gfx_virgin = FALSE;
   }
}



/* set_color_depth:
 *  Sets the pixel size (in bits) which will be used by subsequent calls to 
 *  set_gfx_mode() and create_bitmap(). Valid depths are 8, 15, 16, and 32.
 */
void set_color_depth(int depth)
{
   if ((depth == 8) || (depth == 15) || (depth == 16) || (depth == 24) || (depth == 32))
      _color_depth = depth;
}



/* set_color_conversion:
 *  Sets a bit mask specifying which types of color format conversions are
 *  valid when loading data from disk.
 */
void set_color_conversion(int mode)
{
   _color_conv = mode;
}



/* _color_load_depth:
 *  Works out which color depth an image should be loaded as, given the
 *  current conversion mode.
 */
int _color_load_depth(int depth)
{
   if (depth == _color_depth)
      return depth;

   if (_color_depth == 8)
      return (_color_conv & COLORCONV_REDUCE_TO_256) ? _color_depth : depth;

   switch (depth) {

      case 8:
	 return (_color_conv & COLORCONV_EXPAND_256) ? _color_depth : depth;

      case 15:
	 if (_color_depth == 16)
	    return (_color_conv & COLORCONV_EXPAND_15_TO_16) ? _color_depth : depth;
	 else
	    return (_color_conv & COLORCONV_EXPAND_HI_TO_TRUE) ? _color_depth : depth;

      case 16:
	 if (_color_depth == 15)
	    return (_color_conv & COLORCONV_REDUCE_16_TO_15) ? _color_depth : depth;
	 else
	    return (_color_conv & COLORCONV_EXPAND_HI_TO_TRUE) ? _color_depth : depth;

      case 24:
      case 32:
	 if ((_color_depth == 15) || (_color_depth == 16))
	    return (_color_conv & COLORCONV_REDUCE_TRUE_TO_HI) ? _color_depth : depth;
	 else
	    return (_color_conv & COLORCONV_24_EQUALS_32) ? _color_depth : depth;

      default:
	 return 0;
   }
}



/* _get_vtable:
 *  Returns a pointer to the linear vtable for the specified color depth.
 */
GFX_VTABLE *_get_vtable(int color_depth)
{
   int i;

   for (i=0; _vtable_list[i].vtable; i++)
      if (_vtable_list[i].color_depth == color_depth)
	 return _vtable_list[i].vtable;

   return NULL;
}



/* set_gfx_mode:
 *  Sets the graphics mode. The card should be one of the GFX_* constants
 *  from allegro.h, or GFX_AUTODETECT to accept any graphics driver. Pass
 *  GFX_TEXT to return to text mode (although allegro_exit() will usually 
 *  do this for you). The w and h parameters specify the screen resolution 
 *  you want, and v_w and v_h specify the minumum virtual screen size. 
 *  The graphics drivers may actually create a much larger virtual screen, 
 *  so you should check the values of VIRTUAL_W and VIRTUAL_H after you
 *  set the mode. If unable to select an appropriate mode, this function 
 *  returns -1.
 */
int set_gfx_mode(int card, int w, int h, int v_w, int v_h)
{
   int c, n;
   char buf[32];
   int retrace_enabled = _timer_use_retrace;
   int tried = FALSE;

   _check_gfx_virginity();

   timer_simulate_retrace(FALSE);
   _modex_split_position = 0;

   /* close down any existing graphics driver */
   if (gfx_driver) {
      show_mouse(NULL);

      while (vram_bitmap_list)
	 destroy_bitmap(vram_bitmap_list->bmp);

      bmp_read_line(screen, 0);
      bmp_write_line(screen, 0);

      if (gfx_driver->scroll)
	 gfx_driver->scroll(0, 0);

      if (gfx_driver->exit)
	 gfx_driver->exit(screen);

      destroy_bitmap(screen);

      gfx_driver = NULL;
      screen = NULL;
      gfx_capabilities = 0;
   }

   /* return to text mode? */
   if (card == GFX_TEXT) {
      #ifdef DJGPP
	 /* djgpp set text mode */
	 __dpmi_regs r;

	 r.x.ax = a_rez;
	 __dpmi_int(0x10, &r);

	 if (a_lines > 0)
	    _set_screen_lines(a_lines);

      #else
	 /* linux set text mode */

      #endif

      if (_gfx_bank) {
	 free(_gfx_bank);
	 _gfx_bank = NULL;
      }

      return 0;
   }

   /* restore default truecolor pixel format */
   _rgb_r_shift_15 = 10; 
   _rgb_g_shift_15 = 5;
   _rgb_b_shift_15 = 0;
   _rgb_r_shift_16 = 11;
   _rgb_g_shift_16 = 5;
   _rgb_b_shift_16 = 0;
   _rgb_r_shift_24 = 16;
   _rgb_g_shift_24 = 8;
   _rgb_b_shift_24 = 0;
   _rgb_r_shift_32 = 16;
   _rgb_g_shift_32 = 8;
   _rgb_b_shift_32 = 0;

   gfx_capabilities = 0;

   if (card == GFX_AUTODETECT) {
      /* try the drivers that are listed in the config file */
      for (n=0; n<255; n++) {
	 if (n) {
	    sprintf(buf, "gfx_card%d", n);
	    card = get_config_id(NULL, buf, GFX_AUTODETECT);
	 }
	 else
	    card = get_config_id(NULL, "gfx_card", GFX_AUTODETECT);

	 if (card != GFX_AUTODETECT) {
	    for (c=0; _gfx_driver_list[c].driver; c++) {
	       if (_gfx_driver_list[c].driver_id == card) {
		  gfx_driver = _gfx_driver_list[c].driver;
		  break;
	       }
	    }
	    if (gfx_driver) {
	       tried = TRUE;
	       screen = gfx_driver->init(w, h, v_w, v_h, _color_depth);
	       if (screen)
		  break;
	       else
		  gfx_driver = NULL;
	    }
	 }
	 else {
	    if (n > 1)
	       break;
	 }
      }
   }

   if (!tried) {
      /* search table for the requested driver */
      for (c=0; _gfx_driver_list[c].driver; c++) {
	 if (_gfx_driver_list[c].driver_id == card) {
	    gfx_driver = _gfx_driver_list[c].driver;
	    break;
	 }
      }

      if (gfx_driver) {                            /* specific driver? */
	 screen = gfx_driver->init(w, h, v_w, v_h, _color_depth);
      }
      else {                                       /* otherwise autodetect */
	 for (c=0; _gfx_driver_list[c].driver; c++) {
	    if (_gfx_driver_list[c].autodetect) {
	       gfx_driver = _gfx_driver_list[c].driver;
	       screen = gfx_driver->init(w, h, v_w, v_h, _color_depth);
	       if (screen)
		  break;
	    }
	 }
      }
   }

   if (!screen) {
      gfx_driver = NULL;
      screen = NULL;
      return -1;
   }

   if ((VIRTUAL_W > SCREEN_W) || (VIRTUAL_H > SCREEN_H)) {
      if (gfx_driver->scroll)
	 gfx_capabilities |= GFX_CAN_SCROLL;

      if (gfx_driver->request_scroll)
	 gfx_capabilities |= GFX_CAN_TRIPLE_BUFFER;
   }

   if (_color_depth == 8)
      for (c=0; c<256; c++)
	 pallete_color[c] = c;

   clear(screen);
   _set_mouse_range();
   LOCK_DATA(gfx_driver, sizeof(GFX_DRIVER));

   if (retrace_enabled)
      timer_simulate_retrace(TRUE);

   return 0;
} 



/* _sort_out_virtual_width:
 *  Decides how wide the virtual screen really needs to be. That is more 
 *  complicated than it sounds, because the Allegro graphics primitives 
 *  require that each scanline be contained within a single bank. That 
 *  causes problems on cards that don't have overlapping banks, unless the 
 *  bank size is a multiple of the virtual width. So we may need to adjust 
 *  the width just to keep things running smoothly...
 */
void _sort_out_virtual_width(int *width, GFX_DRIVER *driver)
{
   int w = *width;

   /* hah! I love VBE 2.0... */
   if (driver->linear)
      return;

   /* if banks can overlap, we are ok... */ 
   if (driver->bank_size > driver->bank_gran)
      return;

   /* damn, have to increase the virtual width */
   while (((driver->bank_size / w) * w) != driver->bank_size) {
      w++;
      if (w > driver->bank_size)
	 break; /* oh shit */
   }

   *width = w;
}



/* _make_bitmap:
 *  Helper function for creating the screen bitmap. Sets up a bitmap 
 *  structure for addressing video memory at addr, and fills the bank 
 *  switching table using bank size/granularity information from the 
 *  specified graphics driver.
 */
BITMAP *_make_bitmap(int w, int h, unsigned long addr, GFX_DRIVER *driver, int color_depth, int bpl)
{
   BITMAP *b;
   int i;
   int bank;
   int size;
   GFX_VTABLE *vtable = _get_vtable(color_depth);

   if (!vtable)
      return NULL;

   size = sizeof(BITMAP) + sizeof(char *) * h;

   b = (BITMAP *)malloc(size);
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
   b->vtable = &_screen_vtable;
   b->write_bank = b->read_bank = _stub_bank_switch;
   b->dat = NULL;
   b->bitmap_id = 0;
   b->extra = NULL;
   b->x_ofs = 0;
   b->y_ofs = 0;
   b->seg = _dos_ds;

   memcpy(&_screen_vtable, vtable, sizeof(GFX_VTABLE));
   LOCK_DATA(&_screen_vtable, sizeof(GFX_VTABLE));

   _last_bank_1 = _last_bank_2 = -1;

   driver->vid_phys_base = addr;

   b->line[0] = (char *)addr;
   _gfx_bank[0] = 0;

   if (driver->linear) {
      for (i=1; i<h; i++) {
	 b->line[i] = b->line[i-1] + bpl;
	 _gfx_bank[i] = 0;
      }
   }
   else {
      bank = 0;

      for (i=1; i<h; i++) {
	 b->line[i] = b->line[i-1] + bpl;
	 if (b->line[i]+bpl-1 >= (unsigned char *)addr + driver->bank_size) {
	    while (b->line[i] >= (unsigned char *)addr + driver->bank_gran) {
	       b->line[i] -= driver->bank_gran;
	       bank++;
	    }
	 }
	 _gfx_bank[i] = bank;
      }
   }

   return b;
}



/* create_bitmap_ex
 *  Creates a new memory butmap in the specified color_depth
 */
BITMAP *create_bitmap_ex(int color_depth, int width, int height)
{
   BITMAP *bitmap;
   int i;
   GFX_VTABLE *vtable = _get_vtable(color_depth);

   if (!vtable)
      return NULL;

   bitmap = malloc(sizeof(BITMAP) + (sizeof(char *) * height));
   if (!bitmap)
      return NULL;

   bitmap->dat = malloc(width * height * BYTES_PER_PIXEL(color_depth));
   if (!bitmap->dat) {
      free(bitmap);
      return NULL;
   }

   bitmap->w = bitmap->cr = width;
   bitmap->h = bitmap->cb = height;
   bitmap->clip = TRUE;
   bitmap->cl = bitmap->ct = 0;
   bitmap->vtable = vtable;
   bitmap->write_bank = bitmap->read_bank = _stub_bank_switch;
   bitmap->bitmap_id = 0;
   bitmap->extra = NULL;
   bitmap->x_ofs = 0;
   bitmap->y_ofs = 0;
   bitmap->seg = _my_ds();

   bitmap->line[0] = bitmap->dat;
   for (i=1; i<height; i++)
      bitmap->line[i] = bitmap->line[i-1] + width * BYTES_PER_PIXEL(color_depth);

   return bitmap;
}



/* create_bitmap:
 *  Creates a new memory bitmap.
 */
BITMAP *create_bitmap(int width, int height)
{
   return create_bitmap_ex(_color_depth, width, height);
}



/* create_sub_bitmap:
 *  Creates a sub bitmap, ie. a bitmap sharing drawing memory with a
 *  pre-existing bitmap, but possibly with different clipping settings.
 *  Usually will be smaller, and positioned at some arbitrary point.
 *
 *  Mark Wodrich is the owner of the brain responsible this hugely useful 
 *  and beautiful function.
 */
BITMAP *create_sub_bitmap(BITMAP *parent, int x, int y, int width, int height)
{
   BITMAP *bitmap;
   int i;

   if (!parent) 
      return NULL;

   if (x < 0) 
      x = 0; 

   if (y < 0) 
      y = 0;

   if (x+width > parent->w) 
      width = parent->w-x;

   if (y+height > parent->h) 
      height = parent->h-y;

   /* get memory for structure and line pointers */
   bitmap = malloc(sizeof(BITMAP) + (sizeof(char *) * height));
   if (!bitmap)
      return NULL;

   bitmap->w = bitmap->cr = width;
   bitmap->h = bitmap->cb = height;
   bitmap->clip = TRUE;
   bitmap->cl = bitmap->ct = 0;
   bitmap->vtable = parent->vtable;
   bitmap->write_bank = parent->write_bank;
   bitmap->read_bank = parent->read_bank;
   bitmap->dat = NULL;
   bitmap->extra = NULL;
   bitmap->x_ofs = x + parent->x_ofs;
   bitmap->y_ofs = y + parent->y_ofs;
   bitmap->seg = parent->seg;

   /* All bitmaps are created with zero ID's. When a sub-bitmap is created,
    * a unique ID is needed to identify the relationship when blitting from
    * one to the other. This is obtained from the global variable
    * _sub_bitmap_id_count, which provides a sequence of integers (yes I
    * know it will wrap eventually, but not for a long time :-) If the
    * parent already has an ID the sub-bitmap adopts it, otherwise a new
    * ID is given to both the parent and the child.
    */
   if (parent->bitmap_id)
      bitmap->bitmap_id = parent->bitmap_id;
   else
      bitmap->bitmap_id = parent->bitmap_id = _sub_bitmap_id_count++;

   if (is_planar_bitmap(bitmap))
      x /= 4;

   x *= BYTES_PER_PIXEL(bitmap_color_depth(bitmap));

   /* setup line pointers: each line points to a line in the parent bitmap */
   for (i=0; i<height; i++)
      bitmap->line[i] = parent->line[y+i] + x;

   if (bitmap->vtable->set_clip)
      bitmap->vtable->set_clip(bitmap);

   return bitmap;
}



/* try_vram_location:
 *  Attempts to create a video memory bitmap out of the specified region
 *  of the larger screen surface, returning a pointer to it if that area
 *  is free.
 */
static BITMAP *try_vram_location(int x, int y, int w, int h)
{
   VRAM_BITMAP *block = vram_bitmap_list;

   if ((x<0) || (y<0) || (x+w > VIRTUAL_W) || (y+h > VIRTUAL_H))
      return NULL;

   while (block) {
      if (((x < block->x+block->w) && (x+w > block->x)) &&
	  ((y < block->y+block->h) && (y+h > block->y)))
	 return NULL;

      block = block->next;
   }

   block = malloc(sizeof(VRAM_BITMAP));
   if (!block)
      return NULL;

   block->x = x;
   block->y = y;
   block->w = w;
   block->h = h;

   block->bmp = create_sub_bitmap(screen, x, y, w, h);
   if (!(block->bmp)) {
      free(block);
      return NULL;
   }

   block->next = vram_bitmap_list;
   vram_bitmap_list = block;

   return block->bmp;
}



/* create_video_bitmap:
 *  Attempts to make a bitmap object for accessing offscreen video memory.
 */
BITMAP *create_video_bitmap(int width, int height)
{
   VRAM_BITMAP *blockx;
   VRAM_BITMAP *blocky;
   BITMAP *bmp;

   /* let the driver handle the request if it can */
   if (gfx_driver->create_video_bitmap) {
      VRAM_BITMAP *block = malloc(sizeof(VRAM_BITMAP));

      if (!block)
	 return NULL;

      bmp = gfx_driver->create_video_bitmap(width, height);
      if (!bmp) {
	 free(block);
	 return NULL;
      }

      block->x = -1;
      block->y = -1;
      block->w = 0;
      block->h = 0;
      block->bmp = bmp;
      block->next = vram_bitmap_list;
      vram_bitmap_list = block;

      return bmp;
   }

   /* otherwise fall back on subdividing the normal screen surface */
   if ((bmp = try_vram_location(0, 0, width, height)) != NULL)
      return bmp;

   for (blocky = vram_bitmap_list; blocky != NULL; blocky = blocky->next) {
      for (blockx = vram_bitmap_list; blockx != NULL; blockx = blockx->next) {
         if ((bmp = try_vram_location((blockx->x+blockx->w+15)&~15, blocky->y, width, height)) != NULL)
            return bmp;
         if ((bmp = try_vram_location((blockx->x-width)&~15, blocky->y, width, height)) != NULL)
            return bmp;
         if ((bmp = try_vram_location(blockx->x, blocky->y+blocky->h, width, height)) != NULL)
            return bmp;
         if ((bmp = try_vram_location(blockx->x, blocky->y-height, width, height)) != NULL)
            return bmp;
      }
   }

   return NULL;
}



/* destroy_bitmap:
 *  Destroys a memory bitmap.
 */
void destroy_bitmap(BITMAP *bitmap)
{
   VRAM_BITMAP *prev, *pos;

   if (bitmap) {
      if (is_screen_bitmap(bitmap)) {
	 /* special case for getting rid of video memory bitmaps */
	 prev = NULL;
	 pos = vram_bitmap_list;

	 while (pos) {
	    if (pos->bmp == bitmap) {
	       if (prev)
		  prev->next = pos->next;
	       else
		  vram_bitmap_list = pos->next;

	       if (pos->x < 0) {
		  /* the driver is in charge of this object */
		  gfx_driver->destroy_video_bitmap(bitmap);
		  free(pos);
		  return;
	       } 

	       free(pos);
	       break;
	    }

	    prev = pos;
	    pos = pos->next;
	 }
      }

      /* normal memory or sub-bitmap destruction */
      if (bitmap->dat)
	 free(bitmap->dat);

      free(bitmap);
   }
}



/* set_clip:
 *  Sets the two opposite corners of the clipping rectangle to be used when
 *  drawing to the bitmap. Nothing will be drawn to positions outside of this 
 *  rectangle. When a new bitmap is created the clipping rectangle will be 
 *  set to the full area of the bitmap. If x1, y1, x2 and y2 are all zero 
 *  clipping will be turned off, which will slightly speed up drawing 
 *  operations but will allow memory to be corrupted if you attempt to draw 
 *  off the edge of the bitmap.
 */
void set_clip(BITMAP *bitmap, int x1, int y1, int x2, int y2)
{
   int t;

   if ((x1==0) && (y1==0) && (x2==0) && (y2==0)) {
      bitmap->clip = FALSE;
      bitmap->cl = bitmap->ct = 0;
      bitmap->cr = bitmap->w;
      bitmap->cb = bitmap->h;

      if (bitmap->vtable->set_clip)
	 bitmap->vtable->set_clip(bitmap);

      return;
   }

   if (x2 < x1) {
      t = x1;
      x1 = x2;
      x2 = t;
   }

   if (y2 < y1) {
      t = y1;
      y1 = y2;
      y2 = t;
   }

   x2++;
   y2++;

   bitmap->clip = TRUE;
   bitmap->cl = MID(0, x1, bitmap->w-1);
   bitmap->ct = MID(0, y1, bitmap->h-1);
   bitmap->cr = MID(0, x2, bitmap->w);
   bitmap->cb = MID(0, y2, bitmap->h);

   if (bitmap->vtable->set_clip)
      bitmap->vtable->set_clip(bitmap);
}



/* scroll_screen:
 *  Attempts to scroll the hardware screen, returning 0 on success. 
 *  Check the VIRTUAL_W and VIRTUAL_H values to see how far the screen
 *  can be scrolled. Note that a lot of VESA drivers can only handle
 *  horizontal scrolling in four pixel increments.
 */
int scroll_screen(int x, int y)
{
   int ret = 0;
   int h;

   /* can driver handle hardware scrolling? */
   if (!gfx_driver->scroll)
      return -1;

   /* clip x */
   if (x < 0) {
      x = 0;
      ret = -1;
   }
   else if (x > (VIRTUAL_W - SCREEN_W)) {
      x = VIRTUAL_W - SCREEN_W;
      ret = -1;
   }

   /* clip y */
   if (y < 0) {
      y = 0;
      ret = -1;
   }
   else {
      h = (_modex_split_position > 0) ? _modex_split_position : SCREEN_H;
      if (y > (VIRTUAL_H - h)) {
	 y = VIRTUAL_H - h;
	 ret = -1;
      }
   }

   /* scroll! */
   if (gfx_driver->scroll(x, y) != 0)
      ret = -1;

   return ret;
}



/* request_scroll:
 *  Attempts to initiate a triple buffered hardware scroll, which will
 *  take place during the next retrace. Returns 0 on success.
 */
int request_scroll(int x, int y)
{
   int ret = 0;
   int h;

   /* can driver handle triple buffering? */
   if (!gfx_driver->request_scroll)
      return -1;

   /* clip x */
   if (x < 0) {
      x = 0;
      ret = -1;
   }
   else if (x > (VIRTUAL_W - SCREEN_W)) {
      x = VIRTUAL_W - SCREEN_W;
      ret = -1;
   }

   /* clip y */
   if (y < 0) {
      y = 0;
      ret = -1;
   }
   else {
      h = (_modex_split_position > 0) ? _modex_split_position : SCREEN_H;
      if (y > (VIRTUAL_H - h)) {
	 y = VIRTUAL_H - h;
	 ret = -1;
      }
   }

   /* scroll! */
   if (gfx_driver->request_scroll(x, y) != 0)
      ret = -1;

   return ret;
}



/* poll_scroll:
 *  Checks whether a requested triple buffer flip has actually taken place.
 */
int poll_scroll()
{
   if (!gfx_driver->poll_scroll)
      return FALSE;

   return gfx_driver->poll_scroll();
}



/* show_video_bitmap:
 *  Page flipping function: swaps to display the specified video memory 
 *  bitmap object (this must be the same size as the physical screen).
 */
int show_video_bitmap(BITMAP *bitmap)
{
   if ((!is_screen_bitmap(bitmap)) || 
       (bitmap->w != SCREEN_W) || (bitmap->h != SCREEN_H))
      return -1;

   if (gfx_driver->show_video_bitmap)
      return gfx_driver->show_video_bitmap(bitmap);

   return scroll_screen(bitmap->x_ofs, bitmap->y_ofs);
}



/* request_video_bitmap:
 *  Triple buffering function: triggers a swap to display the specified 
 *  video memory bitmap object, which will take place on the next retrace.
 */
int request_video_bitmap(BITMAP *bitmap)
{
   if ((!is_screen_bitmap(bitmap)) || 
       (bitmap->w != SCREEN_W) || (bitmap->h != SCREEN_H))
      return -1;

   if (gfx_driver->request_video_bitmap)
      return gfx_driver->request_video_bitmap(bitmap);

   return request_scroll(bitmap->x_ofs, bitmap->y_ofs);
}

