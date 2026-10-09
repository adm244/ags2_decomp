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
 *      Wrappers for Xlib functions.
 *
 *      By Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/aintern.h"
#include "allegro/aintunix.h"
#include "xwin.h"

#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>

#ifdef ALLEGRO_XWINDOWS_WITH_SHM
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#endif

#ifdef ALLEGRO_XWINDOWS_WITH_XF86DGA
#include <X11/extensions/xf86dga.h>
#endif


#define XWIN_DEFAULT_WINDOW_TITLE "Allegro application"
#define XWIN_DEFAULT_APPLICATION_NAME "allegro"
#define XWIN_DEFAULT_APPLICATION_CLASS "Allegro"


/* X-Windows resources used by the library.  */
static struct
{
   Display *display;
   Window window;
   GC gc;
   Visual *visual;
   Colormap colormap;
   XImage *ximage;
   Cursor cursor;
   int cursor_shape;

   void (*screen_to_ximage)(int sx, int sy, int sw, int sh);
   void (*set_color)(int index, RGB *color);

   unsigned char *screen_data;
   unsigned char **screen_line;
   unsigned char **ximage_line;

   int scroll_x;
   int scroll_y;

   int window_width;
   int window_height;
   int window_depth;

   int screen_width;
   int screen_height;
   int screen_depth;

   int virtual_width;
   int virtual_height;

   int mouse_warped;
   int keycode_to_scancode[256];

   int matching_formats;
   int fast_ximage_depth;
   int ximage_is_truecolor;

   int rsize;
   int gsize;
   int bsize;
   int rshift;
   int gshift;
   int bshift;

   unsigned long cmap[0x1000];
   unsigned long rmap[0x100];
   unsigned long gmap[0x100];
   unsigned long bmap[0x100];

#ifdef ALLEGRO_XWINDOWS_WITH_SHM
   XShmSegmentInfo shminfo;
#endif
   int use_shm;

   char window_title[1024];
   char application_name[1024];
   char application_class[1024];
} _xwin =
{
   0,		/* display */
   None,	/* window */
   None,	/* gc */
   0,		/* visual */
   None,	/* colormap */
   0,		/* ximage */
   None,	/* cursor */
   XC_heart,	/* cursor_shape */

   0,		/* screen_to_ximage */
   0,		/* set_color */

   0,		/* screen_data */
   0,		/* screen_line */
   0,		/* ximage_line */

   0,		/* scroll_x */
   0,		/* scroll_y */

   320,		/* window_width */
   200,		/* window_height */
   8,		/* window_depth */

   320,		/* screen_width */
   200,		/* screen_height */
   8,		/* screen_depth */

   320,		/* virtual width */
   200,		/* virtual_height */

   1,		/* mouse_warped */
   { 0 },	/* keycode_to_scancode */

   0,		/* matching formats */
   0,		/* fast_ximage_depth */
   0,		/* ximage_is_truecolor */

   1,		/* rsize */
   1,		/* gsize */
   1,		/* bsize */
   0,		/* rshift */
   0,		/* gshift */
   0,		/* bshift */

   { 0 },	/* cmap */
   { 0 },	/* rmap */
   { 0 },	/* gmap */
   { 0 },	/* bmap */

#ifdef ALLEGRO_XWINDOWS_WITH_SHM
   { 0 },	/* shminfo */
#endif
   0,		/* use_shm */

   XWIN_DEFAULT_WINDOW_TITLE,		/* window_title */
   XWIN_DEFAULT_APPLICATION_NAME,	/* application_name */
   XWIN_DEFAULT_APPLICATION_CLASS	/* application_class */
};

int _xwin_last_line = -1;
int _xwin_in_gfx_call = 0;

static char _xwin_driver_desc[256] = EMPTY_STRING;



/* Forward declarations for private functions.  */
static char *_xwin_safe_copy(char *dst, const char *src, int len);

static int _xwin_private_open_display(char *name);
static void _xwin_private_close_display(void);
static int _xwin_private_create_window(void);
static void _xwin_private_destroy_window(void);
static void _xwin_private_setup_driver_desc(GFX_DRIVER *drv);
static struct BITMAP *_xwin_private_create_screen(GFX_DRIVER *drv, int w, int h,
						  int vw, int vh, int depth);
static void _xwin_private_destroy_screen(void);
static void _xwin_private_create_mapping(unsigned long *map, int ssize, int dsize, int dshift);
static int _xwin_private_create_ximage(int w, int h);
static void _xwin_private_destroy_ximage(void);
static int _xwin_private_fast_ximage_depth(int depth);
static void _xwin_private_set_palette_range(PALETTE p, int from, int to);
static void _xwin_private_set_window_defaults(void);
static void _xwin_private_sync(void);
static void _xwin_private_resize_window(int w, int h);
static void _xwin_private_process_event(XEvent *event);
static void _xwin_private_handle_input(void);
static void _xwin_private_redraw_window(int x, int y, int w, int h);
static void _xwin_private_update_screen(int x, int y, int w, int h);
static void _xwin_private_set_window_title(const char *name);
static void _xwin_private_change_keyboard_control(int led, int on);
static int _xwin_private_get_pointer_mapping(unsigned char map[], int nmap);
static void _xwin_private_init_keyboard_tables(void);

