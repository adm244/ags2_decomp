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
 *      Datafile loader definitions.
 *
 *      See readme.txt for copyright information.
 */


#ifndef DATAFILE_H
#define DATAFILE_H


/* data file ID's for compatibility with the old datafile format */
#define V1_DAT_MAGIC             0x616C6C2EL

#define V1_DAT_DATA              0
#define V1_DAT_FONT              1
#define V1_DAT_BITMAP_16         2 
#define V1_DAT_BITMAP_256        3
#define V1_DAT_SPRITE_16         4
#define V1_DAT_SPRITE_256        5
#define V1_DAT_PALLETE_16        6
#define V1_DAT_PALLETE_256       7
#define V1_DAT_FONT_8x8          8
#define V1_DAT_FONT_PROP         9
#define V1_DAT_BITMAP            10
#define V1_DAT_PALLETE           11
#define V1_DAT_SAMPLE            12
#define V1_DAT_MIDI              13
#define V1_DAT_RLE_SPRITE        14
#define V1_DAT_FLI               15
#define V1_DAT_C_SPRITE          16
#define V1_DAT_XC_SPRITE         17

#define OLD_FONT_SIZE            95


/* object loading functions */
void *load_data_object(PACKFILE *f, long size);
void *load_file_object(PACKFILE *f, long size);
void *load_font_object(PACKFILE *f, long size);
void *load_sample_object(PACKFILE *f, long size);
void *load_midi_object(PACKFILE *f, long size);
void *load_bitmap_object(PACKFILE *f, long size);
void *load_rle_sprite_object(PACKFILE *f, long size);
void *load_compiled_sprite_object(PACKFILE *f, long size);
void *load_xcompiled_sprite_object(PACKFILE *f, long size);


/* object unloading functions */
void unload_sample(SAMPLE *s);
void unload_midi(MIDI *m);


/* information about a datafile object */
typedef struct DATAFILE_TYPE
{
   int type;
   void *(*load)(PACKFILE *f, long size);
   void (*destroy)();
} DATAFILE_TYPE;


#define MAX_DATAFILE_TYPES    32

extern DATAFILE_TYPE datafile_type[MAX_DATAFILE_TYPES];


#endif          /* ifndef DATAFILE_H */
