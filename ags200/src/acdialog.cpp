// DIALOG SYSTEM STUFF below
#include <cstdio>
#define WGT2ALLEGRO_NOFUNCTIONS
#include "wgt2allg.h"
#include <string.h>
#include <conio.h>
#include <dir.h>

const int NONE=-1,LEFT=0,RIGHT=1;

#define MSG_OK            25
#define MSG_CANCEL        26
#define MSG_LOSEGAME      27
#define MSG_INVTEXT       28
#define MSG_SCORE         29
#define MSG_FATALERROR    30
#define MSG_RESTORE       31
#define MSG_SAVEBUTTON    32
#define MSG_REPLACE       33
#define MSG_REPLACEWITH1  34
#define MSG_REPLACEWITH2  35
#define MSG_CONTINUE      36
#define MSG_QUITBUTTON    37
#define MSG_PLAYBUTTON    38

#define MSG_SCRIPTERROR   40
#define MSG_WRONGSAVE     41

#define MSG_SELECTLOAD    50
#define MSG_SAVEDIALOG    51
#define MSG_MUSTREPLACE   52
#define MSG_QUITDIALOG    54

// Setup main menu
#define MSG_SETUP_DIGI    205
#define MSG_SETUP_MIDI    206
#define MSG_SETUP_SCRN    207
#define MSG_SETUP_EXIT    208
// introduction messages
#define MSG_INTRODIALOG   300
#define MSG_SKIPBUTTON    301
#define MSG_WATCHBUTTON   302
#define MSG_RESTOREBUTTON 303

struct GameSetup
{
  int digicard, midicard;
  int screenres;
  int mod_player;
  int textheight;
  int mp3_player;
};

extern void quit(char *);
extern int mousex, mousey;
extern int sxmult, symult;
extern void domouse(int);
extern int  mgetbutton();
extern void poll_mp3();
extern int misbuttondown(int);
extern char *get_language_text(int);
extern int sgsiglen;
extern GameSetup usetup;
extern const char* sgnametemplate;

// char buff[200];
int myscrnwid=320,myscrnhit=200;

/*#define COL251 26
#define COL252 28
#define COL253 29
#define COL254 27
#define COL255 24*/
#define COL253 15
#define COL254 7
#define COL255 8

void __my_wbutt(int x1, int y1, int x2, int y2)
{
  wsetcolor(COL254);            //wsetcolor(15);
  wbar(x1, y1, x2, y2);
  wsetcolor(0);
  wrectangle(x1, y1, x2, y2);
}

#define wbutt __my_wbutt

#define mbutrelease !misbuttondown
#define TEXT_HT usetup.textheight

//  =========  DEFINES  ========
// Control types
#define CNT_PUSHBUTTON 0x001
#define CNT_LISTBOX    0x002
#define CNT_LABEL      0x003
#define CNT_TEXTBOX    0x004
// Control properties
#define CNF_DEFAULT    0x100
#define CNF_CANCEL     0x200

// Dialog messages
#define CM_COMMAND   1
#define CM_KEYPRESS  2
#define CM_SELCHANGE 3
// System messages
#define SM_SAVEGAME  100
#define SM_LOADGAME  101
#define SM_QUIT      102
// System messages (to ADVEN)
#define SM_SETTRANSFERMEM 120
#define SM_GETINIVALUE    121
// System messages (to driver)
#define SM_QUERYQUIT 110
#define SM_KEYPRESS  111
#define SM_TIMER     112
// ListBox messages
#define CLB_ADDITEM   1
#define CLB_CLEAR     2
#define CLB_GETCURSEL 3
#define CLB_GETTEXT   4
#define CLB_SETTEXT   5
// TextBox messages
#define CTB_GETTEXT   1
#define CTB_SETTEXT   2

//  =========  STRUCTS  ========
struct CSCIMessage
{
  int code;
  int id;
  int wParam;
};

int windowbackgroundcolor = COL254, pushbuttondarkcolor = COL255;
int pushbuttonlightcolor = COL253;
int topwindowhandle = -1;
wgtfont fonts[10];

#define CTB_KEYPRESS 91

struct OnScreenWindow
{
  block buffer;
  int x, y;
  int oldtop;
};

struct NewControl
{
  int x, y, wid, hit, state, typeandflags, wlevel;
  char visible, enabled;        // not implemented
  char needredraw;
  virtual void draw() = 0;
  virtual int pressedon() = 0;
  virtual int processmessage(int, int, long) = 0;

