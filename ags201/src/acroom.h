#ifndef __CROOM_H
#define __CROOM_H
#ifndef CROOM_NOFUNCTIONS
extern FILE *clibfopen(char *, char *);
#include <compress.cpp>
#endif
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <seer.h>

#define ROOM_FILE_VERSION 11
/* room file versions history
  8:  final v1.14 release
  9:  intermediate v2 alpha releases
 10:  v2 alpha-7 release
 11:  final v2.00 release
 */

#define MAX_INIT_SPR 10
#define MAX_OBJ 15
#define NUM_MISC 20
#define MAXMESS 40
#define NUMOTCON 7    // number of conditions before standing on
#define NUMTYPCON 60  // number of conditions for hotspot
#define NUMOBJCON 40  // number of conditions for object
#define NUM_CONDIT (120+NUMOTCON)
const int MISC_COND=MAX_OBJ*4+NUMOTCON+MAX_INIT_SPR*4;
#define MAX_HOTSPOTS 20
// NUMCONDIT : whataction[0]:  Char walks off left
//                       [1]:  Char walks off right
//                       [2]:  Char walks off bottom
//                       [3]:  Char walks off top
//			 [4]:  First enters screen
//                       [5]:  Every time enters screen
//                       [6]:  execute every loop
//                [5]...[19]:  Char stands on lookat type
//               [20]...[35]:  Look at type
//               [36]...[49]:  Action on type
//               [50]...[65]:  Use inv on type
//               [66]...[75]:  Look at object
//               [76]...[85]:  Action on object
//               [86]...[95]:  Speak to object
//		[96]...[105]:  Use inv on object
//             [106]...[124]:  Misc conditions 1-20

// game ver     whataction[]=
// v1.00              0  :  Go to screen
//                    1  :  Don't do anything
//                    2  :  Can't walk
//                    3  :  Man dies
//                    4  :  Run animation
//                    5  :  Display message
//                    6  :  Remove an object (set object.on=0)
//		      7  :  Remove object & add Val2 to inventory
//                    8  :  Add Val1 to inventory (Val2=num times)
//                    9  :  Run a script
// v1.00 SR-1        10  :  Run graphical script
// v1.1              11  :  Play sound effect SOUND%d.WAV
// v1.12             12  :  Play FLI/FLC animation FLIC%d.FLC or FLIC%d.FLI
//                   13  :  Turn object on
// v2.00             14  :  Run conversation
#define NUMRESPONSE   14
#define NUMCOMMANDS   15
#define GO_TO_SCREEN  0
#define NO_ACTION     1
#define NO_WALK       2
#define MAN_DIES      3
#define RUN_ANIMATE   4
#define SHOW_MESSAGE  5
#define OBJECT_OFF    6
#define OBJECT_INV    7
#define ADD_INV       8
#define RUNSCRIPT     9
#define GRAPHSCRIPT   10
#define PLAY_SOUND    11
#define PLAY_FLI      12
#define OBJECT_ON     13
#define RUN_DIALOG    14

#include <unistd.h>


// thisroom.options[0] = startup music
// thisroom.options[1] = can save/load on screen (0=yes, 1=no)
// thisroom.options[2] = player character disabled? (0=no, 1=yes)
// thisroom.options[3] = player special view (0=normal)
//                 [4] = music volume (0=normal, <0 quiter, >0 louder)

const int ST_TUNE = 0, ST_SAVELOAD = 1, ST_MANDISABLED = 2, ST_MANVIEW = 3, ST_VOLUME = 4;
#ifndef CROOM_NOFUNCTIONS
char *croom_h_copyright = "ChrisRoom v2.00 - CRM reader/writer copyright (c) 1995, 1998, 1999 by Chris Jones.";
char *game_file_sig = "Adventure Creator Game File v2";
block backups[5];

void fputstring(char*sss,FILE*ddd) {
  int b = 0;
  while (sss[b] != 0) {
    fputc(sss[b], ddd);
    b++;
  }
  fputc(0,ddd);
}

void fgetstring(char*sss,FILE*ddd) {
  int b = -1;
  do {
    b++;
    sss[b] = fgetc(ddd);
    if (feof(ddd))
      return;
  } while (sss[b] != 0);
}

int cunpackbitl(unsigned char*,int size,FILE*infile);

/*long cloadcompfile(FILE*outpt,block tobesaved,color*pal,long poot=0) {
  fseek(outpt,poot,SEEK_SET);
  int widt,hit,hh;
  for (hh=0;hh<4;hh++) *tobesaved++=fgetc(outpt);
  tobesaved-=4;
  widt=*tobesaved++;  widt+=(*tobesaved++)*256;
  hit=*tobesaved++; hit+=(*tobesaved++)*256;
  unsigned char* ress=(unsigned char*)malloc(widt+1);
  for (int ww=0;ww<hit;ww++) {
    cunpackbitl(ress,widt,outpt);
    for (int ss=0;ss<widt;ss++)  (*tobesaved++)=ress[ss];
    }
  for (ww=0;ww<256;ww++) {
    pal[ww].r=fgetc(outpt);
    pal[ww].g=fgetc(outpt);
    pal[ww].b=fgetc(outpt);
    }
  poot=ftell(outpt); free(ress); tobesaved-=(widt*hit+4);
  return poot;
  }*/
#endif

void quit(char*);

#ifdef DJGPP
#define PCKD __attribute__((packed))
#else
#define PCKD
#endif
struct sprstruc {
  short sprnum PCKD;  // number from array
  short x,y   PCKD;    // x,y co-ords
  short room  PCKD;  // room number
  short on    PCKD;
  sprstruc() { on=0; }
  };

