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
 *      Configuration routines.
 *
 *      Hook functions added by Martijn Versteegh.
 *
 *      See readme.txt for copyright information.
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#ifdef DJGPP
#include <dir.h>
#endif

#include "internal.h"



typedef struct CONFIG_ENTRY
{
   char *name;                      /* variable name (NULL if comment) */
   char *data;                      /* variable value */
   struct CONFIG_ENTRY *next;       /* linked list */
} CONFIG_ENTRY;


typedef struct CONFIG
{
   CONFIG_ENTRY *head;              /* linked list of config entries */
   char *filename;                  /* where were we loaded from? */
   int dirty;                       /* has our data changed? */
} CONFIG;


typedef struct CONFIG_HOOK
{
   char *section;                   /* hooked config section info */
   int (*intgetter)(char *name, int def);
   char *(*stringgetter)(char *name, char *def);
   void (*stringsetter)(char *name, char *value);
   struct CONFIG_HOOK *next; 
} CONFIG_HOOK;


#define MAX_CONFIGS     4

static CONFIG *config[MAX_CONFIGS] = { NULL, NULL, NULL, NULL };
static CONFIG *config_override = NULL;
static CONFIG *config_language = NULL;
static CONFIG *system_config = NULL;

static CONFIG_HOOK *config_hook = NULL;

static int config_installed = FALSE;



/* destroy_config:
 *  Destroys a config structure, writing it out to disk if the contents
 *  have changed.
 */
static void destroy_config(CONFIG *cfg)
{
   CONFIG_ENTRY *pos, *prev;

   if (cfg) {
      if (cfg->filename) {
	 if (cfg->dirty) {
	    /* write changed data to disk */
	    PACKFILE *f = pack_fopen(cfg->filename, F_WRITE);

	    if (f) {
	       pos = cfg->head;

	       while (pos) {
		  if (pos->name) {
		     pack_fputs(pos->name, f);
		     if (pos->name[0] != '[')
			pack_fputs(" = ", f);
		  }
		  if (pos->data)
		     pack_fputs(pos->data, f);

		  pack_fputs("\n", f);
		  pos = pos->next;
	       }

	       pack_fclose(f);
	    }
	 }

	 free(cfg->filename);
      }

      /* destroy the variable list */
      pos = cfg->head;

      while (pos) {
	 prev = pos;
	 pos = pos->next;

	 if (prev->name)
	    free(prev->name);

	 if (prev->data)
	    free(prev->data);

	 free(prev);
      }

      free(cfg);
   }
}



/* config_cleanup:
 *  Called at shutdown time to free memory being used by the config routines,
 *  and write any changed data out to disk.
 */
static void config_cleanup()
{
   CONFIG_HOOK *hook, *nexthook;
   int i;

   for (i=0; i<MAX_CONFIGS; i++) {
      if (config[i]) {
	 destroy_config(config[i]);
	 config[i] = NULL;
      }
   }

   if (config_override) {
      destroy_config(config_override);
      config_override = NULL;
   }

   if (config_language) {
      destroy_config(config_language);
      config_language = NULL;
   }

   if (system_config) {
      destroy_config(system_config);
      system_config = NULL;
   }

   if (config_hook) {
      hook = config_hook;

      while (hook) {
	 if (hook->section)
	    free(hook->section);

	 nexthook = hook->next; 
	 free(hook);
	 hook = nexthook;
      }

      config_hook = NULL;
   }

   _remove_exit_func(config_cleanup);
   config_installed = FALSE;
}



/* init_config:
 *  Sets up the configuration routines ready for use, also loading the
 *  default config file if the loaddata flag is set and no other config
 *  file is in memory.
 */
