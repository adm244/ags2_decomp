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
 *      DOS joystick routines.
 *
 *      Based on code provided by Jonathan Tarbox and Marcel de Kogel.
 *
 *      CH Flightstick Pro and Logitech Wingman Extreme
 *      support by Fabian Nunez.
 *
 *      Matthew Bowie added support for 4-button joysticks.
 *
 *      Richard Mitton added support for 6-button joysticks.
 *
 *      Stefan Eilert added support for dual joysticks.
 *
 *      See readme.txt for copyright information.
 */


#ifndef DJGPP
#error This file should only be used by the djgpp version of Allegro
#endif

#include <stdlib.h>
#include <stdio.h>
#include <dos.h>
#include <errno.h>

#include "internal.h"



/* driver functions */
static int joy_init(); 
static void joy_exit(); 
static int joy_poll();
static int joy_save_data();
static int joy_load_data();
static char *joy_calibrate_name(int n);
static int joy_calibrate(int n);


/* the driver structure (shared by all analogue style stick variants) */
JOYSTICK_DRIVER joystick_standard =
{
   JOY_TYPE_AUTODETECT,
   "",
   "",
   joy_init,
   joy_exit,
   joy_poll,
   joy_save_data,
   joy_load_data,
   joy_calibrate_name,
   joy_calibrate
};


/* flags describing different variants of the basic joystick */
#define JDESC_STANDARD           0x0000
#define JDESC_STICK2             0x0001
#define JDESC_4BUTTON            0x0002
#define JDESC_6BUTTON            0x0004
#define JDESC_8BUTTON            0x0008
#define JDESC_Y2_THROTTLE        0x0010
#define JDESC_Y2_HAT             0x0020
#define JDESC_FSPRO_HAT          0x0040


typedef struct JOYSTICK_VARIANT
{
   int id;
   char *name;
   int flags;
} JOYSTICK_VARIANT;


static char variant_name[][20] =
{
   "Standard joystick",
   "Dual joysticks",
   "4-button joystick",
   "6-button joystick",
   "8-button joystick",
   "Flightstick Pro",
   "Wingman Extreme"
};


static JOYSTICK_VARIANT joystick_variant[] =
{
   {  JOY_TYPE_STANDARD,   variant_name[0],  JDESC_STANDARD },
   {  JOY_TYPE_2PADS,      variant_name[1],  JDESC_STICK2 },
   {  JOY_TYPE_4BUTTON,    variant_name[2],  JDESC_4BUTTON },
   {  JOY_TYPE_6BUTTON,    variant_name[3],  JDESC_4BUTTON | JDESC_6BUTTON },
   {  JOY_TYPE_8BUTTON,    variant_name[4],  JDESC_4BUTTON | JDESC_8BUTTON },
   {  JOY_TYPE_FSPRO,      variant_name[5],  JDESC_4BUTTON | JDESC_Y2_THROTTLE | JDESC_FSPRO_HAT },
   {  JOY_TYPE_WINGEX,     variant_name[6],  JDESC_4BUTTON | JDESC_Y2_HAT },
   {  JOY_TYPE_NONE,       NULL,             0 }
};


/* calibration state information */
#define JOYSTICK_CALIB_TL1             0x00010000
#define JOYSTICK_CALIB_BR1             0x00020000
#define JOYSTICK_CALIB_TL2             0x00040000
#define JOYSTICK_CALIB_BR2             0x00080000
#define JOYSTICK_CALIB_THRTL_MIN       0x00100000
#define JOYSTICK_CALIB_THRTL_MAX       0x00200000
#define JOYSTICK_CALIB_HAT_CENTRE      0x00400000
#define JOYSTICK_CALIB_HAT_LEFT        0x00800000
#define JOYSTICK_CALIB_HAT_DOWN        0x01000000
#define JOYSTICK_CALIB_HAT_RIGHT       0x02000000
#define JOYSTICK_CALIB_HAT_UP          0x04000000

#define JOYSTICK_CALIB_HAT    (JOYSTICK_CALIB_HAT_CENTRE |     \
			       JOYSTICK_CALIB_HAT_LEFT |       \
			       JOYSTICK_CALIB_HAT_DOWN |       \
			       JOYSTICK_CALIB_HAT_RIGHT |      \
			       JOYSTICK_CALIB_HAT_UP)


/* driver state information */
static int joystick_flags = 0;

static int joycentre_x, joycentre_y;
static int joyx_min, joyx_low_margin, joyx_high_margin, joyx_max;
static int joyy_min, joyy_low_margin, joyy_high_margin, joyy_max;

static int joycentre2_x, joycentre2_y;
static int joyx2_min, joyx2_low_margin, joyx2_high_margin, joyx2_max;
static int joyy2_min, joyy2_low_margin, joyy2_high_margin, joyy2_max;

static int joy_thr_min, joy_thr_max;

static int joy_hat_pos[5], joy_hat_threshold[4];

static int joy_old_x, joy_old_y;
static int joy2_old_x, joy2_old_y;


/* masks indicating which axis to read */
#define MASK_1X      1
#define MASK_1Y      2
#define MASK_2X      4
#define MASK_2Y      8



