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
 *      DOS mouse routines (using the int 33 driver).
 *
 *      3-button support added by Fabian Nunez.
 *
 *      Mark Wodrich added double-buffered drawing of the mouse pointer and
 *      the set_mouse_sprite_focus() function.
 *
 *      See readme.txt for copyright information.
 */


#ifndef DJGPP
#error This file should only be used by the djgpp version of Allegro
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <go32.h>
#include <dpmi.h>

#include "internal.h"


volatile int mouse_x = 0;           /* mouse x pos */
volatile int mouse_y = 0;           /* mouse y pos */
volatile int mouse_b = 0;           /* mouse button state */
volatile int mouse_pos = 0;         /* unified position */

static int _mouse_x = 0;            /* internal position */
static int _mouse_y = 0;
static int _mouse_b = 0;

static int mickeymode = FALSE;      /* which input mode are we using? */
static int ntmode = FALSE;

static int mouse_mx = 0;            /* internal position, in mickeys */
static int mouse_my = 0;

static int mouse_sx = 2;            /* mickey -> pixel scaling factor */
static int mouse_sy = 2;

static int mouse_minx = 0;          /* mouse range */
static int mouse_miny = 0;
static int mouse_maxx = 319;
static int mouse_maxy = 199;

static int mymickey_x = 0;          /* for get_mouse_mickeys() */
static int mymickey_y = 0;
static int mymickey_ox = 0; 
static int mymickey_oy = 0;

int freeze_mouse_flag = FALSE;

void (*mouse_callback)(int flags) = NULL;

int _mouse_installed = FALSE;

int _mouse_x_focus = 1;             /* focus point in mouse sprite */
int _mouse_y_focus = 1;


#define MICKEY_TO_COORD_X(n)        ((n) / mouse_sx)
#define MICKEY_TO_COORD_Y(n)        ((n) / mouse_sy)

#define COORD_TO_MICKEY_X(n)        ((n) * mouse_sx)
#define COORD_TO_MICKEY_Y(n)        ((n) * mouse_sy)


#define CLEAR_MICKEYS()             \
   {                                \
      __dpmi_regs regs;             \
      regs.x.ax = 11;               \
      __dpmi_int(0x33, &regs);      \
      mymickey_ox = 0;              \
      mymickey_oy = 0;              \
   }


static unsigned char mouse_pointer_data[256] =
{
   1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   1, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   1, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   1, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   1, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   1, 2, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 
   1, 2, 2, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 
   1, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 
   1, 2, 2, 2, 2, 2, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 
   1, 2, 2, 1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
   1, 2, 1, 0, 1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 
   0, 1, 0, 0, 1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 
   0, 0, 0, 0, 0, 1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 
   0, 0, 0, 0, 0, 1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 
   0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0
};


BITMAP *_mouse_pointer = NULL;               /* default mouse pointer */

BITMAP *_mouse_sprite = NULL;                /* mouse pointer */

BITMAP *_mouse_screen = NULL;                /* where to draw the pointer */

static int got_hw_cursor = FALSE;            /* hardware pointer available? */
static int hw_cursor_dirty = FALSE;          /* need to set a new pointer? */

static int mx, my;                           /* previous mouse position */
static BITMAP *ms = NULL;                    /* previous screen data */
static BITMAP *mtemp = NULL;                 /* double-buffer drawing area */

#define SCARED_SIZE   16                     /* for unscare_mouse() */

static BITMAP *scared_screen[SCARED_SIZE]; 
static int scared_size = 0;

static int mouse_semaphore = FALSE;          /* reentrant interrupt? */

static _go32_dpmi_seginfo mouse_seginfo;
static _go32_dpmi_registers mouse_regs;



/* draw_mouse_doublebuffer:
 *  Eliminates mouse-cursor flicker by using an off-screen buffer for
 *  updating the cursor, and blitting only the final screen image.
 *  newx and newy contain the new cursor position, and mx and my are 
 *  assumed to contain previous cursor pos. This routine is called if 
 *  mouse cursor is to be erased and redrawn, and the two position overlap.
 */
