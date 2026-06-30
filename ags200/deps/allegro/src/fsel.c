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
 *      The file selector.
 *
 *      Guilherme Silveira and Theuzifan Sumachingun both independently 
 *      modified it to only list valid drive letters.
 *
 *      See readme.txt for copyright information.
 */


#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>
#include <ctype.h>
#include <fcntl.h>

#ifdef DJGPP
#include <dir.h>
#endif

#include "allegro.h"


#ifndef _USE_LFN
#define _USE_LFN  0
#endif



static int fs_edit_proc(int, DIALOG *, int );
static int fs_flist_proc(int, DIALOG *, int );
static int fs_dlist_proc(int, DIALOG *, int );
static char *fs_flist_getter(int, int *);
static char *fs_dlist_getter(int, int *);


#define FLIST_SIZE      2048

typedef struct FLIST
{
   char dir[256];
   int size;
   char *name[FLIST_SIZE];
} FLIST;

static FLIST *flist = NULL;

static char *fext = NULL;



static DIALOG file_selector[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key) (flags)  (d1)  (d2)  (dp) */
   { d_shadow_box_proc, 0,    0,    304,  160,  0,    0,    0,    0,       0,    0,    NULL },
   { d_ctext_proc,      152,  8,    1,    1,    0,    0,    0,    0,       0,    0,    NULL },
   { d_button_proc,     208,  107,  80,   16,   0,    0,    0,    D_EXIT,  0,    0,    NULL },
   { d_button_proc,     208,  129,  80,   16,   0,    0,    27,   D_EXIT,  0,    0,    NULL },
   { fs_edit_proc,      16,   28,   272,  8,    0,    0,    0,    0,       79,   0,    NULL },
   { fs_flist_proc,     16,   46,   176,  99,   0,    0,    0,    D_EXIT,  0,    0,    fs_flist_getter },
   { fs_dlist_proc,     208,  46,   80,   51,   0,    0,    0,    D_EXIT,  0,    0,    fs_dlist_getter },
   { NULL }
};


#define FS_MESSAGE   1
#define FS_OK        2
#define FS_CANCEL    3
#define FS_EDIT      4
#define FS_FILES     5
#define FS_DISKS     6



/* drive_exists:
 *  Checks whether the specified drive is valid.
 */
static int drive_exists(int x)
{
   int old, ret;

   old = getdisk();
   setdisk(x);
   ret = (getdisk() == x);
   setdisk(old); 

   return ret;
}



/* count_disks:
 *  Counts the number of valid drives.
 */
static int count_disks()
{
   int c, i;

   c = 0;

   for (i=0; i<26; i++)
      if (drive_exists(i))
	 c++;

   return c;
}



/* get_x_drive:
 *  Returns the drive letter matching the specified list index.
 */
static int get_x_drive(int index)
{
   int c, i;

   c = 0;

   for (i=0; i<26; i++) {
      if (drive_exists(i)) {
	 c++;
	 if (c==index)
	    return i;
      }
   }

   return -1;
}



/* fs_dlist_getter:
 *  Listbox data getter routine for the file selector disk list.
 */
static char *fs_dlist_getter(int index, int *list_size)
{
   static char d[] = "A:\\";

   if (index < 0) {
      if (list_size)
	 *list_size = count_disks();
      return NULL;
   }

   d[0] = 'A' + get_x_drive(index+1);
   if ((d[0]<'A') || (d[0]>'Z'))
      d[0] = 'C';

   return d;
}



/* fs_dlist_proc:
 *  Dialog procedure for the file selector disk list.
 */
static int fs_dlist_proc(int msg, DIALOG *d, int c)
{
   int ret, i;
   char *s = file_selector[FS_EDIT].dp;
   char temp;

   if (msg == MSG_START) {
      d->d1 = d->d2 = 0;
      if ((isalpha(s[0])) && (s[1] == ':'))
	 for (i=0; i<toupper(s[0])-'A'; i++)
	    if (drive_exists(i))
	       d->d1++;
   }

   ret = d_list_proc(msg, d, c);

   if (ret == D_CLOSE) {
      temp = 'A' + get_x_drive(d->d1+1);
      if ((temp<'A') || (temp>'Z'))
	 temp = 'C';
      *(s++) = temp;
      *(s++) = ':';
      *(s++) = '\\';
      *s = 0;
      scare_mouse();
      SEND_MESSAGE(file_selector+FS_FILES, MSG_START, 0);
      SEND_MESSAGE(file_selector+FS_FILES, MSG_DRAW, 0);
      SEND_MESSAGE(file_selector+FS_EDIT, MSG_START, 0);
      SEND_MESSAGE(file_selector+FS_EDIT, MSG_DRAW, 0);
      unscare_mouse();
      return D_O_K;
   }

   return ret;
}