  NewControl() {
    visible = 1;
    enabled = 1;
  }
  int mouseisinarea()
  {
    if (topwindowhandle != wlevel)
      return 0;

    if ((mousex > x) & (mousex < x + wid) & (mousey > y) & (mousey < y + hit))
      return 1;

    return 0;
  }
  void drawifneeded()
  {
    if (topwindowhandle != wlevel)
      return;
    if (needredraw) {
      needredraw = 0;
      draw();
    }
  }
  void drawandmouse()
  {
    domouse(2);
    draw();
    domouse(1);
  }
};

struct PushButton:public NewControl
{
  char text[32];
  PushButton(int xx, int yy, int wi, int hi, char *tex)
  {                             //wlevel=2;
    x = xx;
    y = yy;
    wid = wi;
    hit = hi + 1;               //hit=hi;
    state = 0;
    strcpy(text, tex);
  };

  void draw()
  {
    wtextcolor(0);
    wsetcolor(COL254);
    wbar(x, y, x + wid, y + hit);
    if (state == 0)
      wsetcolor(pushbuttondarkcolor);
    else
      wsetcolor(pushbuttonlightcolor);

    wrectangle(x, y, x + wid, y + hit);
    if (state == 0)
      wsetcolor(pushbuttonlightcolor);
    else
      wsetcolor(pushbuttondarkcolor);

    wline(x, y, x + wid - 1, y);
    wline(x, y, x, y + hit - 1);
    wouttextxy(x + (wid / 2 - wgettextwidth(text, cbuttfont) / 2), y + 2, cbuttfont, text);
    if (typeandflags & CNF_DEFAULT)
      wsetcolor(0);
    else
      wsetcolor(windowbackgroundcolor);

    wrectangle(x - 1, y - 1, x + wid + 1, y + hit + 1);
  }

  int pressedon()
  {
    int wasstat;
    while (mbutrelease(LEFT) == 0) {
      wasstat = state;
      state = mouseisinarea();
      if (wasstat != state) {
        domouse(2);
        draw();
        domouse(1);
      }

      domouse(0);
    }
    wasstat = state;
    state = 0;
    domouse(2);
    draw();
    domouse(1);
    return wasstat;
  }

  int processmessage(int mcode, int wParam, long lParam)
  {
    return -1;                  // doesn't support messages
  }
};

#define MAXLISTITEM 20
int smcode = 0;
struct MyListBox:public NewControl
{
  int items, topitem, numonscreen, selected;
  char *itemnames[MAXLISTITEM];
  MyListBox(int xx, int yy, int wii, int hii)
  {
    x = xx;
    y = yy;
    wid = wii;
    hit = hii;
    hit -= (hit - 4) % TEXT_HT; // resize to multiple of text height
    numonscreen = (hit - 4) / TEXT_HT;
    items = 0;
    topitem = 0;
    selected = 0;
  }

  void clearlist()
  {
    for (int kk = 0; kk < items; kk++)
      free(itemnames[kk]);

    items = 0;
  }

#define ARROWWIDTH 8
  void draw()
  {
    wsetcolor(windowbackgroundcolor);
    wbar(x, y, x + wid, y + hit);
    wsetcolor(0);
    wrectangle(x, y, x + wid, y + hit);
  
    int widwas = wid;
    wid -= ARROWWIDTH;
    wline(x + wid, y, x + wid, y + hit);        // draw the up/down arrows
    wline(x + wid, y + hit / 2, x + widwas, y + hit / 2);

    int xmidd = x + wid + (widwas - wid) / 2;
    if (topitem < 1)
      wsetcolor(7);

    wline(xmidd, y + 2, xmidd, y + 10); // up arrow
    wline(xmidd - 1, y + 3, xmidd + 1, y + 3);
    wline(xmidd - 2, y + 4, xmidd + 2, y + 4);
    wsetcolor(0);
    if (topitem + numonscreen >= items)
      wsetcolor(7);

    wline(xmidd, y + hit - 10, xmidd, y + hit - 3);     // down arrow
    wline(xmidd - 1, y + hit - 4, xmidd + 1, y + hit - 4);
    wline(xmidd - 2, y + hit - 5, xmidd + 2, y + hit - 5);
    wsetcolor(0);

    for (int tt = 0; tt < numonscreen; tt++) {
      int inum = tt + topitem;
      if (inum >= items)
        break;

      int thisypos = y + 2 + tt * TEXT_HT;
      if (inum == selected) {
        wsetcolor(0);
        wbar(x, thisypos, x + wid, thisypos + TEXT_HT - 1);
        wtextcolor(7);
      } else
        wtextcolor(0);

      wouttextxy(x + 2, thisypos, cbuttfont, itemnames[inum]);
    }
    wid = widwas;
  }