static void draw_mouse_doublebuffer(int newx, int newy) 
{
   int x1, y1, w, h;

   /* grab bit of screen containing where we are and where we'll be */
   x1 = MIN(mx, newx) - _mouse_x_focus;
   y1 = MIN(my, newy) - _mouse_y_focus;

   /* get width of area */
   w = MAX(mx, newx) - x1 + _mouse_sprite->w+1;
   h = MAX(my, newy) - y1 + _mouse_sprite->h+1;

   /* make new co-ords relative to 'mtemp' bitmap co-ords */
   newx -= _mouse_x_focus+x1;
   newy -= _mouse_y_focus+y1;

   /* save screen image in 'mtemp' */
   blit(_mouse_screen, mtemp, x1, y1, 0, 0, w, h);

   /* blit saved image in 'ms' to corect place in this buffer */
   blit(ms, mtemp, 0, 0, mx-_mouse_x_focus-x1, my-_mouse_y_focus-y1,
			 _mouse_sprite->w, _mouse_sprite->h);

   /* draw mouse at correct place in 'mtemp' */
   blit(mtemp, ms, newx, newy, 0, 0, _mouse_sprite->w, _mouse_sprite->h);
   draw_sprite(mtemp, _mouse_sprite, newx, newy);

   /* blit 'mtemp' to screen */
   blit(mtemp, _mouse_screen, 0, 0, x1, y1, w, h);
}

static END_OF_FUNCTION(draw_mouse_doublebuffer);



/* draw_mouse:
 *  Mouse pointer drawing routine. If remove is set, deletes the old mouse
 *  pointer. If add is set, draws a new one.
 */
static void draw_mouse(int remove, int add)
{
   int normal_draw = (remove ^ add); 

   int newmx = mouse_x;
   int newmy = mouse_y;

   int cf = _mouse_screen->clip;
   int cl = _mouse_screen->cl;
   int cr = _mouse_screen->cr;
   int ct = _mouse_screen->ct;
   int cb = _mouse_screen->cb;

   _mouse_screen->clip = TRUE;
   _mouse_screen->cl = _mouse_screen->ct = 0;
   _mouse_screen->cr = _mouse_screen->w;
   _mouse_screen->cb = _mouse_screen->h;

   if (!normal_draw) {
      if ((newmx <= mx-_mouse_sprite->w) || (newmx >= mx+_mouse_sprite->w) ||
	  (newmy <= my-_mouse_sprite->h) || (newmy >= my+_mouse_sprite->h))
	 normal_draw = 1;
   }

   if (normal_draw) {
      if (remove)
	 blit(ms, _mouse_screen, 0, 0, mx-_mouse_x_focus, my-_mouse_y_focus, _mouse_sprite->w, _mouse_sprite->h);

      if (add) {
	 blit(_mouse_screen, ms, newmx-_mouse_x_focus, newmy-_mouse_y_focus, 0, 0, _mouse_sprite->w, _mouse_sprite->h);
	 draw_sprite(_mouse_screen, _mouse_sprite, newmx-_mouse_x_focus, newmy-_mouse_y_focus);
      }
   }
   else
      draw_mouse_doublebuffer(newmx, newmy);

   mx = newmx;
   my = newmy;

   _mouse_screen->clip = cf;
   _mouse_screen->cl = cl;
   _mouse_screen->cr = cr;
   _mouse_screen->ct = ct;
   _mouse_screen->cb = cb;
}

static END_OF_FUNCTION(draw_mouse);



/* update_mouse:
 *  Updates the mouse position variables with new values.
 */
