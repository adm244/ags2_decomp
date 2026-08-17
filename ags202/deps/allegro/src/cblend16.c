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
 *      Interpolation routines for 16 bit truecolor pixels.
 *
 *      Optimised by Cloud Wu and Burton Radons.
 *
 *      See readme.txt for copyright information.
 */


#include <stdlib.h>

#include "allegro.h"


#ifdef ALLEGRO_COLOR16



/* macro for constructing the blender routines */
#define BLEND(n)                                                             \
									     \
   static unsigned long blend16_##n(unsigned long x, unsigned long y)        \
   {                                                                         \
      unsigned long result;                                                  \
									     \
      x = ((x&0xFFFF) | (x<<16)) & 0x7E0F81F;                                \
      y = ((y&0xFFFF) | (y<<16)) & 0x7E0F81F;                                \
									     \
      result = ((x - y) * (n*32/31) / 32 + y) & 0x7E0F81F;                   \
									     \
      return ((result&0xFFFF) | (result>>16));                               \
   }



/* declare 32 interpolation functions (one for each alpha) */ 
BLEND(0)   BLEND(1)   BLEND(2)   BLEND(3) 
BLEND(4)   BLEND(5)   BLEND(6)   BLEND(7) 
BLEND(8)   BLEND(9)   BLEND(10)  BLEND(11) 
BLEND(12)  BLEND(13)  BLEND(14)  BLEND(15) 
BLEND(16)  BLEND(17)  BLEND(18)  BLEND(19) 
BLEND(20)  BLEND(21)  BLEND(22)  BLEND(23) 
BLEND(24)  BLEND(25)  BLEND(26)  BLEND(27) 
BLEND(28)  BLEND(29)  BLEND(30)  BLEND(31) 



/* and list them all in a table */ 
BLENDER_MAP _trans_blender16 = 
{ {
   blend16_0,  blend16_0,  blend16_0,  blend16_0,
   blend16_0,  blend16_0,  blend16_0,  blend16_0,
   blend16_1,  blend16_1,  blend16_1,  blend16_1,
   blend16_1,  blend16_1,  blend16_1,  blend16_1,
   blend16_2,  blend16_2,  blend16_2,  blend16_2,
   blend16_2,  blend16_2,  blend16_2,  blend16_2,
   blend16_3,  blend16_3,  blend16_3,  blend16_3,
   blend16_3,  blend16_3,  blend16_3,  blend16_3,
   blend16_4,  blend16_4,  blend16_4,  blend16_4,
   blend16_4,  blend16_4,  blend16_4,  blend16_4,
   blend16_5,  blend16_5,  blend16_5,  blend16_5,
   blend16_5,  blend16_5,  blend16_5,  blend16_5,
   blend16_6,  blend16_6,  blend16_6,  blend16_6,
   blend16_6,  blend16_6,  blend16_6,  blend16_6,
   blend16_7,  blend16_7,  blend16_7,  blend16_7,
   blend16_7,  blend16_7,  blend16_7,  blend16_7,
   blend16_8,  blend16_8,  blend16_8,  blend16_8,
   blend16_8,  blend16_8,  blend16_8,  blend16_8,
   blend16_9,  blend16_9,  blend16_9,  blend16_9,
   blend16_9,  blend16_9,  blend16_9,  blend16_9,
   blend16_10, blend16_10, blend16_10, blend16_10,
   blend16_10, blend16_10, blend16_10, blend16_10,
   blend16_11, blend16_11, blend16_11, blend16_11,
   blend16_11, blend16_11, blend16_11, blend16_11,
   blend16_12, blend16_12, blend16_12, blend16_12,
   blend16_12, blend16_12, blend16_12, blend16_12,
   blend16_13, blend16_13, blend16_13, blend16_13,
   blend16_13, blend16_13, blend16_13, blend16_13,
   blend16_14, blend16_14, blend16_14, blend16_14,
   blend16_14, blend16_14, blend16_14, blend16_14,
   blend16_15, blend16_15, blend16_15, blend16_15,
   blend16_15, blend16_15, blend16_15, blend16_15,
   blend16_16, blend16_16, blend16_16, blend16_16,
   blend16_16, blend16_16, blend16_16, blend16_16,
   blend16_17, blend16_17, blend16_17, blend16_17,
   blend16_17, blend16_17, blend16_17, blend16_17,
   blend16_18, blend16_18, blend16_18, blend16_18,
   blend16_18, blend16_18, blend16_18, blend16_18,
   blend16_19, blend16_19, blend16_19, blend16_19,
   blend16_19, blend16_19, blend16_19, blend16_19,
   blend16_20, blend16_20, blend16_20, blend16_20,
   blend16_20, blend16_20, blend16_20, blend16_20,
   blend16_21, blend16_21, blend16_21, blend16_21,
   blend16_21, blend16_21, blend16_21, blend16_21,
   blend16_22, blend16_22, blend16_22, blend16_22,
   blend16_22, blend16_22, blend16_22, blend16_22,
   blend16_23, blend16_23, blend16_23, blend16_23,
   blend16_23, blend16_23, blend16_23, blend16_23,
   blend16_24, blend16_24, blend16_24, blend16_24,
   blend16_24, blend16_24, blend16_24, blend16_24,
   blend16_25, blend16_25, blend16_25, blend16_25,
   blend16_25, blend16_25, blend16_25, blend16_25,
   blend16_26, blend16_26, blend16_26, blend16_26,
   blend16_26, blend16_26, blend16_26, blend16_26,
   blend16_27, blend16_27, blend16_27, blend16_27,
   blend16_27, blend16_27, blend16_27, blend16_27,
   blend16_28, blend16_28, blend16_28, blend16_28,
   blend16_28, blend16_28, blend16_28, blend16_28,
   blend16_29, blend16_29, blend16_29, blend16_29,
   blend16_29, blend16_29, blend16_29, blend16_29,
   blend16_30, blend16_30, blend16_30, blend16_30,
   blend16_30, blend16_30, blend16_30, blend16_30,
   blend16_31, blend16_31, blend16_31, blend16_31,
   blend16_31, blend16_31, blend16_31, blend16_31,
} };



#endif
