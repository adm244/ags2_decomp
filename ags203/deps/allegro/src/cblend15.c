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
 *      Interpolation routines for 15 bit truecolor pixels.
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
   static unsigned long blend15_##n(unsigned long x, unsigned long y)        \
   {                                                                         \
      unsigned long result;                                                  \
									     \
      x = ((x&0xFFFF) | (x<<16)) & 0x3E07C1F;                                \
      y = ((y&0xFFFF) | (y<<16)) & 0x3E07C1F;                                \
									     \
      result = ((x - y) * (n*32/31) / 32 + y) & 0x3E07C1F;                   \
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
BLENDER_MAP _trans_blender15 = 
{ {
   blend15_0,  blend15_0,  blend15_0,  blend15_0,
   blend15_0,  blend15_0,  blend15_0,  blend15_0,
   blend15_1,  blend15_1,  blend15_1,  blend15_1,
   blend15_1,  blend15_1,  blend15_1,  blend15_1,
   blend15_2,  blend15_2,  blend15_2,  blend15_2,
   blend15_2,  blend15_2,  blend15_2,  blend15_2,
   blend15_3,  blend15_3,  blend15_3,  blend15_3,
   blend15_3,  blend15_3,  blend15_3,  blend15_3,
   blend15_4,  blend15_4,  blend15_4,  blend15_4,
   blend15_4,  blend15_4,  blend15_4,  blend15_4,
   blend15_5,  blend15_5,  blend15_5,  blend15_5,
   blend15_5,  blend15_5,  blend15_5,  blend15_5,
   blend15_6,  blend15_6,  blend15_6,  blend15_6,
   blend15_6,  blend15_6,  blend15_6,  blend15_6,
   blend15_7,  blend15_7,  blend15_7,  blend15_7,
   blend15_7,  blend15_7,  blend15_7,  blend15_7,
   blend15_8,  blend15_8,  blend15_8,  blend15_8,
   blend15_8,  blend15_8,  blend15_8,  blend15_8,
   blend15_9,  blend15_9,  blend15_9,  blend15_9,
   blend15_9,  blend15_9,  blend15_9,  blend15_9,
   blend15_10, blend15_10, blend15_10, blend15_10,
   blend15_10, blend15_10, blend15_10, blend15_10,
   blend15_11, blend15_11, blend15_11, blend15_11,
   blend15_11, blend15_11, blend15_11, blend15_11,
   blend15_12, blend15_12, blend15_12, blend15_12,
   blend15_12, blend15_12, blend15_12, blend15_12,
   blend15_13, blend15_13, blend15_13, blend15_13,
   blend15_13, blend15_13, blend15_13, blend15_13,
   blend15_14, blend15_14, blend15_14, blend15_14,
   blend15_14, blend15_14, blend15_14, blend15_14,
   blend15_15, blend15_15, blend15_15, blend15_15,
   blend15_15, blend15_15, blend15_15, blend15_15,
   blend15_16, blend15_16, blend15_16, blend15_16,
   blend15_16, blend15_16, blend15_16, blend15_16,
   blend15_17, blend15_17, blend15_17, blend15_17,
   blend15_17, blend15_17, blend15_17, blend15_17,
   blend15_18, blend15_18, blend15_18, blend15_18,
   blend15_18, blend15_18, blend15_18, blend15_18,
   blend15_19, blend15_19, blend15_19, blend15_19,
   blend15_19, blend15_19, blend15_19, blend15_19,
   blend15_20, blend15_20, blend15_20, blend15_20,
   blend15_20, blend15_20, blend15_20, blend15_20,
   blend15_21, blend15_21, blend15_21, blend15_21,
   blend15_21, blend15_21, blend15_21, blend15_21,
   blend15_22, blend15_22, blend15_22, blend15_22,
   blend15_22, blend15_22, blend15_22, blend15_22,
   blend15_23, blend15_23, blend15_23, blend15_23,
   blend15_23, blend15_23, blend15_23, blend15_23,
   blend15_24, blend15_24, blend15_24, blend15_24,
   blend15_24, blend15_24, blend15_24, blend15_24,
   blend15_25, blend15_25, blend15_25, blend15_25,
   blend15_25, blend15_25, blend15_25, blend15_25,
   blend15_26, blend15_26, blend15_26, blend15_26,
   blend15_26, blend15_26, blend15_26, blend15_26,
   blend15_27, blend15_27, blend15_27, blend15_27,
   blend15_27, blend15_27, blend15_27, blend15_27,
   blend15_28, blend15_28, blend15_28, blend15_28,
   blend15_28, blend15_28, blend15_28, blend15_28,
   blend15_29, blend15_29, blend15_29, blend15_29,
   blend15_29, blend15_29, blend15_29, blend15_29,
   blend15_30, blend15_30, blend15_30, blend15_30,
   blend15_30, blend15_30, blend15_30, blend15_30,
   blend15_31, blend15_31, blend15_31, blend15_31,
   blend15_31, blend15_31, blend15_31, blend15_31,
} };



#endif