/* poll_mask:
 *  Returns a mask indicating which axes to poll.
 */
static int poll_mask()
{
   int mask = MASK_1X | MASK_1Y;

   if (joystick_flags & (JDESC_STICK2 | JDESC_6BUTTON | JDESC_8BUTTON))
      mask |= (MASK_2X | MASK_2Y);

   if (joystick_flags & (JDESC_Y2_THROTTLE | JDESC_Y2_HAT))
      mask |= MASK_2Y;

   return mask;
}



/* poll:
 *  Polls the joystick to read the axis position. Returns raw position
 *  values, zero for success, non-zero if no joystick found.
 */
static int poll(int *x, int *y, int *x2, int *y2, int poll_mask)
{
   int p1, p2, p3, p4;
   int ret;

   enter_critical();

   asm (
      "  movl $0, %0 ; "                  /* init counters to zero */
      "  movl $0, %1 ; "
      "  movl $0, %2 ; "
      "  movl $0, %3 ; "

      "  outb %%al, %%dx ; "              /* prime the joystick hardware */
      "  jmp 0f ; "

      "  .align 4 ; "                     /* poll loop */
      " 0: "
      "  inb %%dx, %%al ; "               /* read joystick port */
      "  movl %%eax, %%ebx ; "

      "  shrl $1, %%ebx ; "               /* test x axis bit */
      "  adcl $0, %0 ; "
      "  shrl $1, %%ebx ; "               /* test y axis bit */
      "  adcl $0, %1 ; "
      "  shrl $1, %%ebx ; "               /* test stick 2 x axis bit */
      "  adcl $0, %2 ; "
      "  shrl $1, %%ebx ; "               /* test stick 2 y axis bit */
      "  adcl $0, %3 ; "

      "  testb %7, %%al ; "               /* repeat? */
      "  loopnz 0b ; "

      "  cmpl $100000, %0 ; "             /* check for timeout */
      "  jge 1f ; "

      "  cmpl $100000, %1 ; "
      "  jge 1f ; "

      "  testb $4, %7 ; "                 /* poll joystick 2 x axis? */
      "  jz 2f ; "

      "  cmpl $100000, %2 ; "             /* check for timeout */
      "  jge 1f ; "

      " 2: "
      "  testb $8, %7 ; "                 /* poll joystick 2 y axis? */
      "  jz 3f ; "

      "  cmpl $100000, %3 ; "
      "  jge 1f ; "

      " 3: "                              /* return 0 on success */
      "  movl $0, %4 ; "
      "  jmp 4f ; "

      " 1: "                              /* return -1 on error */
      "  movl $-1, %4 ; "

      " 4: "

   : "=m" (p1),
     "=m" (p2),
     "=m" (p3),
     "=m" (p4),
     "=a" (ret)

   : "d" (0x201),
     "c" (100000),
     "m" (poll_mask)

   : "%ebx"
   );

   exit_critical();

   *x = p1;
   *y = p2;
   *x2 = p3;
   *y2 = p4;

   return ret;
}



/* averaged_poll:
 *  For calibration it is crucial that we get the right results, so we
 *  average out several attempts.
 */
static int averaged_poll(int *x, int *y, int *x2, int *y2, int mask)
{
   int x_tmp, y_tmp, x2_tmp, y2_tmp;
   int x_total, y_total, x2_total, y2_total;
   int c;

   #define AVERAGE_COUNT   4

   x_total = y_total = x2_total = y2_total = 0;

   for (c=0; c<AVERAGE_COUNT; c++) {
      if (poll(&x_tmp, &y_tmp, &x2_tmp, &y2_tmp, mask) != 0)
	 return -1;

      x_total += x_tmp;
      y_total += y_tmp;
      x2_total += x2_tmp;
      y2_total += y2_tmp;
   }

   *x = x_total / AVERAGE_COUNT;
   *y = y_total / AVERAGE_COUNT;
   *x2 = x2_total / AVERAGE_COUNT;
   *y2 = y2_total / AVERAGE_COUNT;

   return 0;
}



/* recalc_calibration_flags:
 *  Called after each calibration operation, to calculate what else might
 *  need to be measured for the current hardware.
 */
