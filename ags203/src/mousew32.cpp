/* MOUSELIBW32.CPP

  Library of mouse functions for graphics and text mode

  (c) 1994 Chris Jones

  Win32 (allegro) update (c) 1999 Chris Jones
*/

#include <dos.h>
#include <conio.h>
#include <process.h>

#include <stdio.h>

#define WGT2ALLEGRO_NOFUNCTIONS
#include "wgt2allg.h"
#ifndef WINDOWS_VERSION
#error This is the Windows 32-bit version
#endif

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#define MAXCURSORS 20
/*
int  minstalled();    // this returns number of buttons (or 0)
void minst();   // this exits if not installed
void mshow();
void mhide();
int  mgetbutton();
void mchangestyle(int,int);
void mgetpos();
void mgetgraphpos();
void msetpos(int,int);
void msetgraphpos(int,int);
void mconfine(int,int,int,int); // left top right bottom
void mgraphconfine(int,int,int,int);
int  mbutrelease(int);
void domouse(int=0);   // graphics mode cursor
void mfreemem();
void mloadcursor(char*);  // load from file
void mloadwcursor(char*);
void mnewcursor(char);
int  ismouseinbox(int,int,int,int);
void msethotspot(int,int);   // Graphics mode only. Useful for crosshair.
*/

extern long cliboffset(char*);
extern char lib_file_name[13];

char *mouselibcopyr = "MouseLib32 (c) 1994, 1998 Chris Jones";
const int NONE = -1, LEFT = 0, RIGHT = 1, MIDDLE = 2;
int aa;
char mouseturnedon = FALSE, currentcursor = 0;
int mousex = 0, mousey = 0, numcurso = -1, hotx = 0, hoty = 0;
block savebk, mousecurs[MAXCURSORS];
extern int vesa_xres, vesa_yres;

void mgetgraphpos()
{
  poll_mouse();
  mousex = mouse_x;
  mousey = mouse_y;

  if (mousex >= vesa_xres)
    mousex = vesa_xres - 1;
}

int hotxwas = 0, hotywas = 0;
void domouse(int str)
{
  /*
     TO USE THIS ROUTINE YOU MUST LOAD A MOUSE CURSOR USING mloadcursor.
     YOU MUST ALSO REMEMBER TO CALL mfreemem AT THE END OF THE PROGRAM.
  */
  int poow = wgetblockwidth(mousecurs[currentcursor]);
  int pooh = wgetblockheight(mousecurs[currentcursor]);
  int smx = mousex - hotxwas, smy = mousey - hotywas;

  mgetgraphpos();
  mousex -= hotx;
  mousey -= hoty;

  if (mousex + poow >= vesa_xres)
    poow = vesa_xres - mousex;

  if (mousey + pooh >= vesa_yres)
    pooh = vesa_yres - mousey;

  wclip(0, 0, vesa_xres - 1, vesa_yres - 1);
  if ((str == 0) & (mouseturnedon == TRUE)) {
    if ((mousex != smx) | (mousey != smy)) {    // the mouse has moved
      wputblock(smx, smy, savebk, 0);
      wfreeblock(savebk);
      savebk = wnewblock(mousex, mousey, mousex + poow, mousey + pooh);
      wputblock(mousex, mousey, mousecurs[currentcursor], 1);
    }
  }
  else if ((str == 1) & (mouseturnedon == FALSE)) {
    // the mouse is just being turned on
    savebk = wnewblock(mousex, mousey, mousex + poow, mousey + pooh);
    wputblock(mousex, mousey, mousecurs[currentcursor], 1);
    mouseturnedon = TRUE;
  }
  else if ((str == 2) & (mouseturnedon == TRUE)) {    // the mouse is being turned off
    wputblock(smx, smy, savebk, 0);
    wfreeblock(savebk);
    mouseturnedon = FALSE;
  }

  mousex += hotx;
  mousey += hoty;
  hotxwas = hotx;
  hotywas = hoty;
}

int ismouseinbox(int lf, int tp, int rt, int bt)
{
  if ((mousex >= lf) & (mousex <= rt) & (mousey >= tp) & (mousey <= bt))
    return TRUE;
  else
    return FALSE;
}

void mfreemem()
{
  for (int re = 0; re < numcurso; re++) {
    if (mousecurs[re] != NULL)
      wfreeblock(mousecurs[re]);
  }
}

void mnewcursor(char cursno)
{
  domouse(2);
  currentcursor = cursno;
  domouse(1);
}

void mloadwcursor(char *namm)
{
  color dummypal[256];
  if (wloadsprites(&dummypal[0], namm, mousecurs, 0, MAXCURSORS)) {
    // printf("C_Load_wCursor: Error reading mouse cursor file\n");
    exit(1);
  }
}

int butwas = 0;
int mgetbutton()
{
  int toret = NONE;
  poll_mouse();
  int butis = mouse_b;

  if ((butis > 0) & (butwas > 0))
    return NONE;  // don't allow holding button down

  if (butis & 1)
    toret = LEFT;
  else if (butis & 2)
    toret = RIGHT;
  else if (butis & 4)
    toret = MIDDLE;

  butwas = butis;
  return toret;
}

const int MB_ARRAY[3] = { 1, 2, 4 };
int misbuttondown(int buno)
{
  poll_mouse();
  if (mouse_b & MB_ARRAY[buno])
    return TRUE;
  return FALSE;
}

void mgraphconfine(int x1, int y1, int x2, int y2)
{
  x1 *= (vesa_xres / 320);
  x2 *= (vesa_xres / 320);
  y1 *= (vesa_yres / 200);
  y2 *= (vesa_yres / 200);
  set_mouse_range(x1, y1, x2, y2);
}

void msetgraphpos(int xa, int ya)
{ 
  position_mouse(xa, ya); // xa -= hotx; ya -= hoty;
}

void msethotspot(int xx, int yy)
{
  hotx = xx;  // mousex -= hotx; mousey -= hoty;
  hoty = yy;  // mousex += hotx; mousey += hoty;
}

int minstalled()
{
  int nbuts;
  if ((nbuts = install_mouse()) < 1)
    return 0;

  if (nbuts < 2)
    nbuts = 2;

  return nbuts;
}