static void init_config(int loaddata)
{
   char buf[4][256];
   char *s;
   int i;

   if (!config_installed) {
      _add_exit_func(config_cleanup);
      config_installed = TRUE;
   }

   if ((loaddata) && (!config[0])) {
      /* look for allegro.cfg in the same directory as the program */
      replace_filename(buf[0], __crt0_argv[0], "allegro.cfg", sizeof(buf[0]));

      /* if that fails, try sound.cfg */
      replace_filename(buf[1], __crt0_argv[0], "sound.cfg", sizeof(buf[1]));

      /* no luck? try the ALLEGRO enviroment variable... */
      s = getenv("ALLEGRO");
      if (s) {
	 append_filename(buf[2], s, "allegro.cfg", sizeof(buf[2]));
	 append_filename(buf[3], s, "sound.cfg", sizeof(buf[3]));
      }
      else {
	 strcpy(buf[2], buf[0]);
	 strcpy(buf[3], buf[1]);
      }

      /* see which of these files actually exist */
      for (i=0; i<4; i++) {
	 if (file_exists(buf[i], FA_RDONLY | FA_ARCH, NULL)) {
	    set_config_file(buf[i]);
	    break;
	 }
      }

      if (i >= 4)
	 set_config_file(buf[0]);
   }

   if (!system_config) {
      system_config = malloc(sizeof(CONFIG));
      if (system_config) {
	 system_config->head = NULL;
	 system_config->filename = NULL;
	 system_config->dirty = FALSE;
      }
   }
}



/* get_line: 
 *  Helper for splitting files up into individual lines.
 */
static int get_line(char *data, int length, char *name, char *val)
{
   char buf[256], buf2[256];
   int pos, i, j;

   for (pos=0; (pos<length) && (pos<255); pos++) {
      if ((data[pos] == '\r') || (data[pos] == '\n')) {
	 buf[pos] = 0;
	 if ((pos < length-1) && 
	     (((data[pos] == '\r') && (data[pos+1] == '\n')) ||
	      ((data[pos] == '\n') && (data[pos+1] == '\r')))) {
	    pos++;
	 }
	 pos++;
	 break;
      }

      buf[pos] = data[pos];
   }

   buf[MIN(pos,255)] = 0;

   /* skip leading spaces */
   i = 0;
   while ((buf[i]) && (isspace(buf[i])))
      i++;

   /* read name string */
   j = 0;
   while ((buf[i]) && (!isspace(buf[i])) && (buf[i] != '=') && (buf[i] != '#'))
      buf2[j++] = buf[i++];

   if (j) {
      /* got a variable */
      buf2[j] = 0;
      strcpy(name, buf2);

      while ((buf[i]) && ((isspace(buf[i])) || (buf[i] == '=')))
	 i++;

      strcpy(val, buf+i);

      /* strip trailing spaces */
      i = strlen(val) - 1;
      while ((i >= 0) && (isspace(val[i])))
	 val[i--] = 0;
   }
   else {
      /* blank line or comment */
      name[0] = 0;
      strcpy(val, buf);
   }

   return pos;
}



/* set_config:
 *  Does the work of setting up a config structure.
 */
static void set_config(CONFIG **config, char *data, int length, char *filename)
{
   char name[256];
   char val[256];
   CONFIG_ENTRY **prev, *p;
   int pos;

   init_config(FALSE);

   if (*config) {
      destroy_config(*config);
      *config = NULL;
   }

   *config = malloc(sizeof(CONFIG));
   if (!(*config))
      return;

   (*config)->head = NULL;
   (*config)->dirty = FALSE;

   if (filename) {
      (*config)->filename = malloc(strlen(filename)+1);
      if ((*config)->filename)
	 strcpy((*config)->filename, filename); 
   }
   else
      (*config)->filename = NULL;

   prev = &(*config)->head;
   pos = 0;

   while (pos < length) {
      pos += get_line(data+pos, length-pos, name, val);

      p = malloc(sizeof(CONFIG_ENTRY));
      if (!p)
	 return;

      if (name[0]) {
	 p->name = malloc(strlen(name)+1);
	 if (p->name)
	    strcpy(p->name, name);
      }
      else
	 p->name = NULL;

      p->data = malloc(strlen(val)+1);
      if (p->data)
	 strcpy(p->data, val);

      p->next = NULL;
      *prev = p;
      prev = &p->next;
   }
}



/* load_config_file:
 *  Does the work of loading a config file.
 */
static void load_config_file(CONFIG **config, char *filename, char *savefile)
{
   int length;

   if (*config) {
      destroy_config(*config);
      *config = NULL;
   }

   length = file_size(filename);

   if (length > 0) {
      PACKFILE *f = pack_fopen(filename, F_READ);
      if (f) {
	 char *tmp = malloc(length);
	 if (tmp) {
	    pack_fread(tmp, length, f);
	    set_config(config, tmp, length, savefile);
	    free(tmp);
	 }
	 else
	    set_config(config, NULL, 0, savefile);
	 pack_fclose(f);
      }
      else
	 set_config(config, NULL, 0, savefile);
   }
   else
      set_config(config, NULL, 0, savefile);
}



