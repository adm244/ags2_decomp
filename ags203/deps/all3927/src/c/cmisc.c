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
 *      Math routines, compiled sprite wrapper, etc.
 *
 *      By Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"



/* This is a horrible hack. For testing purposes, it is useful if I can
 * build and run the C version on an Intel machine. But that requires
 * using hardware drivers that provide their own bank switch mechanisms,
 * so we have to stick with the asm calling convention. Hence this horrible
 * hybrid where it uses the C drawing code alongside asm bank switchers.
 * Even worse is that I can't be bothered to figure out how to get 
 * asmdefs.inc to work with the below code fragment, so I resort to a
 * horrible global variable instead.
 */
#if (defined ALLEGRO_I386) && (defined ALLEGRO_GCC)

#ifdef ALLEGRO_DJGPP
   int _cmisc_bmp_line_offset = offsetof(BITMAP, line);
#else
   int __cmisc_bmp_line_offset = offsetof(BITMAP, line);
#endif

asm ("

#ifdef ALLEGRO_DJGPP

.globl __stub_bank_switch
__stub_bank_switch:

#else

.globl _stub_bank_switch
_stub_bank_switch:

#endif

   addl __cmisc_bmp_line_offset, %edx
   movl (%edx, %eax, 4), %eax
   subl __cmisc_bmp_line_offset, %edx
   ret

");

#else

void *_stub_bank_switch(BITMAP *bmp, int y)
{
   return bmp->line[y];
}

#endif

void _stub_unbank_switch(BITMAP *bmp)
{
}

void _stub_bank_switch_end(void)
{
}



/* apply_matrix_f:
 *  Floating point vector by matrix multiplication routine.
 */
void apply_matrix_f(MATRIX_f *m, float x, float y, float z,
		    float *xout, float *yout, float *zout)
{
#define CALC_ROW(n) (x * m->v[(n)][0] + y * m->v[(n)][1] + z * m->v[(n)][2] + m->t[(n)])
   *xout = CALC_ROW(0);
   *yout = CALC_ROW(1);
   *zout = CALC_ROW(2);
#undef CALC_ROW
}