  int pressedon()
  {
    if (mousex > x + wid - ARROWWIDTH) {
      if ((mousey - y < hit / 2) & (topitem > 0))
        topitem--;
      else if ((mousey - y > hit / 2) & (topitem + numonscreen < items))
        topitem++;

    } else {
      selected = ((mousey - y) - 2) / TEXT_HT + topitem;
      if (selected >= items)
        selected = items - 1;

    }

    domouse(2);
    draw();
    domouse(1);
    smcode = CM_SELCHANGE;
    return 0;
  }

  void additem(char *texx)
  {
    if (items >= MAXLISTITEM)
      quit("!CSCIUSER16: Too many items added to listbox");
    itemnames[items] = (char *)malloc(strlen(texx) + 1);
    strcpy(itemnames[items], texx);
    items++;
    needredraw = 1;
  }

  int processmessage(int mcode, int wParam, long lParam)
  {
    if (mcode == CLB_ADDITEM) {
      additem((char *)lParam);
    } else if (mcode == CLB_CLEAR)
      clearlist();
    else if (mcode == CLB_GETCURSEL)
      return selected;
    else if (mcode == CLB_GETTEXT)
      strcpy((char *)lParam, itemnames[wParam]);
    else if (mcode == CLB_SETTEXT) {
      if (wParam < items)
        free(itemnames[wParam]);

      char *newstri = (char *)lParam;
      itemnames[wParam] = (char *)malloc(strlen(newstri) + 2);
      strcpy(itemnames[wParam], newstri);

    } else if (mcode == CTB_KEYPRESS) {
      if ((wParam == 380) & (selected < items - 1))
        selected++;

      if ((wParam == 372) & (selected > 0))
        selected--;

      if (wParam == 373)
        selected -= (numonscreen - 1);

      if (wParam == 381)
        selected += (numonscreen - 1);

      if ((selected < 0) & (items > 0))
        selected = 0;

      if (selected >= items)
        selected = items - 1;

      if ((selected < topitem) & (selected >= 0))
        topitem = selected;

      if (topitem + numonscreen <= selected)
        topitem = (selected + 1) - numonscreen;

      drawandmouse();
      smcode = CM_SELCHANGE;
    } else
      return -1;

    return 0;
  }
};

struct MyLabel:public NewControl
{
  char text[80];
  MyLabel(int xx, int yy, int wii, char *tee)
  {
    strcpy(text, tee);
    x = xx;
    y = yy;
    wid = wii;
    hit = TEXT_HT;
  }

  void draw()
  {
    int curofs = 0, lastspac = 0, cyp = y;
    char *teptr = &text[0];
    wtextcolor(0);
    while (1) {
      if ((teptr[curofs] == ' ') | (teptr[curofs] == 0)) {
        int itwas = teptr[curofs];
        teptr[curofs] = 0;
        if (wgettextwidth(teptr, cbuttfont) > wid) {
          teptr[curofs] = itwas;
          teptr[lastspac] = 0;
          wouttextxy(x, cyp, cbuttfont, teptr);
          teptr[lastspac] = ' ';
          teptr += lastspac + 1;
          curofs = 0;
          cyp += TEXT_HT;
        } else
          teptr[curofs] = itwas;

        lastspac = curofs;
      }

      if (teptr[curofs] == 0)
        break;

      curofs++;
    }
    wouttextxy(x, cyp, cbuttfont, teptr);
  }

  int pressedon()
  {
    return 0;
  }

  int processmessage(int mcode, int wParam, long lParam)
  {
    return -1;                  // doesn't support messages
  }
};

#define TEXTBOX_MAXLEN 49
struct MyTextBox:public NewControl
{
  char text[TEXTBOX_MAXLEN + 1];
  MyTextBox(int xx, int yy, int wii, char *tee)
  {
    x = xx;
    y = yy;
    wid = wii;
    if (tee != NULL)
      strcpy(text, tee);
    else
      text[0] = 0;

    hit = TEXT_HT + 1;
  }