/* set_config_file:
 *  Sets the file to be used for all future configuration operations.
 */
void set_config_file(char *filename)
{
   load_config_file(&config[0], filename, filename);
}



/* set_config_data:
 *  Sets the block of data to be used for all future configuration 
 *  operations.
 */
void set_config_data(char *data, int length)
{
   set_config(&config[0], data, length, NULL);
}



/* override_config_file:
 *  Sets the file that will override all future configuration operations.
 */
void override_config_file(char *filename)
{
   load_config_file(&config_override, filename, NULL);
}



/* override_config_data:
 *  Sets the block of data that will override all future configuration 
 *  operations.
 */
void override_config_data(char *data, int length)
{
   set_config(&config_override, data, length, NULL);
}



/* push_config_state:
 *  Pushes the current config state onto the stack.
 */
void push_config_state()
{
   int i;

   if (config[MAX_CONFIGS-1])
      destroy_config(config[MAX_CONFIGS-1]);

   for (i=MAX_CONFIGS-1; i>0; i--)
      config[i] = config[i-1];

   config[0] = NULL;
}



/* pop_config_state:
 *  Pops the current config state off the stack.
 */
void pop_config_state()
{
   int i;

   if (config[0])
      destroy_config(config[0]);

   for (i=0; i<MAX_CONFIGS-1; i++)
      config[i] = config[i+1];

   config[MAX_CONFIGS-1] = NULL;
}



/* prettify_section_name:
 *  Helper for ensuring that a section name is enclosed by [ ] braces.
 */
static void prettify_section_name(char *in, char *out)
{
   if (in) {
      if (in[0] != '[')
	 strcpy(out, "[");
      else
	 out[0] = 0;

      strcat(out, in);

      if (out[strlen(out)-1] != ']')
	 strcat(out, "]");
   }
   else
      out[0] = 0;
}



/* hook_config_section:
 *  Hooks a config section to a set of getter/setter functions. This will 
 *  override the normal table of values, and give the provider of the hooks 
 *  complete control over that section.
 */
void hook_config_section(char *section, int (*intgetter)(char *, int), char *(*stringgetter)(char *, char *), void (*stringsetter)(char *,char *))
{
   CONFIG_HOOK *hook, **prev;
   char section_name[256];

   init_config(FALSE);

   prettify_section_name(section, section_name);

   hook = config_hook;
   prev = &config_hook;

   while (hook) {
      if (stricmp(section_name, hook->section) == 0) {
	 if ((intgetter) || (stringgetter) || (stringsetter)) {
	    /* modify existing hook */
	    hook->intgetter = intgetter;
	    hook->stringgetter = stringgetter;
	    hook->stringsetter = stringsetter;
	 }
	 else {
	    /* remove a hook */
	    *prev = hook->next;
	    free(hook->section);
	    free(hook);
	 }

	 return;
      }

      prev = &hook->next;
      hook = hook->next;
   }

   /* add a new hook */
   hook = malloc(sizeof(CONFIG_HOOK));
   if (!hook)
      return;

   hook->section = malloc(strlen(section_name)+1);
   if (!(hook->section)) {
      free(hook);
      return;
   }
   strcpy(hook->section, section_name);

   hook->intgetter = intgetter;
   hook->stringgetter = stringgetter;
   hook->stringsetter = stringsetter;

   hook->next = config_hook;
   config_hook = hook;
}



/* is_config_hooked:
 *  Checks whether a specific section is hooked in any way.
 */
int config_is_hooked(char *section)
{
   CONFIG_HOOK *hook = config_hook;
   char section_name[256];

   prettify_section_name(section, section_name);

   while (hook) {
      if (stricmp(section_name, hook->section) == 0)
	 return TRUE;

      hook = hook->next;
   }

   return FALSE;
}



/* find_config_string:
 *  Helper for finding an entry in the configuration file.
 */