struct SpritesFolder {
  int count;
  short sprites[120];
  short parent;
  char name[20];
  };

#define MSG_DISPLAYNEXT 1   // supersedes using alt-200 at end of message
#define MSG_TIMELIMIT   2
struct MessageInfo {
  char displayas PCKD;   // 0=normal window, 1=as speech
  char flags PCKD;  // combination of MSG_xxx flags
  };

#define AE_WAITFLAG 0x80000000
#define MAXANIMSTAGES 10
struct AnimationStruct {
  int  x,y;
  int  data;
  int  object;
  int  speed;
  char action;
  char wait;
  AnimationStruct() { action=0; object=0; wait=1; speed=5; }
  };
struct FullAnimation {
  AnimationStruct stage[MAXANIMSTAGES];
  int numstages;
  FullAnimation() { numstages=0; }
  };

struct _Point {
  short x,y;
  };

#define MAXCOMMANDS 8
struct EventBlock {
  int list[MAXCOMMANDS];
  int respond[MAXCOMMANDS];
  int respondval[MAXCOMMANDS];
  int data[MAXCOMMANDS];
  int numcmd;
  short score[MAXCOMMANDS];
};

#define MAX_WALK_AREAS 15
#define MAXPOINTS 30
struct PolyPoints {
  int x[MAXPOINTS];
  int y[MAXPOINTS];
  int numpoints;
  void add_point(int xxx,int yyy) {
    x[numpoints]=xxx;
    y[numpoints]=yyy;
    numpoints++;
    if (numpoints>=MAXPOINTS) quit("too many poly points added");
    }
  PolyPoints() { numpoints=0; }
};

#define POPUP_NONE      0
#define POPUP_MOUSEY    1
#define POPUP_SCRIPT    2
#define VTA_LEFT        0
#define VTA_RIGHT       1
#define VTA_CENTRE      2
#define IFLG_TEXTWINDOW 1

#define MAXBUTTON 20
#define IBACT_SETMODE   1
#define IBACT_SCRIPT    2
#define IBFLG_ENABLED   1
#define IBFLG_INVBOX    2
struct InterfaceButton {
  int x, y, pic, overpic, pushpic, leftclick;
  int rightclick; // if inv, then leftclick = wid, rightclick = hit
  int reserved_for_future;
  char flags;
  void set(int xx, int yy, int picc, int overpicc, int actionn) {
    x = xx; y = yy; pic = picc; overpic = overpicc; leftclick = actionn; pushpic = 0;
    rightclick = 0; flags = IBFLG_ENABLED;
    reserved_for_future = 0;
  }
  };
// this struct should go in a Game struct, not the room structure.
struct InterfaceElement {
  int             x, y, x2, y2;
  int             bgcol, fgcol, bordercol;
  int             vtextxp, vtextyp, vtextalign;  // X & Y relative to topleft of interface
  char            vtext[40];
  int             numbuttons;
  InterfaceButton button[MAXBUTTON];
  int             flags;
  int             reserved_for_future;
  int             popupyp;   // pops up when mousey < this
  char            popup;     // does it pop up? (like sierra icon bar)
  char            on;
  InterfaceElement() {
    vtextxp = 0; vtextyp = 1; strcpy(vtext,"@SCORETEXT@$r@GAMENAME@");
    numbuttons = 0; bgcol = 8; fgcol = 15; bordercol = 0; on = 1; flags = 0;
  }
  };
/*struct InterfaceStyle {
  int  playareax1,playareay1,playareax2,playareay2; // where the game takes place
  int  vtextxp,vtextyp;
  char vtext[40];
  int  numbuttons,popupbuttons;
  InterfaceButton button[MAXBUTTON];
  int  invx1,invy1,invx2,invy2;  // set invx1=-1 for Sierra-style inventory
  InterfaceStyle() {  // sierra interface
    playareax1=0; playareay1=13; playareax2=319; playareay2=199;
    vtextxp=160; vtextyp=2; strcpy(vtext,"@SCORETEXT@$r@GAMENAME@");
    invx1=-1; numbuttons=2; popupbuttons=1;
    button[0].set(0,13,3,-1,0);
    }
  };*/

#define MAX_ROOMS 300