static void recalc_calibration_flags()
{
   #define FLAG_SET(n)  ((joystick_flags & (n)) == (n))

   /* stick 1 analogue input? */
   if (FLAG_SET(JOYSTICK_CALIB_TL1 | JOYSTICK_CALIB_BR1)) {
      joy[0].stick[0].flags |= JOYFLAG_ANALOGUE;
      joy[0].stick[0].flags &= ~JOYFLAG_CALIB_ANALOGUE;
   }
   else {
      joy[0].stick[0].flags &= ~JOYFLAG_ANALOGUE;
      joy[0].stick[0].flags |= JOYFLAG_CALIB_ANALOGUE;
   }

   /* stick 2 analogue input? */
   if (joystick_flags & JDESC_STICK2) {
      if (FLAG_SET(JOYSTICK_CALIB_TL2 | JOYSTICK_CALIB_BR2)) {
	 joy[1].stick[0].flags |= JOYFLAG_ANALOGUE;
	 joy[1].stick[0].flags &= ~JOYFLAG_CALIB_ANALOGUE;
      }
      else {
	 joy[1].stick[0].flags &= ~JOYFLAG_ANALOGUE;
	 joy[1].stick[0].flags |= JOYFLAG_CALIB_ANALOGUE;
      }
   }

   /* Wingman Extreme hat input? */
   if (joystick_flags & JDESC_Y2_HAT) {
      if (FLAG_SET(JOYSTICK_CALIB_HAT)) {
	 joy[0].stick[1].flags |= JOYFLAG_DIGITAL;
	 joy[0].stick[1].flags &= ~JOYFLAG_CALIB_DIGITAL;
      }
      else {
	 joy[0].stick[1].flags &= ~JOYFLAG_DIGITAL;
	 joy[0].stick[1].flags |= JOYFLAG_CALIB_DIGITAL;
      }
   }

   /* FSPro throttle input? */
   if (joystick_flags & JDESC_Y2_THROTTLE) {
      if (FLAG_SET(JOYSTICK_CALIB_THRTL_MIN | JOYSTICK_CALIB_THRTL_MAX)) {
	 joy[0].stick[2].flags |= JOYFLAG_ANALOGUE;
	 joy[0].stick[2].flags &= ~JOYFLAG_CALIB_ANALOGUE;
      }
      else {
	 joy[0].stick[2].flags &= ~JOYFLAG_ANALOGUE;
	 joy[0].stick[2].flags |= JOYFLAG_CALIB_ANALOGUE;
      }
   }
}



/* joy_init:
 *  Initialises the driver.
 */
static int joy_init()
{
   static char name_x[] = "X";
   static char name_y[] = "Y";
   static char *name_b[] = { "B1", "B2", "B3", "B4", "B5", "B6", "B7", "B8" };
   char *name_stick = get_config_text("Stick");
   char *name_hat = get_config_text("Hat");
   char *name_throttle = get_config_text("Throttle");
   int i;

   for (i=0; joystick_variant[i].id != JOY_TYPE_NONE; i++)
      joystick_variant[i].name = get_config_text(variant_name[i]);

   /* find the description and flag values for this type of stick */
   for (i=0; joystick_variant[i].id != JOY_TYPE_NONE; i++)
      if (joystick_variant[i].id == joy_type)
	 break;

   if (joystick_variant[i].id == JOY_TYPE_NONE)
      return -1;

   /* store info about the stick type */
   joystick_standard.id = joystick_variant[i].id;

   joystick_standard.name = joystick_variant[i].name;
   joystick_standard.desc = joystick_variant[i].name;

   joystick_flags = joystick_variant[i].flags;

   joy_old_x = joy_old_y = 0;
   joy2_old_x = joy2_old_y = 0;

   /* make sure the hardware is really present */
   if (averaged_poll(&joycentre_x, &joycentre_y, &joycentre2_x, &joycentre2_y, poll_mask()) != 0)
      return -1;

   /* fill in the joystick structure */
   num_joysticks = (joystick_flags & JDESC_STICK2) ? 2 : 1;

   joy[0].flags = JOYFLAG_DIGITAL;

   /* how many buttons? */
   if (joystick_flags & JDESC_8BUTTON)
      joy[0].num_buttons = 8;
   else if (joystick_flags & JDESC_6BUTTON)
      joy[0].num_buttons = 6;
   else if (joystick_flags & JDESC_4BUTTON)
      joy[0].num_buttons = 4;
   else
      joy[0].num_buttons = 2;

   /* main analogue stick */
   joy[0].stick[0].flags = JOYFLAG_DIGITAL | JOYFLAG_SIGNED;
   joy[0].stick[0].num_axis = 2;
   joy[0].stick[0].axis[0].name = name_x;
   joy[0].stick[0].axis[1].name = name_y;
   joy[0].stick[0].name = name_stick;
   joy[0].num_sticks = 1;

   /* hat control? */
   if (joystick_flags & (JDESC_FSPRO_HAT | JDESC_Y2_HAT)) {
      joy[0].stick[joy[0].num_sticks].flags = JOYFLAG_DIGITAL | JOYFLAG_SIGNED;
      joy[0].stick[joy[0].num_sticks].num_axis = 2;
      joy[0].stick[joy[0].num_sticks].axis[0].name = name_x;
      joy[0].stick[joy[0].num_sticks].axis[1].name = name_y;
      joy[0].stick[joy[0].num_sticks].name = name_hat;
      joy[0].num_sticks++;
   }

   /* throttle control? */
   if (joystick_flags & JDESC_Y2_THROTTLE) {
      joy[0].stick[joy[0].num_sticks].flags = JOYFLAG_UNSIGNED;
      joy[0].stick[joy[0].num_sticks].num_axis = 1;
      joy[0].stick[joy[0].num_sticks].axis[0].name = name_throttle;
      joy[0].stick[joy[0].num_sticks].name = name_throttle;
      joy[0].num_sticks++;
   }

   /* clone everything for a second joystick? */
   if (joystick_flags & JDESC_STICK2)
      joy[1] = joy[0];

   /* fill in the button names */
   for (i=0; i<joy[0].num_buttons; i++)
      joy[0].button[i].name = name_b[i];

   for (i=0; i<joy[1].num_buttons; i++)
      joy[1].button[i].name = name_b[i];

   recalc_calibration_flags();

   return 0;
}



