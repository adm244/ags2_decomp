/*
   WGT -> Allegro portability interface (c) 1998 Chris Jones
*/
#define _WGT45_

#ifndef __WGT4_H
#define __WGT4_H

#ifdef _MSC_VER
#ifndef WINDOWS_VERSION
#define WINDOWS_VERSION
#endif
#endif

#include <stdio.h>
#include "allegro.h"

#ifndef WINDOWS_VERSION
#include <dos.h>
#include <io.h>
#endif

#include <stdarg.h>

#ifdef WINDOWS_VERSION
#include "winalleg.h"
#endif

typedef BITMAP * block;
//typedef FONT * wgtfont;
typedef unsigned char*  wgtfont;

#if (WGTMAP_SIZE==1)
  typedef unsigned char * wgtmap;
#else
  typedef short * wgtmap;
#endif

#define color RGB
#define cbuttfont fonts[0]
#define TEXTFG   0
#define TEXTBG   1
#define TEXTFGBG 2

#define fpos_t unsigned long
#ifdef __cplusplus
extern "C"
{
#endif

#ifdef WINDOWS_VERSION
#include <time.h>
  struct time
  {
    int ti_hund, ti_sec, ti_min, ti_hour;
  };
#endif

#ifndef WGT2ALLEGRO_NOFUNCTIONS
  char *wgt2allgcopyright="WGT2Allegro (c) 1997,1998 Chris Jones";
  fpos_t lfpos;
  FILE *libf;
  short lresult;
  int   lsize;
  char  password[16];
  char  *wgtlibrary;
  unsigned char currentcolor;
  unsigned char textcol;
  int vesa_xres,vesa_yres;
  block abuf;

#ifdef WINDOWS_VERSION
#define GFX_VGA GFX_DIRECTX
#endif

  void vga256()
  {
    allegro_init();
    set_gfx_mode(GFX_VGA, 320, 200, 320, 200);
    abuf = screen;
    vesa_xres = 320;
    vesa_yres = 200;
  }

#ifndef WINDOWS_VERSION
  union REGS r;
  void wsetmode(int nnn)
  {
    r.x.ax = nnn;
    int86(0x10, &r, &r);
  }

  int wgetmode()
  {
    r.x.ax = 0x0f00;
    int86(0x10, &r, &r);
    return r.h.al;
  }
#endif

#ifdef USE_CLIB
  extern "C"
  {
    extern FILE *clibfopen(char *, char *);
    extern long cliboffset(char *);
    extern long clibfilesize(char *);
    extern long last_opened_size;
  }
#define fopen clibfopen
#endif

  void wtextcolor(int coo)
  {
    textcol = coo;
  }

  void wsetscreen(block nss)
  {
    if (nss == NULL)
      abuf = screen;
    else
      abuf = nss;
  }

  void wsetrgb(int coll, int r, int g, int b, color * pall)
  {
    pall[coll].r = r;
    pall[coll].g = g;
    pall[coll].b = b;
  }

  int wloadpalette(char *filnam, color * pall)
  {
    FILE *fff;
    int kk;

    fff = fopen(filnam, "rb");
    if (fff == NULL)
      return -1;

    for (kk = 0; kk < 256; kk++)        // there's a filler byte
      fread(&pall[kk], 3, 1, fff);

    fclose(fff);
    return 0;
  }

  void wcolrotate(unsigned char start, unsigned char finish, int dir, color * pall)
  {
    int jj;
    color tempp;
    
    tempp = pall[start];
    for (jj = start; jj < finish; jj++)
      pall[jj]=pall[jj + 1];

    pall[finish] = tempp;
  }

  block tempbitm;
  block wnewblock(int x1, int y1, int x2, int y2)
  {
    int twid = (x2 - x1) + 1, thit = (y2 - y1) + 1;

    if (twid < 1)
      twid = 1;

    if (thit < 1)
      thit = 1;

    tempbitm = create_bitmap(twid, thit);

    if (tempbitm == NULL)
      return NULL;

    blit(abuf, tempbitm, x1, y1, 0, 0, tempbitm->w, tempbitm->h);
    return tempbitm;
  }

  short getshort(FILE * fff)
  {
    short sss;
    fread(&sss, 2, 1, fff);
    return sss;
  }

  block wloadblock(char *fill)
  {
    short widd, hitt;
    FILE *fff = fopen(fill, "rb");
    int ff;

    if (fff == NULL)
      return NULL;

    widd = getshort(fff);
    hitt = getshort(fff);
    tempbitm = create_bitmap(widd, hitt);

    for (ff = 0; ff < hitt; ff++)
      fread(&tempbitm->line[ff][0], widd, 1, fff);

    fclose(fff);
    return tempbitm;
  }

  int wloadsprites(color * pall, char *filnam, block * sarray, int strt, int eend)
  {
    FILE *ff;
    int vers;
    char buff[20];
    int numspri = 0, vv, wdd, htt;
    int hh;

    ff = fopen(filnam, "rb");
    if (ff == NULL)
      return -1;

    vers = getshort(ff);
    fread(&buff[0], 13, 1, ff);
    for (vv = 0; vv < 256; vv++)        // there's a filler byte
      fread(&pall[vv], 3, 1, ff);

    if (vers > 4)
      return -1;

    if (vers == 4)
      numspri = getshort(ff);
    else {
      getshort(ff);
      numspri = 200;
    }

    for (vv = strt; vv <= eend; vv++)
      sarray[vv] = NULL;

    for (vv = 0; vv <= numspri; vv++) {
      if (getshort(ff) == 0) {
        sarray[vv] = NULL;
        if (feof(ff))
          break;

        continue;
      }

      if (feof(ff))
        break;

      if (vv > eend)
        break;

      wdd = getshort(ff);
      htt = getshort(ff);
      if (vv < strt) {
        fseek(ff, wdd * htt, SEEK_CUR);
        continue;
      }
      sarray[vv] = create_bitmap(wdd, htt);

      if (sarray[vv] == NULL) {
        fclose(ff);
        return -1;
      }

      for (hh = 0; hh < htt; hh++)
        fread(&sarray[vv]->line[hh][0], wdd, 1, ff);
    }
    fclose(ff);
    return 0;
  }

  void wfreesprites(block * blar, int stt, int end)
  {
    int hh;

    for (hh = stt; hh <= end; hh++) {
      if (blar[hh] != NULL)
        destroy_bitmap(blar[hh]);

      blar[hh] = NULL;
    }
  }

  void wsavesprites(color * pll, char *fnm, block * spre, int strt, int eend)
  {
    FILE *ooo = fopen(fnm, "wb");
    short topu = 4;
    int aa, lastsp = 0;
    char *spsig = " Sprite File ";

    fwrite(&topu, 2, 1, ooo);
    fwrite(spsig, 13, 1, ooo);

    for (aa = 0; aa < 256; aa++)
      fwrite(&pll[aa], 3, 1, ooo);

    for (aa = strt; aa <= eend; aa++) {
      if (spre[aa] != NULL)
        lastsp = aa;
    }

    topu = lastsp;
    fwrite(&topu, 2, 1, ooo);

    for (aa = strt; aa <= lastsp; aa++) {
      if (spre[aa] == NULL) {
        topu = 0;
        fwrite(&topu, 2, 1, ooo);
        continue;
      }

      topu = 1;
      fwrite(&topu, 2, 1, ooo);

      topu = spre[aa]->w;
      fwrite(&topu, 2, 1, ooo);

      topu = spre[aa]->h;
      fwrite(&topu, 2, 1, ooo);

      fwrite(&spre[aa]->line[0][0], spre[aa]->w, spre[aa]->h, ooo);
    }
    fclose(ooo);
  }

  char *testst = "WGT Font File  ";
  wgtfont tempalloc;
  wgtfont wloadfont(char *filnm)
  {
    FILE *ffi = NULL;
    char mbuffer[16];
    long lenof;

    ffi = fopen(filnm, "rb");
    if (ffi == NULL)
      return NULL;

    mbuffer[15] = 0;
    fread(mbuffer, 15, 1, ffi);
    if (strcmp(mbuffer, testst) != 0) {
      fclose(ffi);
      return NULL;
    }

    lenof = filelength(fileno(ffi));
    // clibfopen will have set last_opened_size
    lenof = last_opened_size;

    tempalloc = (wgtfont) malloc(lenof + 40);
    fclose(ffi);

    ffi = fopen(filnm, "rb");
    fread(tempalloc, lenof, 1, ffi);
    fclose(ffi);
    return tempalloc;
  }

  // text output stuff
  int texttrans = 0;
  int textbgcol = 0;
  void wtextbackground(int coll)
  {
    textbgcol = coll;
  }

  void wtexttransparent(int coo)
  {
    texttrans = coo;
  }

#define fastputpix(xl, yl) putpixel(abuf, xl, yl, textcol)

  int printchar(int xxx, int yyy, wgtfont foo, int charr)
  {
    if ((charr > 127) | (charr < 0))
      return 0;

    short *tabaddr = (short *)&foo[15];
    tabaddr = (short *)&foo[tabaddr[0]];   // get address table
    tabaddr = (short *)&foo[tabaddr[charr]];  // use table to find character

    unsigned char *actdata;
    actdata = (unsigned char *)&tabaddr[2];

    int tt,ss;
    int bytewid=((tabaddr[0] - 1) / 8) + 1;

    for (tt = 0; tt < tabaddr[1]; tt++) {
      for (ss = 0; ss < tabaddr[0]; ss++) {
        if (((actdata[tt * bytewid + (ss / 8)] & (0x80 >> (ss % 8))) != 0) & (texttrans != TEXTBG))
          fastputpix(xxx + ss, yyy + tt);
        else if (texttrans != TEXTFG)
          putpixel(abuf, xxx + ss, yyy + tt, textbgcol);
      }
    }
    return tabaddr[0];
  }

  int wgettextwidth(char *texx, wgtfont foon)
  {
    short *tabaddr;
    int totlen = 0;
    unsigned int dd;

    for (dd = 0; dd < strlen(texx); dd++) {
      tabaddr = (short *)&foon[15];
      tabaddr = (short *)&foon[tabaddr[0]];     // get address table
      tabaddr = (short *)&foon[tabaddr[texx[dd]]];      // use table to find character
      totlen += tabaddr[0];
    }
    return totlen;
  }

  int wgettextheight(char *texx, wgtfont foon) {
    short *tabaddr;
    int highest = 0;
    unsigned int dd;

    for (dd = 0; dd < strlen(texx); dd++) {
      tabaddr = (short *)&foon[15];
      tabaddr = (short *)&foon[tabaddr[0]];     // get address table
      tabaddr = (short *)&foon[tabaddr[texx[dd]]];      // use table to find character

      if (tabaddr[1] > highest)
        highest = tabaddr[1];
    }
    return highest;
  }

  void wouttextxy(int xxx, int yyy, wgtfont fonn, char *texx)
  {
    unsigned int ee;

    if (yyy > abuf->cb)
      return;  // each char is clipped but this speeds it up

    for (ee = 0; ee < strlen(texx); ee++)
      xxx += printchar(xxx, yyy, fonn, texx[ee]);
  }

  void wgtprintf(int xxx, int yyy, wgtfont fonn, char *fmt, ...)
  {
    char tbuffer[2000];
    va_list ap;

    va_start(ap, fmt);
    vsprintf(tbuffer, fmt, ap);
    va_end(ap);
    wouttextxy(xxx, yyy, fonn, tbuffer);
  }

  void wputblock(int xx, int yy, block bll, int xray)
  {
    if (xray)
      draw_sprite(abuf, bll, xx, yy);
    else
      blit(bll, abuf, 0, 0, xx, yy, bll->w, bll->h);
  }

  int __wremap_keep_transparent = 1;
  void wremap(color * pal1, block picc, color * pal2)
  {
    int jj;
    unsigned char color_mapped_table[256];

    for (jj = 1; jj < 256; jj++)
      color_mapped_table[jj] = bestfit_color(pal2, pal1[jj].r, pal1[jj].g, pal1[jj].b);

    if (__wremap_keep_transparent > 0)
      color_mapped_table[0] = 0;  // keep transparency

    for (jj = 0; jj < (picc->w) * (picc->h); jj++) {
      int xxl = jj % (picc->w), yyl = jj / (picc->w);
      int rr = getpixel(picc, xxl, yyl);
      putpixel(picc, xxl, yyl, color_mapped_table[rr]);
    }
  }

  void wremapall(color * pal1, block picc, color * pal2)
  {
    __wremap_keep_transparent--;
    wremap(pal1, picc, pal2);
    __wremap_keep_transparent++;
  }

  // library file functions
  void readheader()
  {
  }

  void findfile(char *filnam)
  {
    // should set lfpos = offset of file and set lresult = 1
    // or set lresult = 0 on failure (and exit with message)
  }

  int checkpassword(char *passw)
  {
    return 0;                   // 0 = incorrect   ! = 0 correct
  }

// #ifdef WINDOWS_VERSION
//   void gettime(struct time *tpt)
//   {
//     time_t ltt;
//     struct tm *tis;

//     time(&ltt);
//     tis = localtime(&ltt);
//     tpt->ti_sec = tis->tm_sec;
//     tpt->ti_min = tis->tm_min;
//     tpt->ti_hour = tis->tm_hour;
//     tpt->ti_hund = 0;
//   }
// #endif

  long wtimer(struct time tt1, struct time tt2)
  {
    long timm1 = tt1.ti_hund + (long)tt1.ti_sec * 100 + (long)tt1.ti_min * 6000 + (long)tt1.ti_hour * 6000 * 60;

    long timm2 = tt2.ti_hund + (long)tt2.ti_sec * 100 + (long)tt2.ti_min * 6000 + (long)tt2.ti_hour * 6000 * 60;

    return timm2 - timm1;
  }

  void wcopyscreen(int x1, int y1, int x2, int y2, block src, int dx, int dy, block dest)
  {
    if (src == NULL)
      src = screen;

    if (dest == NULL)
      dest = screen;

    blit(src, dest, x1, y1, dx, dy, (x2 - x1) + 1, (y2 - y1) + 1);
  }

#ifdef USE_CLIB
#undef fopen
#endif
#else

  extern fpos_t lfpos;
  extern FILE *libf;
  extern short lresult;
  extern int lsize;
  extern char password[16];
  extern char *wgtlibrary;

  extern void readheader();
  extern void findfile(char *);
  extern int checkpassword(char *);

  extern unsigned char currentcolor;
  extern unsigned char textcol;
  extern block abuf;
  extern int vesa_xres, vesa_yres;

  extern void vga256();
  extern void wbutt(int, int, int, int);
  extern void wsetmode(int);
  extern int wgetmode();
  extern void wtextcolor(int);
  extern void wgtprintf(int, int, wgtfont, char *, ...);
  extern void wsetscreen(block);
  extern void wnormscreen();
  extern void wcopyscreen(int, int, int, int, block, int, int, block);
  extern void wsetrgb(int, int, int, int, color *);
  extern int wloadpalette(char *, color *);
  extern void wcolrotate(unsigned char, unsigned char, int, color *);
  extern block wnewblock(int, int, int, int);
  extern void wfreesprites(block *, int, int);
  extern block wloadblock(char *);
  extern int wloadsprites(color *, char *, block *, int, int);
  extern wgtfont wloadfont(char *);
  extern void wouttextxy(int, int, wgtfont, char *);
  extern void wgtprintf(int, int, wgtfont, char *, ...);
  extern int wgettextheight(char *, wgtfont);
  extern int wgettextwidth(char *, wgtfont);
  extern void wputblock(int, int, block, int);
  extern void wremap(color *, block, color *);
  extern void wtextbackground(int);
  extern void wtexttransparent(int);
  extern long wtimer(struct time, struct time);
  extern void gettime(struct time *);

#endif
#ifdef __cplusplus
}
#endif

