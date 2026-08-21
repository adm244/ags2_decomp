#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <io.h>

char clib32copyright[] = "CLIB32 v1.1 (c) 1995,1996,1998 Chris Jones";
char lib_file_name[80] = " ";
char clbuff[20];
#define MAX_FILES 80

struct CLIBFile {
  short numfiles;
  char *filenamebuf;
  char *filenames[MAX_FILES];
  long filesizes[MAX_FILES];
  long datastart;
};

CLIBFile usethis;
char *clibendfilesig = "CLIB\x1\x2\x3\x4SIGE";

extern "C"
{
  long last_opened_size;

  int csetlib(char *namm, char *passw)
  {
    if (namm == NULL) {
      lib_file_name[0] = ' ';
      lib_file_name[1] = 0;
      return 0;
    }

    CLIBFile *clib = &usethis;
    int passwmodifier = 0;
    FILE *fff = fopen(namm, "rb");
    if (fff == NULL)
      return -1;

    long absoffs = 0;
    fread(&clbuff[0], 5, 1, fff);

    if (strncmp(clbuff, "CLIB", 4) != 0) {
      fseek(fff, -12, SEEK_END);
      fread(&clbuff[0], 12, 1, fff);

      if (strncmp(clbuff, clibendfilesig, 12) != 0)
        return -2;

      fseek(fff, -16, SEEK_END);  // it's an appended-to-end-of-exe thing
      fread(&absoffs, 4, 1, fff);
      fseek(fff, absoffs + 5, SEEK_SET);
    }

    if (fgetc(fff) != 6)
      return -3; // unsupported version

    passwmodifier = fgetc(fff);
    fgetc(fff); // unused byte
    fread(&clib->numfiles, 2, 1, fff);
    fread(clbuff, 13, 1, fff);  // skip password dooberry

    clib->filenamebuf = (char *)malloc(clib->numfiles * 13);
    int aa;
    for (aa = 0; aa < clib->numfiles; aa++) {
      clib->filenames[aa] = clib->filenamebuf+aa*13;
    }
    fread(clib->filenames[0], 13, clib->numfiles, fff);
    fread(clib->filesizes, 4, clib->numfiles, fff);
    fseek(fff, 2 * clib->numfiles, SEEK_CUR);  // skip flags & ratio

    clib->datastart = ftell(fff);
    for (aa = 0; aa < clib->numfiles * 13; aa++) {
      if (clib->filenamebuf[aa]) {
        clib->filenamebuf[aa] -= passwmodifier;
      }
    }
    strcpy(lib_file_name, namm);
    fclose(fff);

    return 0;
  }

  long clibfilesize(char *fill)
  {
    if (lib_file_name[0] == ' ')
      return -1;

    int bb;
    long offs=usethis.datastart;
    for (bb = 0; bb < usethis.numfiles; bb++) {
      if (stricmp(usethis.filenames[bb], fill) == 0)
        return usethis.filesizes[bb];
    }
    return -1;
  }

  long cliboffset(char *fill)
  {
    if (lib_file_name[0] == ' ')
      return -1;

    int bb;
    long offs=usethis.datastart;
    for (bb = 0; bb < usethis.numfiles; bb++) {
      if (stricmp(usethis.filenames[bb], fill) == 0) {
        bb = 1000;
        break;
      }
      offs += usethis.filesizes[bb];
    }

    if (bb < 999)
      return -1;

    return offs;
  }

#define PR_DATAFIRST 1
#define PR_FILEFIRST 2
  int cfopenpriority = PR_DATAFIRST;
  FILE *tfil;

  FILE *clibfopen(char *filnamm, char *fmt)
  {
    last_opened_size = -1;
    if (cfopenpriority == PR_FILEFIRST) {
      // check for file, otherwise use datafile
      if (fmt[0] != 'r') {
        tfil = fopen(filnamm, fmt);
      } else {
        tfil = fopen(filnamm, fmt);

        if ((tfil == NULL) && (lib_file_name[0] != ' ') && (cliboffset(filnamm) >= 0)) {
          tfil = fopen(lib_file_name, fmt);
          fseek(tfil, cliboffset(filnamm), SEEK_SET);
          last_opened_size = clibfilesize(filnamm);
        }
      }

    } else {
      // check datafile first, then scan directory
      if ((cliboffset(filnamm) < 1) | (fmt[0] != 'r'))
        tfil = fopen(filnamm, fmt);
      else {
        tfil = fopen(lib_file_name, fmt);
        fseek(tfil, cliboffset(filnamm), SEEK_SET);
        last_opened_size = clibfilesize(filnamm);
      }

    }

    if (last_opened_size < 0)
      last_opened_size = filelength(fileno(tfil));

    return tfil;
  }
} // extern "C"
