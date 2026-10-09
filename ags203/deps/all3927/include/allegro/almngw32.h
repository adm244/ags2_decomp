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
 *      Configuration defines for use with Mingw32.
 *
 *      By Michael Rickmann.
 *
 *      See readme.txt for copyright information.
 */


#ifndef __MINGW32__
   #error bad include
#endif

#ifdef ALLEGRO_SRC
   #error Currently MINGW32 should only use the DLL
#endif

#include <io.h>
#include <fcntl.h>
#include <direct.h>
#include <malloc.h>


/* describe this platform */
#define ALLEGRO_PLATFORM_STR  "Mingw32"
#define ALLEGRO_MINGW32
#define ALLEGRO_WINDOWS
#define ALLEGRO_I386
#define ALLEGRO_LITTLE_ENDIAN
#define ALLEGRO_LOSE_BITMAPS

#ifdef USE_CONSOLE
   #define ALLEGRO_CONSOLE_OK
#endif


/* describe how function prototypes look to MINGW32 __declspec(dllexport)
      works reliably only for functions and standard C varibles (Apr. 99) */

#define AL_METHOD(type, name, args)          type (*name) args
#ifndef AL_INLINE
  #define AL_INLINE(type, name, args, code)    extern inline type name args code
#endif

#define _AL_DLL   __declspec(dllimport)
#define _FIX_DLL  _AL_DLL
#define AL_VAR(type, name)                   extern _AL_DLL type name
#define AL_ARRAY(type, name)                 extern _AL_DLL type name[]
#define AL_FUNCPTR(type, name, args)         extern _AL_DLL type (*name) args
#define AL_FUNC(type, name, args)            type __cdecl name args

/* windows specific defines */
#define NONAMELESSUNION

/* arrange for other headers to be included later on */
#define ALLEGRO_EXTRA_HEADER     "allegro/alwin.h"
#define ALLEGRO_INTERNAL_HEADER  "allegro/aintwin.h"
#define ALLEGRO_MMX_HEADER       "obj/mingw32/mmx.h"