static CONFIG_ENTRY *find_config_string(CONFIG *config, char *section, char *name, CONFIG_ENTRY **prev)
{
   CONFIG_ENTRY *p;
   int in_section = TRUE;

   if (config) {
      p = config->head;

      if (prev)
	 *prev = NULL;

      while (p) {
	 if (p->name) {
	    if ((section) && (p->name[0] == '[') && (p->name[strlen(p->name)-1] == ']')) {
	       /* change section */
	       in_section = (stricmp(section, p->name) == 0);
	    }
	    if ((in_section) || (name[0] == '[')) {
	       /* is this the one? */
	       if (stricmp(p->name, name) == 0)
		  return p;
	    }
	 }

	 if (prev)
	    *prev = p;

	 p = p->next;
      }
   }

   return NULL;
}



/* get_config_string:
 *  Reads a string from the configuration file.
 */
char *get_config_string(char *section, char *name, char *def)
{
   char section_name[256];
   CONFIG_HOOK *hook;
   CONFIG_ENTRY *p;

   init_config(TRUE);

   prettify_section_name(section, section_name);

   /* check for hooked sections */
   hook = config_hook;

   while (hook) {
      if (stricmp(section_name, hook->section) == 0) {
	 if (hook->stringgetter)
	    return hook->stringgetter(name, def);
	 else
	    return def;
      }
      hook = hook->next;
   }

   /* find the string */
   p = find_config_string(config_override, section_name, name, NULL);

   if (!p) {
      if ((name[0] == '#') || ((section_name[0] == '[') && (section_name[1] == '#')))
	 p = find_config_string(system_config, section_name, name, NULL);
      else
	 p = find_config_string(config[0], section_name, name, NULL);
   }

   if (p)
      return (p->data ? p->data : "");
   else
      return def;
}



/* get_config_int:
 *  Reads an integer from the configuration file.
 */
int get_config_int(char *section, char *name, int def)
{
   CONFIG_HOOK *hook;
   char section_name[256];
   char *s;

   prettify_section_name(section, section_name);

   /* check for hooked sections */
   hook = config_hook;

   while (hook) {
      if (stricmp(section_name, hook->section) == 0) {
	 if (hook->intgetter) {
	    return hook->intgetter(name, def);
	 }
	 else if (hook->stringgetter) {
	    s = hook->stringgetter(name, NULL);
	    if ((s) && (*s))
	       return strtol(s, NULL, 0);
	    else
	       return def;
	 }
	 else
	    return def;
      }
      hook = hook->next;
   }

   /* read normal data */
   s = get_config_string(section_name, name, NULL);

   if ((s) && (*s))
      return strtol(s, NULL, 0);

   return def;
}



/* get_config_hex:
 *  Reads a hexadecimal integer from the configuration file.
 */
int get_config_hex(char *section, char *name, int def)
{
   char *s = get_config_string(section, name, NULL);
   int i;

   if ((s) && (*s)) {
      i = strtol(s, NULL, 16);
      if ((i == 0x7FFFFFFF) && (stricmp(s, "7FFFFFFF") != 0))
	 i = -1;
      return i;
   }

   return def;
}



/* get_config_float:
 *  Reads a float from the configuration file.
 */
float get_config_float(char *section, char *name, float def)
{
   char *s = get_config_string(section, name, NULL);

   if ((s) && (*s))
      return atof(s);

   return def;
}



/* get_config_id:
 *  Reads a driver ID number from the configuration file.
 */
int get_config_id(char *section, char *name, int def)
{
   char *s = get_config_string(section, name, NULL);
   char tmp[4];
   char *endp;
   int val, i;

   if ((s) && (*s)) {
      val = strtol(s, &endp, 0);
      if (!*endp)
	 return val;

      tmp[0] = tmp[1] = tmp[2] = tmp[3] = ' ';

      for (i=0; i<4; i++) {
	 if (s[i])
	    tmp[i] = toupper(s[i]);
	 else
	    break;
      }

      return AL_ID(tmp[0], tmp[1], tmp[2], tmp[3]);
   }

   return def;
}



/* get_config_argv:
 *  Reads an argc/argv style token list from the configuration file.
 */