  void draw()
  {
    wsetcolor(windowbackgroundcolor);
    wbar(x, y, x + wid, y + hit);
    wsetcolor(0);
    wrectangle(x, y, x + wid, y + hit);
    wtextcolor(0);
    wouttextxy(x + 2, y + 1, cbuttfont, text);
  
    char tbu[2] = "_";
    wouttextxy(x + 2 + wgettextwidth(text, cbuttfont), y + 1, cbuttfont, tbu);
  }

  int pressedon()
  {
    return 0;
  }

  int processmessage(int mcode, int wParam, long lParam)
  {
    if (mcode == CTB_SETTEXT) {
      strcpy(text, (char *)lParam);
      needredraw = 1;
    } else if (mcode == CTB_GETTEXT)
      strcpy((char *)lParam, text);
    else if (mcode == CTB_KEYPRESS) {
      if (wParam == 8) {
        if (text[0] != 0)
          text[strlen(text) - 1] = 0;

        drawandmouse();
      } else if (strlen(text) >= TEXTBOX_MAXLEN - 1)
        ;
      else if (wgettextwidth(text, cbuttfont) >= wid - 5)
        ;
      else if (wParam > 127)
        ;  // font only has 128 chars
      else {
        text[strlen(text) + 1] = 0;
        text[strlen(text)] = wParam;
        drawandmouse();
      }
    } else
      return -1;

    return 0;
  }
};

#define MAXCONTROLS 20
#define MAXSCREENWINDOWS 5
NewControl *vobjs[MAXCONTROLS];
OnScreenWindow oswi[MAXSCREENWINDOWS];

int CSCIGetVersion()
{
  return 0x0100;
}

void multiply_up(int *x1, int *y1, int *x2, int *y2)
{
  x1[0] *= sxmult;
  x2[0] *= sxmult;
  y1[0] *= symult;
  y2[0] *= symult;
}

int windowcount = 0, curswas = 0;
int CSCIDrawWindow(int xx, int yy, int wid, int hit)
{
  multiply_up(&xx, &yy, &wid, &hit);
  int drawit = -1;
  for (int aa = 0; aa < MAXSCREENWINDOWS; aa++) {
    if (oswi[aa].buffer == NULL) {
      drawit = aa;
      break;
    }
  }

  if (drawit < 0)
    quit("Too many windows created.");

  windowcount++;
  domouse(2);
  xx -= 2;
  yy -= 2;
  wid += 4;
  hit += 4;
  oswi[drawit].buffer = wnewblock(xx, yy, xx + wid, yy + hit);
  oswi[drawit].x = xx;
  oswi[drawit].y = yy;
  wbutt(xx + 1, yy + 1, xx + wid - 1, yy + hit - 1);    // wbutt goes outside its area
  domouse(1);
  oswi[drawit].oldtop = topwindowhandle;
  topwindowhandle = drawit;
  return drawit;
}

void CSCIEraseWindow(int handl)
{
  domouse(2);
  topwindowhandle = oswi[handl].oldtop;
  wputblock(oswi[handl].x, oswi[handl].y, oswi[handl].buffer, 0);
  wfreeblock(oswi[handl].buffer);
  domouse(1);
  oswi[handl].buffer = NULL;
  windowcount--;
}

int controlid = 0;
int checkcontrols()
{
  smcode = 0;
  for (int kk = 0; kk < MAXCONTROLS; kk++) {
    if (vobjs[kk] != NULL) {
      if (vobjs[kk]->mouseisinarea()) {
        controlid = kk;
        return vobjs[kk]->pressedon();
      }
    }
  }
  return 0;
}

int finddefaultcontrol(int flagmask)
{
  for (int ff = 0; ff < MAXCONTROLS; ff++) {
    if (vobjs[ff] == NULL)
      continue;
   
    if (vobjs[ff]->wlevel != topwindowhandle)
      continue;

    if (vobjs[ff]->typeandflags & flagmask)
      return ff;
  }

  return -1;
}

