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
 *      Table of datafile loader routines.
 *
 *      See readme.txt for copyright information.
 */


#include "internal.h"
#include "datafile.h"



/* list of objects, and methods for loading and destroying them */
DATAFILE_TYPE datafile_type[MAX_DATAFILE_TYPES] =
{
   {  DAT_END, NULL, NULL }, { DAT_END, NULL, NULL }, { DAT_END, NULL, NULL },
   {  DAT_END, NULL, NULL }, { DAT_END, NULL, NULL }, { DAT_END, NULL, NULL },
   {  DAT_END, NULL, NULL }, { DAT_END, NULL, NULL }, { DAT_END, NULL, NULL },
   {  DAT_END, NULL, NULL }, { DAT_END, NULL, NULL }, { DAT_END, NULL, NULL },
   {  DAT_END, NULL, NULL }, { DAT_END, NULL, NULL }, { DAT_END, NULL, NULL },
   {  DAT_END, NULL, NULL }, { DAT_END, NULL, NULL }, { DAT_END, NULL, NULL },
   {  DAT_END, NULL, NULL }, { DAT_END, NULL, NULL }, { DAT_END, NULL, NULL },
   {  DAT_END, NULL, NULL }, { DAT_END, NULL, NULL }, { DAT_END, NULL, NULL },
   {  DAT_END, NULL, NULL }, { DAT_END, NULL, NULL }, { DAT_END, NULL, NULL },
   {  DAT_END, NULL, NULL }, { DAT_END, NULL, NULL }, { DAT_END, NULL, NULL },
   {  DAT_END, NULL, NULL }, { DAT_END, NULL, NULL }
};



/* register_datafile_object: 
 *  Registers a custom datafile object, providing functions for loading
 *  and destroying the objects.
 */
void register_datafile_object(int id, void *(*load)(PACKFILE *f, long size), void (*destroy)(void *data))
{
   int i;

   /* replacing an existing type? */
   for (i=0; i<MAX_DATAFILE_TYPES; i++) {
      if (datafile_type[i].type == id) {
	 if (load)
	    datafile_type[i].load = load;
	 if (destroy)
	    datafile_type[i].destroy = destroy;
	 return;
      }
   }

   /* add a new type */
   for (i=0; i<MAX_DATAFILE_TYPES; i++) {
      if (datafile_type[i].type == DAT_END) {
	 datafile_type[i].type = id;
	 datafile_type[i].load = load;
	 datafile_type[i].destroy = destroy;
	 return;
      }
   }
}