char **get_config_argv(char *section, char *name, int *argc)
{
   #define MAX_ARGV  16

   static char buf[256];
   static char *argv[MAX_ARGV];
   int pos, ac;

   char *s = get_config_string(section, name, NULL);

   if (!s) {
      *argc = 0;
      return NULL;
   }

   strcpy(buf, s);
   pos = 0;
   ac = 0;

   while ((ac<MAX_ARGV) && (buf[pos]) && (buf[pos] != '#')) {
      while ((buf[pos]) && (isspace(buf[pos])))
	 pos++;

      if ((buf[pos]) && (buf[pos] != '#')) {
	 argv[ac++] = buf+pos;

	 while ((buf[pos]) && (!isspace(buf[pos])))
	    pos++;

	 if (buf[pos])
	    buf[pos++] = 0;
      }
   }

   *argc = ac;
   return argv;
}



/* insert_variable:
 *  Helper for inserting a new variable into a configuration file.
 */
static CONFIG_ENTRY *insert_variable(CONFIG *the_config, CONFIG_ENTRY *p, char *name, char *data)
{
   CONFIG_ENTRY *n = malloc(sizeof(CONFIG_ENTRY));

   if (!n)
      return NULL;

   if (name) {
      n->name = malloc(strlen(name)+1);
      if (n->name)
	 strcpy(n->name, name);
   }
   else
      n->name = NULL;

   if (data) {
      n->data = malloc(strlen(data)+1);
      if (n->data)
	 strcpy(n->data, data);
   }
   else
      n->data = NULL;

   if (p) {
      n->next = p->next;
      p->next = n; 
   }
   else {
      n->next = NULL;
      the_config->head = n;
   }

   return n;
}



/* set_config_string:
 *  Writes a string to the configuration file.
 */
void set_config_string(char *section, char *name, char *val)
{
   CONFIG *the_config;
   CONFIG_HOOK *hook;
   CONFIG_ENTRY *p, *prev;
   char section_name[256];

   init_config(TRUE);

   prettify_section_name(section, section_name);

   /* check for hooked sections */
   hook = config_hook;

   while (hook) {
      if (stricmp(section_name, hook->section) == 0) {
	 if (hook->stringsetter)
	    hook->stringsetter(name, val);
	 return;
      }
      hook = hook->next;
   }

   /* decide which config file to use */
   if ((name[0] == '#') || ((section_name[0] == '[') && (section_name[1] == '#')))
      the_config = system_config;
   else
      the_config = config[0];

   if (the_config) {
      p = find_config_string(the_config, section_name, name, &prev);

      if (p) {
	 if ((val) && (*val)) {
	    /* modify existing variable */
	    if (p->data)
	       free(p->data);

	    p->data = malloc(strlen(val)+1);
	    if (p->data)
	       strcpy(p->data, val);
	 }
	 else {
	    /* delete variable */
	    if (p->name)
	       free(p->name);

	    if (p->data)
	       free(p->data);

	    if (prev)
	       prev->next = p->next;
	    else
	       the_config->head = p->next;

	    free(p);
	 }
      }
      else {
	 if ((val) && (*val)) {
	    /* add a new variable */
	    if (section_name[0]) {
	       p = find_config_string(the_config, NULL, section_name, &prev);

	       if (!p) {
		  /* create a new section */
		  p = the_config->head;
		  while ((p) && (p->next))
		     p = p->next;

		  if ((p) && (p->data) && (*p->data))
		     p = insert_variable(the_config, p, NULL, NULL);

		  p = insert_variable(the_config, p, section_name, NULL);
	       }

	       /* append to the end of the section */
	       while ((p) && (p->next) && 
		      (((p->next->name) && (*p->next->name)) || 
		       ((p->next->data) && (*p->next->data))))
		  p = p->next;

	       p = insert_variable(the_config, p, name, val);
	    }
	    else {
	       /* global variable */
	       p = the_config->head;
	       insert_variable(the_config, NULL, name, val);
	       the_config->head->next = p;
	    }
	 } 
      }

      the_config->dirty = TRUE;
   }
}



/* set_config_int:
 *  Writes an integer to the configuration file.
 */
void set_config_int(char *section, char *name, int val)
{
   char buf[32];
   sprintf(buf, "%d", val);
   set_config_string(section, name, buf);
}



/* set_config_hex:
 *  Writes a hexadecimal integer to the configuration file.
 */
void set_config_hex(char *section, char *name, int val)
{
   if (val >= 0) {
      char buf[32];
      sprintf(buf, "%X", val);
      set_config_string(section, name, buf);
   }
   else
      set_config_string(section, name, "-1");
}



