#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
  extern FILE*clibfopen(char*,char*);
}

extern void quit(char*);

char lang_file[20]="notinitialized";
int lang_read_count=0;
int lang_index=-1;
char lang_default[8]="en";
char*lang_text=NULL;
char lang_text_buffer1[250];
char lang_text_buffer2[250];

char*get_language_text(int indxx)
{
  if (indxx==lang_index) return lang_text;
  lang_index=indxx;
  FILE*fff=clibfopen(lang_file,"rt");
  if ((lang_read_count % 2)==0) lang_text=lang_text_buffer1;
  else lang_text=lang_text_buffer2;
  lang_read_count++;
  if (fff==NULL) quit("Language file not found. Version may be out of date.");
  lang_text[0]=0;
  char buffr[200];
  int idx;
  for (;;) {
try_again:
    if (feof(fff)) goto notfound;
    fgets(buffr,199,fff);
    if (strnicmp(buffr,"end_of_file",5)==0) goto notfound;
    if (buffr[0]==';') goto try_again;
    sscanf(buffr,"%d %[^\n]s",&idx,lang_text);
    if (idx!=indxx) goto try_again;
    while (lang_text[strlen(lang_text)-1]=='\\') {
      sprintf(lang_text+(strlen(lang_text)-1),"\n");
      fgets(lang_text+strlen(lang_text),100,fff);
      lang_text[strlen(lang_text)-1]=0;
    }
    fclose(fff);
    return lang_text;
  }
notfound:
  fclose(fff);
  return "[language string not found]";
}

void init_language_text(char*deflang)
{
  char*langenv=getenv("LANGUAGE");
  if (langenv==NULL) langenv=deflang;
  if (langenv==NULL) langenv=lang_default;
try_again:
  sprintf(lang_file,"aclang%s.dat",langenv);
  FILE*iii=clibfopen(lang_file,"rb");
  if (iii==NULL) {
    if (stricmp(langenv,"en")==0) {
      printf("Could not load the language definition file. Check you have a valid\n"
        "copy of the Adventure Creator, and that the LANGUAGE environment is valid.\n");
      exit(9);
    }
    else {
      if (langenv==lang_default) langenv="en";
      else {
        printf("Language definitions for language '%s' were not found. Using English\n"
          "defaults instead.\n",langenv);
        getch();
        langenv=lang_default;
      }
      goto try_again;
    }
  }
  fclose(iii);
}