int CSCIWaitMessage(CSCIMessage * cscim)
{
  wtexttransparent(TEXTFG);
  for (int uu = 0; uu < MAXCONTROLS; uu++) {
    if (vobjs[uu] != NULL) {
      domouse(2);
      vobjs[uu]->drawifneeded();
      domouse(1);
    }
  }

  while (1) {
    cscim->id = -1;
    cscim->code = 0;
    smcode = 0;
    if (kbhit()) {
      int keywas = getch();
      if (keywas == 0)
        keywas = getch() + 300;

      if (keywas == 13) {
        cscim->id = finddefaultcontrol(CNF_DEFAULT);
        cscim->code = CM_COMMAND;
      } else if (keywas == 27) {
        cscim->id = finddefaultcontrol(CNF_CANCEL);
        cscim->code = CM_COMMAND;
      } else if ((keywas >= 372) & (keywas <= 381) & (finddefaultcontrol(CNT_LISTBOX) >= 0))
        vobjs[finddefaultcontrol(CNT_LISTBOX)]->processmessage(CTB_KEYPRESS, keywas, 0);
      else if (finddefaultcontrol(CNT_TEXTBOX) >= 0)
        vobjs[finddefaultcontrol(CNT_TEXTBOX)]->processmessage(CTB_KEYPRESS, keywas, 0);

      if (cscim->id < 0) {
        cscim->code = CM_KEYPRESS;
        cscim->wParam = keywas;
      }
    }

    if (mgetbutton() != NONE) {
      if (checkcontrols()) {
        cscim->id = controlid;
        cscim->code = CM_COMMAND;
      }
    }

    if (smcode) {
      cscim->code = smcode;
      cscim->id = controlid;
    }

    if (cscim->code > 0)
      break;

    domouse(0);
    poll_mp3();
  }

  return 0;
}

int CSCICreateControl(int typeandflags, int xx, int yy, int wii, int hii, char *title)
{
  multiply_up(&xx, &yy, &wii, &hii);
  int usec = -1;
  for (int hh = 1; hh < MAXCONTROLS; hh++) {
    if (vobjs[hh] == NULL) {
      usec = hh;
      break;
    }
  }

  if (usec < 0)
    quit("Too many controls created");

  int type = typeandflags & 0x00ff;     // 256 control types
  if (type == CNT_PUSHBUTTON) {
    if (wii == -1)
      wii = wgettextwidth(title, cbuttfont) + 20;

    vobjs[usec] = new PushButton(xx, yy, wii, hii, title);

  } else if (type == CNT_LISTBOX) {
    vobjs[usec] = new MyListBox(xx, yy, wii, hii);
  } else if (type == CNT_LABEL) {
    vobjs[usec] = new MyLabel(xx, yy, wii, title);
  } else if (type == CNT_TEXTBOX) {
    vobjs[usec] = new MyTextBox(xx, yy, wii, title);
  } else
    quit("Unknown control type requested");

  vobjs[usec]->typeandflags = typeandflags;
  wtexttransparent(TEXTFG);
  vobjs[usec]->wlevel = topwindowhandle;
  domouse(2);
  vobjs[usec]->draw();
  domouse(1);
  return usec;
}

void CSCIDeleteControl(int haa)
{
  delete vobjs[haa];
  vobjs[haa] = NULL;
}

int CSCISendControlMessage(int haa, int mess, int wPar, long lPar)
{
  if (vobjs[haa] == NULL)
    return -1;
  return vobjs[haa]->processmessage(mess, wPar, lPar);
}

#define MAXSAVEGAMES 20
char *lpTemp, *lpTemp2;
char bufTemp[50], buffer2[60];
int toomanygames;
int filenumbers[MAXSAVEGAMES];
unsigned long filedates[MAXSAVEGAMES];

int numsaves=0;
char descrp[200];

void preparesavegamelist(int ctrllist)
{
  struct ffblk ffb;
  FILE*ooo;
  int ii,don,nn,kk;
  numsaves=0;
  toomanygames=0;
  ii=0;
  don=findfirst("agssave.*",&ffb,0);
  while (!don) {
    ii=0;
    if (numsaves>=MAXSAVEGAMES) {
      toomanygames=1;
      break;
    }
    // get description
    ooo=fopen(ffb.ff_name,"rb");
    fseek(ooo,sgsiglen,SEEK_CUR);
    for (;(descrp[ii]=fgetc(ooo));ii++) ;
    fclose(ooo);
    CSCISendControlMessage(ctrllist,CLB_ADDITEM,0,(long)&descrp[0]);
    filenumbers[numsaves]=atol(ffb.ff_name+(strlen(ffb.ff_name)-2));
    filedates[numsaves]=(ffb.ff_fdate<<16)+ffb.ff_ftime;
    numsaves++;
    don=findnext(&ffb);
  }
  if (numsaves>=MAXSAVEGAMES)
    toomanygames=1;
  for (nn=0;nn<numsaves-1;nn++) {
    for (kk=0;kk<numsaves-1;kk++) { // Date order the games
      if (filedates[kk]<filedates[kk+1]) {  // swap them round
        CSCISendControlMessage(ctrllist,CLB_GETTEXT,kk,(long)&descrp[0]);
        CSCISendControlMessage(ctrllist,CLB_GETTEXT,kk+1,(long)&buffer2[0]);
        CSCISendControlMessage(ctrllist,CLB_SETTEXT,kk+1,(long)&descrp[0]);
        CSCISendControlMessage(ctrllist,CLB_SETTEXT,kk,(long)&buffer2[0]);
        int numtem=filenumbers[kk];
        filenumbers[kk]=filenumbers[kk+1];
        filenumbers[kk+1]=numtem;
        long numted=filedates[kk];
        filedates[kk]=filedates[kk+1];
        filedates[kk+1]=numted;
      }
    }
  }
}

