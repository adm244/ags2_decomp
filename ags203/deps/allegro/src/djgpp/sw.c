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
 *      Joystick driver for the Microsoft Sidewinder.
 *      By Marius Fodor, using information provided by Robert Grubbs.
 *
 *      See readme.txt for copyright information.
 */


#ifndef DJGPP
#error This file should only be used by the djgpp version of Allegro
#endif

#include <stdlib.h>

#include "allegro.h"



/* driver functions */
static int sw_init(); 
static void sw_exit(); 
static int sw_poll();


JOYSTICK_DRIVER joystick_sw =
{
   JOY_TYPE_SIDEWINDER,
   "Sidewinder",
   "Sidewinder",
   sw_init,
   sw_exit,
   sw_poll,
   NULL, NULL,
   NULL, NULL
};


int sw_raw_input[4];

int read_sidewinder();



/* int read_sidewinder;
 *  Reads the Sidewinder(s) and updates the global array sw_raw_input[4].
 *  Returns 1 if the Sidewinder(s) returned corrupt data, or if no Sidewinders
 *  were detected. Otherwise, returns 0 when successful.
 */
asm ("                              ;\
                                    ;\
   .comm gDump, 256                 ;\
   .comm bDump, 128                 ;\
                                    ;\
   .macro ParityCheckSW x           ;\
      movl %ebx, %ecx               ;\
      xorb %ch, %cl                 ;\
      jp 0f                         ;\
      movl %ebx, \\x                ;\
   0:                               ;\
   .endm                            ;\
                                    ;\
   .globl _read_sidewinder          ;\
      .align 4                      ;\
   _read_sidewinder:                ;\
      pushal                        ;\
                                    ;\
      movl $200, %ecx               ;\
      movl $gDump, %ebx             ;\
      movw $0x200, %dx              ;\
                                    ;\
      cli                           ;\
   GetSWDataLoop:                   ;\
      outb %al, %dx                 ;\
      nop                           ;\
      nop                           ;\
      nop                           ;\
      nop                           ;\
      nop                           ;\
      nop                           ;\
      inb %dx, %al                  ;\
      movb %al, (%ebx)              ;\
      incl %ebx                     ;\
      decl %ecx                     ;\
      jnz GetSWDataLoop             ;\
      sti                           ;\
                                    ;\
      xorl %ebx, %ebx               ;\
      xorl %ecx, %ecx               ;\
      xorl %edi, %edi               ;\
      movl $1, %esi                 ;\
                                    ;\
   FindCycle:                       ;\
      movb gDump(%edi), %al         ;\
      incl %edi                     ;\
      cmpl $200, %edi               ;\
      je SWNoFind                   ;\
      testb $0x10, %al              ;\
      jnz WMFCS1                    ;\
      xorl %ecx, %ecx               ;\
      jmp FindCycle                 ;\
   WMFCS1:                          ;\
      incl %ecx                     ;\
      cmpl $15, %ecx                ;\
      jne FindCycle                 ;\
                                    ;\
      xorl %ebp, %ebp               ;\
                                    ;\
   FindStrobeLow:                   ;\
      movb gDump(%edi), %al         ;\
      incl %edi                     ;\
      cmpl $200, %edi               ;\
      je SWNoFind                   ;\
      testb $0x10, %al              ;\
      jnz FindStrobeLow             ;\
      xorl %ecx, %ecx               ;\
                                    ;\
   FindStrobeHigh:                  ;\
      incl %ecx                     ;\
      cmpl $15, %ecx                ;\
      je SWModeCheck                ;\
      movb gDump(%edi), %al         ;\
      incl %edi                     ;\
      cmpl $200, %edi               ;\
      je SWNoFind                   ;\
      testb $0x10, %al              ;\
      jz FindStrobeHigh             ;\
                                    ;\
      movb %al, bDump(%ebp)         ;\
      incl %ebp                     ;\
      jmp FindStrobeLow             ;\
                                    ;\
   SMWDone:                         ;\
      popal                         ;\
      movl $0, %eax                 ;\
      ret                           ;\
                                    ;\
   SWNoFind:                        ;\
      popal                         ;\
      movl $1, %eax                 ;\
      ret                           ;\
                                    ;\
   SWModeCheck:                     ;\
      cmpl $5, %ebp                 ;\
      je ModeB1                     ;\
      cmpl $15, %ebp                ;\
      je ModeA1                     ;\
      cmpl $10, %ebp                ;\
      je ModeB2                     ;\
      cmpl $30, %ebp                ;\
      je ModeA2                     ;\
      cmpl $45, %ebp                ;\
      je ModeA3                     ;\
      cmpl $20, %ebp                ;\
      je ModeB4                     ;\
      cmpl $60, %ebp                ;\
      je ModeA4                     ;\
      jmp SWNoFind                  ;\
                                    ;\
   ModeA1:                          ;\
      cmpl $3, _num_joysticks       ;\
      je ModeB3                     ;\
      xorl %ebp, %ebp               ;\
      call DoModeA                  ;\
      ParityCheckSW _sw_raw_input   ;\
      jmp SMWDone                   ;\
                                    ;\
   ModeA4:                          ;\
      movl $45, %ebp                ;\
      call DoModeA                  ;\
      ParityCheckSW _sw_raw_input+12;\
   ModeA3:                          ;\
      movl $30, %ebp                ;\
      call DoModeA                  ;\
      ParityCheckSW _sw_raw_input+8 ;\
   ModeA2:                          ;\
      movl $15, %ebp                ;\
      call DoModeA                  ;\
      ParityCheckSW _sw_raw_input+4 ;\
      xorl %ebp, %ebp               ;\
      call DoModeA                  ;\
      ParityCheckSW _sw_raw_input   ;\
      jmp SMWDone                   ;\
                                    ;\
   ModeB4:                          ;\
      movl $15, %ebp                ;\
      call DoModeB                  ;\
      ParityCheckSW _sw_raw_input+12;\
   ModeB3:                          ;\
      movl $10, %ebp                ;\
      call DoModeB                  ;\
      ParityCheckSW _sw_raw_input+8 ;\
   ModeB2:                          ;\
      movl $5, %ebp                 ;\
      call DoModeB                  ;\
      ParityCheckSW _sw_raw_input+4 ;\
   ModeB1:                          ;\
      xorl %ebp, %ebp               ;\
      call DoModeB                  ;\
      ParityCheckSW _sw_raw_input   ;\
      jmp SMWDone                   ;\
                                    ;\
   DoModeB:                         ;\
      xorl %ebx, %ebx               ;\
      movl $2, %eax                 ;\
      movl $5, %ecx                 ;\
      addl $bDump, %ebp             ;\
   ModeBLoop:                       ;\
      testb $0x20, (%ebp)           ;\
      jmp 0f                        ;\
   0:                               ;\
      orl %eax, %ebx                ;\
      shll $1, %eax                 ;\
      testb $0x40, (%ebp)           ;\
      jmp 0f                        ;\
   0:                               ;\
      orl %eax, %ebx                ;\
      shll $1, %eax                 ;\
      testb $0x80, (%ebp)           ;\
      jmp 0f                        ;\
   0:                               ;\
      orl %eax, %ebx                ;\
      shll $1, %eax                 ;\
      incl %ebp                     ;\
      decl %ecx                     ;\
      jnz ModeBLoop                 ;\
      ret                           ;\
                                    ;\
   DoModeA:                         ;\
      xorl %ebx, %ebx               ;\
      movl $2, %eax                 ;\
      movl $15, %ecx                ;\
      addl $bDump, %ebp             ;\
   ModeALoop:                       ;\
      testb $0x20, (%ebp)           ;\
      jmp 0f                        ;\
   0:                               ;\
      orl %eax, %ebx                ;\
      shll $1, %eax                 ;\
      incl %ebp                     ;\
      decl %ecx                     ;\
      jnz ModeALoop                 ;\
      ret                           ;\
");