/* fs_edit_proc:
 *  Dialog procedure for the file selector editable string.
 */
static int fs_edit_proc(int msg, DIALOG *d, int c)
{
   char *s = d->dp;
   int ch;
   int attr;
   char b[256];

   if (msg == MSG_START) {
      fix_filename_path(b, s, sizeof(b));
      strcpy(s, b);
   }

   if (msg == MSG_KEY) {
      if ((s[0] == 0) || (s[strlen(s)-1] == DEVICE_SEPARATOR))
	 strcat(s, "./");

      ch = s[strlen(s)-1];
      if ((ch != '/') && (ch != OTHER_PATH_SEPARATOR)) {
	 if (file_exists(s, FA_RDONLY | FA_HIDDEN | FA_DIREC, &attr)) {
	    if (attr & FA_DIREC)
	       put_backslash(s);
	    else
	       return D_CLOSE;
	 }
	 else
	    return D_CLOSE;
      }
      scare_mouse();
      SEND_MESSAGE(file_selector+FS_FILES, MSG_START, 0);
      SEND_MESSAGE(file_selector+FS_FILES, MSG_DRAW, 0);
      SEND_MESSAGE(d, MSG_START, 0);
      SEND_MESSAGE(d, MSG_DRAW, 0);
      unscare_mouse();
      return D_O_K;
   }

   if (msg==MSG_CHAR) {
      ch = c & 0xFF;
      if ((ch >= 'a') && (ch <= 'z')) {
#ifdef DJGPP
	 c = (c & 0xFFFFFF00L) | toupper(ch);
#endif
      }
      else if (ch == '/')
	 c = (c & 0xFFFFFF00L) | OTHER_PATH_SEPARATOR;
      else if (_USE_LFN) {
	 if ((ch > 127) || ((ch < 32) && (ch != 8) && (ch != 0)))
	    return D_O_K;
      }
      else {
	 if ((ch != OTHER_PATH_SEPARATOR) && (ch != '_') &&
	     (ch != DEVICE_SEPARATOR) && (ch != '.') &&
	     ((ch < 'A') || (ch > 'Z')) && ((ch < '0') || (ch > '9')) &&
	     (ch != 8) && (ch != 127) && (ch != 0))
	    return D_O_K;
      }
   }

   return d_edit_proc(msg, d, c); 
}



/* fs_flist_putter:
 *  Callback routine for for_each_file() to fill the file selector listbox.
 */
static void fs_flist_putter(char *str, int attrib, int param)
{
   int c, c2;
   char *s, *ext, *tok, *name;
   char tmp[80];
   static char ext_tokens[] = " ,;";

   s = get_filename(str);
   fix_filename_case(s);

   if ((fext) && (!(attrib & FA_DIREC))) {
      strcpy(tmp, fext);
      ext = get_extension(s);
      tok = strtok(tmp, ext_tokens);
      while (tok) {
	 if (stricmp(ext, tok) == 0)
	    break;
	 tok = strtok(NULL, ext_tokens);
      }
      if (!tok)
	 return;
   }

   if ((flist->size < FLIST_SIZE) && (strcmp(s,".") != 0)) {
      name = malloc(strlen(s) + ((attrib & FA_DIREC) ? 2 : 1));
      if (name == NULL)
	 return;

      for (c=0; c<flist->size; c++) {
	 if (flist->name[c][strlen(flist->name[c])-1]==OTHER_PATH_SEPARATOR) {
	    if (attrib & FA_DIREC)
	       if (stricmp(s, flist->name[c]) < 0)
		  break;
	 }
	 else {
	    if (attrib & FA_DIREC)
	       break;
	    if (stricmp(s, flist->name[c]) < 0)
	       break;
	 }
      }
      for (c2=flist->size; c2>c; c2--)
	 flist->name[c2] = flist->name[c2-1];

      flist->name[c] = name;
      strcpy(flist->name[c], s);
      if (attrib & FA_DIREC)
	 put_backslash(flist->name[c]);
      flist->size++;
   }
}