int loadgamedialog()
{
  int boxleft = myscrnwid / 2 - 100;
  int boxtop = myscrnhit / 2 - 60;
  int buttonhit = usetup.textheight + 5;
  int handl = CSCIDrawWindow(boxleft, boxtop, 200, 120);
  int ctrlok =
    CSCICreateControl(CNT_PUSHBUTTON | CNF_DEFAULT, boxleft + 135, boxtop + 5, 60, 10, get_language_text(MSG_RESTORE));
  int ctrlcancel =
    CSCICreateControl(CNT_PUSHBUTTON | CNF_CANCEL, boxleft + 135, boxtop + 5 + buttonhit, 60, 10,
                      get_language_text(MSG_CANCEL));
  int ctrllist = CSCICreateControl(CNT_LISTBOX, boxleft + 10, boxtop + 30, 120, 80, NULL);
  int ctrltex1 = CSCICreateControl(CNT_LABEL, boxleft + 10, boxtop + 5, 120, 0, get_language_text(MSG_SELECTLOAD));
  CSCISendControlMessage(ctrllist, CLB_CLEAR, 0, 0);

  preparesavegamelist(ctrllist);
  CSCIMessage mes;
  lpTemp = NULL;
  int toret = -1;
  while (1) {
    CSCIWaitMessage(&mes);      //printf("mess: %d, id %d ",mes.code,mes.id);
    if (mes.code == CM_COMMAND) {
      if (mes.id == ctrlok) {
        if (CSCISendControlMessage(ctrllist, CLB_GETCURSEL, 0, 0) >= numsaves)
          lpTemp = NULL;
        else {
          toret = filenumbers[CSCISendControlMessage(ctrllist, CLB_GETCURSEL, 0, 0)];
          sprintf(bufTemp, sgnametemplate, toret);
          lpTemp = &bufTemp[0];
        }
      } else if (mes.id == ctrlcancel) {
        lpTemp = NULL;
      }

      break;
    }
  }

  CSCIDeleteControl(ctrltex1);
  CSCIDeleteControl(ctrllist);
  CSCIDeleteControl(ctrlok);
  CSCIDeleteControl(ctrlcancel);
  CSCIEraseWindow(handl);
  return toret;
}

int enternumberwindow(char *prompttext)
{
  int boxleft = 60, boxtop = 80;
  int handl = CSCIDrawWindow(boxleft, boxtop, 200, 40);
  int ctrlok = CSCICreateControl(CNT_PUSHBUTTON | CNF_DEFAULT, boxleft + 135, boxtop + 5, 60, 10, "OK");
  int ctrltbox = CSCICreateControl(CNT_TEXTBOX, boxleft + 10, boxtop + 29, 120, 0, NULL);
  int ctrltex1 = CSCICreateControl(CNT_LABEL, boxleft + 10, boxtop + 5, 120, 0, prompttext);

  CSCIMessage mes;
  while (1) {
    CSCIWaitMessage(&mes);
    if (mes.code == CM_COMMAND) break;
  }

  CSCISendControlMessage(ctrltbox, CTB_GETTEXT, 0, (long)&buffer2[0]);

  CSCIDeleteControl(ctrltex1);
  CSCIDeleteControl(ctrltbox);
  CSCIDeleteControl(ctrlok);
  CSCIEraseWindow(handl);
  return atol(buffer2);
}