/* joy_exit:
 *  Shuts down the driver.
 */
static void joy_exit()
{
   joystick_flags = 0;
}



/* sort_out_middle_values:
 *  Sets up the values used by sort_out_analogue() to create a 'dead'
 *  region in the centre of the joystick range.
 */
static void sort_out_middle_values(int n)
{
   if (n == 0) {
      joyx_low_margin  = joycentre_x - (joycentre_x - joyx_min) / 8;
      joyx_high_margin = joycentre_x + (joyx_max - joycentre_x) / 8;
      joyy_low_margin  = joycentre_y - (joycentre_y - joyy_min) / 8;
      joyy_high_margin = joycentre_y + (joyy_max - joycentre_y) / 8;
   }
   else {
      joyx2_low_margin  = joycentre2_x - (joycentre2_x - joyx2_min) / 8;
      joyx2_high_margin = joycentre2_x + (joyx2_max - joycentre2_x) / 8;
      joyy2_low_margin  = joycentre2_y - (joycentre2_y - joyy2_min) / 8;
      joyy2_high_margin = joycentre2_y + (joyy2_max - joycentre2_y) / 8;
   }
}



/* calibrate_corner:
 *  For analogue access to the joystick, this is used to measure the top
 *  left and bottom right corner positions.
 */
static int calibrate_corner(int stick, int corner)
{
   int flag, other, ret, nowhere;

   if (stick == 0) {
      /* stick 1 calibration */
      flag = (corner) ? JOYSTICK_CALIB_BR1 : JOYSTICK_CALIB_TL1;
      other = (corner) ? JOYSTICK_CALIB_TL1 : JOYSTICK_CALIB_BR1;

      if (corner)
	 ret = averaged_poll(&joyx_max, &joyy_max, &nowhere, &nowhere, MASK_1X | MASK_1Y);
      else
	 ret = averaged_poll(&joyx_min, &joyy_min, &nowhere, &nowhere, MASK_1X | MASK_1Y);
   }
   else {
      /* stick 2 calibration */
      flag = (corner) ? JOYSTICK_CALIB_BR2 : JOYSTICK_CALIB_TL2;
      other = (corner) ? JOYSTICK_CALIB_TL2 : JOYSTICK_CALIB_BR2;

      if (corner)
	 ret = averaged_poll(&nowhere, &nowhere, &joyx2_max, &joyy2_max, MASK_2X | MASK_2Y);
      else
	 ret = averaged_poll(&nowhere, &nowhere, &joyx2_min, &joyy2_min, MASK_2X | MASK_2Y);
   }

   if (ret != 0) {
      joystick_flags &= ~flag;
      return -1;
   }

   joystick_flags |= flag;

   /* once we've done both corners, we are ready for full analogue input */
   if (joystick_flags & other) {
      sort_out_middle_values(stick);
      recalc_calibration_flags();
   }

   return 0;
}



/* calibrate_joystick_tl:
 *  For backward compatibility with the old API.
 */
int calibrate_joystick_tl()
{
   int ret;

   if (!_joystick_installed)
      return -1;

   ret = calibrate_corner(0, 0);

   if ((ret == 0) && (joystick_flags & JDESC_STICK2))
      ret = calibrate_corner(1, 0);

   return ret;
}



/* calibrate_joystick_br:
 *  For backward compatibility with the old API.
 */
int calibrate_joystick_br()
{
   int ret;

   if (!_joystick_installed)
      return -1;

   ret = calibrate_corner(0, 1);

   if ((ret == 0) && (joystick_flags & JDESC_STICK2))
      ret = calibrate_corner(1, 1);

   return ret;
}



/* calibrate_joystick_throttle_min:
 *  For analogue access to the FSPro's throttle, call this after 
 *  initialise_joystick(), with the throttle at the "minimum" extreme
 *  (the user decides whether this is all the way forwards or all the
 *  way back), and also call calibrate_joystick_throttle_max().
 */
int calibrate_joystick_throttle_min()
{
   int dummy;

   if (!_joystick_installed)
      return -1;

   if (averaged_poll(&dummy, &dummy, &dummy, &joy_thr_min, MASK_2Y) != 0)
      return -1;

   /* prevent division by zero errors if user miscalibrated */
   if ((joystick_flags & JOYSTICK_CALIB_THRTL_MAX) &&
       (joy_thr_min == joy_thr_max))
     joy_thr_min = 255 - joy_thr_max;

   joystick_flags |= JOYSTICK_CALIB_THRTL_MIN;
   recalc_calibration_flags();

   return 0;
}



/* calibrate_joystick_throttle_max:
 *  For analogue access to the FSPro's throttle, call this after 
 *  initialise_joystick(), with the throttle at the "maximum" extreme
 *  (the user decides whether this is all the way forwards or all the
 *  way back), and also call calibrate_joystick_throttle_min().
 */