#define MAXANIMS 10
#define MAX_FLAGS 15
struct roomstruct {
  block bscene,walls,object,lookat;  // 'object' is the walk-behind
  block shading;  // specifies different view number to be used
  color pal[256];
  short numobj;
  short objyval[MAX_OBJ];   // below:  +3 for internal use
  short whataction[NUM_CONDIT+3]; // what to do if condition appears
  short val1[NUM_CONDIT+3];  // variable, eg. screen number to go to
  short val2[NUM_CONDIT+3];  // 2nd var, optional, eg. which side of screen to come on
  short otcond[NUM_CONDIT+3]; // do extra misc condition
  char  points[NUM_CONDIT+3];  // extra points for doing it
  short left,right,top,bottom;  // to walk off screen
  short numsprs,nummes; // number of initial sprites and messages
  sprstruc sprs[MAX_INIT_SPR]; // structures for each sprite
  char  password[11];
  char  options[10];  // [0]=startup music
  char* message[MAXMESS];
  MessageInfo msgi[MAXMESS];
  short wasversion;   // when loaded from file
  short flagstates[MAX_FLAGS];
  FullAnimation anims[MAXANIMS];
  short numanims;
  short shadinginfo[16];
  // new version 2 roommake stuff below
  int    numwalkareas;
  PolyPoints wallpoints[MAX_WALK_AREAS];
  int    numhotspots;
  _Point hswalkto[MAX_HOTSPOTS];
  char   hotspotnames[MAX_HOTSPOTS][30];
  EventBlock hscond[MAX_HOTSPOTS];
  EventBlock objcond[MAX_INIT_SPR];
  EventBlock misccond;
  int    objbaseline[MAX_INIT_SPR];            // or -1 (use bottom of object graphic)
  short  width,height;                         // in 320x200 terms (scrolling room size)
  short  resolution;                           // 1 = 320x200, 2 = 640x400
  short  walk_area_zoom[MAX_WALK_AREAS];       // 0 = 100%, 1 = 101%, -1 = 99%
  char   *scripts;
  scScript compiled_script;
  int    cscriptsize;
  roomstruct() {
    bscene = NULL; walls = NULL; object = NULL; lookat = NULL; nummes = 0;
    left = 0; right = 317; top = 40; bottom = 199; numobj = MAX_OBJ; numsprs = 0; password[0] = 0;
    wasversion = ROOM_FILE_VERSION; numanims = 0; shading = NULL; numwalkareas = 0;
    numhotspots = 0;
    memset(&objbaseline[0], 0xff, sizeof(int) * MAX_INIT_SPR);
    width = 320; height = 200; scripts = NULL; compiled_script = NULL;
    cscriptsize = 0;
    memset(&walk_area_zoom[0], 0, sizeof(short) * MAX_WALK_AREAS);
    resolution = 1;
    }
  void allocall();
  void freeall();
  void freemessage();
  };

#ifndef CROOM_NOFUNCTIONS
int ff;
void roomstruct::allocall() {
//  printf("Before %ld\n",farcoreleft());
  for (ff=0;ff<5;ff++) { backups[ff]=wnewblock(0,0,319,199);
//    printf("%d ",ff); if (kbhit()) break;
    if (backups[ff]==NULL) quit("ROOM.C, AllocMem: Out of memory"); }
  walls=::backups[0];  // this is because blocks in a struct don't work
  object=::backups[1]; // properly
  lookat=::backups[2];
  bscene=::backups[3];
  shading=::backups[4];
//  printf("After %ld\n",farcoreleft());
  }

void roomstruct::freemessage() {
  for (int f=0;f<nummes;f++) if (message[f]!=NULL) free(message[f]);
  }

void roomstruct::freeall() {
//  for (int f=0;f<4;f++) wfreeblock(::backups[f]);
  wfreeblock(walls); wfreeblock(lookat); wfreeblock(bscene);
  wfreeblock(object); if (shading!=NULL) wfreeblock(shading);
  freemessage();
  }

/*void roomstruct::freeall() { wfreeblock(walls); wfreeblock(bscene);
  wfreeblock(object); wfreeblock(lookat);
  for (int f=0;f<nummes;f++) if (message[f]!=NULL) free(message[f]); }*/

struct room_file_header {
  short version PCKD;
  };

#ifdef DJGPP
extern void lzwcompress(FILE*,FILE*);
extern void lzwexpand(FILE*,FILE*);
extern long maxsize,outbytes,putbytes;
char*lztempfnm="~aclzw.tmp";
long save_lzw(char*fnn,BITMAP* bmpp,color*pall,long offe) {
  FILE*ooo=fopen(lztempfnm,"wb");
  putw(bmpp->w,ooo); putw(bmpp->h,ooo);
  fwrite(&bmpp->line[0][0],bmpp->w,bmpp->h,ooo);
  fclose(ooo);
  FILE*iii=fopen(fnn,"r+b");
  fseek(iii,offe,SEEK_SET);
  ooo=fopen(lztempfnm,"rb");
  long fll=filelength(fileno(ooo));
  fwrite(&pall[0],sizeof(color),256,iii);
  fwrite(&fll,4,1,iii);
  long gobacto=ftell(iii);
  // reserve space for compressed size
  fwrite(&fll,4,1,iii);
  lzwcompress(ooo,iii);
  long toret=ftell(iii);
  fseek(iii,gobacto,SEEK_SET); fll=(toret-gobacto)-4;
  fwrite(&fll,4,1,iii);  // write compressed size
  fclose(ooo); fclose(iii);
  unlink(lztempfnm);
  return toret;
  }
BITMAP* recalced;
long load_lzw(char*fnn,BITMAP*bmm,color*pall,long ooff) {
  recalced=bmm;
  FILE*iii=clibfopen(fnn,"rb");
  fseek(iii,ooff,SEEK_SET);
  fread(&pall[0],sizeof(color),256,iii);
  fread(&maxsize,4,1,iii);
  long uncompsiz;
  fread(&uncompsiz,4,1,iii);
  uncompsiz+=ftell(iii);
//  long maxswas=ooff+maxsize+256*sizeof(color)+4;
  FILE*ooo=fopen(lztempfnm,"wb");
  outbytes=0; putbytes=0;
  lzwexpand(iii,ooo);
//  long maxswas=ftell(iii);
  long maxswas=uncompsiz;
  fclose(ooo);
  fclose(iii);
  iii=fopen(lztempfnm,"rb");
  int neww=getw(iii),newh; newh=getw(iii);
  if ((neww!=bmm->w) | (newh!=bmm->h)) {
//    quit("load_lzw: wrong size");
    destroy_bitmap(bmm); bmm=create_bitmap(neww,newh);
    recalced=bmm;
    }
  bmm->w=neww; bmm->h=newh;
  fread(&bmm->line[0][0],bmm->w,bmm->h,iii);
  fclose(iii);
  unlink(lztempfnm);
  return maxswas;
  }