static void update_mouse()
{
   int flags = 0;

   if ((mouse_x != _mouse_x) || 
       (mouse_y != _mouse_y) || 
       (mouse_b != _mouse_b)) {

      if (mouse_callback) {

	 if ((mouse_x != _mouse_x) || (mouse_y != _mouse_y))
	    flags |= MOUSE_FLAG_MOVE;

	 if ((_mouse_b & 1) && !(mouse_b & 1))
	    flags |= MOUSE_FLAG_LEFT_DOWN;
	 else if (!(_mouse_b & 1) && (mouse_b & 1))
	    flags |= MOUSE_FLAG_LEFT_UP;

	 if ((_mouse_b & 2) && !(mouse_b & 2))
	    flags |= MOUSE_FLAG_RIGHT_DOWN;
	 else if (!(_mouse_b & 2) && (mouse_b & 2))
	    flags |= MOUSE_FLAG_RIGHT_UP;

	 if ((_mouse_b & 4) && !(mouse_b & 4))
	    flags |= MOUSE_FLAG_MIDDLE_DOWN;
	 else if (!(_mouse_b & 4) && (mouse_b & 4))
	    flags |= MOUSE_FLAG_MIDDLE_UP;

	 mouse_x = _mouse_x;
	 mouse_y = _mouse_y;
	 mouse_b = _mouse_b;
	 mouse_pos = ((mouse_x & 0xFFFF) << 16) | (mouse_y & 0xFFFF);

	 mouse_callback(flags);
      }
      else {
	 mouse_x = _mouse_x;
	 mouse_y = _mouse_y;
	 mouse_b = _mouse_b;
	 mouse_pos = ((mouse_x & 0xFFFF) << 16) | (mouse_y & 0xFFFF);
      }
   }
}

static END_OF_FUNCTION(update_mouse);



/* mouse_move:
 *  Do we need to redraw the mouse pointer?
 */
static void mouse_move() 
{
   __dpmi_regs r;

   if (mouse_semaphore)
      return;

   mouse_semaphore = TRUE;

   if (ntmode) {
      r.x.ax = 3; 
      __dpmi_int(0x33, &r);

      _mouse_b = r.x.bx;
      _mouse_x = r.x.cx / 8;
      _mouse_y = r.x.dx / 8;
   }

   if (!freeze_mouse_flag) {
      update_mouse();

      if ((_mouse_screen) && ((mx != mouse_x) || (my != mouse_y))) {
	 if (gfx_capabilities & GFX_HW_CURSOR)
	    gfx_driver->move_mouse(mx=mouse_x, my=mouse_y);
	 else
	    draw_mouse(TRUE, TRUE);
      }
   }

   mouse_semaphore = FALSE;
}

static END_OF_FUNCTION(mouse_move);



/* mouseint:
 *  Mouse movement callback for the int 33 mouse driver.
 */
static void mouseint(_go32_dpmi_registers *r)
{
   int x, y;

   _mouse_b = r->x.bx;

   if (mickeymode) {
      x = (signed short)r->x.si;
      y = (signed short)r->x.di;

      mymickey_x += x - mymickey_ox;
      mymickey_y += y - mymickey_oy;

      mymickey_ox = x;
      mymickey_oy = y;

      _mouse_x = MICKEY_TO_COORD_X(mouse_mx + x);
      _mouse_y = MICKEY_TO_COORD_Y(mouse_my + y);

      if ((_mouse_x < mouse_minx) || (_mouse_x > mouse_maxx) ||
	  (_mouse_y < mouse_miny) || (_mouse_y > mouse_maxy)) {

	 _mouse_x = MID(mouse_minx, _mouse_x, mouse_maxx);
	 _mouse_y = MID(mouse_miny, _mouse_y, mouse_maxy);

	 mouse_mx = COORD_TO_MICKEY_X(_mouse_x);
	 mouse_my = COORD_TO_MICKEY_Y(_mouse_y);

	 CLEAR_MICKEYS();
      }
   }
   else {
      _mouse_x = r->x.cx / 8;
      _mouse_y = r->x.dx / 8;
   }

   if (!_mouse_screen)
      update_mouse();
}

static END_OF_FUNCTION(mouseint);



/* set_hw_cursor:
 *  Tries to download the current mouse sprite to the video driver, for
 *  use as a hardware cursor.
 */
static void set_hw_cursor()
{
   got_hw_cursor = FALSE;

   if ((gfx_driver) && (gfx_driver->set_mouse_sprite))
      if (gfx_driver->set_mouse_sprite(_mouse_sprite, _mouse_x_focus, _mouse_y_focus) == 0)
	 got_hw_cursor = TRUE;

   hw_cursor_dirty = FALSE;
}



/* set_mouse_sprite:
 *  Sets the sprite to be used for the mouse pointer. If the sprite is
 *  NULL, restores the default arrow.
 */
