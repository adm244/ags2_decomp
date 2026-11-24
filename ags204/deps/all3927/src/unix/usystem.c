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
 *      List of system drivers for the Unix library.
 *
 *      By Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/aintunix.h"



/* list the available drivers */
_DRIVER_INFO _system_driver_list[] =
{
#ifdef ALLEGRO_WITH_XWINDOWS
   {  SYSTEM_XWINDOWS,  &system_xwin,  TRUE  },
#endif
#ifdef ALLEGRO_LINUX
   {  SYSTEM_LINUX,     &system_linux,     TRUE  },
#endif
   {  SYSTEM_NONE,      &system_none,      FALSE },
   {  0,                NULL,              0     }
};



/* _unix_find_resource:
 *  Helper for locating a Unix config file. Looks in the home directory
 *  of the current user, and in /etc.
 */
int _unix_find_resource(char *dest, char *resource, int size)
{
   char buf[256], tmp[256];
   char *home = getenv("HOME");

   if (home) {
      /* look for ~/file */
      append_filename(buf, uconvert_ascii(home, tmp), resource, sizeof(buf));
      if (exists(buf)) {
	 ustrncpy(dest, buf, size-ucwidth(0));
	 return 0;
      }

      /* if it is a .cfg, look for ~/.filerc */
      if (ustricmp(get_extension(resource), uconvert_ascii("cfg", tmp)) == 0) {
	 ustrncpy(buf, uconvert_ascii(home, tmp), sizeof(buf)-ucwidth(0));
	 put_backslash(buf);
	 ustrncat(buf, uconvert_ascii(".", tmp), sizeof(buf)-ucwidth(0));
	 ustrncpy(tmp, resource, sizeof(tmp)-ucwidth(0));
	 ustrncat(buf, ustrtok(tmp, "."), sizeof(buf)-ucwidth(0));
	 ustrncat(buf, uconvert_ascii("rc", tmp), sizeof(buf)-ucwidth(0));
	 if (exists(buf)) {
	    ustrncpy(dest, buf, size-ucwidth(0));
	    return 0;
	 }
      }
   }

   /* look for /etc/file */
   append_filename(buf, uconvert_ascii("/etc/", tmp), resource, sizeof(buf));
   if (exists(buf)) {
      ustrncpy(dest, buf, size-ucwidth(0));
      return 0;
   }

   /* if it is a .cfg, look for /etc/filerc */
   if (ustricmp(get_extension(resource), uconvert_ascii("cfg", tmp)) == 0) {
      ustrncpy(buf, uconvert_ascii("/etc/", tmp), sizeof(buf)-ucwidth(0));
      ustrncpy(tmp, resource, sizeof(tmp)-ucwidth(0));
      ustrncat(buf, ustrtok(tmp, "."), sizeof(buf)-ucwidth(0));
      ustrncat(buf, uconvert_ascii("rc", tmp), sizeof(buf)-ucwidth(0));
      if (exists(buf)) {
	 ustrncpy(dest, buf, size-ucwidth(0));
	 return 0;
      }
   }

   return -1;
}