long savecompressed_allegro(char*fnn,BITMAP* bmpp,color*pall,long ooo) {
  unsigned char*wgtbl=(unsigned char*)malloc(bmpp->w * bmpp->h + 4);
  short*sss=(short*)wgtbl;
  sss[0]=bmpp->w; sss[1]=bmpp->h;
  memcpy(&wgtbl[4],&bmpp->line[0][0],bmpp->w * bmpp->h);
  long toret=csavecompressed(fnn,wgtbl,pall,ooo);
  free(wgtbl);
  return toret;
  }

/*long loadcompressed_allegro(char*fnn,BITMAP* bim,color*pall,long ooo) {
  unsigned char*wgtbl=(unsigned char*)malloc(320 * 200 + 4);
  long toret=cloadcompressed(fnn,wgtbl,pall,ooo);
  short*sss=(short*)wgtbl;
  bim->w=sss[0];
  bim->h=sss[1];
  memcpy(&bim->line[0][0],&wgtbl[4],sss[0] * sss[1]);
  free(wgtbl);
  return toret;
  }*/
long loadcompressed_allegro(FILE *fpp, BITMAP **bimpp, color *pall, long ooo) {
  fseek(fpp,ooo,SEEK_SET);
  BITMAP*bim=*bimpp;
  destroy_bitmap(bim);
  short widd,hitt;
  fread(&widd,2,1,fpp);
  fread(&hitt,2,1,fpp);
  bim=create_bitmap(widd,hitt);
  *bimpp=bim;
  int ii;
  for (ii=0;ii<hitt;ii++) {
    cunpackbitl(&bim->line[ii][0],widd,fpp);
  }
  fseek(fpp,768,SEEK_CUR);  // skip palette
  long toret=ftell(fpp);
  return toret;
}
#endif

#define BLOCKTYPE_MAIN       1
#define BLOCKTYPE_SCRIPT     2
#define BLOCKTYPE_COMPSCRIPT 3
#define BLOCKTYPE_EOF        0xff

extern void load_script_configuration(FILE*);
extern void save_script_configuration(FILE*);
extern void load_graphical_scripts(FILE*,roomstruct*);
extern void save_graphical_scripts(FILE*,roomstruct*);
static char *passwencstring = "Avis Durgan";