void set_mouse_sprite(struct BITMAP *sprite)
{
   BITMAP *old_mouse_screen = _mouse_screen;

   if (_mouse_screen)
      show_mouse(NULL);

   if (sprite)
      _mouse_sprite = sprite;
   else
      _mouse_sprite = _mouse_pointer;

   lock_bitmap(_mouse_sprite);

   /* make sure the ms bitmap is big enough */
   if ((!ms) || (ms->w < _mouse_sprite->w) || (ms->h < _mouse_sprite->h)) {
      if (ms) {
	 destroy_bitmap(ms);
	 destroy_bitmap(mtemp);
      }
      ms = create_bitmap(_mouse_sprite->w, _mouse_sprite->h);
      lock_bitmap(ms);

      mtemp = create_bitmap(_mouse_sprite->w*2, _mouse_sprite->h*2);
      lock_bitmap(mtemp);
   }

   _mouse_x_focus = _mouse_y_focus = 1;

   hw_cursor_dirty = TRUE;

   if (old_mouse_screen)
      show_mouse(old_mouse_screen);
}



/* set_mouse_sprite_focus:
 *  Sets co-ordinate (x, y) in the sprite to be the mouse location.
 *  Call after set_mouse_sprite(). Doesn't redraw the sprite.
 */
void set_mouse_sprite_focus(int x, int y) 
{
   _mouse_x_focus = x;
   _mouse_y_focus = y;

   hw_cursor_dirty = TRUE;
}



/* show_mouse:
 *  Tells Allegro to display a mouse pointer. This only works when the timer 
 *  module is active. The mouse pointer will be drawn onto the bitmap bmp, 
 *  which should normally be the hardware screen. To turn off the mouse 
 *  pointer, which you must do before you draw anything onto the screen, call 
 *  show_mouse(NULL). If you forget to turn off the mouse pointer when 
 *  drawing something, the SVGA bank switching code will become confused and 
 *  will produce garbage all over the screen.
 */
void show_mouse(BITMAP *bmp)
{
   remove_int(mouse_move);

   if (_mouse_screen) {
      if (gfx_capabilities & GFX_HW_CURSOR) {
	 gfx_driver->hide_mouse();
	 gfx_capabilities &= ~GFX_HW_CURSOR;
      }
      else
	 draw_mouse(TRUE, FALSE);
   }

   _mouse_screen = bmp;

   if (bmp) {
      if (hw_cursor_dirty)
	 set_hw_cursor(TRUE);

      if ((got_hw_cursor) && (bmp->vtable == &_screen_vtable))
	 if (gfx_driver->show_mouse(bmp, mx=mouse_x, my=mouse_y) == 0)
	    gfx_capabilities |= GFX_HW_CURSOR;

      if (!(gfx_capabilities & GFX_HW_CURSOR))
	 draw_mouse(FALSE, TRUE);

      if (_mouse_installed)
	 install_int(mouse_move, 20);
   }
   else {
      if (ntmode) 
	 install_int(mouse_move, 20);
      else
	 update_mouse();
   }
}



/* scare_mouse:
 *  Removes the mouse pointer prior to a drawing operation, if that is
 *  required (ie. noop if the mouse is on a memory bitmap, or a hardware
 *  cursor is in use). This operation can later be reversed by calling
 *  unscare_mouse().
 */
void scare_mouse()
{
   if ((is_same_bitmap(screen ,_mouse_screen)) &&
       (!(gfx_capabilities & GFX_HW_CURSOR))) {
      if (scared_size < SCARED_SIZE)
	 scared_screen[scared_size] = _mouse_screen;
      show_mouse(NULL);
   }
   else {
      if (scared_size < SCARED_SIZE)
	 scared_screen[scared_size] = NULL;
   }

   scared_size++;
}



/* unscare_mouse:
 *  Restores the original mouse state, after a call to scare_mouse().
 */
void unscare_mouse()
{
   if (scared_size > 0)
      scared_size--;

   if (scared_size < SCARED_SIZE) {
      if (scared_screen[scared_size])
	 show_mouse(scared_screen[scared_size]);

      scared_screen[scared_size] = NULL;
   }
}