int calibrate_joystick_throttle_max()
{
   int dummy;

   if (!_joystick_installed)
      return -1;

   if (averaged_poll(&dummy, &dummy, &dummy, &joy_thr_max, MASK_2Y) != 0)
      return -1;

   /* prevent division by zero errors if user miscalibrated */
   if ((joystick_flags & JOYSTICK_CALIB_THRTL_MIN) &&
       (joy_thr_min == joy_thr_max))
     joy_thr_max = 255 - joy_thr_min;

   joystick_flags |= JOYSTICK_CALIB_THRTL_MAX;
   recalc_calibration_flags();

   return 0;
}



/* calibrate_joystick_hat:
 *  For access to the Wingman Extreme's hat (I think this will work on all 
 *  Thrustmaster compatible joysticks), call this after initialise_joystick(), 
 *  passing the JOY_HAT constant you wish to calibrate.
 */
int calibrate_joystick_hat(int direction)
{
   static int pos_const[] = 
   { 
      JOYSTICK_CALIB_HAT_CENTRE,
      JOYSTICK_CALIB_HAT_LEFT,
      JOYSTICK_CALIB_HAT_DOWN,
      JOYSTICK_CALIB_HAT_RIGHT,
      JOYSTICK_CALIB_HAT_UP
   };

   int dummy, value;

   if ((direction > JOY_HAT_UP) || (!_joystick_installed))
      return -1;

   if (averaged_poll(&dummy, &dummy, &dummy, &value, MASK_2Y) != 0)
      return -1;

   joy_hat_pos[direction] = value;
   joystick_flags |= pos_const[direction];

   /* when all directions have been calibrated, calculate deltas */
   if ((joystick_flags & JOYSTICK_CALIB_HAT) == JOYSTICK_CALIB_HAT) {
      joy_hat_threshold[0] = (joy_hat_pos[0] + joy_hat_pos[1]) / 2;
      joy_hat_threshold[1] = (joy_hat_pos[1] + joy_hat_pos[2]) / 2;
      joy_hat_threshold[2] = (joy_hat_pos[2] + joy_hat_pos[3]) / 2;
      joy_hat_threshold[3] = (joy_hat_pos[3] + joy_hat_pos[4]) / 2;

      recalc_calibration_flags();
   }

   return 0;
}



/* sort_out_analogue:
 *  There are a couple of problems with reading analogue input from the PC
 *  joystick. For one thing, joysticks tend not to centre repeatably, so
 *  we need a small 'dead' zone in the middle. Also a lot of joysticks aren't
 *  linear, so the positions less than centre need to be handled differently
 *  to those above the centre.
 */
static int sort_out_analogue(int x, int min, int low_margin, int high_margin, int max)
{
   if (x < min) {
      return -128;
   }
   else if (x < low_margin) {
      return -128 + (x - min) * 128 / (low_margin - min);
   }
   else if (x <= high_margin) {
      return 0;
   }
   else if (x <= max) {
      return 128 - (max - x) * 128 / (max - high_margin);
   }
   else
      return 128;
}



/* joy_poll:
 *  Updates the joystick status variables.
 */