#define tx    abuf->cl
#define ty    abuf->ct
//#define bx abuf->cr   // can't do this because of REGS.bx
#define by    abuf->cb
#define kbdon key

#define installkbd()          install_keyboard()
#define uninstallkbd()        remove_keyboard()
#define wallocblock(wii,hii)  create_bitmap(wii,hii)
#define wbar(x1, y1, x2, y2)  rectfill(abuf, x1, y1, x2, y2, currentcolor)
#define wclip(x1, y1, x2, y2) set_clip(abuf, x1, y1, x2, y2)
#define wcls(coll)            clear_to_color(abuf,coll)

#define wfade_in(from, to, speed, pal)  fade_in_range(pal, 5 /* 64 - speed * 7 */, from, to)
#define wfade_out(from, to, speed, pal) fade_out_range(5, from, to)
#define wfastputpixel(x1, y1)           _putpixel(abuf, x1, y1, currentcolor)
#define wfreeblock(bll)                 destroy_bitmap(bll)
#define wfreefont(foo)                  free(foo)
#define wgetblockheight(bll)            bll->h
#define wgetblockwidth(bll)             bll->w
#define wgetpixel(xx, yy)               getpixel(abuf, xx, yy)
#define whline(x1, x2, yy)              hline(abuf, x1, yy, x2, currentcolor)
#define wline(x1, y1, x2, y2)           line(abuf, x1, y1, x2, y2, currentcolor)
#define wloadpcx256(fnm, pall)          load_pcx(fnm, pall)
#define wnormscreen()                   abuf = screen
#define wputpixel(x1, y1)               putpixel(abuf, x1, y1, currentcolor)
#define wreadpalette(from, to, dd)      get_palette_range(dd, from, to)
#define wrectangle(x1, y1, x2, y2)      rect(abuf, x1, y1, x2, y2, currentcolor)
#define wregionfill(xx, yy)             floodfill(abuf, xx, yy, currentcolor)
#define wretrace()                      vsync()
#define wsetcolor(coll)                 currentcolor = coll
#define setlib(lll)                     csetlib(lll, "")
#define wsetpalette(from, to, pall)     set_palette_range(pall, from, to, 0)
#define vgadetected()                   1

#ifndef WGT2ALLEGRO_NOFUNCTIONS
#ifdef __cplusplus
extern "C"
{
#endif
  void wbutt(int x1, int y1, int x2, int y2)
  {
    wsetcolor(254);
    wbar(x1, y1, x2, y2);
    wsetcolor(253);
    whline(x1 - 1, x2 + 1, y1 - 1);
    wline(x1 - 1, y1 - 1, x1 - 1, y2 + 1);
    wsetcolor(255);
    whline(x1 - 1, x2 + 1, y2 + 1);
    wline(x2 + 1, y1 - 1, x2 + 1, y2 + 1);
  }
#ifdef __cplusplus
}
#endif
#endif

// now define the wvesa_xxx to the normal names, since we use SVGA normally
#define wvesa_bar       wbar
#define wvesa_clip      wclip
#define wvesa_cls       wcls
#define wvesa_outtextxy wouttextxy
#define wvesa_rectangle wrectangle

#define XRAY    1
#define NORMAL  0

#endif