/* position_mouse:
 *  Moves the mouse to screen position x, y. This is safe to call even
 *  when a mouse pointer is being displayed.
 */
void position_mouse(int x, int y)
{
   __dpmi_regs r;
   int nowhere = 0;
   BITMAP *old_mouse_screen = _mouse_screen;

   if (_mouse_screen)
      show_mouse(NULL);

   if (!_mouse_installed) {
      mouse_x = _mouse_x = x;
      mouse_y = _mouse_y = y;
   }
   else if (mickeymode) {
      DISABLE();

      mouse_x = _mouse_x = x;
      mouse_y = _mouse_y = y;

      mouse_mx = COORD_TO_MICKEY_X(x);
      mouse_my = COORD_TO_MICKEY_Y(y);

      CLEAR_MICKEYS();

      get_mouse_mickeys(&nowhere, &nowhere);

      ENABLE();
   }
   else {
      r.x.ax = 4;
      r.x.cx = x * 8;
      r.x.dx = y * 8;
      __dpmi_int(0x33, &r); 

      mouse_x = _mouse_x = x;
      mouse_y = _mouse_y = y;

      get_mouse_mickeys(&nowhere, &nowhere);
   }

   mouse_pos = ((mouse_x & 0xFFFF) << 16) | (mouse_y & 0xFFFF);

   if (old_mouse_screen)
      show_mouse(old_mouse_screen);
}



/* set_mouse_range:
 *  Sets the screen area within which the mouse can move. Pass the top left 
 *  corner and the bottom right corner (inclusive). If you don't call this 
 *  function the range defaults to (0, 0, SCREEN_W-1, SCREEN_H-1).
 */
void set_mouse_range(int x1, int y1, int x2, int y2)
{
   __dpmi_regs r;
   BITMAP *old_mouse_screen = _mouse_screen;

   if (!_mouse_installed)
      return;

   if (_mouse_screen)
      show_mouse(NULL);

   if (mickeymode) {
      mouse_minx = x1;
      mouse_miny = y1;
      mouse_maxx = x2;
      mouse_maxy = y2;

      DISABLE();

      mouse_x = _mouse_x = MID(mouse_minx, _mouse_x, mouse_maxx);
      mouse_y = _mouse_y = MID(mouse_miny, _mouse_y, mouse_maxy);

      mouse_mx = COORD_TO_MICKEY_X(_mouse_x);
      mouse_my = COORD_TO_MICKEY_Y(_mouse_y);

      CLEAR_MICKEYS();

      ENABLE();
   }
   else {
      r.x.ax = 7;
      r.x.cx = x1 * 8;
      r.x.dx = x2 * 8;
      __dpmi_int(0x33, &r);         /* set horizontal range */

      r.x.ax = 8;
      r.x.cx = y1 * 8;
      r.x.dx = y2 * 8;
      __dpmi_int(0x33, &r);         /* set vertical range */

      r.x.ax = 3; 
      __dpmi_int(0x33, &r);         /* check the position */
      mouse_x = _mouse_x = r.x.cx / 8;
      mouse_y = _mouse_y = r.x.dx / 8;
   }

   mouse_pos = ((mouse_x & 0xFFFF) << 16) | (mouse_y & 0xFFFF);

   if (old_mouse_screen)
      show_mouse(old_mouse_screen);
}



/* set_mouse_speed:
 *  Sets the mouse speed. Larger values of xspeed and yspeed represent 
 *  slower mouse movement: the default for both is 2.
 */
void set_mouse_speed(int xspeed, int yspeed)
{
   __dpmi_regs r;

   if (!_mouse_installed)
      return;

   if (mickeymode) {
      DISABLE();

      mouse_sx = MAX(1, xspeed);
      mouse_sy = MAX(1, yspeed);

      mouse_mx = COORD_TO_MICKEY_X(_mouse_x);
      mouse_my = COORD_TO_MICKEY_Y(_mouse_y);

      CLEAR_MICKEYS();

      ENABLE();
   }
   else {
      r.x.ax = 15;
      r.x.cx = xspeed;
      r.x.dx = yspeed;
      __dpmi_int(0x33, &r); 
   }
}



/* get_mouse_mickeys:
 *  Measures the mickey count (how far the mouse has moved since the last
 *  call to this function).
 */