static int joy_poll()
{
   int x, y, x2, y2, i;
   unsigned char status;

   /* read the hardware */
   if (poll(&x, &y, &x2, &y2, poll_mask()) != 0)
      return -1;

   status = inportb(0x201);

   /* stick 1 position */
   if ((ABS(x-joy_old_x) < x/4) && (ABS(y-joy_old_y) < y/4)) {
      if ((joystick_flags & JOYSTICK_CALIB_TL1) && 
	  (joystick_flags & JOYSTICK_CALIB_BR1)) {
	 joy[0].stick[0].axis[0].pos = sort_out_analogue(x, joyx_min, joyx_low_margin, joyx_high_margin, joyx_max);
	 joy[0].stick[0].axis[1].pos = sort_out_analogue(y, joyy_min, joyy_low_margin, joyy_high_margin, joyy_max);
      }
      else {
	 joy[0].stick[0].axis[0].pos = x - joycentre_x;
	 joy[0].stick[0].axis[1].pos = y - joycentre_y;
      }

      joy[0].stick[0].axis[0].d1 = (x < (joycentre_x/2));
      joy[0].stick[0].axis[0].d2 = (x > (joycentre_x*4/3));
      joy[0].stick[0].axis[1].d1 = (y < (joycentre_y/2));
      joy[0].stick[0].axis[1].d2 = (y > (joycentre_y*4/3));
   }

   if (joystick_flags & JDESC_STICK2) {
      /* stick 2 position */
      if ((ABS(x2-joy2_old_x) < x2/4) && (ABS(y2-joy2_old_y) < y2/4)) {
	 if ((joystick_flags & JOYSTICK_CALIB_TL2) && 
	     (joystick_flags & JOYSTICK_CALIB_BR2)) {
	    joy[1].stick[0].axis[0].pos = sort_out_analogue(x2, joyx2_min, joyx2_low_margin, joyx2_high_margin, joyx2_max);
	    joy[1].stick[0].axis[1].pos = sort_out_analogue(y2, joyy2_min, joyy2_low_margin, joyy2_high_margin, joyy2_max);
	 }
	 else {
	    joy[1].stick[0].axis[0].pos = x2 - joycentre2_x;
	    joy[1].stick[0].axis[1].pos = y2 - joycentre2_y;
	 }

	 joy[1].stick[0].axis[0].d1 = (x2 < (joycentre2_x/2));
	 joy[1].stick[0].axis[0].d2 = (x2 > (joycentre2_x*3/2));
	 joy[1].stick[0].axis[1].d1 = (y2 < (joycentre2_y/2));
	 joy[1].stick[0].axis[1].d2 = (y2 > (joycentre2_y*3/2));
      }

      joy[1].button[0].b = ((status & 0x40) == 0);
      joy[1].button[1].b = ((status & 0x80) == 0);
   }

   /* button state */
   joy[0].button[0].b = ((status & 0x10) == 0);
   joy[0].button[1].b = ((status & 0x20) == 0);

   if (joystick_flags & JDESC_4BUTTON) {
      /* four button state */
      joy[0].button[2].b = ((status & 0x40) == 0);
      joy[0].button[3].b = ((status & 0x80) == 0);
   }

   if (joystick_flags & JDESC_8BUTTON) {
      /* eight button state */
      joy[0].button[4].b = (x2 < (joycentre2_x/2));
      joy[0].button[5].b = (y2 < (joycentre2_y/2));
      joy[0].button[6].b = (x2 > (joycentre2_x*3/2));
      joy[0].button[7].b = (y2 > (joycentre2_y*3)/2);
   }
   else if (joystick_flags & JDESC_6BUTTON) {
      /* six button state */
      joy[0].button[4].b = (x2 < 128);
      joy[0].button[5].b = (y2 < 128);
   }

   if (joystick_flags & JDESC_Y2_THROTTLE) {
      /* throttle */
      if ((joystick_flags & JOYSTICK_CALIB_THRTL_MIN) && 
	  (joystick_flags & JOYSTICK_CALIB_THRTL_MAX)) {
	 i = (y2 - joy_thr_min) * 255 / (joy_thr_max - joy_thr_min);
	 joy[0].stick[2].axis[0].pos = MID(0, i, 255);
      } 
   }

   if (joystick_flags & JDESC_FSPRO_HAT) {
      /* FSPro hat control (accessed via special button values) */
      joy[0].stick[1].axis[0].pos = 0;
      joy[0].stick[1].axis[1].pos = 0;
      joy[0].stick[1].axis[0].d1 = joy[0].stick[1].axis[0].d2 = 0;
      joy[0].stick[1].axis[1].d1 = joy[0].stick[1].axis[1].d2 = 0;

      if ((status & 0x30) == 0) {
	 joy[0].button[0].b = FALSE;
	 joy[0].button[1].b = FALSE;
	 joy[0].button[2].b = FALSE;
	 joy[0].button[3].b = FALSE;

	 switch (status & 0xC0) {

	    case 0x00:
	       /* up */
	       joy[0].stick[1].axis[1].pos = -128;
	       joy[0].stick[1].axis[1].d1 = TRUE;
	       break;

	    case 0x40:
	       /* right */
	       joy[0].stick[1].axis[0].pos = 128;
	       joy[0].stick[1].axis[0].d2 = TRUE;
	       break;

	    case 0x80:
	       /* down */
	       joy[0].stick[1].axis[1].pos = 128;
	       joy[0].stick[1].axis[1].d2 = TRUE;
	       break;

	    case 0xC0:
	       /* left */
	       joy[0].stick[1].axis[0].pos = -128;
	       joy[0].stick[1].axis[0].d1 = TRUE;
	       break;
	 }
      }
   }

   if (joystick_flags & JDESC_Y2_HAT) {
      /* Wingman Extreme hat control (accessed via the y2 pot) */
      joy[0].stick[1].axis[0].pos = 0;
      joy[0].stick[1].axis[1].pos = 0;
      joy[0].stick[1].axis[0].d1 = joy[0].stick[1].axis[0].d2 = 0;
      joy[0].stick[1].axis[1].d1 = joy[0].stick[1].axis[1].d2 = 0;

      if ((joystick_flags & JOYSTICK_CALIB_HAT) == JOYSTICK_CALIB_HAT) {
	 if (y2 >= joy_hat_threshold[0]) {
	    /* centre */
	 }
	 else if (y2 >= joy_hat_threshold[1]) {
	    /* left */
	    joy[0].stick[1].axis[0].pos = -128;
	    joy[0].stick[1].axis[0].d1 = TRUE;
	 }
	 else if (y2 >= joy_hat_threshold[2]) {
	    /* down */
	    joy[0].stick[1].axis[1].pos = 128;
	    joy[0].stick[1].axis[1].d2 = TRUE;
	 }
	 else if (y2 >= joy_hat_threshold[3]) {
	    /* right */
	    joy[0].stick[1].axis[0].pos = 128;
	    joy[0].stick[1].axis[0].d2 = TRUE;
	 }
	 else {
	    /* up */
	    joy[0].stick[1].axis[1].pos = -128;
	    joy[0].stick[1].axis[1].d1 = TRUE;
	 }
      } 
   } 

   joy_old_x = x;
   joy_old_y = y;

   joy2_old_x = x2;
   joy2_old_y = y2;

   return 0;
}