// Note: we don't use GETW or PUTW for compatibility between 16-bit and
// 32-bit versions; hence all the FREAD/FWRITEs
void save_room(char*files,roomstruct rstruc) {
  if (rstruc.wasversion<ROOM_FILE_VERSION)
    quit("save_room: can no longer save old format rooms");

  int f;
  if (rstruc.wasversion < 9) for (f = 0; f < 11; f++) rstruc.password[f]-=60;
  else for (f=0;f<11;f++) rstruc.password[f]-=passwencstring[f];

  FILE*opty=fopen(files,"wb");

  room_file_header rfh;
  rfh.version = rstruc.wasversion; //ROOM_FILE_VERSION;
  fwrite(&rfh, sizeof(room_file_header), 1, opty);

  if (rfh.version >= 5) {
    fputc(BLOCKTYPE_MAIN, opty);
    long blsii = 0;
    fwrite(&blsii, 4, 1, opty);
  }

  fwrite(&rstruc.numobj, 2, 1, opty);
  fwrite(&rstruc.objyval[0], 2, rstruc.numobj, opty);

  if (rfh.version >= 9) {
    fwrite(&rstruc.numhotspots, sizeof(int), 1, opty);
    fwrite(&rstruc.hscond[0], sizeof(EventBlock), MAX_HOTSPOTS, opty);
    fwrite(&rstruc.objcond[0], sizeof(EventBlock), MAX_INIT_SPR, opty);
    fwrite(&rstruc.misccond, sizeof(EventBlock), 1, opty);
    fwrite(&rstruc.hswalkto[0], sizeof(_Point), MAX_HOTSPOTS, opty);
    fwrite(&rstruc.hotspotnames[0][0], 30, MAX_HOTSPOTS, opty);
    fwrite(&rstruc.numwalkareas, sizeof(int), 1, opty);
    fwrite(&rstruc.wallpoints[0], sizeof(PolyPoints), rstruc.numwalkareas, opty);
  } else { // Adventure Creator
    fwrite(&rstruc.whataction[0], 2, NUM_CONDIT, opty);
    fwrite(&rstruc.val1[0], 2, NUM_CONDIT, opty);
    fwrite(&rstruc.val2[0], 2, NUM_CONDIT, opty);
    fwrite(&rstruc.otcond[0], 2, NUM_CONDIT, opty);
    fwrite(&rstruc.points[0], 1, NUM_CONDIT, opty);
  }

  fwrite(&rstruc.top, 2, 1, opty);
  fwrite(&rstruc.bottom, 2, 1, opty);
  fwrite(&rstruc.left, 2, 1, opty);
  fwrite(&rstruc.right, 2, 1, opty);
  fwrite(&rstruc.numsprs, 2, 1, opty);
  fwrite(&rstruc.sprs[0], sizeof(sprstruc), rstruc.numsprs, opty);

  if (rfh.version >= 9) { // v2
    fwrite(&rstruc.objbaseline[0], sizeof(int), rstruc.numsprs, opty);
    fwrite(&rstruc.width, 2, 1, opty);
    fwrite(&rstruc.height, 2, 1, opty);
  }

  if (rfh.version >= 11)
    fwrite(&rstruc.resolution, 2, 1, opty);

  if (rfh.version >= 10)
    fwrite(&rstruc.walk_area_zoom[0], sizeof(short), MAX_WALK_AREAS, opty);

  fwrite(&rstruc.password[0], 11, 1, opty);
  fwrite(&rstruc.options[0], 10, 1, opty);
  fwrite(&rstruc.nummes, 2, 1, opty);

  if (rfh.version >= 3)
    fwrite(&rstruc.msgi[0], sizeof(MessageInfo), rstruc.nummes, opty);

  for (f = 0; f < rstruc.nummes; f++)
    fputstring(rstruc.message[f], opty);

  if (rfh.version >= 6) {
    fwrite(&rstruc.numanims, 2, 1, opty);
    if (rstruc.numanims > 0)
      fwrite(&rstruc.anims[0], sizeof(FullAnimation), rstruc.numanims, opty);
  }

  if (rfh.version >= 4) {
    save_script_configuration(opty);
    save_graphical_scripts(opty, &rstruc);
  }

  if (rfh.version >= 8)
    fwrite(&rstruc.shadinginfo[0], sizeof(short), 16, opty);

  long xoff = ftell(opty);
  fclose(opty);

  long tesl;
  if (rfh.version >= 5)
    tesl = save_lzw(files, rstruc.bscene, rstruc.pal, xoff);
  else
    tesl = savecompressed_allegro(files, rstruc.bscene, rstruc.pal, xoff);

  if (rfh.version >= 8)
    tesl = savecompressed_allegro(files, rstruc.shading, rstruc.pal, tesl);

  tesl = savecompressed_allegro(files, rstruc.walls, rstruc.pal, tesl);
  tesl = savecompressed_allegro(files, rstruc.object, rstruc.pal, tesl);
  tesl = savecompressed_allegro(files, rstruc.lookat, rstruc.pal, tesl);

  if (rfh.version >= 5) {
    opty = fopen(files, "r+b");
    long lee = filelength(fileno(opty)) - 7;
    fseek(opty, 3, SEEK_SET);
    fwrite(&lee, 4, 1, opty);
    fseek(opty, 0, SEEK_END);

    if (rstruc.scripts!=NULL) {
      int ff;
      fputc(BLOCKTYPE_SCRIPT, opty);
      lee = strlen(rstruc.scripts) + 4;
      fwrite(&lee, sizeof(int), 1, opty);
      lee -= 4;

      for (ff = 0; ff < lee; ff++)
        rstruc.scripts[ff] -= passwencstring[ff % 11];

      fwrite(&lee, 4, 1, opty);
      fwrite(rstruc.scripts, lee, 1, opty);

      for (ff = 0; ff < lee; ff++)
        rstruc.scripts[ff] += passwencstring[ff % 11];
    }

    if (rstruc.compiled_script != NULL) {
      fputc(BLOCKTYPE_COMPSCRIPT, opty);
      lee = rstruc.cscriptsize + 4;
      fwrite(&lee, 4, 1, opty);
      lee -= 4;
      fwrite(&lee, 4, 1, opty);
      fwrite(rstruc.compiled_script, lee, 1, opty);
    }

    fputc(BLOCKTYPE_EOF, opty);
    fclose(opty);
  }

  if (rfh.version < 9) {
    for (f = 0; f < 11; f++)
      rstruc.password[f] += 60;
  }
  else {
    for (f = 0; f < 11; f++)
      rstruc.password[f] += passwencstring[f];
  }
}

void encrypt_text(char *toenc) {
  int adx = 0, tobreak = 0;

  while (tobreak == 0) {
    if (toenc[0] == 0)
      tobreak = 1;

    toenc[0] += passwencstring[adx];
    adx++;
    toenc++;

    if (adx > 10)
      adx = 0;
  }
}

void decrypt_text(char*toenc) {
  int adx = 0;

  while (1) {
    toenc[0] -= passwencstring[adx];
    if (toenc[0] == 0)
      break;

    adx++;
    toenc++;

    if (adx > 10)
      adx = 0;
  }
}

void freadmissout(short*pptr,FILE*opty) {
  fread(&pptr[0],2,5,opty);
  fread(&pptr[7],2,NUM_CONDIT-7,opty);
  pptr[5]=pptr[6]=0;
}

#define HS_STANDON  0
#define HS_LOOKAT   1
#define HS_INTERACT 2
#define HS_USEINV   3
#define HS_TALKTO   4
#define OBJ_LOOKAT   0
#define OBJ_INTERACT 1
#define OBJ_TALKTO   2
#define OBJ_USEINV   3
void add_to_eventblock(EventBlock*evpt,int evnt,int whatac,int val1,int data,short scorr) {
  evpt->list[evpt->numcmd]=evnt;
  evpt->respond[evpt->numcmd]=whatac;
  evpt->respondval[evpt->numcmd]=val1;
  evpt->data[evpt->numcmd]=data;
  evpt->score[evpt->numcmd]=scorr;
  evpt->numcmd++;
}

int usesmisccond = 0;