void get_mouse_mickeys(int *mickeyx, int *mickeyy)
{
   __dpmi_regs r;

   if (!_mouse_installed) {
      *mickeyx = 0;
      *mickeyy = 0;
      return;
   }

   if (mickeymode) {
      int temp_x = mymickey_x;
      int temp_y = mymickey_y;

      mymickey_x -= temp_x;
      mymickey_y -= temp_y;

      *mickeyx = temp_x;
      *mickeyy = temp_y;
   }
   else {
      r.x.ax = 11;
      __dpmi_int(0x33, &r); 

      *mickeyx = (signed short)r.x.cx;
      *mickeyy = (signed short)r.x.dx;
   }
}



/* _set_mouse_range:
 *  The int33 driver tends to report mouse movements in chunks of 4 or 8
 *  pixels when in an svga mode. So, we increase the mouse range and
 *  sensitivity, and then divide all the values it returns by 8.
 */
void _set_mouse_range()
{
   int i;
   int col;

   if (!gfx_driver)
      return;

   if ((!_mouse_pointer) || 
       ((screen) && (_mouse_pointer) &&
	(bitmap_color_depth(_mouse_pointer) != bitmap_color_depth(screen)))) {

      if (_mouse_pointer) {
	 destroy_bitmap(_mouse_pointer);
	 _mouse_pointer = NULL;
      }

      if (ms) {
	 destroy_bitmap(ms);
	 ms = NULL;

	 destroy_bitmap(mtemp);
	 mtemp = NULL;
      }

      _mouse_pointer = create_bitmap(16, 16);

      for (i=0; i<256; i++) {
	 if (bitmap_color_depth(_mouse_pointer) == 8) {
	    switch (mouse_pointer_data[i]) {
	       case 1:  _mouse_pointer->line[i/16][i&15] = 16;  break;
	       case 2:  _mouse_pointer->line[i/16][i&15] = 255; break;
	       default: _mouse_pointer->line[i/16][i&15] = 0;   break;
	    }
	 }
	 else {
	    switch (mouse_pointer_data[i]) {
	       case 1:  col = makecol(255, 255, 255);             break;
	       case 2:  col = makecol(0, 0, 0);                   break;
	       default: col = _mouse_pointer->vtable->mask_color; break;
	    }
	    putpixel(_mouse_pointer, i&15, i/16, col);
	 } 
      }

      set_mouse_sprite(_mouse_pointer);
   }
   else
      hw_cursor_dirty = TRUE;

   set_mouse_range(0, 0, SCREEN_W-1, SCREEN_H-1);
   set_mouse_speed(2, 2);
   position_mouse(SCREEN_W/2, SCREEN_H/2);
}



/* install_mouse:
 *  Installs the Allegro mouse handler. You must do this before using any
 *  other mouse functions. Return -1 if it can't find a mouse driver,
 *  otherwise the number of buttons on the mouse.
 */
