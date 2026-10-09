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
 *      X-Windows mouse module.
 *
 *      By Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/aintern.h"
#include "allegro/aintunix.h"
#include "xwin.h"



static int mouse_mx = 0;
static int mouse_my = 0;

static int mouse_sx = 2;
static int mouse_sy = 2;

static int mouse_minx = 0;
static int mouse_miny = 0;
static int mouse_maxx = 319;
static int mouse_maxy = 199;

static int mymickey_x = 0;
static int mymickey_y = 0;


#define MICKEY_TO_COORD_X(n)        ((n) / mouse_sx)
#define MICKEY_TO_COORD_Y(n)        ((n) / mouse_sy)

#define COORD_TO_MICKEY_X(n)        ((n) * mouse_sx)
#define COORD_TO_MICKEY_Y(n)        ((n) * mouse_sy)


#define CLEAR_MICKEYS()



static int _xwin_mousedrv_init(void);
static void _xwin_mousedrv_exit(void);
static void _xwin_mousedrv_position(int x, int y);
static void _xwin_mousedrv_set_range(int x1, int y1, int x2, int y2);
static void _xwin_mousedrv_set_speed(int xspeed, int yspeed);
static void _xwin_mousedrv_get_mickeys(int *mickeyx, int *mickeyy);


MOUSE_DRIVER mousedrv_xwin =
{
   MOUSEDRV_XWINDOWS,
   empty_string,
   empty_string,
   "X-Windows mouse",
   _xwin_mousedrv_init,
   _xwin_mousedrv_exit,
   NULL,
   NULL,
   _xwin_mousedrv_position,
   _xwin_mousedrv_set_range,
   _xwin_mousedrv_set_speed,
   _xwin_mousedrv_get_mickeys,
   NULL
};



/* list the available drivers */
_DRIVER_INFO _xwin_mouse_driver_list[] =
{
   {  MOUSEDRV_XWINDOWS, &mousedrv_xwin, TRUE  },
   {  0,                 NULL,           0     }
};



/* _xwin_mousedrv_handler:
 *  Mouse "interrupt" handler for mickey-mode driver.
 */
static void _xwin_mousedrv_handler(int x, int y, int buttons)
{
   _mouse_b = buttons;

   mymickey_x += x;
   mymickey_y += y;

   mouse_mx += x;
   mouse_my += y;

   _mouse_x = MICKEY_TO_COORD_X(mouse_mx);
   _mouse_y = MICKEY_TO_COORD_Y(mouse_my);

   if ((_mouse_x < mouse_minx) || (_mouse_x > mouse_maxx)
       || (_mouse_y < mouse_miny) || (_mouse_y > mouse_maxy)) {
      _mouse_x = MID(mouse_minx, _mouse_x, mouse_maxx);
      _mouse_y = MID(mouse_miny, _mouse_y, mouse_maxy);

      mouse_mx = COORD_TO_MICKEY_X(_mouse_x);
      mouse_my = COORD_TO_MICKEY_Y(_mouse_y);

      CLEAR_MICKEYS();
   }

   _handle_mouse_input();
}



/* _xwin_mousedrv_init:
 *  Initializes the mickey-mode driver.
 */
static int _xwin_mousedrv_init(void)
{
   int num_buttons;
   unsigned char map[8];

   num_buttons = _xwin_get_pointer_mapping(map, sizeof(map));
   num_buttons = MID(2, num_buttons, 3);

   DISABLE();

   _xwin_mouse_interrupt = _xwin_mousedrv_handler;

   ENABLE();

   return num_buttons;
}



/* _xwin_mousedrv_exit:
 *  Shuts down the mickey-mode driver.
 */
static void _xwin_mousedrv_exit(void)
{
   DISABLE();

   _xwin_mouse_interrupt = 0;

   ENABLE();
}



/* _xwin_mousedrv_position:
 *  Sets the position of the mickey-mode mouse.
 */
static void _xwin_mousedrv_position(int x, int y)
{
   DISABLE();

   _mouse_x = x;
   _mouse_y = y;

   mouse_mx = COORD_TO_MICKEY_X(x);
   mouse_my = COORD_TO_MICKEY_Y(y);

   CLEAR_MICKEYS();

   mymickey_x = mymickey_y = 0;

   ENABLE();
}



/* _xwin_mousedrv_set_range:
 *  Sets the range of the mickey-mode mouse.
 */
static void _xwin_mousedrv_set_range(int x1, int y1, int x2, int y2)
{
   mouse_minx = x1;
   mouse_miny = y1;
   mouse_maxx = x2;
   mouse_maxy = y2;

   DISABLE();

   _mouse_x = MID(mouse_minx, _mouse_x, mouse_maxx);
   _mouse_y = MID(mouse_miny, _mouse_y, mouse_maxy);

   mouse_mx = COORD_TO_MICKEY_X(_mouse_x);
   mouse_my = COORD_TO_MICKEY_Y(_mouse_y);

   CLEAR_MICKEYS();

   ENABLE();
}



/* _xwin_mousedrv_set_speed:
 *  Sets the speed of the mickey-mode mouse.
 */
static void _xwin_mousedrv_set_speed(int xspeed, int yspeed)
{
   DISABLE();

   mouse_sx = MAX(1, xspeed);
   mouse_sy = MAX(1, yspeed);

   mouse_mx = COORD_TO_MICKEY_X(_mouse_x);
   mouse_my = COORD_TO_MICKEY_Y(_mouse_y);

   CLEAR_MICKEYS();

   ENABLE();
}



/* _xwin_mousedrv_get_mickeys:
 *  Reads the mickey-mode count.
 */
static void _xwin_mousedrv_get_mickeys(int *mickeyx, int *mickeyy)
{
   int temp_x = mymickey_x;
   int temp_y = mymickey_y;

   mymickey_x -= temp_x;
   mymickey_y -= temp_y;

   *mickeyx = temp_x;
   *mickeyy = temp_y;
}