void load_main_block(roomstruct *rstruc, char *files, FILE *opty, room_file_header rfh) {
  int   f, NUMREAD, gsmod;
  long  tesl;

  usesmisccond = 0;
  rstruc->width = 320;
  rstruc->height = 200;
  rstruc->resolution = 1;
  rstruc->numwalkareas = 0;
  rstruc->numhotspots = 0;

  memset(&rstruc->objbaseline[0], 0xff, sizeof(int) * MAX_INIT_SPR);
  memset(&rstruc->hswalkto[0], 0, sizeof(_Point) * MAX_HOTSPOTS);
  memset(&rstruc->walk_area_zoom[0], 0, sizeof(short) * MAX_WALK_AREAS);

  for (f=0;f<MAX_HOTSPOTS;f++) {
    sprintf(rstruc->hotspotnames[f],"HOTSPOT %d",f);
    if (f==0) strcpy(rstruc->hotspotnames[f],"NO HOTSPOT");
  }

  memset(&rstruc->hscond[0], 0, sizeof(EventBlock) * MAX_HOTSPOTS);
  memset(&rstruc->objcond[0], 0, sizeof(EventBlock) * MAX_INIT_SPR);
  memset(&rstruc->misccond, 0, sizeof(EventBlock));

  fread(&rstruc->numobj,2,1,opty);
  NUMREAD = NUM_CONDIT;
  fread(&rstruc->objyval[0],2,rstruc->numobj,opty);

  gsmod = 0;

  if (rfh.version >= 9) {
    fread(&rstruc->numhotspots, sizeof(int), 1, opty);
    fread(&rstruc->hscond[0], sizeof(EventBlock), MAX_HOTSPOTS, opty);
    fread(&rstruc->objcond[0], sizeof(EventBlock), MAX_INIT_SPR, opty);
    fread(&rstruc->misccond, sizeof(EventBlock), 1, opty);
    fread(&rstruc->hswalkto[0], sizeof(_Point), MAX_HOTSPOTS, opty);
    fread(&rstruc->hotspotnames[0], 30, MAX_HOTSPOTS, opty);
    fread(&rstruc->numwalkareas, 4, 1, opty);
    fread(&rstruc->wallpoints[0], sizeof(PolyPoints), rstruc->numwalkareas, opty);
  }
  else {
    if (rfh.version < 7) {
      freadmissout(&rstruc->whataction[0],opty);
      rstruc->whataction[5]=rstruc->whataction[6]=NO_ACTION;
      freadmissout(&rstruc->val1[0],opty);
      freadmissout(&rstruc->val2[0],opty);
      freadmissout(&rstruc->otcond[0],opty);
      fread(&rstruc->points[0],1,5,opty);
      fread(&rstruc->points[7],1,NUMREAD-7,opty);
      gsmod=-2;
    }
    else {
      fread(&rstruc->whataction[0],2,NUMREAD,opty);
      fread(&rstruc->val1[0],2,NUMREAD,opty);
      fread(&rstruc->val2[0],2,NUMREAD,opty);
      fread(&rstruc->otcond[0],2,NUMREAD,opty);
      fread(&rstruc->points[0],1,NUMREAD,opty);
    }
  }

  if (rfh.version < 9) {
    // whataction[7..66] are for 4 hotspot actions (15 each)
    for (f = 0; f < NUMTYPCON; f++) {
      tesl=f+NUMOTCON;
      if (rstruc->whataction[tesl] == GRAPHSCRIPT)
        rstruc->val1[tesl] = tesl+gsmod;
      
      if (rstruc->whataction[tesl] == RUNSCRIPT)
        usesmisccond |= 0x80;
      else if (rstruc->whataction[tesl] != NO_ACTION)
        add_to_eventblock(&rstruc->hscond[f%MAX_OBJ+1], f/MAX_OBJ, rstruc->whataction[tesl],
          rstruc->val1[tesl], rstruc->val2[tesl], rstruc->points[tesl]);
    }

    // whataction[67..106] are for 4 object actions (10 each)
    for (f = 0; f < NUMOBJCON; f++) {
      tesl=f+NUMOTCON+NUMTYPCON;
      if (rstruc->whataction[tesl] == GRAPHSCRIPT)
        rstruc->val1[tesl] = tesl+gsmod;

      if (rstruc->whataction[tesl] == RUNSCRIPT)
        usesmisccond |= 0x80;
      else if (rstruc->whataction[tesl] != NO_ACTION)
        add_to_eventblock(&rstruc->objcond[f%MAX_INIT_SPR], f/MAX_INIT_SPR,
          rstruc->whataction[tesl], rstruc->val1[tesl],
          rstruc->val2[tesl], rstruc->points[tesl]);
    }

    // whataction[0..6] are for other actions
    for (f = 0; f < NUMOTCON; f++) {
      tesl=f;
      if (rstruc->whataction[tesl] == GRAPHSCRIPT)
        rstruc->val1[tesl] = tesl;

      if (rstruc->whataction[tesl] == RUNSCRIPT)
        usesmisccond |= 0x80;
      else if (rstruc->whataction[tesl] == NO_ACTION) ;
      else
        add_to_eventblock(&rstruc->misccond, f, rstruc->whataction[tesl],
          rstruc->val1[tesl], rstruc->val2[tesl], rstruc->points[tesl]);
    }

    // extra misc conditions are not converted
    for (f = 0; f < MISC_COND; f++) {
      if (rstruc->otcond[f] != 0)
        usesmisccond += 1;
    }
  }

  fread(&rstruc->top, 2, 1, opty);
  fread(&rstruc->bottom, 2, 1, opty);
  fread(&rstruc->left, 2, 1, opty);
  fread(&rstruc->right, 2, 1, opty);

  memset(&rstruc->sprs[0], 0, sizeof(sprstruc) * MAX_INIT_SPR);

  fread(&rstruc->numsprs, 2, 1, opty);
  fread(&rstruc->sprs[0], sizeof(sprstruc), rstruc->numsprs, opty);

  if (rfh.version >= 9) {
    fread(&rstruc->objbaseline[0], sizeof(int), rstruc->numsprs, opty);
    fread(&rstruc->width, 2, 1, opty);
    fread(&rstruc->height, 2, 1, opty); 
  }

  if (rfh.version >= 11)
    fread(&rstruc->resolution, 2, 1, opty);

  if (rfh.version >= 10)
    fread(&rstruc->walk_area_zoom[0], sizeof(short), MAX_WALK_AREAS, opty);

  fread(&rstruc->password[0], 11, 1, opty);
  fread(&rstruc->options[0], 10, 1, opty);
  fread(&rstruc->nummes, 2, 1, opty);

  if (rfh.version >= 3)
    fread(&rstruc->msgi[0], sizeof(MessageInfo), rstruc->nummes, opty);
  else
    memset(&rstruc->msgi[0], 0, sizeof(MessageInfo) * MAXMESS);

  for (f = 0; f < rstruc->nummes; f++) {
    char buffre[300];
    fgetstring(buffre, opty);
    rstruc->message[f] = (char*)malloc(strlen(buffre) + 2);
    strcpy(rstruc->message[f], buffre);

    if (buffre[strlen(buffre) - 1] == (char)200) {
      rstruc->message[f][strlen(buffre) - 1] = 0;
      rstruc->msgi[f].flags |= MSG_DISPLAYNEXT;
    }
  }

  if (rfh.version >= 6) {
    fread(&rstruc->numanims, 2, 1, opty);

    if (rstruc->numanims > 0)
      fread(&rstruc->anims[0], sizeof(FullAnimation), rstruc->numanims, opty);
    }
  else {
    rstruc->numanims = 0;
    memset(&rstruc->anims[0], 0, sizeof(FullAnimation) * MAXANIMS);
  }

  if (rfh.version >= 4) {
    load_script_configuration(opty);
    load_graphical_scripts(opty, rstruc);
  }

  clear(rstruc->shading);
  memset(&rstruc->shadinginfo[0], 0, sizeof(short) * 16);

  if (rfh.version >= 8)
    fread(&rstruc->shadinginfo[0], sizeof(short), 16, opty);

  if (rfh.version >= 5) {
    tesl = load_lzw(files, rstruc->bscene, rstruc->pal, ftell(opty));
    rstruc->bscene = recalced;
  }
  else
    tesl = loadcompressed_allegro(opty, &rstruc->bscene, rstruc->pal, ftell(opty));

  if ((rstruc->bscene->w > 320) & (rfh.version < 11))
    rstruc->resolution = 2;

  if (rfh.version >= 8)
    tesl = loadcompressed_allegro(opty, &rstruc->shading, rstruc->pal, tesl);

  tesl = loadcompressed_allegro(opty, &rstruc->walls ,rstruc->pal, tesl);
  tesl = loadcompressed_allegro(opty, &rstruc->object ,rstruc->pal, tesl);
  tesl = loadcompressed_allegro(opty, &rstruc->lookat ,rstruc->pal, tesl);

  if (rfh.version < 9) {
    for (f = 0; f < 11; f++)
      rstruc->password[f] += 60;
  }
  else {
    for (f = 0; f < 11; f++)
      rstruc->password[f] += passwencstring[f];
  }
}