int install_mouse()
{
   __dpmi_regs r;
   int num_buttons;
   char *type;

   if (_mouse_installed)
      return -1;

   type = get_config_string(NULL, "mouse", "");

   if ((stricmp(type, "ms") == 0) || (stricmp(type, "microsoft") == 0)) {
      mickeymode = FALSE;
      ntmode = FALSE;
   }
   else if (stricmp(type, "logitech") == 0) {
      mickeymode = TRUE;
      ntmode = FALSE;
   }
   else if ((stricmp(type, "nt") == 0) || (os_type == OSTYPE_WINNT)) {
      mickeymode = FALSE;
      ntmode = TRUE;
   }
   else {
      mickeymode = TRUE;
      ntmode = FALSE;
   }

   r.x.ax = 0;
   __dpmi_int(0x33, &r);         /* initialise mouse driver */

   if (r.x.ax == 0) {
      mickeymode = FALSE;
      ntmode = FALSE;
      return -1;
   }

   num_buttons = r.x.bx;
   if (num_buttons == 0xFFFF)
     num_buttons = 2;

   LOCK_VARIABLE(mickeymode);
   LOCK_VARIABLE(ntmode);
   LOCK_VARIABLE(mouse_x);
   LOCK_VARIABLE(mouse_y);
   LOCK_VARIABLE(mouse_b);
   LOCK_VARIABLE(mouse_pos);
   LOCK_VARIABLE(_mouse_x);
   LOCK_VARIABLE(_mouse_y);
   LOCK_VARIABLE(_mouse_b);
   LOCK_VARIABLE(mouse_mx);
   LOCK_VARIABLE(mouse_my);
   LOCK_VARIABLE(mouse_sx);
   LOCK_VARIABLE(mouse_sy);
   LOCK_VARIABLE(mouse_minx);
   LOCK_VARIABLE(mouse_miny);
   LOCK_VARIABLE(mouse_maxx);
   LOCK_VARIABLE(mouse_maxy);
   LOCK_VARIABLE(mymickey_x);
   LOCK_VARIABLE(mymickey_y);
   LOCK_VARIABLE(mymickey_ox);
   LOCK_VARIABLE(mymickey_oy);
   LOCK_VARIABLE(freeze_mouse_flag);
   LOCK_VARIABLE(mouse_callback);
   LOCK_VARIABLE(_mouse_x_focus);
   LOCK_VARIABLE(_mouse_y_focus);
   LOCK_VARIABLE(_mouse_sprite);
   LOCK_VARIABLE(mouse_pointer_data);
   LOCK_VARIABLE(_mouse_pointer);
   LOCK_VARIABLE(_mouse_screen);
   LOCK_VARIABLE(mx);
   LOCK_VARIABLE(my);
   LOCK_VARIABLE(ms);
   LOCK_VARIABLE(mtemp);
   LOCK_VARIABLE(mouse_semaphore);
   LOCK_VARIABLE(mouse_seginfo);
   LOCK_VARIABLE(mouse_regs);
   LOCK_FUNCTION(draw_mouse_doublebuffer);
   LOCK_FUNCTION(draw_mouse);
   LOCK_FUNCTION(update_mouse);
   LOCK_FUNCTION(mouse_move);
   LOCK_FUNCTION(mouseint);

   /* create real mode callback for the int33 driver */
   if (!ntmode) {
      mouse_seginfo.pm_offset = (int)mouseint;
      mouse_seginfo.pm_selector = _my_cs();
      _go32_dpmi_allocate_real_mode_callback_retf(&mouse_seginfo, &mouse_regs);

      r.x.ax = 0x0C;
      r.x.cx = 0x7F;
      r.x.dx = mouse_seginfo.rm_offset;
      r.x.es = mouse_seginfo.rm_segment;
      __dpmi_int(0x33, &r);            /* install callback */
   }

   _mouse_installed = TRUE;
   _set_mouse_range();
   _add_exit_func(remove_mouse);

   if (ntmode)
      install_int(mouse_move, 20);

   return num_buttons;
}



/* remove_mouse:
 *  Removes the mouse handler. You don't normally need to call this, because
 *  allegro_exit() will do it for you.
 */
void remove_mouse()
{
   __dpmi_regs r;

   if (!_mouse_installed)
      return;

   show_mouse(NULL);

   if (ntmode)
      remove_int(mouse_move);

   r.x.ax = 0x0C;
   r.x.cx = 0;
   r.x.dx = 0;
   r.x.es = 0;
   __dpmi_int(0x33, &r);         /* install NULL callback */

   if (!mickeymode) {
      r.x.ax = 15;
      r.x.cx = 8;
      r.x.dx = 16;
      __dpmi_int(0x33, &r);      /* reset sensitivity */
   }

   mouse_x = mouse_y = _mouse_x = _mouse_y = 0;
   mouse_b = _mouse_b = 0;
   mouse_pos = 0;
   _mouse_screen = NULL;

   if (!ntmode)
      _go32_dpmi_free_real_mode_callback(&mouse_seginfo);

   if (_mouse_pointer) {
      destroy_bitmap(_mouse_pointer);
      _mouse_pointer = NULL;
   }

   if (ms) {
      destroy_bitmap(ms);
      ms = NULL;

      destroy_bitmap(mtemp);
      mtemp = NULL;
   }

   _remove_exit_func(remove_mouse);
   _mouse_installed = FALSE;
}