int savegamedialog()
{
  char okbuttontext[10];
  strcpy(okbuttontext, get_language_text(MSG_SAVEBUTTON));
  char labeltext[80];
  strcpy(labeltext, get_language_text(MSG_SAVEDIALOG));
  int boxleft = myscrnwid / 2 - 100;
  int boxtop = myscrnhit / 2 - 60;
  int buttonhit = usetup.textheight + 5;
  int labeltop = boxtop + 5;
  int handl = CSCIDrawWindow(boxleft, boxtop, 200, 120);
  int ctrlcancel =
    CSCICreateControl(CNT_PUSHBUTTON | CNF_CANCEL, boxleft + 135, boxtop + 5 + buttonhit, 60, 10,
                      get_language_text(MSG_CANCEL));
  int ctrllist = CSCICreateControl(CNT_LISTBOX, boxleft + 10, boxtop + 40, 120, 80, NULL);
  int ctrltbox = 0;

  CSCISendControlMessage(ctrllist, CLB_CLEAR, 0, 0);    // clear the list box
  preparesavegamelist(ctrllist);
  if (toomanygames) {
    strcpy(okbuttontext, get_language_text(MSG_REPLACE));
    strcpy(labeltext, get_language_text(MSG_MUSTREPLACE));
    labeltop = boxtop + 2;
  } else
    ctrltbox = CSCICreateControl(CNT_TEXTBOX, boxleft + 10, boxtop + 29, 120, 0, NULL);

  int ctrlok = CSCICreateControl(CNT_PUSHBUTTON | CNF_DEFAULT, boxleft + 135, boxtop + 5, 60, 10, okbuttontext);
  int ctrltex1 = CSCICreateControl(CNT_LABEL, boxleft + 10, labeltop, 120, 0, labeltext);
  CSCIMessage mes;

  lpTemp = NULL;
  if (numsaves > 0)
    CSCISendControlMessage(ctrllist, CLB_GETTEXT, 0, (long)&buffer2[0]);
  else
    buffer2[0] = 0;

  CSCISendControlMessage(ctrltbox, CTB_SETTEXT, 0, (long)&buffer2[0]);

  int toret = -1;
  while (1) {
    CSCIWaitMessage(&mes);      //printf("mess: %d, id %d ",mes.code,mes.id);
    if (mes.code == CM_COMMAND) {
      if (mes.id == ctrlok) {
        int cursell = CSCISendControlMessage(ctrllist, CLB_GETCURSEL, 0, 0);
        CSCISendControlMessage(ctrltbox, CTB_GETTEXT, 0, (long)&buffer2[0]);

        if (numsaves > 0)
          CSCISendControlMessage(ctrllist, CLB_GETTEXT, cursell, (long)&bufTemp[0]);
        else
          strcpy(bufTemp, "_NOSAVEGAMENAME");

        if (toomanygames) {
          int nwhand = CSCIDrawWindow(boxleft + 5, boxtop + 20, 190, 65);
          int lbl1 =
            CSCICreateControl(CNT_LABEL, boxleft + 20, boxtop + 25, 160, 0, get_language_text(MSG_REPLACEWITH1));
          int lbl2 = CSCICreateControl(CNT_LABEL, boxleft + 30, boxtop + 34, 160, 0, bufTemp);
          int lbl3 =
            CSCICreateControl(CNT_LABEL, boxleft + 20, boxtop + 45, 160, 0, get_language_text(MSG_REPLACEWITH2));
          int txt1 = CSCICreateControl(CNT_TEXTBOX, boxleft + 20, boxtop + 55, 160, 0, bufTemp);
          int btnOk =
            CSCICreateControl(CNT_PUSHBUTTON | CNF_DEFAULT, boxleft + 30, boxtop + 70, 60, 10,
                              get_language_text(MSG_REPLACE));
          int btnCancel =
            CSCICreateControl(CNT_PUSHBUTTON | CNF_CANCEL, boxleft + 100, boxtop + 70, 60, 10,
                              get_language_text(MSG_CANCEL));

          CSCIMessage cmes;
          do {
            CSCIWaitMessage(&cmes);
          } while (cmes.code != CM_COMMAND);

          CSCISendControlMessage(txt1, CTB_GETTEXT, 0, (long)&buffer2[0]);
          CSCIDeleteControl(btnCancel);
          CSCIDeleteControl(btnOk);
          CSCIDeleteControl(txt1);
          CSCIDeleteControl(lbl3);
          CSCIDeleteControl(lbl2);
          CSCIDeleteControl(lbl1);
          CSCIEraseWindow(nwhand);
          bufTemp[0] = 0;

          if (cmes.id == btnCancel) {
            lpTemp = NULL;
            break;
          } else
            toret = filenumbers[cursell];

        } 
        else if (strcmp(buffer2, bufTemp) != 0) {     // create a new game (description different)
          int highestnum = 0;
          for (int pp = 0; pp < numsaves; pp++) {
            if (filenumbers[pp] > highestnum)
              highestnum = filenumbers[pp];
          }

          if (highestnum > 90)
            quit("Save game directory overflow");

          toret = highestnum + 1;
          sprintf(bufTemp, sgnametemplate, toret);
        } 
        else {
          toret = filenumbers[cursell];
          bufTemp[0] = 0;
        }

        if (bufTemp[0] == 0)
          sprintf(bufTemp, sgnametemplate, toret);

        lpTemp = &bufTemp[0];
        lpTemp2 = &buffer2[0];
      } else if (mes.id == ctrlcancel) {
        lpTemp = NULL;
      }
      break;
    } else if (mes.code == CM_SELCHANGE) {
      CSCISendControlMessage(ctrllist, CLB_GETTEXT, CSCISendControlMessage(ctrllist, CLB_GETCURSEL, 0, 0), (long)&buffer2[0]);
      CSCISendControlMessage(ctrltbox, CTB_SETTEXT, 0, (long)&buffer2[0]);
    }
  }

  CSCIDeleteControl(ctrltbox);
  CSCIDeleteControl(ctrltex1);
  CSCIDeleteControl(ctrllist);
  CSCIDeleteControl(ctrlok);
  CSCIDeleteControl(ctrlcancel);
  CSCIEraseWindow(handl);
  return toret;
}