void load_room(char *files, roomstruct *rstruc) {
  FILE *opty;
  room_file_header rfh;

  rstruc->freemessage();
  /*  rstruc->allocall(); */
  if (rstruc->scripts != NULL) {
    free(rstruc->scripts);
    rstruc->scripts = NULL;
  }

  if (rstruc->compiled_script != NULL) {
    free(rstruc->compiled_script);
    rstruc->compiled_script = NULL;
  }

  opty = clibfopen(files, "rb");
  if (opty == NULL)
    quit("Load_Room: File not found");

  fread(&rfh, sizeof(rfh), 1, opty);
  rstruc->wasversion = rfh.version;

  if ((rstruc->wasversion < 2) | (rstruc->wasversion > ROOM_FILE_VERSION))
    quit("Load_Room: Bad packed file. Either the file requires a newer version of\n"
      "this program or the file is corrupt.\n");

  if (rfh.version < 6) {
    if (rfh.version >= 5)
      fseek(opty, 1+4, 1);
    load_main_block(rstruc, files, opty, rfh);
  }
  else {
    int thisblock = 0;
    long bloklen;

    while (thisblock != BLOCKTYPE_EOF) {
      thisblock = fgetc(opty);
      if (thisblock == BLOCKTYPE_EOF)
        break;

      fread(&bloklen, 4, 1, opty);
      bloklen += ftell(opty);  // make it the new position for after block read

      long lee;
      int hh;
      if (thisblock == BLOCKTYPE_MAIN)
        load_main_block(rstruc, files, opty, rfh);
      else if (thisblock == BLOCKTYPE_SCRIPT) {
        fread(&lee, 4, 1, opty);
        rstruc->scripts = (char *)malloc(lee + 5);
        fread(rstruc->scripts, lee, 1, opty);
        rstruc->scripts[lee] = 0;

        for (hh = 0; hh < lee; hh++)
          rstruc->scripts[hh] += passwencstring[hh % 11];
      }
      else if (thisblock == BLOCKTYPE_COMPSCRIPT) {
        fread(&hh, 4, 1, opty);
        rstruc->compiled_script = (scScript)malloc(hh + 5);
        fread(rstruc->compiled_script, hh, 1, opty);
      }
      else
        quit("Load_room: Unknown block type encountered");

      fseek(opty, bloklen, SEEK_SET);
    }
  }

  fclose(opty);
}
#endif  // NOFUNCTIONS

