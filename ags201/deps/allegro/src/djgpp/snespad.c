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
 *      Joystick driver for the SNES controller, by Kerry High.
 *      Based on sample code by Earle F. Philhower, III.
 *
 *      See readme.txt for copyright information.
 */


#ifndef DJGPP
#error This file should only be used by the djgpp version of Allegro
#endif

#include <stdlib.h>
#include <pc.h>

#include "allegro.h"



#define LPT1_BASE 0x378
#define LPT2_BASE 0x278
#define LPT3_BASE 0x3bc
#define SNES_POWER 248
#define SNES_CLOCK 1
#define SNES_LATCH 2



/* driver functions */
static int sp_init(); 
static void sp_exit(); 
static int sp_poll(int);
static int sp1_poll();
static int sp2_poll();
static int sp3_poll();


JOYSTICK_DRIVER joystick_sp1 =
{
   JOY_TYPE_SNESPAD_LPT1,
   "SNESpad-LPT1",
   "SNESpad-LPT1",
   sp_init,
   sp_exit,
   sp1_poll,
   NULL, NULL,
   NULL, NULL
};


JOYSTICK_DRIVER joystick_sp2 =
{
   JOY_TYPE_SNESPAD_LPT2,
   "SNESpad-LPT2",
   "SNESpad-LPT2",
   sp_init,
   sp_exit,
   sp2_poll,
   NULL, NULL,
   NULL, NULL
};


JOYSTICK_DRIVER joystick_sp3 =
{
   JOY_TYPE_SNESPAD_LPT3,
   "SNESpad-LPT3",
   "SNESpad-LPT3",
   sp_init,
   sp_exit,
   sp3_poll,
   NULL, NULL,
   NULL, NULL
};



/* sp_init:
 *  Initialises the driver.
 */
static int sp_init()
{
   static char name_x[] = "X";
   static char name_y[] = "Y";
   static char name_pad[] = "Pad";
   static char *name_b[] = { "B", "Y", "A", "X", "Select", "Start", "L", "R"};
   int i, b;

   /* Can't autodetect this... */
   num_joysticks = 4;

   for (i=0; i<num_joysticks; i++) {
      joy[i].flags = JOYFLAG_DIGITAL;

      joy[i].num_sticks = 1;
      joy[i].stick[0].flags = JOYFLAG_DIGITAL | JOYFLAG_SIGNED;
      joy[i].stick[0].num_axis = 2;
      joy[i].stick[0].axis[0].name = name_x;
      joy[i].stick[0].axis[1].name = name_y;
      joy[i].stick[0].name = name_pad;

      joy[i].num_buttons = 8;

      for (b=0; b<8; b++)
	 joy[i].button[b].name = name_b[b];
   }

   return 0;
}



/* sp_exit:
 *  Shuts down the driver.
 */
static void sp_exit()
{
}



/* sp_poll:
 *  Common - Updates the joystick status variables.
 */
static int sp_poll(int base)
{
   int i, b, snes_in;

   for (i=0; i<num_joysticks; i++) {
      /* Which pad? */
      snes_in=(1<<(6-i));

      /* First, get buttons b, y, select, and start */
      /* b */
      outportb(base, SNES_POWER+SNES_CLOCK);
      outportb(base, SNES_POWER+SNES_CLOCK+SNES_LATCH);
      outportb(base, SNES_POWER+SNES_CLOCK);
      joy[i].button[0].b=((inportb(base+1)&snes_in)==0);
      /* y */
      outportb(base, SNES_POWER);
      outportb(base, SNES_POWER+SNES_CLOCK);
      joy[i].button[1].b=((inportb(base+1)&snes_in)==0);
      /* select */
      outportb(base, SNES_POWER);
      outportb(base, SNES_POWER+SNES_CLOCK);
      joy[i].button[4].b=((inportb(base+1)&snes_in)==0);
      /* start */
      outportb(base, SNES_POWER);
      outportb(base, SNES_POWER+SNES_CLOCK);
      joy[i].button[5].b=((inportb(base+1)&snes_in)==0);

      /* Now, do the direction */
      outportb(base, SNES_POWER);
      outportb(base, SNES_POWER+SNES_CLOCK);
      joy[i].stick[0].axis[1].d1 = ((inportb(base+1)&snes_in)==0);
      outportb(base, SNES_POWER);
      outportb(base, SNES_POWER+SNES_CLOCK);
      joy[i].stick[0].axis[1].d2 = ((inportb(base+1)&snes_in)==0);
      outportb(base, SNES_POWER);
      outportb(base, SNES_POWER+SNES_CLOCK);
      joy[i].stick[0].axis[0].d1 = ((inportb(base+1)&snes_in)==0);
      outportb(base, SNES_POWER);
      outportb(base, SNES_POWER+SNES_CLOCK);
      joy[i].stick[0].axis[0].d2 = ((inportb(base+1)&snes_in)==0);

      for (b=0; b<2; b++) {
	 if (joy[i].stick[0].axis[b].d1)
	    joy[i].stick[0].axis[b].pos = -128;
	 else if (joy[i].stick[0].axis[b].d2)
	    joy[i].stick[0].axis[b].pos = 128;
	 else
	    joy[i].stick[0].axis[b].pos = 0;
      }

      /* now, for the rest of the buttons... */
      /* a */
      outportb(base, SNES_POWER);
      outportb(base, SNES_POWER+SNES_CLOCK);
      joy[i].button[2].b=((inportb(base+1)&snes_in)==0);
      /* x */
      outportb(base, SNES_POWER);
      outportb(base, SNES_POWER+SNES_CLOCK);
      joy[i].button[3].b=((inportb(base+1)&snes_in)==0);
      /* l */
      outportb(base, SNES_POWER);
      outportb(base, SNES_POWER+SNES_CLOCK);
      joy[i].button[6].b=((inportb(base+1)&snes_in)==0);
      /* r */
      outportb(base, SNES_POWER);
      outportb(base, SNES_POWER+SNES_CLOCK);
      joy[i].button[7].b=((inportb(base+1)&snes_in)==0);
      outportb(base, 0);
   }

   return 0;
}



/* sp1_poll:
 *  LPT1 - Updates the joystick status variables.
 */
static int sp1_poll()
{
   return sp_poll(LPT1_BASE);
}



/* sp2_poll:
 *  LPT2 - Updates the joystick status variables.
 */
static int sp2_poll()
{
   return sp_poll(LPT2_BASE);
}



/* sp3_poll:
 *  LPT3 - Updates the joystick status variables.
 */
static int sp3_poll()
{
   return sp_poll(LPT3_BASE);
}

