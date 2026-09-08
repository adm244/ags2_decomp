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
 *      Joystick driver for the Gravis GamePad Pro.
 *      By Marius Fodor, using information provided by Benji York.
 *
 *      See readme.txt for copyright information.
 */


#ifndef DJGPP
#error This file should only be used by the djgpp version of Allegro
#endif

#include <stdlib.h>

#include "allegro.h"



/* driver functions */
static int gpro_init(); 
static void gpro_exit(); 
static int gpro_poll();


JOYSTICK_DRIVER joystick_gpro =
{
   JOY_TYPE_GAMEPAD_PRO,
   "GamePad Pro",
   "GamePad Pro",
   gpro_init,
   gpro_exit,
   gpro_poll,
   NULL, NULL,
   NULL, NULL
};



/* int read_gpp(int pad_num);
 *  Reads the GamePad Pro pointed to by pad_num (0 or 1) and returns its 
 *  status. Returns 1 if the GPP returned corrupt data, or if no GPP was 
 *  detected.
 */
static int read_gpp(int pad_num)
{
   char samples[60];
   char clock_mask, data_mask;
   int ret;

   asm (
      "  cmpb $0, %0 ; "
      "  jne 14f ; "
      "  movb $0x10, %b2 ; "
      "  movb $0x20, %b3 ; "
      "  jmp 15f ; "
      " 14: "
      "  movb $0x40, %b2 ; "
      "  movb $0x80, %b3 ; "

      " 15: "
      "  xorl %%ebx, %%ebx ; "
      "  xorl %%edi, %%edi ; "
      "  movw $0x201, %%dx ; "

      "  cli ; "
      "  inb %%dx, %%al ; "
      "  movb %%al, %%ah ; "

      " 4: "
      "  xorl %%ecx, %%ecx ; "
      " 0: "
      "  inb %%dx, %%al ; "
      "  cmpb %%ah, %%al ; "
      "  jne 1f ; "
      "  incl %%ecx ; "
      "  cmpl $255, %%ecx ; "
      "  jl 0b ; "

      " 1: "
      "  cmpl $255, %%ecx ; "
      "  je 16f ; "

      "  testb %%ah, %b2 ; "
      "  jz 2f ; "
      "  testb %%al, %b2 ; "
      "  jnz 2f ; "

      "  addl %4, %%edi ; "
      "  testb %%al, %b3 ; "
      "  jz 3f ; "
      "  movb $1, (%%edi) ; "
      "  jmp 12f ; "
      " 3: "
      "  movb $0, (%%edi) ; "
      " 12: "
      "  subl %4, %%edi ; "
      "  incl %%edi ; "

      " 2: "
      "  movb %%al, %%ah ; "
      "  cmpl $200, %%ebx ; "
      "  je 13f ; "
      "  incl %%ebx ; "
      "  cmpl $50, %%edi ; "
      "  jl 4b ; "
      "  sti ; "

      " 13: "
      "  xorl %%ecx, %%ecx ; "
      "  movl $1, %%esi ; "
      " 7: "
      "  addl %4, %%esi ; "
      "  movb (%%esi), %%dl ; "
      "  subl %4, %%esi ; "
      "  cmpb $1, %%dl ; "
      "  jg 16f ; "
      "  jne 6f ; "
      "  incl %%ecx ; "
      "  jmp 5f ; "
      " 6: "
      "  xorl %%ecx, %%ecx ; "

      " 5: "
      "  cmpl $5, %%ecx ; "
      "  je 8f ; "
      "  cmpl %%edi, %%esi ; "
      "  je 8f ; "
      "  incl %%esi ; "
      "  jmp 7b ; "

      " 8: "
      "  cmpl $5, %%ecx ; "
      "  jne 16f ; "
      "  addl $2, %%esi ; "
      "  xorl %%eax, %%eax ; "
      "  xorl %%ebx, %%ebx ; "
      "  xorl %%ecx, %%ecx ; "
      "  xorl %%edx, %%edx ; "

      " 10: "
      "  incl %%ecx ; "
      "  cmpl $5, %%ecx ; "
      "  jne 11f ; "
      "  movl $1, %%ecx ; "
      "  incl %%esi ; "
      " 11: "
      "  addl %4, %%esi ; "
      "  movb (%%esi), %%dl ; "
      "  subl %4, %%esi ; "
      "  orl %%edx, %%eax ; "
      "  shll $1, %%eax ; "
      "  cmpl $13, %%ebx ; "
      "  je 9f ; "
      "  incl %%ebx ; "
      "  incl %%esi ; "
      "  jmp 10b ; "

      " 16: "
      "  movl $1, %%eax ; "

      " 9: "
      "  sti ; "

   : "=a" (ret)

   : "0" (pad_num),
     "m" (clock_mask),
     "m" (data_mask),
     "m" (samples)

   : "%ebx", "%ecx", "%edx", "%esi", "%edi"
   );

   return ret;
}



/* gpro_init:
 *  Initialises the driver.
 */
static int gpro_init()
{
   static char name_x[] = "X";
   static char name_y[] = "Y";
   static char name_pad[] = "Pad";
   static char *name_b[] = { "R1", "L1", "Red", "Yellow", "Green", "L2", "Blue", "R2", "Start", "Select" };
   int i, b;

   if (read_gpp(0) == 1)
      return -1;

   if (read_gpp(1) == 1)
      num_joysticks = 1;
   else
      num_joysticks = 2;

   for (i=0; i<num_joysticks; i++) {
      joy[i].flags = JOYFLAG_DIGITAL;

      joy[i].num_sticks = 1;
      joy[i].stick[0].flags = JOYFLAG_DIGITAL | JOYFLAG_SIGNED;
      joy[i].stick[0].num_axis = 2;
      joy[i].stick[0].axis[0].name = name_x;
      joy[i].stick[0].axis[1].name = name_y;
      joy[i].stick[0].name = name_pad;

      joy[i].num_buttons = 10;

      for (b=0; b<10; b++)
	 joy[i].button[b].name = name_b[b];
   }

   return 0;
}



/* gpro_exit:
 *  Shuts down the driver.
 */
static void gpro_exit()
{
}



/* gpro_poll:
 *  Updates the joystick status variables.
 */
static int gpro_poll()
{
   int i, b, gpp, mask;

   for (i=0; i<num_joysticks; i++) {
      if ((gpp = read_gpp(i)) == 1)
	 return -1;

      joy[i].stick[0].axis[0].d1 = ((gpp & 0x02) != 0);
      joy[i].stick[0].axis[0].d2 = ((gpp & 0x04) != 0);
      joy[i].stick[0].axis[1].d1 = ((gpp & 0x10) != 0);
      joy[i].stick[0].axis[1].d2 = ((gpp & 0x08) != 0);

      for (b=0; b<2; b++) {
	 if (joy[i].stick[0].axis[b].d1)
	    joy[i].stick[0].axis[b].pos = -128;
	 else if (joy[i].stick[0].axis[b].d2)
	    joy[i].stick[0].axis[b].pos = 128;
	 else
	    joy[i].stick[0].axis[b].pos = 0;
      }

      mask = 0x20;

      for (b=0; b<10; b++) {
	 joy[i].button[b].b = ((gpp & mask) != 0);
	 mask <<= 1;
      }
   }

   return 0;
}