CSCIMessage smes;
int myscimessagebox(char *lpprompt, char *btn1, char *btn2)
{
  int windl = CSCIDrawWindow(80, 80, 240 - 80, 120 - 80);
  int lbl1 = CSCICreateControl(CNT_LABEL, 90, 85, 150, 0, lpprompt);
  int btflag = CNT_PUSHBUTTON;

  if (btn2 == NULL)
    btflag |= CNF_DEFAULT | CNF_CANCEL;
  else
    btflag |= CNF_DEFAULT;

  int btnQuit = CSCICreateControl(btflag, 90, 105, 60, 10, btn1);
  int btnPlay = 0;

  if (btn2 != NULL)
    btnPlay = CSCICreateControl(CNT_PUSHBUTTON | CNF_CANCEL, 165, 105, 60, 10, btn2);

  smes.code = 0;

  do {
    CSCIWaitMessage(&smes);
  } while (smes.code != CM_COMMAND);

  if (btnPlay)
    CSCIDeleteControl(btnPlay);

  CSCIDeleteControl(btnQuit);
  CSCIDeleteControl(lbl1);
  CSCIEraseWindow(windl);

  if (smes.id == btnQuit)
    return 1;

  return 0;
}

int introdialog()
{
  int handl = CSCIDrawWindow(90, 70, 150, 60);
  int lbl = CSCICreateControl(CNT_LABEL, 93, 72, 150, 0, get_language_text(MSG_INTRODIALOG));
  int flg = CNT_PUSHBUTTON | CNF_CANCEL;
  int btn1 = CSCICreateControl(flg, 93, 92, 60, 10, get_language_text(MSG_SKIPBUTTON));
  int btn2 = CSCICreateControl(CNT_PUSHBUTTON | CNF_DEFAULT, 93, 105, 60, 10, get_language_text(MSG_WATCHBUTTON));
  int btn3 = CSCICreateControl(CNT_PUSHBUTTON, 93, 119, 100, 10, get_language_text(MSG_RESTOREBUTTON));

  smes.code = 0;

  do {
    CSCIWaitMessage(&smes);
  } while (smes.code != CM_COMMAND);

  CSCIDeleteControl(btn2);
  CSCIDeleteControl(btn3);
  CSCIDeleteControl(btn1);
  CSCIDeleteControl(lbl);
  CSCIEraseWindow(handl);

  if (smes.id == btn1)
    return 0;
  else if (smes.id == btn2)
    return 1;

  return 2;
}

int quitdialog()
{
  char quitbut[16], playbut[16];
  strcpy(quitbut, get_language_text(MSG_QUITBUTTON));
  strcpy(playbut, get_language_text(MSG_PLAYBUTTON));
  return myscimessagebox(get_language_text(MSG_QUITDIALOG), quitbut, playbut);
}