/* joy_save_data:
 *  Writes calibration data into the config file.
 */
static int joy_save_data()
{
   set_config_int("joystick", "joystick_flags",    joystick_flags);

   set_config_int("joystick", "joycentre_x",       joycentre_x);
   set_config_int("joystick", "joycentre_y",       joycentre_y);
   set_config_int("joystick", "joyx_min",          joyx_min);
   set_config_int("joystick", "joyx_low_margin",   joyx_low_margin);
   set_config_int("joystick", "joyx_high_margin",  joyx_high_margin);
   set_config_int("joystick", "joyx_max",          joyx_max);
   set_config_int("joystick", "joyy_min",          joyy_min);
   set_config_int("joystick", "joyy_low_margin",   joyy_low_margin);
   set_config_int("joystick", "joyy_high_margin",  joyy_high_margin);
   set_config_int("joystick", "joyy_max",          joyy_max);

   set_config_int("joystick", "joycentre2_x",      joycentre2_x);
   set_config_int("joystick", "joycentre2_y",      joycentre2_y);
   set_config_int("joystick", "joyx2_min",         joyx2_min);
   set_config_int("joystick", "joyx2_low_margin",  joyx2_low_margin);
   set_config_int("joystick", "joyx2_high_margin", joyx2_high_margin);
   set_config_int("joystick", "joyx2_max",         joyx2_max);
   set_config_int("joystick", "joyy2_min",         joyy2_min);
   set_config_int("joystick", "joyy2_low_margin",  joyy2_low_margin);
   set_config_int("joystick", "joyy2_high_margin", joyy2_high_margin);
   set_config_int("joystick", "joyy2_max",         joyy2_max);

   set_config_int("joystick", "joythr_min",        joy_thr_min);
   set_config_int("joystick", "joythr_max",        joy_thr_max);

   set_config_int("joystick", "joyhat_0",          joy_hat_threshold[0]);
   set_config_int("joystick", "joyhat_1",          joy_hat_threshold[1]);
   set_config_int("joystick", "joyhat_2",          joy_hat_threshold[2]);
   set_config_int("joystick", "joyhat_3",          joy_hat_threshold[3]);

   return 0;
}



/* joy_load_data:
 *  Loads calibration data from the config file.
 */
static int joy_load_data()
{
   joystick_flags    = get_config_int("joystick", "joystick_flags", 0);

   joycentre_x       = get_config_int("joystick", "joycentre_x", 0);
   joycentre_y       = get_config_int("joystick", "joycentre_y", 0);
   joyx_min          = get_config_int("joystick", "joyx_min", 0);
   joyx_low_margin   = get_config_int("joystick", "joyx_low_margin", 0);
   joyx_high_margin  = get_config_int("joystick", "joyx_high_margin", 0);
   joyx_max          = get_config_int("joystick", "joyx_max", 0);
   joyy_min          = get_config_int("joystick", "joyy_min", 0);
   joyy_low_margin   = get_config_int("joystick", "joyy_low_margin", 0);
   joyy_high_margin  = get_config_int("joystick", "joyy_high_margin", 0);
   joyy_max          = get_config_int("joystick", "joyy_max", 0);

   joycentre2_x      = get_config_int("joystick", "joycentre2_x", 0);
   joycentre2_y      = get_config_int("joystick", "joycentre2_y", 0);
   joyx2_min         = get_config_int("joystick", "joyx2_min", 0);
   joyx2_low_margin  = get_config_int("joystick", "joyx_low2_margin", 0);
   joyx2_high_margin = get_config_int("joystick", "joyx_high2_margin", 0);
   joyx2_max         = get_config_int("joystick", "joyx2_max", 0);
   joyy2_min         = get_config_int("joystick", "joyy2_min", 0);
   joyy2_low_margin  = get_config_int("joystick", "joyy2_low_margin", 0);
   joyy2_high_margin = get_config_int("joystick", "joyy2_high_margin", 0);
   joyy2_max         = get_config_int("joystick", "joyy2_max", 0);

   joy_thr_min       = get_config_int("joystick", "joythr_min", 0);
   joy_thr_max       = get_config_int("joystick", "joythr_max", 0);

   joy_hat_threshold[0] = get_config_int("joystick", "joyhat_0", 0);
   joy_hat_threshold[1] = get_config_int("joystick", "joyhat_1", 0);
   joy_hat_threshold[2] = get_config_int("joystick", "joyhat_2", 0);
   joy_hat_threshold[3] = get_config_int("joystick", "joyhat_3", 0);

   joy_old_x = joy_old_y = 0;

   recalc_calibration_flags();

   return 0;
}



/* next_calib_action:
 *  Returns a flag indicating the next thing that needs to be calibrated.
 */
