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
 *      Unix-specific header defines.
 *
 *      By Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */

#ifndef _ALLEGRO_ALUNIX_H
#define _ALLEGRO_ALUNIX_H

#ifndef ALLEGRO_UNIX
   #error bad include
#endif



/**************************************/
/************ General Unix ************/
/**************************************/

/* magic to capture name of executable file */
extern int    __crt0_argc;
extern char **__crt0_argv;

#ifndef USE_CONSOLE
# define main _mangled_main
# undef END_OF_MAIN
# define END_OF_MAIN() void *_mangled_main_address = (void*) _mangled_main;
#else
# undef END_OF_MAIN
# define END_OF_MAIN() void *_mangled_main_address;
#endif


#define TIMERDRV_UNIX            AL_ID('U','N','I','X')
AL_VAR(TIMER_DRIVER, timerdrv_unix);



/************************************/
/*********** Sound drivers **********/
/************************************/

#ifdef ALLEGRO_WITH_OSSDIGI

#define DIGI_OSS              AL_ID('O','S','S','D')

AL_VAR(DIGI_DRIVER, digi_oss);

#define DIGI_DRIVER_OSS                                          \
      {  DIGI_OSS,        &digi_oss,            TRUE  },

#endif /* ALLEGRO_WITH_OSSDIGI */

#ifdef ALLEGRO_WITH_ESDDIGI

#define DIGI_ESD              AL_ID('E','S','D','D')

AL_VAR(DIGI_DRIVER, digi_esd);

#define DIGI_DRIVER_ESD                                          \
      {  DIGI_ESD,        &digi_esd,            FALSE  },

#endif /* ALLEGRO_WITH_ESDDIGI */



/************************************/
/************ X-specific ************/
/************************************/

#ifdef ALLEGRO_WITH_XWINDOWS


#define SYSTEM_XWINDOWS          AL_ID('X','W','I','N')
AL_VAR(SYSTEM_DRIVER, system_xwin);

#define GFX_XWINDOWS             AL_ID('X','W','I','N')
#define KEYDRV_XWINDOWS          AL_ID('X','W','I','N')
#define MOUSEDRV_XWINDOWS        AL_ID('X','W','I','N')


#endif /* ALLEGRO_WITH_XWINDOWS */



/****************************************/
/************ Linux-specific ************/
/****************************************/

#ifdef ALLEGRO_LINUX


#define SYSTEM_LINUX             AL_ID('L','N','X','C')
AL_VAR(SYSTEM_DRIVER, system_linux);

#ifdef ALLEGRO_LINUX_VGA
   #define GFX_VGA                  AL_ID('V','G','A',' ')
   #define GFX_MODEX                AL_ID('M','O','D','X')
   AL_VAR(GFX_DRIVER, gfx_vga);
   AL_VAR(GFX_DRIVER, gfx_modex);
#endif

#ifdef ALLEGRO_LINUX_FBCON
   #define GFX_FBCON                AL_ID('F','B',' ',' ')
   AL_VAR(GFX_DRIVER, gfx_fbcon);
#endif

#ifdef ALLEGRO_LINUX_VBEAF
   #define GFX_VBEAF                AL_ID('V','B','A','F')
   AL_VAR(GFX_DRIVER, gfx_vbeaf);
#endif

#ifdef ALLEGRO_LINUX_GGI
   #define GFX_GGI                  AL_ID('G','G','I',' ')
   AL_VAR(GFX_DRIVER, gfx_ggi);
#endif

#ifdef ALLEGRO_LINUX_SVGALIB
   #define GFX_SVGALIB              AL_ID('S','V','G','A')
   AL_VAR(GFX_DRIVER, gfx_svgalib);
#endif

#define KEYDRV_LINUX             AL_ID('L','N','X','C')
AL_VAR(KEYBOARD_DRIVER, keydrv_linux_console);

#define MOUSEDRV_LINUX_PS2       AL_ID('L','P','S','2')
AL_VAR(MOUSE_DRIVER, mousedrv_linux_ps2);

#define MOUSEDRV_LINUX_GPMDATA   AL_ID('G','P','M','D')
AL_VAR(MOUSE_DRIVER, mousedrv_linux_gpmdata);

#define MOUSEDRV_LINUX_MS        AL_ID('M','S',' ',' ')
AL_VAR(MOUSE_DRIVER, mousedrv_linux_ms);

#define JOY_TYPE_LINUX_ANALOGUE  AL_ID('L','N','X','A')
AL_VAR(JOYSTICK_DRIVER, joystick_linux_analogue);


AL_FUNC(int, request_modex_scroll, (int x, int y));
AL_FUNC(int, poll_modex_scroll, (void));
AL_FUNC(void, split_modex_screen, (int line));


/* Port I/O functions -- maybe these should be internal */

static inline void outportb(unsigned short port, unsigned char value)
{
   __asm__ volatile ("outb %0, %1" : : "a" (value), "d" (port));
}

static inline void outportw(unsigned short port, unsigned short value)
{
   __asm__ volatile ("outw %0, %1" : : "a" (value), "d" (port));
}

static inline void outportl(unsigned short port, unsigned long value)
{
   __asm__ volatile ("outl %0, %1" : : "a" (value), "d" (port));
}

static inline unsigned char inportb(unsigned short port)
{
   unsigned char value;
   __asm__ volatile ("inb %1, %0" : "=a" (value) : "d" (port));
   return value;
}

static inline unsigned short inportw(unsigned short port)
{
   unsigned short value;
   __asm__ volatile ("inw %1, %0" : "=a" (value) : "d" (port));
   return value;
}

static inline unsigned long inportl(unsigned short port)
{
   unsigned long value;
   __asm__ volatile ("inl %1, %0" : "=a" (value) : "d" (port));
   return value;
}


#endif /* ALLEGRO_LINUX */



#endif /* !_ALLEGRO_ALUNIX_H */