/* set_config_float:
 *  Writes a float to the configuration file.
 */
void set_config_float(char *section, char *name, float val)
{
   char buf[32];
   sprintf(buf, "%f", val);
   set_config_string(section, name, buf);
}



/* set_config_id:
 *  Writes a driver ID to the configuration file.
 */
void set_config_id(char *section, char *name, int val)
{
   char buf[32];
   int i;

   if (val < 256) {
      sprintf(buf, "%d", val);
   }
   else {
      buf[0] = (val>>24)&0xFF;
      buf[1] = (val>>16)&0xFF;
      buf[2] = (val>>8)&0xFF;
      buf[3] = val&0xFF;
      buf[4] = 0;

      for (i=0; buf[i]; i++) {
	 if (buf[i] == ' ') {
	    buf[i] = 0;
	    break;
	 }
      }
   }

   set_config_string(section, name, buf);
}



/* try_text_location:
 *  Tries to read language data from the specified location, looking
 *  both for a regular config file and a language.dat containing the file.
 */
static int try_text_location(char *path, char *file)
{
   char buf[256], *s;

   /* try a regular file */
   append_filename(buf, path, file, sizeof(buf));

   if (file_exists(buf, FA_RDONLY | FA_ARCH, NULL)) {
      load_config_file(&config_language, buf, NULL);
      return TRUE;
   }

   /* try a datafile member */ 
   if (!strpbrk(file, "\\/#")) {
      sprintf(buf, "%slanguage.dat#%s", path, file);
      s = get_extension(buf);
      if ((s > buf) && (*(s-1) == '.'))
	 *(s-1) = '_';

      if (file_exists(buf, FA_RDONLY | FA_ARCH, NULL)) {
	 load_config_file(&config_language, buf, NULL);
	 return TRUE;
      }
   }

   return FALSE;
}



/* _load_config_text:
 *  Reads in a block of translated system text, looking for either a
 *  user-specified file, a ??text.cfg file, or a language.dat#??TEXT_CFG 
 *  datafile object.
 */
void _load_config_text()
{
   char *name = get_config_string(NULL, "language", NULL);
   char buf[256], path[256], *s;

   if (config_language) {
      destroy_config(config_language);
      config_language = NULL;
   }

   if ((!name) || (!name[0]))
      return;

   /* fully qualified path? */
   if (strpbrk(name, "\\/#")) {
      try_text_location("", name);
      return;
   }

   /* try in same dir as the program */
   strcpy(buf, name);
   s = get_extension(buf);
   if ((s <= buf) || (*(s-1) != '.')) {
      if ((s < buf+4) || (stricmp(s-4, "text") != 0))
	 strcpy(s, "text.cfg");
      else
	 strcpy(s, ".cfg");
   }

   replace_filename(path, __crt0_argv[0], "", sizeof(path));

   if (try_text_location(path, buf))
      return;

   /* try the ALLEGRO environment variable */
   s = getenv("ALLEGRO");
   if (s) {
      append_filename(path, s, "", sizeof(path));

      if (try_text_location(path, buf))
	 return; 
   }
}



/* get_config_text:
 *  Looks up a translated version of the specified English string,
 *  returning a suitable message in the current language if one is
 *  available, or a copy of the parameter if no translation can be found.
 */
char *get_config_text(char *msg)
{
   static char section[] = "[language]";
   char name[256];
   CONFIG_HOOK *hook;
   CONFIG_ENTRY *p;
   int i;

   init_config(TRUE);

   for (i=0; msg[i]; i++) {
      if ((isspace(msg[i])) || (msg[i] == '=') || (msg[i] == '#'))
	 name[i] = '_';
      else
	 name[i] = msg[i]; 
   }

   name[i] = 0;

   /* check for hooked sections */
   hook = config_hook;

   while (hook) {
      if (stricmp(section, hook->section) == 0) {
	 if (hook->stringgetter)
	    return hook->stringgetter(name, msg);
	 else
	    return msg;
      }
      hook = hook->next;
   }

   /* find the string */
   p = find_config_string(config_override, section, name, NULL);

   if (!p) {
      p = find_config_string(config[0], section, name, NULL);

      if (!p)
	 p = find_config_string(config_language, section, name, NULL);
   }

   if (p)
      return (p->data ? p->data : "");
   else
      return msg;
}