/* fs_flist_getter:
 *  Listbox data getter routine for the file selector list.
 */
static char *fs_flist_getter(int index, int *list_size)
{
   if (index < 0) {
      if (list_size)
	 *list_size = flist->size;
      return NULL;
   }
   return flist->name[index];
}



/* fs_flist_proc:
 *  Dialog procedure for the file selector list.
 */
static int fs_flist_proc(int msg, DIALOG *d, int c)
{
   int i, ret;
   int sel = d->d1;
   char *s = file_selector[FS_EDIT].dp;
   static int recurse_flag = 0;

   if (msg == MSG_START) {
      if (!flist)
	 flist = malloc(sizeof(FLIST));
      else {
	 for (i=0; i<flist->size; i++)
	    if (flist->name[i])
	       free(flist->name[i]);
      }
      if (!flist) {
	 errno = ENOMEM;
	 return D_CLOSE; 
      }
      flist->size = 0;
      for (i=0; i<flist->size; i++)
	 flist->name[i] = NULL;
      replace_filename(flist->dir, s, "*.*", sizeof(flist->dir));
      for_each_file(flist->dir, FA_RDONLY | FA_DIREC | FA_ARCH, fs_flist_putter, 0);
      if (errno)
	 alert(NULL, get_config_text("Disk error"), NULL, get_config_text("OK"), NULL, 13, 0);
      *get_filename(flist->dir) = 0;
      d->d1 = d->d2 = 0;
      sel = 0;
   }

   if (msg == MSG_END) {
      if (flist) {
	 for (i=0; i<flist->size; i++)
	    if (flist->name[i])
	       free(flist->name[i]);
	 free(flist);
      }
      flist = NULL;
   }

   recurse_flag++;
   ret = d_list_proc(msg,d,c);     /* call the parent procedure */
   recurse_flag--;

   if (((sel != d->d1) || (ret == D_CLOSE)) && (recurse_flag == 0)) {
      replace_filename(s, flist->dir, flist->name[d->d1], 256);
      scare_mouse();
      SEND_MESSAGE(file_selector+FS_EDIT, MSG_START, 0);
      SEND_MESSAGE(file_selector+FS_EDIT, MSG_DRAW, 0);
      unscare_mouse();

      if (ret == D_CLOSE)
	 return SEND_MESSAGE(file_selector+FS_EDIT, MSG_KEY, 0);
   }

   return ret;
}



/* file_select:
 *  Displays the Allegro file selector, with the message as caption. 
 *  Allows the user to select a file, and stores the selection in path 
 *  (which should have room for at least 80 characters). The files are
 *  filtered according to the file extensions in ext. Passing NULL
 *  includes all files, "PCX;BMP" includes only files with .PCX or .BMP
 *  extensions. Returns zero if it was closed with the Cancel button, 
 *  non-zero if it was OK'd.
 */
int file_select(char *message, char *path, char *ext)
{
   int ret;
   char *p;

   file_selector[FS_MESSAGE].dp = message;
   file_selector[FS_EDIT].dp = path;
   file_selector[FS_OK].dp = get_config_text("OK");
   file_selector[FS_CANCEL].dp = get_config_text("Cancel");
   fext = ext;

   if (!path[0]) {
      /* actually, this is not needed */
      getcwd(path, 80);
      fix_filename_case(path);
      fix_filename_slashes(path);
      put_backslash(path);
   }

   clear_keybuf();

   do {
   } while (gui_mouse_b());

   centre_dialog(file_selector);
   set_dialog_color(file_selector, gui_fg_color, gui_bg_color);
   ret = do_dialog(file_selector, FS_EDIT);

   if ((ret == FS_CANCEL) || (*get_filename(path) == 0))
      return FALSE;

   p = get_extension(path);
   if ((*p == 0) && (ext) && (!strpbrk(ext, " ,;"))) {
      *p = '.';
      strcpy(p+1, ext);
   }

   return TRUE; 
}