static void _xwin_private_fast_truecolor_8_to_8(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_8_to_16(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_8_to_32(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_15_to_8(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_15_to_16(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_15_to_32(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_16_to_8(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_16_to_16(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_16_to_32(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_24_to_8(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_24_to_16(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_24_to_32(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_32_to_8(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_32_to_16(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_32_to_32(int sx, int sy, int sw, int sh);

static void _xwin_private_slow_truecolor_8(int sx, int sy, int sw, int sh);
static void _xwin_private_slow_truecolor_15(int sx, int sy, int sw, int sh);
static void _xwin_private_slow_truecolor_16(int sx, int sy, int sw, int sh);
static void _xwin_private_slow_truecolor_24(int sx, int sy, int sw, int sh);
static void _xwin_private_slow_truecolor_32(int sx, int sy, int sw, int sh);

static void _xwin_private_fast_palette_8_to_8(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_palette_8_to_16(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_palette_8_to_32(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_palette_15_to_8(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_palette_15_to_16(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_palette_15_to_32(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_palette_16_to_8(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_palette_16_to_16(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_palette_16_to_32(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_palette_24_to_8(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_palette_24_to_16(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_palette_24_to_32(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_palette_32_to_8(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_palette_32_to_16(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_palette_32_to_32(int sx, int sy, int sw, int sh);

static void _xwin_private_slow_palette_8(int sx, int sy, int sw, int sh);
static void _xwin_private_slow_palette_15(int sx, int sy, int sw, int sh);
static void _xwin_private_slow_palette_16(int sx, int sy, int sw, int sh);
static void _xwin_private_slow_palette_24(int sx, int sy, int sw, int sh);
static void _xwin_private_slow_palette_32(int sx, int sy, int sw, int sh);

static void _xwin_private_set_matching_color(int index, RGB *color);
static void _xwin_private_set_truecolor_color(int index, RGB *color);
static void _xwin_private_set_palette_color(int index, RGB *color);

#if defined(__GNUC__) && defined(__i386__)
unsigned long _xwin_read_line_asm(BITMAP *bmp, int line);
unsigned long _xwin_write_line_asm(BITMAP *bmp, int line);
void _xwin_unwrite_line_asm(BITMAP *bmp);
#endif



/* _xwin_safe_copy:
 *  Copy string, testing for buffer overrun (why isn't it in ANSI C?).
 */
static char* _xwin_safe_copy(char *dst, const char *src, int len)
{
   if (len <= 0)
      return dst;
   dst[0] = 0;
   strncat(dst, src, len - 1);
   return dst;
}



/* _xwin_open_display:
 *  Wrapper for XOpenDisplay.
 */
static int _xwin_private_open_display(char *name)
{
   if (_xwin.display != 0)
      return -1;

   _xwin.display = XOpenDisplay(name);

   return ((_xwin.display != 0) ? 0 : -1);
}

int _xwin_open_display(char *name)
{
   int result;
   DISABLE();
   result = _xwin_private_open_display(name);
   ENABLE();
   return result;
}



/* _xwin_close_display:
 *  Wrapper for XCloseDisplay.
 */
static void _xwin_private_close_display(void)
{
   _xwin_private_destroy_window();

   if (_xwin.display != 0) {
      XCloseDisplay(_xwin.display);
      _xwin.display = 0;
   }
}

void _xwin_close_display(void)
{
   DISABLE();
   _xwin_private_close_display();
   ENABLE();
}



/* _xwin_create_window:
 *  Wrapper for XCreateWindow.
 */
static int _xwin_private_create_window(void)
{
   XEvent event;
   unsigned long gcmask;
   XGCValues gcvalues;
   XSetWindowAttributes setattr;
   XWindowAttributes getattr;
   Pixmap pixmap;

   if (_xwin.display == 0)
      return -1;

   /* Create window.  */
   setattr.border_pixel = XBlackPixel(_xwin.display, XDefaultScreen(_xwin.display));
   setattr.event_mask = (KeyPressMask | KeyReleaseMask | EnterWindowMask | ExposureMask
			 | ButtonPressMask | ButtonReleaseMask | PointerMotionMask
			 /*| MappingNotifyMask (SubstructureRedirectMask?)*/);
   _xwin.window = XCreateWindow(_xwin.display, XDefaultRootWindow(_xwin.display),
				0, 0, 320, 200, 0,
				CopyFromParent, InputOutput, CopyFromParent,
				CWBorderPixel | CWEventMask, &setattr);

   /* Get associated visual and window depth (bits per pixel).  */
   XGetWindowAttributes(_xwin.display, _xwin.window, &getattr);
   _xwin.visual = getattr.visual;
   _xwin.window_depth = getattr.depth;

   /* Create and install colormap.  */
   if ((_xwin.visual->class == PseudoColor)
       || (_xwin.visual->class == GrayScale)
       || (_xwin.visual->class == DirectColor))
      _xwin.colormap = XCreateColormap(_xwin.display, _xwin.window, _xwin.visual, AllocAll);
   else
      _xwin.colormap = XCreateColormap(_xwin.display, _xwin.window, _xwin.visual, AllocNone);
   XSetWindowColormap(_xwin.display, _xwin.window, _xwin.colormap);
   XInstallColormap(_xwin.display, _xwin.colormap);

   /* Set default window parameters.  */
   _xwin_private_set_window_defaults();

   /* Map window.  */
   XMapWindow(_xwin.display, _xwin.window);

   /* Create graphics context.  */
   gcmask = GCFunction | GCForeground | GCBackground | GCFillStyle | GCPlaneMask;
   gcvalues.function = GXcopy;
   gcvalues.foreground = setattr.border_pixel;
   gcvalues.background = setattr.border_pixel;
   gcvalues.fill_style = FillSolid;
   gcvalues.plane_mask = AllPlanes;
   _xwin.gc = XCreateGC(_xwin.display, _xwin.window, gcmask, &gcvalues);

   /* Create invisible X cursor.  */
   pixmap = XCreatePixmap(_xwin.display, _xwin.window, 1, 1, 1);
   if (pixmap != None) {
      GC temp_gc;
      XColor color;

      gcmask = GCFunction | GCForeground | GCBackground;
      gcvalues.function = GXcopy;
      gcvalues.foreground = 0;
      gcvalues.background = 0;
      temp_gc = XCreateGC(_xwin.display, pixmap, gcmask, &gcvalues);
      XDrawPoint(_xwin.display, pixmap, temp_gc, 0, 0);
      XFreeGC(_xwin.display, temp_gc);
      color.pixel = 0;
      color.red = color.green = color.blue = 0;
      color.flags = DoRed | DoGreen | DoBlue;
      _xwin.cursor = XCreatePixmapCursor(_xwin.display, pixmap, pixmap, &color, &color, 0, 0);
      XDefineCursor(_xwin.display, _xwin.window, _xwin.cursor);
      XFreePixmap(_xwin.display, pixmap);
   }
   else {
      _xwin.cursor = XCreateFontCursor(_xwin.display, _xwin.cursor_shape);
      XDefineCursor(_xwin.display, _xwin.window, _xwin.cursor);
   }

   /* Wait for the first exposure event.  */
   do {
      XNextEvent(_xwin.display, &event);
   } while ((event.type != Expose) || (event.xexpose.count != 0));

   return 0;
}

int _xwin_create_window(void)
{
   int result;
   DISABLE();
   result = _xwin_private_create_window();
   ENABLE();
   return result;
}



/* _xwin_destroy_window:
 *  Wrapper for XDestroyWindow.
 */
static void _xwin_private_destroy_window(void)
{
   _xwin_private_destroy_screen ();

   if (_xwin.cursor != None) {
      XUndefineCursor(_xwin.display, _xwin.window);
      XFreeCursor(_xwin.display, _xwin.cursor);
      _xwin.cursor = None;
   }

   _xwin.visual = 0;

   if (_xwin.gc != None) {
      XFreeGC(_xwin.display, _xwin.gc);
      _xwin.gc = None;
   }

   if (_xwin.colormap != None) {
      XUninstallColormap(_xwin.display, _xwin.colormap);
      XFreeColormap(_xwin.display, _xwin.colormap);
      _xwin.colormap = None;
   }

   if (_xwin.window != None) {
      XUnmapWindow(_xwin.display, _xwin.window);
      XDestroyWindow(_xwin.display, _xwin.window);
      _xwin.window = None;
   }
}

void _xwin_destroy_window(void)
{
   DISABLE();
   _xwin_private_destroy_window();
   ENABLE();
}



typedef void (*_XWIN_SCREEN_TO_XIMAGE)(int x, int y, int w, int h);
static _XWIN_SCREEN_TO_XIMAGE _xwin_screen_to_ximage_function[5][8] =
{
   {
      _xwin_private_slow_truecolor_8,
      _xwin_private_fast_truecolor_8_to_8,
      _xwin_private_fast_truecolor_8_to_16,
      _xwin_private_fast_truecolor_8_to_32,
      _xwin_private_slow_palette_8,
      _xwin_private_fast_palette_8_to_8,
      _xwin_private_fast_palette_8_to_16,
      _xwin_private_fast_palette_8_to_32
   },
   {
      _xwin_private_slow_truecolor_15,
      _xwin_private_fast_truecolor_15_to_8,
      _xwin_private_fast_truecolor_15_to_16,
      _xwin_private_fast_truecolor_15_to_32,
      _xwin_private_slow_palette_15,
      _xwin_private_fast_palette_15_to_8,
      _xwin_private_fast_palette_15_to_16,
      _xwin_private_fast_palette_15_to_32
   },
   {
      _xwin_private_slow_truecolor_16,
      _xwin_private_fast_truecolor_16_to_8,
      _xwin_private_fast_truecolor_16_to_16,
      _xwin_private_fast_truecolor_16_to_32,
      _xwin_private_slow_palette_16,
      _xwin_private_fast_palette_16_to_8,
      _xwin_private_fast_palette_16_to_16,
      _xwin_private_fast_palette_16_to_32
   },
   {
      _xwin_private_slow_truecolor_24,
      _xwin_private_fast_truecolor_24_to_8,
      _xwin_private_fast_truecolor_24_to_16,
      _xwin_private_fast_truecolor_24_to_32,
      _xwin_private_slow_palette_24,
      _xwin_private_fast_palette_24_to_8,
      _xwin_private_fast_palette_24_to_16,
      _xwin_private_fast_palette_24_to_32
   },
   {
      _xwin_private_slow_truecolor_32,
      _xwin_private_fast_truecolor_32_to_8,
      _xwin_private_fast_truecolor_32_to_16,
      _xwin_private_fast_truecolor_32_to_32,
      _xwin_private_slow_palette_32,
      _xwin_private_fast_palette_32_to_8,
      _xwin_private_fast_palette_32_to_16,
      _xwin_private_fast_palette_32_to_32
   },
};



/* _xwin_setup_driver_desc:
 *  Sets up the X-Windows driver description string.
 */
static void _xwin_private_setup_driver_desc(GFX_DRIVER *drv)
{
   char tmp1[80], tmp2[80], tmp3[80];

   /* Prepare driver description.  */
   if (_xwin.matching_formats) {
      usprintf(_xwin_driver_desc,
	       uconvert_ascii("X-Windows graphics, in matching, %d bpp window", tmp1),
	       _xwin.window_depth);
   }
   else {
      usprintf(_xwin_driver_desc,
	       uconvert_ascii("X-Windows graphics, in %s %s, %d bpp window", tmp1),
	       uconvert_ascii((_xwin.fast_ximage_depth ? "fast" : "slow"), tmp2),
	       uconvert_ascii((_xwin.ximage_is_truecolor ? "truecolor" : "paletted"), tmp3),
	       _xwin.window_depth);
   }
   drv->desc = _xwin_driver_desc;
}



/* _xwin_create_screen:
 *  Creates screen data and other resources.
 */
static BITMAP *_xwin_private_create_screen(GFX_DRIVER *drv, int w, int h,
					   int vw, int vh, int depth)
{
   int line;
   int bytes_per_line;
   BITMAP *bmp;

   if (_xwin.window == None)
      return 0;

   /* Choose convenient size.  */
   if ((w == 0) && (h == 0)) {
      w = 320;
      h = 200;
   }

   if ((w < 80) || (h < 80) || (w > 4096) || (h > 4096)
       || (vw > 4096) || (vh > 4096))
      return 0;

   if (vw < w)
      vw = w;
   if (vh < h)
      vh = h;

   if (1
#ifdef ALLEGRO_COLOR8
       && (depth != 8)
#endif
#ifdef ALLEGRO_COLOR16
       && (depth != 15)
       && (depth != 16)
#endif
#ifdef ALLEGRO_COLOR24
       && (depth != 24)
#endif
#ifdef ALLEGRO_COLOR32
       && (depth != 32)
#endif
       )
      return 0;

   /* Set window size and save dimensions.  */
   _xwin_private_resize_window(w, h);
   _xwin.screen_width = w;
   _xwin.screen_height = h;
   _xwin.screen_depth = depth;
   _xwin.virtual_width = vw;
   _xwin.virtual_height = vh;

   /* Create XImage with the size of virtual screen.  */
   if (_xwin_private_create_ximage(vw, vh) != 0)
      return 0;

   /* Calculate R,G,B sizes and shifts for true color visuals.  */
   if ((_xwin.visual->class == TrueColor)
       || (_xwin.visual->class == DirectColor)) {
      int j;
      unsigned long i;

      /* Red shift and size.  */
      for (i = _xwin.visual->red_mask, j = 0; (i & 1) != 1; i >>= 1)
	 j++;
      _xwin.rshift = j;
      for (j = 0; i != 0; i >>= 1)
	 j++;
      _xwin.rsize = 1 << j;

      /* Green shift and size.  */
      for (i = _xwin.visual->green_mask, j = 0; (i & 1) != 1; i >>= 1)
	 j++;
      _xwin.gshift = j;
      for (j = 0; i != 0; i >>= 1)
	 j++;
      _xwin.gsize = 1 << j;

      /* Blue shift and size.  */
      for (i = _xwin.visual->blue_mask, j = 0; (i & 1) != 1; i >>= 1)
	 j++;
      _xwin.bshift = j;
      for (j = 0; i != 0; i >>= 1)
	 j++;
      _xwin.bsize = 1 << j;

      _xwin.ximage_is_truecolor = 1;
   }
   else {
      /* Not useful for other visual types.  */
      _xwin.rsize = 1;
      _xwin.bsize = 1;
      _xwin.gsize = 1;
      _xwin.rshift = 0;
      _xwin.gshift = 0;
      _xwin.bshift = 0;

      _xwin.ximage_is_truecolor = 0;
   }

   /* Test that XImage is fast (can be accessed directly).  */
   _xwin.fast_ximage_depth = _xwin_private_fast_ximage_depth(_xwin.window_depth);

   /* Test that Allegro and X-Windows pixel formats are the same.  */
   _xwin.matching_formats = 0;
   if (_xwin.fast_ximage_depth != 0) {
      do {
	 if (depth == 8) {
	    /* For matching 8 bpp modes visual must be PseudoColor or GrayScale.  */
	    if (((_xwin.visual->class != PseudoColor)
		 && (_xwin.visual->class != GrayScale))
		|| (_xwin.fast_ximage_depth != 8)
		|| (_xwin.window_depth != 8)
		/* || (_xwin.visual->map_entries != 256) */)
	       break;
	 }
	 else if ((_xwin.visual->class != TrueColor)
		  && (_xwin.visual->class != DirectColor)) {
	    /* For matching true color modes visual must be TrueColor or DirectColor.  */
	    break;
	 }
	 else if (depth == 15) {
	    if ((_xwin.fast_ximage_depth != 16)
		|| (_xwin.rsize != 32) || (_xwin.gsize != 32) || (_xwin.bsize != 32)
		|| ((_xwin.rshift != 0) && (_xwin.rshift != 10))
		|| ((_xwin.bshift != 0) && (_xwin.bshift != 10))
		|| (_xwin.gshift != 5))
	       break;
	    _rgb_r_shift_15 = _xwin.rshift;
	    _rgb_g_shift_15 = _xwin.gshift;
	    _rgb_b_shift_15 = _xwin.bshift;
	 }
	 else if (depth == 16) {
	    if ((_xwin.fast_ximage_depth != 16)
		|| (_xwin.rsize != 32) || (_xwin.gsize != 64) || (_xwin.bsize != 32)
		|| ((_xwin.rshift != 0) && (_xwin.rshift != 11))
		|| ((_xwin.bshift != 0) && (_xwin.bshift != 11))
		|| (_xwin.gshift != 5))
	       break;
	    _rgb_r_shift_16 = _xwin.rshift;
	    _rgb_g_shift_16 = _xwin.gshift;
	    _rgb_b_shift_16 = _xwin.bshift;
	 }
	 else if (depth == 32) {
	    if ((_xwin.fast_ximage_depth != 32)
		|| (_xwin.rsize != 256) || (_xwin.gsize != 256) || (_xwin.bsize != 256)
		|| ((_xwin.rshift != 0) && (_xwin.rshift != 16))
		|| ((_xwin.bshift != 0) && (_xwin.bshift != 16))
		|| (_xwin.gshift != 8))
	       break;
	    _rgb_r_shift_32 = _xwin.rshift;
	    _rgb_g_shift_32 = _xwin.gshift;
	    _rgb_b_shift_32 = _xwin.bshift;
	 }
	 else {
	    /* Useless 24 bpp mode.  */
	    break;
	 }
	 _xwin.matching_formats = 1;
      } while (0);
   }

   /* Convert DirectColor visual into true color visual.  */
   if (_xwin.visual->class == DirectColor) {
      int i;
      XColor color;

      color.flags = DoRed;
      for (i = 0; i < _xwin.rsize; i++) {
	 color.pixel = i << _xwin.rshift;
	 color.red = (i * 65536L) / _xwin.rsize;
	 XStoreColor(_xwin.display, _xwin.colormap, &color);
      }
      color.flags = DoGreen;
      for (i = 0; i < _xwin.gsize; i++) {
	 color.pixel = i << _xwin.gshift;
	 color.green = (i * 65536L) / _xwin.gsize;
	 XStoreColor(_xwin.display, _xwin.colormap, &color);
      }
      color.flags = DoBlue;
      for (i = 0; i < _xwin.bsize; i++) {
	 color.pixel = i << _xwin.bshift;
	 color.blue = (i * 65536L) / _xwin.bsize;
	 XStoreColor(_xwin.display, _xwin.colormap, &color);
      }
   }

   /* Convert paletted visual into true color visual.  */
   if (!_xwin.matching_formats
       && ((_xwin.visual->class == PseudoColor)
	   || (_xwin.visual->class == GrayScale))) {
      int b = _xwin.window_depth / 3;
      int r = (_xwin.window_depth - b) / 2;
      int g = _xwin.window_depth - r - b;
      XColor color;

      _xwin.rsize = 1 << r;
      _xwin.gsize = 1 << g;
      _xwin.bsize = 1 << b;
      _xwin.rshift = g + b;
      _xwin.gshift = b;
      _xwin.bshift = 0;

      _xwin.ximage_is_truecolor = 1;

      color.flags = DoRed | DoGreen | DoBlue;
      for (r = 0; r < _xwin.rsize; r++) {
	 for (g = 0; g < _xwin.gsize; g++) {
	    for (b = 0; b < _xwin.bsize; b++) {
	       color.pixel = (r << _xwin.rshift) | (g << _xwin.gshift) | (b << _xwin.bshift);
	       color.red = (r * 65536L) / _xwin.rsize;
	       color.green = (g * 65536L) / _xwin.gsize;
	       color.blue = (b * 65536L) / _xwin.bsize;
	       XStoreColor(_xwin.display, _xwin.colormap, &color);
	    }
	 }
      }
   }

   /* Create mapping tables for color components.  */
   if (!_xwin.matching_formats) {
      if (_xwin.ximage_is_truecolor) {
	 switch (depth) {
	    case 8:
	       /* Will be modified later in set_palette.  */
	       _xwin_private_create_mapping(_xwin.rmap, 256, 0, 0);
	       _xwin_private_create_mapping(_xwin.gmap, 256, 0, 0);
	       _xwin_private_create_mapping(_xwin.bmap, 256, 0, 0);
	       break;
	    case 15:
	       _xwin_private_create_mapping(_xwin.rmap, 32, _xwin.rsize, _xwin.rshift);
	       _xwin_private_create_mapping(_xwin.gmap, 32, _xwin.gsize, _xwin.gshift);
	       _xwin_private_create_mapping(_xwin.bmap, 32, _xwin.bsize, _xwin.bshift);
	       break;
	    case 16:
	       _xwin_private_create_mapping(_xwin.rmap, 32, _xwin.rsize, _xwin.rshift);
	       _xwin_private_create_mapping(_xwin.gmap, 64, _xwin.gsize, _xwin.gshift);
	       _xwin_private_create_mapping(_xwin.bmap, 32, _xwin.bsize, _xwin.bshift);
	       break;
	    case 24:
	    case 32:
	       _xwin_private_create_mapping(_xwin.rmap, 256, _xwin.rsize, _xwin.rshift);
	       _xwin_private_create_mapping(_xwin.gmap, 256, _xwin.gsize, _xwin.gshift);
	       _xwin_private_create_mapping(_xwin.bmap, 256, _xwin.bsize, _xwin.bshift);
	       break;
	 }
      }
      else {
	 /* Make fixed palette and create mapping RRRRGGGGBBBB -> palette index.  */
	 int i, r, g, b;
	 XColor color;

	 for (r = 0; r < 16; r++) {
	    for (g = 0; g < 16; g++) {
	       for (b = 0; b < 16; b++) {
		  color.red = (r * 65536L) / 16;
		  color.green = (g * 65536L) / 16;
		  color.blue = (b * 65536L) / 16;
		  XAllocColor(_xwin.display, _xwin.colormap, &color);
		  _xwin.cmap[(r << 8) | (g << 4) | b] = color.pixel;
	       }
	    }
	 }

	 /* Might be modified later in set_palette.  */
	 for (i = 0; i < 256; i++)
	    _xwin.rmap[i] = _xwin.gmap[i] = _xwin.bmap[i] = 0;
      }
   }

   /*
    * Determine how to update XImage with screen data.
    */
   if (_xwin.matching_formats) {
      _xwin.screen_to_ximage = 0;
   }
   else {
      int i, j;

      switch (depth) {
	 case 8: i = 0; break;
	 case 15: i = 1; break;
	 case 16: i = 2; break;
	 case 24: i = 3; break;
	 case 32: i = 4; break;
	 default: return 0;
      }
      switch (_xwin.fast_ximage_depth) {
	 case 0: j = 0; break;
	 case 8: j = 1; break;
	 case 16: j = 2; break;
	 case 32: j = 3; break;
	 default: return 0;
      }
      if (!_xwin.ximage_is_truecolor)
	 j += 4;
      _xwin.screen_to_ximage = _xwin_screen_to_ximage_function[i][j];
   }

   /*
    * Determine how to set color in "hardware".
    */
   if (depth != 8) {
      _xwin.set_color = 0;
   }
   else {
      if (_xwin.matching_formats) {
	 _xwin.set_color = _xwin_private_set_matching_color;
      }
      else if (_xwin.ximage_is_truecolor) {
	 _xwin.set_color = _xwin_private_set_truecolor_color;
      }
      else {
	 _xwin.set_color = _xwin_private_set_palette_color;
      }
   }

   /* Create line accelerators for screen data.  */
   _xwin.screen_line = malloc(vh * sizeof(unsigned char*));
   if (_xwin.screen_line == 0)
      return 0;

   /* If formats match, then use ximage as screen data, otherwise malloc.  */
   if (_xwin.matching_formats) {
      _xwin.screen_data = 0;
      _xwin.screen_line[0] = _xwin.ximage->data + _xwin.ximage->xoffset;
      bytes_per_line = _xwin.ximage->bytes_per_line;
   }
   else {
      _xwin.screen_data = malloc(vw * vh * BYTES_PER_PIXEL(depth));
      if (_xwin.screen_data == 0)
	 return 0;
      _xwin.screen_line[0] = _xwin.screen_data;
      bytes_per_line = vw * BYTES_PER_PIXEL(depth);
   }

   /* Initialize line starts.  */
   for (line = 1; line < vh; line++)
      _xwin.screen_line[line] = _xwin.screen_line[line - 1] + bytes_per_line;

   /* Create line accelerators for ximage.  */
   if (!_xwin.matching_formats && _xwin.fast_ximage_depth) {
      _xwin.ximage_line = malloc(vh * sizeof(unsigned char*));
      if (_xwin.ximage_line == 0)
	 return 0;

      _xwin.ximage_line[0] = _xwin.ximage->data + _xwin.ximage->xoffset;
      for (line = 1; line < vh; line++)
	 _xwin.ximage_line[line] = _xwin.ximage_line[line - 1] + _xwin.ximage->bytes_per_line;
   }

   /* Create bitmap.  */
   bmp = _make_bitmap(vw, vh, (unsigned long) (_xwin.screen_line[0]), drv, depth, bytes_per_line);
   if (bmp == 0)
      return 0;

   /* Fixup bitmap fields.  */
   drv->w = bmp->cr = w;
   drv->h = bmp->cb = h;

   /* Set bank switch routines.  */
#if defined(__GNUC__) && defined(__i386__)
   bmp->read_bank = _xwin_read_line_asm;
   bmp->write_bank = _xwin_write_line_asm;
   bmp->vtable->unwrite_bank = _xwin_unwrite_line_asm;
#else
   bmp->read_bank = _xwin_read_line;
   bmp->write_bank = _xwin_write_line;
   bmp->vtable->unwrite_bank = _xwin_unwrite_line;
#endif

   /* Replace entries in vtable with magical wrappers.  */
   _xwin_replace_vtable(bmp->vtable);

   /* Initialize other fields in _xwin structure.  */
   _xwin_last_line = -1;
   _xwin_in_gfx_call = 0;
   _xwin.scroll_x = 0;
   _xwin.scroll_y = 0;

   /* Setup driver description string.  */
   _xwin_private_setup_driver_desc(drv);

   return bmp;
}

BITMAP *_xwin_create_screen(GFX_DRIVER *drv, int w, int h,
			    int vw, int vh, int depth)
{
   BITMAP *bmp;
   DISABLE();
   bmp = _xwin_private_create_screen(drv, w, h, vw, vh, depth);
   if (bmp == 0)
      _xwin_private_destroy_screen();
   ENABLE();
   return bmp;
}



/* _xwin_destroy_screen:
 *  Destroys screen resources.
 */
static void _xwin_private_destroy_screen(void)
{
   if (_xwin.ximage_line != 0) {
      free(_xwin.ximage_line);
      _xwin.ximage_line = 0;
   }

   if (_xwin.screen_line != 0) {
      free(_xwin.screen_line);
      _xwin.screen_line = 0;
   }

   if (_xwin.screen_data != 0) {
      free(_xwin.screen_data);
      _xwin.screen_data = 0;
   }

   _xwin_private_destroy_ximage();
   _xwin_private_set_window_defaults();
}

void _xwin_destroy_screen(void)
{
   DISABLE();
   _xwin_private_destroy_screen();
   ENABLE();
}



/* _xwin_create_mapping:
 *  Create mapping between Allegro color component and X-Windows color component.
 */
static void _xwin_private_create_mapping(unsigned long *map, int ssize, int dsize, int dshift)
{
   int i;
   for (i = 0; i < ssize; i++)
      map[i] = ((dsize * i) / ssize) << dshift;
   for (; i < 256; i++)
      map[i] = map[i % ssize];
}



/* _xwin_create_ximage:
 *  Create XImage for accessing window.
 */
static int _xwin_private_create_ximage(int w, int h)
{
#ifdef ALLEGRO_XWINDOWS_WITH_SHM
   int is_local;
   char *name;
#endif
   XImage *image = 0; /* I'm mage or I'm old?  */

   if (_xwin.display == 0)
      return -1;

#ifdef ALLEGRO_XWINDOWS_WITH_SHM
   /* Get display name and test for local display.  */
   name = XDisplayName(0);

   if ((name == 0) || (name[0] == ':') || (strncmp(name, "unix:", 5) == 0))
      is_local = 1;
   else
      is_local = 0;

   if (is_local && XShmQueryExtension(_xwin.display))
      _xwin.use_shm = 1;
   else
      _xwin.use_shm = 0;
#else
   _xwin.use_shm = 0;
#endif

#ifdef ALLEGRO_XWINDOWS_WITH_SHM
   if (_xwin.use_shm) {
      /* Try to create shared memory XImage.  */
      image = XShmCreateImage(_xwin.display, _xwin.visual, _xwin.window_depth,
			      ZPixmap, 0, &_xwin.shminfo, w, h);
      do {
	 if (image != 0) {
	    /* Create shared memory segment.  */
	    _xwin.shminfo.shmid = shmget(IPC_PRIVATE, image->bytes_per_line * image->height,
					 IPC_CREAT | 0777);
	    if (_xwin.shminfo.shmid != -1) {
	       /* Attach shared memory to our address space.  */
	       _xwin.shminfo.shmaddr = image->data = shmat(_xwin.shminfo.shmid, 0, 0);
	       if (_xwin.shminfo.shmaddr != (char*) -1) {
		  _xwin.shminfo.readOnly = True;

		  /* Attach shared memory to the X-server address space.  */
		  if (XShmAttach(_xwin.display, &_xwin.shminfo))
		     break;

		  shmdt(_xwin.shminfo.shmaddr);
	       }
	       shmctl(_xwin.shminfo.shmid, IPC_RMID, 0);
	    }
	    XDestroyImage(image);
	    image = 0;
	 }
	 _xwin.use_shm = 0;
      } while (0);
   }
#endif

   if (image == 0) {
      /* Try to create ordinary XImage.  */
#if 0
      Pixmap pixmap;

      pixmap = XCreatePixmap(_xwin.display, _xwin.window, w, h, _xwin.window_depth);
      if (pixmap != None) {
	 image = XGetImage(_xwin.display, pixmap, 0, 0, w, h, AllPlanes, ZPixmap);
	 XFreePixmap(_xwin.display, pixmap);
      }
#else
      image = XCreateImage(_xwin.display, _xwin.visual, _xwin.window_depth,
			   ZPixmap, 0, 0, w, h, 32, 0);
      if (image != 0) {
	 image->data = malloc(image->bytes_per_line * image->height);
	 if (image->data == 0) {
	    XDestroyImage(image);
	    image = 0;
	 }
      }
#endif
   }

   _xwin.ximage = image;

   return ((image != 0) ? 0 : -1);
}



/* _xwin_destroy_ximage:
 *  Destroy XImage.
 */
static void _xwin_private_destroy_ximage(void)
{
   if (_xwin.ximage != 0) {
#ifdef ALLEGRO_XWINDOWS_WITH_SHM
      if (_xwin.use_shm) {
	 XShmDetach(_xwin.display, &_xwin.shminfo);
	 shmdt(_xwin.shminfo.shmaddr);
	 shmctl(_xwin.shminfo.shmid, IPC_RMID, 0);
      }
#endif
      XDestroyImage(_xwin.ximage);
      _xwin.ximage = 0;
   }
}



/* _xwin_fast_ximage_depth:
 *  Find which depth is fast (when XImage can be accessed directly).
 */
static int _xwin_private_fast_ximage_depth(int depth)
{
   int ok, x, sizex;
   int test_depth;
   unsigned char *p8;
   unsigned short *p16;
   unsigned long *p32;

   if (_xwin.ximage == 0)
      return 0;

   /* Use first line of XImage for test.  */
   p8 = _xwin.ximage->data + _xwin.ximage->xoffset;
   p16 = (unsigned short*) p8;
   p32 = (unsigned long*) p8;

   sizex = _xwin.ximage->bytes_per_line - _xwin.ximage->xoffset;

   if ((depth < 1) || (depth > 32)) {
      return 0;
   }
   else if (depth > 16) {
      test_depth = 32;
      sizex /= sizeof (unsigned long);
   }
   else if (depth > 8) {
      test_depth = 16;
      sizex /= sizeof (unsigned short);
   }
   else {
      test_depth = 8;
   }
   if (sizex > _xwin.ximage->width)
      sizex = _xwin.ximage->width;

   /* Need at least two pixels wide line for test.  */
   if (sizex < 2)
      return 0;

   ok = 1;
   for (x = 0; x < sizex; x++) {
      int bit;

      for (bit = -1; bit < depth; bit++) {
	 unsigned long color = ((bit < 0) ? 0 : ((unsigned long) 1 << bit));

	 /* Write color through XImage API.  */
	 XPutPixel(_xwin.ximage, x, 0, color);

	 /* Read color with direct access.  */
	 switch (test_depth) {
	    case 8:
	       if (p8[x] != color)
		  ok = 0;
	       break;
	    case 16:
	       if (p16[x] != color)
		  ok = 0;
	       break;
	    case 32:
	       if (p32[x] != color)
		  ok = 0;
	       break;
	    default:
	       ok = 0;
	       break;
	 }
	 XPutPixel(_xwin.ximage, x, 0, 0);

	 if (!ok)
	    return 0;
      }
   }

   return test_depth;
}



/*
 * Functions for copying screen data to XImage.
 */
#define MAKE_FAST_TRUECOLOR(name,stype,dtype,rshift,gshift,bshift,rmask,gmask,bmask)	\
static void name(int sx, int sy, int sw, int sh)					\
{											\
   int y, x;										\
   for (y = 0; y < sh; y++) {								\
      stype *s = (stype*) (_xwin.screen_line[sy + y]) + sx;				\
      dtype *d = (dtype*) (_xwin.ximage_line[sy + y]) + sx;				\
      for (x = sw - 1; x >= 0; x--) {							\
	 unsigned long color = *s++;							\
	 *d++ = (_xwin.rmap[(color >> (rshift)) & (rmask)]				\
		 | _xwin.gmap[(color >> (gshift)) & (gmask)]				\
		 | _xwin.bmap[(color >> (bshift)) & (bmask)]);				\
      }											\
   }											\
}

#define MAKE_FAST_TRUECOLOR24(name,dtype)						\
static void name(int sx, int sy, int sw, int sh)					\
{											\
   int x, y;										\
   for (y = 0; y < sh; y++) {								\
      unsigned char *s = _xwin.screen_line[sy + y] + 3 * sx;				\
      dtype *d = (dtype*) (_xwin.ximage_line[sy + y]) + sx;				\
      for (x = sw - 1; x >= 0; s += 3, x--) {						\
	 *d++ = (_xwin.rmap[s[0]] | _xwin.gmap[s[1]] | _xwin.bmap[s[2]]);		\
      }											\
   }											\
}

MAKE_FAST_TRUECOLOR(_xwin_private_fast_truecolor_8_to_8,
		    unsigned char, unsigned char, 0, 0, 0, 0xFF, 0xFF, 0xFF);
MAKE_FAST_TRUECOLOR(_xwin_private_fast_truecolor_8_to_16,
		    unsigned char, unsigned short, 0, 0, 0, 0xFF, 0xFF, 0xFF);
MAKE_FAST_TRUECOLOR(_xwin_private_fast_truecolor_8_to_32,
		    unsigned char, unsigned long, 0, 0, 0, 0xFF, 0xFF, 0xFF);

MAKE_FAST_TRUECOLOR(_xwin_private_fast_truecolor_15_to_8,
		    unsigned short, unsigned char, 0, 5, 10, 0x1F, 0x1F, 0x1F);
MAKE_FAST_TRUECOLOR(_xwin_private_fast_truecolor_15_to_16,
		    unsigned short, unsigned short, 0, 5, 10, 0x1F, 0x1F, 0x1F);
MAKE_FAST_TRUECOLOR(_xwin_private_fast_truecolor_15_to_32,
		    unsigned short, unsigned long, 0, 5, 10, 0x1F, 0x1F, 0x1F);

MAKE_FAST_TRUECOLOR(_xwin_private_fast_truecolor_16_to_8,
		    unsigned short, unsigned char, 0, 5, 11, 0x1F, 0x3F, 0x1F);
MAKE_FAST_TRUECOLOR(_xwin_private_fast_truecolor_16_to_16,
		    unsigned short, unsigned short, 0, 5, 11, 0x1F, 0x3F, 0x1F);
MAKE_FAST_TRUECOLOR(_xwin_private_fast_truecolor_16_to_32,
		    unsigned short, unsigned long, 0, 5, 11, 0x1F, 0x3F, 0x1F);

MAKE_FAST_TRUECOLOR(_xwin_private_fast_truecolor_32_to_8,
		    unsigned long, unsigned char, 0, 8, 16, 0xFF, 0xFF, 0xFF);
MAKE_FAST_TRUECOLOR(_xwin_private_fast_truecolor_32_to_16,
		    unsigned long, unsigned short, 0, 8, 16, 0xFF, 0xFF, 0xFF);
MAKE_FAST_TRUECOLOR(_xwin_private_fast_truecolor_32_to_32,
		    unsigned long, unsigned long, 0, 8, 16, 0xFF, 0xFF, 0xFF);

MAKE_FAST_TRUECOLOR24(_xwin_private_fast_truecolor_24_to_8,
		      unsigned char);
MAKE_FAST_TRUECOLOR24(_xwin_private_fast_truecolor_24_to_16,
		      unsigned short);
MAKE_FAST_TRUECOLOR24(_xwin_private_fast_truecolor_24_to_32,
		      unsigned long);

#define MAKE_FAST_PALETTE8(name,dtype)							\
static void name(int sx, int sy, int sw, int sh)					\
{											\
   int x, y;										\
   for (y = 0; y < sh; y++) {								\
      unsigned char *s = _xwin.screen_line[sy + y] + sx;				\
      dtype *d = (dtype*) (_xwin.ximage_line[sy + y]) + sx;				\
      for (x = sw - 1; x >= 0; x--) {							\
	 unsigned long color = *s++;							\
	 *d++ = _xwin.cmap[(_xwin.rmap[color]						\
			    | _xwin.gmap[color]						\
			    | _xwin.bmap[color])];					\
      }											\
   }											\
}

#define MAKE_FAST_PALETTE(name,stype,dtype,rshift,gshift,bshift)			\
static void name(int sx, int sy, int sw, int sh)					\
{											\
   int x, y;										\
   for (y = 0; y < sh; y++) {								\
      stype *s = (stype*) (_xwin.screen_line[sy + y]) + sx;				\
      dtype *d = (dtype*) (_xwin.ximage_line[sy + y]) + sx;				\
      for (x = sw - 1; x >= 0; x--) {							\
	 unsigned long color = *s++;							\
	 *d++ = _xwin.cmap[((((color >> (rshift)) & 0x0F) << 8)				\
			    | (((color >> (gshift)) & 0x0F) << 4)			\
			    | ((color >> (bshift)) & 0x0F))];				\
      }											\
   }											\
}

#define MAKE_FAST_PALETTE24(name,dtype)							\
static void name(int sx, int sy, int sw, int sh)					\
{											\
   int x, y;										\
   for (y = 0; y < sh; y++) {								\
      unsigned char *s = _xwin.screen_line[sy + y] + 3 * sx;				\
      dtype *d = (dtype*) (_xwin.ximage_line[sy + y]) + sx;				\
      for (x = sw - 1; x >= 0; s += 3, x--) {						\
	 *d++ = _xwin.cmap[((((unsigned long) s[0] << 4) & 0xF00)			\
			    | ((unsigned long) s[1] & 0xF0)				\
			    | (((unsigned long) s[2] >> 4) & 0x0F))];			\
      }											\
   }											\
}

MAKE_FAST_PALETTE8(_xwin_private_fast_palette_8_to_8,
		   unsigned char);
MAKE_FAST_PALETTE8(_xwin_private_fast_palette_8_to_16,
		   unsigned short);
MAKE_FAST_PALETTE8(_xwin_private_fast_palette_8_to_32,
		   unsigned long);

MAKE_FAST_PALETTE(_xwin_private_fast_palette_15_to_8,
		  unsigned short, unsigned char, 1, 6, 11);
MAKE_FAST_PALETTE(_xwin_private_fast_palette_15_to_16,
		  unsigned short, unsigned short, 1, 6, 11);
MAKE_FAST_PALETTE(_xwin_private_fast_palette_15_to_32,
		  unsigned short, unsigned long, 1, 6, 11);

MAKE_FAST_PALETTE(_xwin_private_fast_palette_16_to_8,
		  unsigned short, unsigned char, 1, 7, 12);
MAKE_FAST_PALETTE(_xwin_private_fast_palette_16_to_16,
		  unsigned short, unsigned short, 1, 7, 12);
MAKE_FAST_PALETTE(_xwin_private_fast_palette_16_to_32,
		  unsigned short, unsigned long, 1, 7, 12);

MAKE_FAST_PALETTE(_xwin_private_fast_palette_32_to_8,
		  unsigned long, unsigned char, 4, 12, 20);
MAKE_FAST_PALETTE(_xwin_private_fast_palette_32_to_16,
		  unsigned long, unsigned short, 4, 12, 20);
MAKE_FAST_PALETTE(_xwin_private_fast_palette_32_to_32,
		  unsigned long, unsigned long, 4, 12, 20);

MAKE_FAST_PALETTE24(_xwin_private_fast_palette_24_to_8,
		    unsigned char);
MAKE_FAST_PALETTE24(_xwin_private_fast_palette_24_to_16,
		    unsigned short);
MAKE_FAST_PALETTE24(_xwin_private_fast_palette_24_to_32,
		    unsigned long);

#define MAKE_SLOW_TRUECOLOR(name,stype,rshift,gshift,bshift,rmask,gmask,bmask)		\
static void name(int sx, int sy, int sw, int sh)					\
{											\
   int x, y;										\
   for (y = 0; y < sh; y++) {								\
      stype *s = (stype*) (_xwin.screen_line[sy + y]) + sx;				\
      for (x = 0; x < sw; x++) {							\
	 unsigned long color = *s++;							\
	 XPutPixel (_xwin.ximage, sx + x, sy + y,					\
		    (_xwin.rmap[(color >> (rshift)) & (rmask)]				\
		     | _xwin.gmap[(color >> (gshift)) & (gmask)]			\
		     | _xwin.bmap[(color >> (bshift)) & (bmask)]));			\
      }											\
   }											\
}

#define MAKE_SLOW_TRUECOLOR24(name)							\
static void name(int sx, int sy, int sw, int sh)					\
{											\
   int x, y;										\
   for (y = 0; y < sh; y++) {								\
      unsigned char *s = _xwin.screen_line[sy + y] + 3 * sx;				\
      for (x = 0; x < sw; s += 3, x++) {						\
	 XPutPixel(_xwin.ximage, sx + x, sy + y,					\
		   (_xwin.rmap[s[0]] | _xwin.gmap[s[1]] | _xwin.bmap[s[2]]));		\
      }											\
   }											\
}

MAKE_SLOW_TRUECOLOR(_xwin_private_slow_truecolor_8, unsigned char, 0, 0, 0, 0xFF, 0xFF, 0xFF);
MAKE_SLOW_TRUECOLOR(_xwin_private_slow_truecolor_15, unsigned short, 0, 5, 10, 0x1F, 0x1F, 0x1F);
MAKE_SLOW_TRUECOLOR(_xwin_private_slow_truecolor_16, unsigned short, 0, 5, 11, 0x1F, 0x3F, 0x1F);
MAKE_SLOW_TRUECOLOR(_xwin_private_slow_truecolor_32, unsigned long, 0, 8, 16, 0xFF, 0xFF, 0xFF);
MAKE_SLOW_TRUECOLOR24(_xwin_private_slow_truecolor_24);

#define MAKE_SLOW_PALETTE8(name)							\
static void name(int sx, int sy, int sw, int sh)					\
{											\
   int x, y;										\
   for (y = 0; y < sh; y++) {								\
      unsigned char *s = _xwin.screen_line[sy + y] + sx;				\
      for (x = 0; x < sw; x++) {							\
	 unsigned long color = *s++;							\
	 XPutPixel(_xwin.ximage, sx + x, sy + y,					\
		   _xwin.cmap[(_xwin.rmap[color]					\
			       | _xwin.gmap[color]					\
			       | _xwin.bmap[color])]);					\
      }											\
   }											\
}

#define MAKE_SLOW_PALETTE(name,stype,rshift,gshift,bshift)				\
static void name(int sx, int sy, int sw, int sh)					\
{											\
   int x, y;										\
   for (y = 0; y < sh; y++) {								\
      stype *s = (stype*) (_xwin.screen_line[sy + y]) + sx;				\
      for (x = 0; x < sw; x++) {							\
	 unsigned long color = *s++;							\
	 XPutPixel(_xwin.ximage, sx + x, sy + y,					\
		   _xwin.cmap[((((color >> (rshift)) & 0x0F) << 8)			\
			       | (((color >> (gshift)) & 0x0F) << 4)			\
			       | ((color >> (bshift)) & 0x0F))]);			\
      }											\
   }											\
}

#define MAKE_SLOW_PALETTE24(name)							\
static void name(int sx, int sy, int sw, int sh)					\
{											\
   int x, y;										\
   for (y = 0; y < sh; y++) {								\
      unsigned char *s = _xwin.screen_line[sy + y] + 3 * sx;				\
      for (x = 0; x < sw; s += 3, x++) {						\
	 XPutPixel(_xwin.ximage, sx + x, sy + y,					\
		   _xwin.cmap[((((unsigned long) s[0] << 4) & 0xF00)			\
			       | ((unsigned long) s[1] & 0xF0)				\
			       | (((unsigned long) s[2] >> 4) & 0x0F))]);		\
      }											\
   }											\
}

MAKE_SLOW_PALETTE8(_xwin_private_slow_palette_8);
MAKE_SLOW_PALETTE(_xwin_private_slow_palette_15, unsigned short, 1, 6, 11);
MAKE_SLOW_PALETTE(_xwin_private_slow_palette_16, unsigned short, 1, 7, 12);
MAKE_SLOW_PALETTE(_xwin_private_slow_palette_32, unsigned long, 4, 12, 20);
MAKE_SLOW_PALETTE24(_xwin_private_slow_palette_24);

/*
 * Functions for setting "hardware" color in 8bpp modes.
 */
static void _xwin_private_set_matching_color(int i, RGB *c)
{
   XColor color;
   color.flags = DoRed | DoGreen | DoBlue;
   color.pixel = i;
   color.red = ((c->r & 0x3F) * 65536) / 64;
   color.green = ((c->g & 0x3F) * 65536) / 64;
   color.blue = ((c->b & 0x3F) * 65536) / 64;
   XStoreColor(_xwin.display, _xwin.colormap, &color);
}

static void _xwin_private_set_truecolor_color(int i, RGB *c)
{
   _xwin.rmap[i] = (((c->r & 0x3F) * _xwin.rsize) / 64) << _xwin.rshift;
   _xwin.gmap[i] = (((c->g & 0x3F) * _xwin.gsize) / 64) << _xwin.gshift;
   _xwin.bmap[i] = (((c->b & 0x3F) * _xwin.bsize) / 64) << _xwin.bshift;
}

static void _xwin_private_set_palette_color(int i, RGB *c)
{
   _xwin.rmap[i] = (((c->r & 0x3F) * 16) / 64) << 8;
   _xwin.gmap[i] = (((c->g & 0x3F) * 16) / 64) << 4;
   _xwin.bmap[i] = (((c->b & 0x3F) * 16) / 64);
}

static void _xwin_private_set_palette_range(PALETTE p, int from, int to)
{
   if (_xwin.set_color != 0) {
      int i;

      /* Set colors.  */
      for (i = from; i <= to; i++)
	 (*(_xwin.set_color))(i, &p[i]);

      /* Update XImage and window.  */
      if (!_xwin.matching_formats)
	 _xwin_private_update_screen(0, 0, _xwin.virtual_width, _xwin.virtual_height);
   }
}

void _xwin_set_palette_range(PALETTE p, int from, int to, int vsync)
{
   /* TODO: wait for VBI.  */

   DISABLE();
   _xwin_private_set_palette_range(p, from, to);
   ENABLE();
}



/* _xwin_set_window_defaults:
 *  Set default window parameters.
 */
static void _xwin_private_set_window_defaults(void)
{
   XClassHint hint;
   XWMHints wm_hints;

   if (_xwin.window == None)
      return;

   /* Set window size and title.  */
   _xwin_private_resize_window(320, 200);
   XStoreName(_xwin.display, _xwin.window, _xwin.window_title);

   /* Set hints.  */
   hint.res_name = _xwin.application_name;
   hint.res_class = _xwin.application_class;
   XSetClassHint(_xwin.display, _xwin.window, &hint);

   wm_hints.flags = InputHint | StateHint;
   wm_hints.input = True;
   wm_hints.initial_state = NormalState;
   XSetWMHints(_xwin.display, _xwin.window, &wm_hints);
}



/* _xwin_sync:
 *  Wrapper for XSync.
 */
static void _xwin_private_sync(void)
{
   if (_xwin.display != 0)
      XSync(_xwin.display, False);
}

void _xwin_sync(void)
{
   DISABLE();
   _xwin_private_sync();
   ENABLE();
}



/* _xwin_resize_window:
 *  Wrapper for XResizeWindow.
 */
static void _xwin_private_resize_window(int w, int h)
{
   XSizeHints *hints;

   if (_xwin.window == None)
      return;

   /* Resize window.  */
   _xwin.window_width = w;
   _xwin.window_height = h;
   XResizeWindow(_xwin.display, _xwin.window, w, h);

   /* Set size hints for Window Manager.  */
   hints = XAllocSizeHints();
   if (hints == 0)
      return;

   hints->flags = USSize | PMinSize | PMaxSize | PBaseSize;
   hints->width  = hints->min_width  = hints->max_width  = hints->base_width  = w;
   hints->height = hints->min_height = hints->max_height = hints->base_height = h;
   XSetWMNormalHints(_xwin.display, _xwin.window, hints);
   XFree(hints);
}



/* _xwin_process_event:
 *  Process one event.
 */
static void _xwin_private_process_event(XEvent *event)
{
   int kcode, scode, dx, dy;
   static int mouse_buttons = 0;
   static int mouse_savedx = 0;
   static int mouse_savedy = 0;

   switch (event->type) {
      case KeyPress:
	 /* Key pressed.  */
	 kcode = event->xkey.keycode;
	 if ((kcode >= 0) && (kcode < 256)) {
	    scode = _xwin.keycode_to_scancode[kcode];
	    if ((scode > 0) && (_xwin_keyboard_interrupt != 0))
	       (*_xwin_keyboard_interrupt)(1, scode);
	 }
	 break;
      case KeyRelease:
	 /* Key release.  */
	 kcode = event->xkey.keycode;
	 if ((kcode >= 0) && (kcode < 256)) {
	    scode = _xwin.keycode_to_scancode[kcode];
	    if ((scode > 0) && (_xwin_keyboard_interrupt != 0))
	       (*_xwin_keyboard_interrupt)(0, scode);
	 }
	 break;
      case ButtonPress:
	 /* Mouse button pressed.  */
	 if (event->xbutton.button == Button1)
	    mouse_buttons |= 1;
	 else if (event->xbutton.button == Button3)
	    mouse_buttons |= 2;
	 else if (event->xbutton.button == Button2)
	    mouse_buttons |= 4;
	 if (_xwin_mouse_interrupt)
	    (*_xwin_mouse_interrupt)(0, 0, mouse_buttons);
	 break;
      case ButtonRelease:
	 /* Mouse button released.  */
	 if (event->xbutton.button == Button1)
	    mouse_buttons &= ~1;
	 else if (event->xbutton.button == Button3)
	    mouse_buttons &= ~2;
	 else if (event->xbutton.button == Button2)
	    mouse_buttons &= ~4;
	 if (_xwin_mouse_interrupt)
	    (*_xwin_mouse_interrupt)(0, 0, mouse_buttons);
	 break;
      case MotionNotify:
	 /* Mouse moved.  */
	 dx = event->xmotion.x - mouse_savedx;
	 dy = event->xmotion.y - mouse_savedy;
	 if ((dx != 0) || (dy != 0)) {
	    if (_xwin.mouse_warped) {
	       mouse_savedx = _xwin.window_width / 2;
	       mouse_savedy = _xwin.window_height / 2;
	       XWarpPointer(_xwin.display, None, _xwin.window,
			    0, 0, 0, 0, mouse_savedx, mouse_savedy);
	    }
	    else {
	       mouse_savedx = event->xmotion.x;
	       mouse_savedy = event->xmotion.y;
	    }
	    if (_xwin_mouse_interrupt)
	       (*_xwin_mouse_interrupt)(dx, dy, mouse_buttons);
	 }
	 break;
      case EnterNotify:
	 /* Mouse entered window.  */
	 if (_xwin.mouse_warped) {
	    mouse_savedx = _xwin.window_width / 2;
	    mouse_savedy = _xwin.window_height / 2;
	    XWarpPointer(_xwin.display, None, _xwin.window,
			 0, 0, 0, 0, mouse_savedx, mouse_savedy);
	 }
	 else {
	    mouse_savedx = event->xcrossing.x;
	    mouse_savedy = event->xcrossing.y;
	 }
	 break;
      case Expose:
	 /* Request to redraw part of the window.  */
	 _xwin_private_redraw_window(event->xexpose.x, event->xexpose.y,
				     event->xexpose.width, event->xexpose.height);
	 break;
      case MappingNotify:
	 /* Keyboard mapping changed.  */
	 if (event->xmapping.request == MappingKeyboard)
	    _xwin_private_init_keyboard_tables();
	 break;
   }
}



/* _xwin_handle_input:
 *  Handle events from the queue.
 */
static void _xwin_private_handle_input(void)
{
   int i, events;
   static XEvent event[5];

   if (_xwin.display == 0)
      return;

   /* How much events are available in the queue.  */
   events = XEventsQueued(_xwin.display, QueuedAlready);
   if (events <= 0)
      return;

   /* Limit amount of events we read at once.  */
   if (events > 5)
      events = 5;

   /* Read pending events.  */
   for (i = 0; i < events; i++)
      XNextEvent(_xwin.display, &event[i]);

   /* Process all events.  */
   for (i = 0; i < events; i++)
      _xwin_private_process_event(&event[i]);
}

void _xwin_handle_input(void)
{
   DISABLE();
   _xwin_private_sync();
   _xwin_private_handle_input();
   ENABLE();
}



/* _xwin_redraw_window:
 *  Redraws part of the window.
 */
static void _xwin_private_redraw_window(int x, int y, int w, int h)
{
   if (_xwin.window == None)
      return;

   /* Clip updated region.  */
   if (x >= _xwin.screen_width)
      return;
   if (x < 0) {
      w += x;
      x = 0;
   }
   if (w >= (_xwin.screen_width - x))
      w = _xwin.screen_width - x;
   if (w <= 0)
      return;

   if (y >= _xwin.screen_height)
      return;
   if (y < 0) {
      h += y;
      y = 0;
   }
   if (h >= (_xwin.screen_height - y))
      h = _xwin.screen_height - y;
   if (h <= 0)
      return;

   if (_xwin.ximage == 0)
      XFillRectangle(_xwin.display, _xwin.window, _xwin.gc, x, y, w, h);
   else {
#ifdef ALLEGRO_XWINDOWS_WITH_SHM
      if (_xwin.use_shm)
	 XShmPutImage(_xwin.display, _xwin.window, _xwin.gc, _xwin.ximage,
		      x + _xwin.scroll_x, y + _xwin.scroll_y, x, y, w, h, False);
      else
#endif
	 XPutImage(_xwin.display, _xwin.window, _xwin.gc, _xwin.ximage,
		   x + _xwin.scroll_x, y + _xwin.scroll_y, x, y, w, h);
   }
}

void _xwin_redraw_window(int x, int y, int w, int h)
{
   DISABLE();
   _xwin_private_redraw_window(x, y, w, h);
   ENABLE();
}



/* _xwin_scroll_screen:
 *  Scroll visible screen in window.
 */
int _xwin_scroll_screen(int x, int y)
{
   if (x < 0)
      x = 0;
   else if (x >= (_xwin.virtual_width - _xwin.screen_width))
      x = _xwin.virtual_width - _xwin.screen_width;
   if (y < 0)
      y = 0;
   else if (y >= (_xwin.virtual_height - _xwin.screen_height))
      y = _xwin.virtual_height - _xwin.screen_height;

   DISABLE();
   _xwin.scroll_x = x;
   _xwin.scroll_y = y;
   _xwin_private_redraw_window(0, 0, _xwin.screen_width, _xwin.screen_height);
   ENABLE();
   return 0;
}



/* _xwin_update_screen:
 *  Update part of the screen.
 */
static void _xwin_private_update_screen(int x, int y, int w, int h)
{
   /* Clip updated region.  */
   if (x >= _xwin.virtual_width)
      return;
   if (x < 0) {
      w += x;
      x = 0;
   }
   if (w >= (_xwin.virtual_width - x))
      w = _xwin.virtual_width - x;
   if (w <= 0)
      return;

   if (y >= _xwin.virtual_height)
      return;
   if (y < 0) {
      h += y;
      y = 0;
   }
   if (h >= (_xwin.virtual_height - y))
      h = _xwin.virtual_height - y;
   if (h <= 0)
      return;

   /* Update XImage with screen contents.  */
   if (_xwin.screen_to_ximage != 0)
      (*(_xwin.screen_to_ximage))(x, y, w, h);

   /* Handle scrolling.  */
   _xwin_private_redraw_window(x - _xwin.scroll_x, y - _xwin.scroll_y, w, h);
}

void _xwin_update_screen(int x, int y, int w, int h)
{
   DISABLE();
   _xwin_private_update_screen(x, y, w, h);
   ENABLE();
}



/* _xwin_read_line:
 *  Return linear offset for reading line.
 */
unsigned long _xwin_read_line(BITMAP *bmp, int line)
{
   return (unsigned long) (bmp->line[line]);
}



/* _xwin_write_line:
 *  Update last selected line and select new line.
 */
unsigned long _xwin_write_line(BITMAP *bmp, int line)
{
   int new_line = line + bmp->y_ofs;
   if ((new_line != _xwin_last_line) && (!_xwin_in_gfx_call) && (_xwin_last_line >= 0))
      _xwin_update_screen(0, _xwin_last_line, _xwin.virtual_width, 1);
   _xwin_last_line = new_line;
   return (unsigned long) (bmp->line[line]);
}



/* _xwin_unwrite_line:
 *  Update last selected line.
 */
void _xwin_unwrite_line(BITMAP *bmp)
{
   if ((!_xwin_in_gfx_call) && (_xwin_last_line >= 0))
      _xwin_update_screen(0, _xwin_last_line, _xwin.virtual_width, 1);
   _xwin_last_line = -1;
}



/* _xwin_set_window_title:
 *  Wrapper for XStoreName.
 */
static void _xwin_private_set_window_title(const char *name)
{
   if (!name)
      _xwin_safe_copy(_xwin.window_title, XWIN_DEFAULT_WINDOW_TITLE, sizeof(_xwin.window_title));
   else
      _xwin_safe_copy(_xwin.window_title, name, sizeof(_xwin.window_title));

   if (_xwin.window != None)
      XStoreName(_xwin.display, _xwin.window, _xwin.window_title);
}

void _xwin_set_window_title(const char *name)
{
   DISABLE();
   _xwin_private_set_window_title(name);
   ENABLE();
}



/* _xwin_change_keyboard_control:
 *  Wrapper for XChangeKeyboardControl.
 */
static void _xwin_private_change_keyboard_control(int led, int on)
{
   XKeyboardControl values;

   if (_xwin.display == 0)
      return;

   values.led = led;
   values.led_mode = (on ? LedModeOn : LedModeOff);

   XChangeKeyboardControl(_xwin.display, KBLed | KBLedMode, &values);
}

void _xwin_change_keyboard_control(int led, int on)
{
   DISABLE();
   _xwin_private_change_keyboard_control(led, on);
   ENABLE();
}



/* _xwin_get_pointer_mapping:
 *  Wrapper for XGetPointerMapping.
 */
static int _xwin_private_get_pointer_mapping(unsigned char map[], int nmap)
{
   return ((_xwin.display == 0) ? -1 : XGetPointerMapping(_xwin.display, map, nmap));
}

int _xwin_get_pointer_mapping(unsigned char map[], int nmap)
{
   int num;
   DISABLE();
   num = _xwin_private_get_pointer_mapping(map, nmap);
   ENABLE();
   return num;
}



/* Mappings between KeySym and Allegro scancodes.  */
static struct
{
   KeySym keysym;
   int scancode;
} _xwin_keysym_to_scancode[] =
{
   { XK_Escape, 0x01 },

   { XK_F1, 0x3B },
   { XK_F2, 0x3C },
   { XK_F3, 0x3D },
   { XK_F4, 0x3E },
   { XK_F5, 0x3F },
   { XK_F6, 0x40 },
   { XK_F7, 0x41 },
   { XK_F8, 0x42 },
   { XK_F9, 0x43 },
   { XK_F10, 0x44 },
   { XK_F11, 0x57 },
   { XK_F12, 0x58 },

   { XK_Print, 0x54 | 0x80 },
   { XK_Scroll_Lock, 0x46 },
   { XK_Pause, 0x00 | 0x100 },

   { XK_grave, 0x29 },
   { XK_quoteleft, 0x29 },
   { XK_asciitilde, 0x29 },
   { XK_1, 0x02 },
   { XK_2, 0x03 },
   { XK_3, 0x04 },
   { XK_4, 0x05 },
   { XK_5, 0x06 },
   { XK_6, 0x07 },
   { XK_7, 0x08 },
   { XK_8, 0x09 },
   { XK_9, 0x0A },
   { XK_0, 0x0B },
   { XK_minus, 0x0C },
   { XK_equal, 0x0D },
   { XK_backslash, 0x2B },
   { XK_BackSpace, 0x0E },

   { XK_Tab, 0x0F },
   { XK_q, 0x10 },
   { XK_w, 0x11 },
   { XK_e, 0x12 },
   { XK_r, 0x13 },
   { XK_t, 0x14 },
   { XK_y, 0x15 },
   { XK_u, 0x16 },
   { XK_i, 0x17 },
   { XK_o, 0x18 },
   { XK_p, 0x19 },
   { XK_bracketleft, 0x1A },
   { XK_bracketright, 0x1B },
   { XK_Return, 0x1C },

   { XK_Caps_Lock, 0x3A },
   { XK_a, 0x1E },
   { XK_s, 0x1F },
   { XK_d, 0x20 },
   { XK_f, 0x21 },
   { XK_g, 0x22 },
   { XK_h, 0x23 },
   { XK_j, 0x24 },
   { XK_k, 0x25 },
   { XK_l, 0x26 },
   { XK_semicolon, 0x27 },
   { XK_apostrophe, 0x28 },

   { XK_Shift_L, 0x2A },
   { XK_z, 0x2C },
   { XK_x, 0x2D },
   { XK_c, 0x2E },
   { XK_v, 0x2F },
   { XK_b, 0x30 },
   { XK_n, 0x31 },
   { XK_m, 0x32 },
   { XK_comma, 0x33 },
   { XK_period, 0x34 },
   { XK_slash, 0x35 },
   { XK_Shift_R, 0x36 },

   { XK_Control_L, 0x1D },
   { XK_Meta_L, 0x5B | 0x80 },
   { XK_Alt_L, 0x38 },
   { XK_space, 0x39 },
   { XK_Alt_R, 0x38 | 0x80 },
   { XK_Meta_R, 0x5C | 0x80 },
   { XK_Menu, 0x5D | 0x80 },
   { XK_Control_R, 0x1D | 0x80 },

   { XK_Insert, 0x52 | 0x80 },
   { XK_Home, 0x47 | 0x80 },
   { XK_Prior, 0x49 | 0x80 },
   { XK_Delete, 0x53 | 0x80 },
   { XK_End, 0x4F | 0x80 },
   { XK_Next, 0x51 | 0x80 },

   { XK_Up, 0x48 | 0x80 },
   { XK_Left, 0x4B | 0x80 },
   { XK_Down, 0x50 | 0x80 },
   { XK_Right, 0x4D | 0x80 },

   { XK_Num_Lock, 0x45 },
   { XK_KP_Divide, 0x35 | 0x80 },
   { XK_KP_Multiply, 0x37 },
   { XK_KP_Subtract, 0x4A | 0x80 },
   { XK_KP_Home, 0x47 },
   { XK_KP_Up, 0x48 },
   { XK_KP_Prior, 0x49 },
   { XK_KP_Add, 0x4E },
   { XK_KP_Left, 0x4B },
   { XK_KP_Begin, 0x4C },
   { XK_KP_Right, 0x4D },
   { XK_KP_End, 0x4F },
   { XK_KP_Down, 0x50 },
   { XK_KP_Next, 0x51 },
   { XK_KP_Enter, 0x1C | 0x80 },
   { XK_KP_Insert, 0x52 },
   { XK_KP_Delete, 0x53 },

   { NoSymbol, 0 },
};



/* _xwin_init_keyboard_tables:
 *  Initialize mapping between X-Windows keycodes and Allegro scancodes.
 */
static void _xwin_private_init_keyboard_tables(void)
{
   int i, j;
   int min_keycode;
   int max_keycode;
   KeySym keysym;

   if (_xwin.display == 0)
      return;

   /* Clear mappings.  */
   for (i = 0; i < 256; i++)
      _xwin.keycode_to_scancode[i] = -1;

   /* Get the number of keycodes.  */
   XDisplayKeycodes(_xwin.display, &min_keycode, &max_keycode);
   if (min_keycode < 0)
      min_keycode = 0;
   if (max_keycode > 255)
      max_keycode = 255;

   /* Setup mappings.  */
   for (i = min_keycode; i <= max_keycode; i++) {
      keysym = XKeycodeToKeysym(_xwin.display, i, 0);
      if (keysym != NoSymbol) {
	 for (j = 0; _xwin_keysym_to_scancode[j].keysym != NoSymbol; j++) {
	    if (_xwin_keysym_to_scancode[j].keysym == keysym) {
	       _xwin.keycode_to_scancode[i] = _xwin_keysym_to_scancode[j].scancode;
	       break;
	    }
	 }
      }
   }
}

void _xwin_init_keyboard_tables(void)
{
   DISABLE();
   _xwin_private_init_keyboard_tables();
   ENABLE();
}