/* sw_init:
 *  Initialises the driver.
 */
static int sw_init()
{
   static char name_x[] = "X";
   static char name_y[] = "Y";
   static char name_pad[] = "Pad";
   static char *name_b[] = { "A", "B", "C", "X", "Y", "Z", "L", "R", "Start", "M" };
   int i, b;

   if (read_sidewinder() == 1)
      return -1;

   /* can we autodetect this??? */
   num_joysticks = 4;

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



/* sw_exit:
 *  Shuts down the driver.
 */
static void sw_exit()
{
}



/* sw_poll:
 *  Updates the joystick status variables.
 */
static int sw_poll()
{
   int i, b, mask;

   read_sidewinder();

   for (i=0; i<num_joysticks; i++) {
      joy[i].stick[0].axis[0].d1 = ((sw_raw_input[i] & 0x10) != 0);
      joy[i].stick[0].axis[0].d2 = ((sw_raw_input[i] & 0x08) != 0);
      joy[i].stick[0].axis[1].d1 = ((sw_raw_input[i] & 0x02) != 0);
      joy[i].stick[0].axis[1].d2 = ((sw_raw_input[i] & 0x04) != 0);

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
	 joy[i].button[b].b = ((sw_raw_input[i] & mask) != 0);
	 mask <<= 1; 
      }
   }

   return 0;
}