static int next_calib_action(int stick)
{
   if (stick == 0) {
      /* stick 1 analogue input? */
      if (!(joystick_flags & JOYSTICK_CALIB_TL1))
	 return JOYSTICK_CALIB_TL1;

      if (!(joystick_flags & JOYSTICK_CALIB_BR1))
	 return JOYSTICK_CALIB_BR1;

      /* FSPro throttle input? */
      if (joystick_flags & JDESC_Y2_THROTTLE) {
	 if (!(joystick_flags & JOYSTICK_CALIB_THRTL_MIN))
	    return JOYSTICK_CALIB_THRTL_MIN;

	 if (!(joystick_flags & JOYSTICK_CALIB_THRTL_MAX))
	    return JOYSTICK_CALIB_THRTL_MAX;
      }

      /* Wingman Extreme hat input? */
      if (joystick_flags & JDESC_Y2_HAT) {
	 if (!(joystick_flags & JOYSTICK_CALIB_HAT_CENTRE))
	    return JOYSTICK_CALIB_HAT_CENTRE;

	 if (!(joystick_flags & JOYSTICK_CALIB_HAT_LEFT))
	    return JOYSTICK_CALIB_HAT_LEFT;

	 if (!(joystick_flags & JOYSTICK_CALIB_HAT_DOWN))
	    return JOYSTICK_CALIB_HAT_DOWN;

	 if (!(joystick_flags & JOYSTICK_CALIB_HAT_RIGHT))
	    return JOYSTICK_CALIB_HAT_RIGHT;

	 if (!(joystick_flags & JOYSTICK_CALIB_HAT_UP))
	    return JOYSTICK_CALIB_HAT_UP;
      }
   }
   else if (stick == 1) {
      if (joystick_flags & JDESC_STICK2) {
	 /* stick 2 analogue input? */
	 if (!(joystick_flags & JOYSTICK_CALIB_TL2))
	    return JOYSTICK_CALIB_TL2;

	 if (!(joystick_flags & JOYSTICK_CALIB_BR2))
	    return JOYSTICK_CALIB_BR2;
      }
   }

   return 0;
}



/* joy_calibrate_name:
 *  Returns the name of the next calibration operation.
 */
static char *joy_calibrate_name(int n)
{
   switch (next_calib_action(n)) {

      case JOYSTICK_CALIB_TL1:         return get_config_text("Move stick to the top left");         break;
      case JOYSTICK_CALIB_BR1:         return get_config_text("Move stick to the bottom right");     break;
      case JOYSTICK_CALIB_TL2:         return get_config_text("Move stick 2 to the top left");       break;
      case JOYSTICK_CALIB_BR2:         return get_config_text("Move stick 2 to the bottom right");   break;
      case JOYSTICK_CALIB_THRTL_MIN:   return get_config_text("Set throttle to minimum");            break;
      case JOYSTICK_CALIB_THRTL_MAX:   return get_config_text("Set throttle to maximum");            break;
      case JOYSTICK_CALIB_HAT_CENTRE:  return get_config_text("Centre the hat");                     break;
      case JOYSTICK_CALIB_HAT_LEFT:    return get_config_text("Move the hat left");                  break;
      case JOYSTICK_CALIB_HAT_DOWN:    return get_config_text("Move the hat down");                  break;
      case JOYSTICK_CALIB_HAT_RIGHT:   return get_config_text("Move the hat right");                 break;
      case JOYSTICK_CALIB_HAT_UP:      return get_config_text("Move the hat up");                    break;
   }

   return NULL;
}



/* joy_calibrate:
 *  Performs the next calibration operation.
 */
static int joy_calibrate(int n)
{
   switch (next_calib_action(n)) {

      case JOYSTICK_CALIB_TL1:
	 return calibrate_corner(0, 0);
	 break;

      case JOYSTICK_CALIB_BR1:
	 return calibrate_corner(0, 1);
	 break;

      case JOYSTICK_CALIB_TL2:
	 return calibrate_corner(1, 0);
	 break;

      case JOYSTICK_CALIB_BR2:
	 return calibrate_corner(1, 1);
	 break;

      case JOYSTICK_CALIB_THRTL_MIN:
	 return calibrate_joystick_throttle_min();
	 break;

      case JOYSTICK_CALIB_THRTL_MAX:
	 return calibrate_joystick_throttle_max();
	 break;

      case JOYSTICK_CALIB_HAT_CENTRE:
	 return calibrate_joystick_hat(JOY_HAT_CENTRE);
	 break;

      case JOYSTICK_CALIB_HAT_LEFT:
	 return calibrate_joystick_hat(JOY_HAT_LEFT);
	 break;

      case JOYSTICK_CALIB_HAT_DOWN:
	 return calibrate_joystick_hat(JOY_HAT_DOWN);
	 break;

      case JOYSTICK_CALIB_HAT_RIGHT:
	 return calibrate_joystick_hat(JOY_HAT_RIGHT);
	 break;

      case JOYSTICK_CALIB_HAT_UP:
	 return calibrate_joystick_hat(JOY_HAT_UP);
	 break;
   }

   return -1;
}