struct ScriptEvent {
  long type     PCKD;   // eg. display message, or if is less
  char sort     PCKD;
  long _using   PCKD;   // ^var1
  long with     PCKD;   // number 3 than 9
  long data     PCKD;
  long branchto PCKD;
  long screeny  PCKD;
  void settype(long);
};

#define MAXINBLOCK 10
struct ScriptBlock {
  long        numevents           PCKD;
  ScriptEvent events[MAXINBLOCK]  PCKD;
};

#define VFLG_FLIPSPRITE 1

struct ViewFrame {
  int   pic;
  short xoffs, yoffs;
  short speed;
  int   flags;
  int   sound;  // play sound when this frame comes round
  int   reserved_for_future[2];
};

#define MAXVIEWFRAME 10
#define MAXVIEWLOOP  8
struct ViewStruct {
  short numloops;
  short numframes[MAXVIEWLOOP];
  ViewFrame frames[MAXVIEWLOOP][MAXVIEWFRAME];
};

#define MCF_ANIMMOVE 1
#define MCF_DISABLED 2
#define MCF_STANDARD 4
#define MCF_HOTSPOT  8  // only animate when over hotspot
struct MouseCursor {
  int   pic;
  short hotx, hoty;
  short view;
  char  name[10];
  char  flags;
  MouseCursor() { pic = 2054; hotx = 0; hoty = 0; name[0] = 0; flags = 0; view = -1; }
};

#define MAX_INV             100
#define CHF_MANUALSCALING   1
#define CHF_FIXVIEW         2     // between SetCharView and ReleaseCharView
#define CHF_NOINTERACT      4
#define CHF_NODIAGONAL      8
#define OCHF_SPEECHCOL      0xff000000
#define OCHF_SPEECHCOLSHIFT 24
struct CharacterInfo {
  int   defview;
  int   talkview;
  int   view;
  int   room, prevroom;
  int   x, y, wait;
  int   flags;
  int   reserved[4];
  int   activeinv;          // this is an INT to support SeeR (no signed shorts)
  short loop, frame;
  short walking, animating;
  short walkspeed, animspeed;
  short inv[100];
  short actx, acty;
  char  name[30];
  char  scrname[16];
  char  on;
};

#define IFLG_STARTWITH 1
struct InventoryItemInfo {
  char name[25];
  int  pic;
  int  cursorPic, hotx, hoty;
  int  reserved[5];
  char flags;
};

#define MAXTOPICOPTIONS     15
#define DFLG_ON             1  // currently enabled
#define DFLG_OFFPERM        2  // off forever (can't be trurned on)
#define DCMD_SAY            1
#define DCMD_OPTOFF         2
#define DCMD_OPTON          3
#define DCMD_RETURN         4
#define DCMD_STOPDIALOG     5
#define DCMD_OPTOFFFOREVER  6
#define DCMD_RUNTEXTSCRIPT  7
#define DCMD_GOTODIALOG     8
#define DCMD_ENDSCRIPT      0xff
struct DialogTopic {
  char          optionnames[MAXTOPICOPTIONS][70];
  int           optionflags[MAXTOPICOPTIONS];
  unsigned char *optionscripts;
  short         entrypoints[MAXTOPICOPTIONS];
  short         startupentrypoint;
  short         codesize;
  int           numoptions;
};

#define MAX_SPRITES         2100
#define MAX_CURSOR          10
#define MAX_CHARACTERS      50
#define PAL_GAMEWIDE        0
#define PAL_LOCKED          1
#define PAL_BACKGROUND      2
#define MAXGLOBALMES        500
#define MAXLANGUAGE         5
#define MAX_FONTS           10
#define OPT_DEBUGMODE       0
#define OPT_SCORESOUND      1
#define OPT_WALKONLOOK      2
#define OPT_DIALOGIFACE     3
#define OPT_ANTIGLIDE       4
#define OPT_TWCUSTOM        5
#define OPT_DIALOGGAP       6

struct OriGameStruct {
  char              gamename[30];
  char              options[20];
  unsigned char     paluses[256];
  color             defpal[256];
  InterfaceElement  iface[10];
  int               numiface;
  int               numviews;
  MouseCursor       mcurs[10];
  char              *globalscript;
  int               numcharacters;
  CharacterInfo     *chars;
  EventBlock        charcond[50];
  EventBlock        invcond[100];
  scScript          compiled_script;
  int               playercharacter;
  unsigned char     spriteflags[MAX_SPRITES];
  int               totalscore;
  short             numinvitems;
  InventoryItemInfo invinfo[100];
  int               numdialog, numdlgmessage;
  int               numfonts;
  int               reserved[7];
  short             numlang;
  char              langcodes[MAXLANGUAGE][3];
  char              *messages[MAXGLOBALMES];
};

struct GameStruct : public OriGameStruct {
  unsigned char     fontflags[MAX_FONTS];
  char              fontoutline[MAX_FONTS];
  int               reserved2[10];
};

#ifndef ROOMEDIT
#define MAXNEEDSTAGES 20
struct MoveList {
  int   pos[MAXNEEDSTAGES];
  int   numstage;
  fixed xpermove[MAXNEEDSTAGES], ypermove[MAXNEEDSTAGES];
  int   fromx, fromy;
  int   onstage, onpart;
  int   lastx, lasty;
  char  doneflag;
  char  direct;
  };
#endif

#endif
