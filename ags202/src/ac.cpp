// Version and build numbers
#define AC_VERSION_TEXT "2.01 "
#define ACI_VERSION_TEXT "2.01.031"
#define THIS_IS_THE_ENGINE

#include <dos.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <io.h>
#include <string.h>

#define USE_CLIB
#include "wgt2allg.h"

#include "acroom.h"

#define INI_READONLY
// #include <myini.h>

// ***** EXTERNS ****
extern "C" {
  extern int csetlib(char*,char*);
  extern int cfopenpriority;
  extern int Executor(scInstance*,int);
  extern int install_amp(void);
  extern int load_amp(char*,int);
  extern int amp_decode(void);
  extern void amp_setvolume(int);
  extern int unload_amp(void);
}
extern void setupmain();
extern int  minstalled();
extern void mgetgraphpos();
extern void msetgraphpos(int,int);
extern void mgraphconfine(int,int,int,int);
extern int  misbuttondown(int);
extern void msethotspot(int,int);
extern int  ismouseinbox(int,int,int,int);
extern char currentcursor;
extern int  mousex,mousey;
extern block mousecurs[10];
extern int   hotx, hoty;
extern char*get_language_text(int);
extern void init_language_text(char*);
extern int  loadgamedialog();
extern int  savegamedialog();
extern int  quitdialog();
extern wgtfont fonts[10];
extern int  enternumberwindow(char*);

// CD Player functions
// flags returned with cd_getstatus
#define CDS_DRIVEOPEN    0x0001  // tray is open
#define CDS_DRIVELOCKED  0x0002  // tray locked shut by software
#define CDS_AUDIOSUPPORT 0x0010  // supports audio CDs
#define CDS_DRIVEEMPTY   0x0800  // no CD in drive
// function definitions
extern int  cd_installed();
extern int  cd_getversion();
extern int  cd_getdriveletters(char*);
extern void cd_driverinit(int);
extern void cd_driverclose(int);
extern long cd_getstatus(int);
extern void cd_playtrack(int,int);
extern void cd_stopmusic(int);
extern void cd_resumemusic(int);
extern void cd_eject(int);
extern void cd_uneject(int);
extern int  cd_getlasttrack(int);
extern int  cd_isplayingaudio(int);

#include "acruntim.h"

extern char*scripttempn;

#define RESTART_POINT_SAVE_GAME_NUMBER 999

block virtual_screen;
int scrnwid,scrnhit;
int scrnwto,scrnhto;
int sxmult=1,symult=1;
roomstruct thisroom;
GameStruct game;
RoomStatus *roomstats;
RoomStatus troom;    // used for non-saveable rooms, eg. intro
GameState play;
GameSetup usetup;
int game_paused=0;
int ifacepopped=-1;  // currently displayed pop-up GUI (-1 if none)
color palette[256];
block images[MAX_SPRITES];
struct time t1,t2;
int cur_mode,cur_cursor;
int moffsx,moffsy;
int spritewidth[MAX_SPRITES],spriteheight[MAX_SPRITES];
RoomObject*objs;
RoomStatus*croom=NULL;
CharacterInfo*playerchar;
scScript compiled_script=NULL;
scInstance*gameinst=NULL,*roominst=NULL;
ScriptBlock*scnf=NULL;
ViewStruct*views=NULL;
ScriptMouse scmouse;
ScriptSystem scsystem;
block _old_screen=NULL;
block screenop = NULL;
int user_disabled_for=0,user_disabled_data=0,user_disabled_data2=0;
int user_disabled_data3=0;
DialogTopic *dialog;
char*messages[MAXGLOBALMES];
int _global_x_offset=-1, _global_y_offset=0;
int screen_state=0;
int ccSymOffset=0;
int proper_exit=0;
int scaddr=0;
#define MAXEVENTS 15
EventHappened event[MAXEVENTS];
int numevents=0;
#define EV_TEXTSCRIPT 1
#define EV_RUNEVBLOCK 2
#define EV_FADEIN     3
#define EV_IFACECLICK 4
#define TS_REPEAT   1
#define TS_KEYPRESS 2
#define TS_MCLICK   3
#define EVB_HOTSPOT 1
#define EVB_ROOM    2
int cur_music_number=0;

#define REP_EXEC_NAME "repeatedly_execute"

char*tsnames[4]={NULL, REP_EXEC_NAME, "on_key_press","on_mouse_click"};
char*evblockbasename;
int evblocknum;
MIDI*midiptr=NULL;
JGMOD*modptr=NULL;
int current_music=0;
int frames_per_second=40;
int in_new_room=0, new_room_was = 0;  // 1 in new room, 2 first time in new room, 3 loading saved game
int new_room_pos=0;
int move_speed=3;
int displayed_room=0;
int restrict_until=0;
int mouse_on_iface=-1;   // mouse cursor is over this interface
int mouse_on_iface_button=-1;
int mouse_pushed_iface=-1;  // this BUTTON on interface MOUSE_ON_IFACE is pushed
int request_newroom=0;
int request_invscreen=0;
int request_restoregame=0;
int inside_script=0,in_graph_script=0;
int request_dialog=-1;
char request_func[20];
int mouse_frame=0,mouse_delay=0;
int lastmx=-1,lastmy=-1;
int new_room_flags=0;
#define MAX_SPRITES_ON_SCREEN 40
SpriteListEntry sprlist[MAX_SPRITES_ON_SCREEN];
int sprlistsize=0;
block actsps[MAX_INIT_SPR+MAX_CHARACTERS];
int fps=0;
unsigned long loopcounter=0,lastcounter=0;
int offsetx=0,offsety=0;

int libamp_loaded=0;
int use_cdplayer=0;
int numcddrives=0;
char cd_driveletters[26],cddrive;

void set_mouse_cursor(int);
int  run_text_script(scInstance*,char*);
void run_graph_script(int);
void run_event_block(EventBlock*,int,int=-1);
void new_room(int,CharacterInfo*);
void NewRoom(int);
void AnimateObject(int,int,int,int);
void SetObjectView(int,int);
void GiveScore(int);
void walk_character(int,int,int,int,int);
void StopMoving(int);
void MoveCharacterToHotspot(int,int);
int  GetCursorMode();
void save_game(int,char*);
int  load_game(int);
void update_music_volume();
int  invscreen();
void process_interface_click(int,int);
void do_conversation(int);
void setevent(int evtyp,int ev1=0,int ev2=-1000,int ev3=0);

char filetouse[80]="C:\\TC\\CHRISJ.INI";
char*mytmpini="~myini.tmp";

int ini_length=0;
int ini_read=0;

#define INI_NO_SECTIONS     0
#define INI_INVALID_SECTION 1
#define INI_NO_VALUE        2
#define INI_EOF_ERROR       3
#define INI_PARSE_ERROR     4
#define INI_DISK_IS_FULL    5
#define INI_TMP_ERROR       6
#define INI_OUT_OF_MEMORY   7

void INIerror(int errnum) {
  const char *errors[8] = {
    "No sections in file",
    "Need ']' to end section line",
    "Missing '=' in definition",
    "Unexpected EOF",
    "Parse Error",
    "Disk full",
    "Temporary file error",
    "Out of memory"
  };
  printf("\n\nINI_Lib (E1%02d): %s\n", errnum + 50, errors[errnum]);
  exit(10);
}

int INIeof() {
  if (ini_read>ini_length) return 1;
  return 0;
}

int INIreadchar(FILE*iii) {
  ini_read++;
  return fgetc(iii);
}

char*INIreaditem(char*sectn,const char*entry,int errornosect=1,int offs=0) {
  ini_read=0;
  FILE*fin=fopen(filetouse,"rb");
  if (fin==NULL) INIerror(INI_EOF_ERROR);
  fseek(fin,offs,SEEK_SET);
  ini_length=filelength(fileno(fin))-offs;
  strupr(sectn);
  int atstart;
  int ch=0,ii;
  char*templine=(char*)malloc(180);
  if (!templine) INIerror(INI_OUT_OF_MEMORY);
  char*tempval;
  atstart=1;
  for (;;) { // find section
    for (;((ch=INIreadchar(fin))!='[') || !atstart;) {
      if ((ch=='\n') | (ch=='\r')) atstart=1;
      else atstart=0;
      if (INIeof()) break;
    }
    if (ch!='[') {
      if (errornosect==1) INIerror(INI_NO_SECTIONS);
      fclose(fin);
      return NULL;
    }
    for (ii=0;(templine[ii]=INIreadchar(fin))!=']';ii++) {
      if (templine[ii]=='\r') INIerror(INI_INVALID_SECTION);
    }
    templine[ii]=0;
    strupr(templine);
    if (strcasecmp(sectn,templine)==0) break;
  }
  for (ii=0;ii<60;ii++) templine[ii]=0;
  tempval=(char*)malloc(160);
  INIreadchar(fin);
  INIreadchar(fin);
  if (!tempval) INIerror(INI_OUT_OF_MEMORY);
  for (;(ch=INIreadchar(fin))!='[';) { // read key=value part
    int jj;ii=1;
    if (ch=='\r') { INIreadchar(fin); continue; }
    if (ch==';') { // skip comments
      for (;INIreadchar(fin)!='\n';) { if (INIeof()) break; }
      continue;
    }
    if (INIeof()) {
      free(tempval);
      fclose(fin);
      return NULL;
    }
    templine[0]=ch;
    for (;(templine[ii]=INIreadchar(fin))!='=';ii++) { // read key
      if (templine[ii]=='\r') INIerror(INI_NO_VALUE);
      if (INIeof()) {
        free(tempval);
        fclose(fin);
        return NULL;
      }
    }
    templine[ii]=0;
    for (jj=ii-1;jj>0;jj--) { // strip trailing whitespace
      if (templine[jj]==' ') templine[jj]=0;
      else break;
    }
    if (strcasecmp(templine,entry)==0) { // key found
      if (INIreadchar(fin)!=' ') {
        fseek(fin,-1,SEEK_CUR);
        ini_read--;
      }
      for (ii=0;(tempval[ii]=INIreadchar(fin))!='\r';) { ii++;
        if (INIeof()) INIerror(INI_EOF_ERROR);
      }
      tempval[ii]=0;
      fclose(fin);
      return tempval;
    }
    for (;INIreadchar(fin)!='\r';) { // skip to newline
      if (INIeof()) INIerror(INI_NO_VALUE);
    }
    INIreadchar(fin);
  }
  free(templine);
  free(tempval);
  fclose(fin);
  return NULL;
}

int INIreadint(char*sectn,char*item,int errornosect=1) {
  char*tempstr=INIreaditem(sectn,item,errornosect);
  if (tempstr==NULL) return -1;
  int toret=atoi(tempstr);
  free(tempstr);
  return toret;
}

// Replace the filename part of complete path WASGV with INIFIL
void INIgetdirec(char*wasgv,char*inifil) {
  int u=strlen(wasgv)-1;
  for (u=strlen(wasgv)-1;u>=0;u--) {
    if (wasgv[u]=='\\') {
      memcpy(&wasgv[u+1],inifil,strlen(inifil)+1);
      break;
    }
  }
  if (u<=0) INIerror(INI_PARSE_ERROR);
}

void restart_game() {
  if (load_game(RESTART_POINT_SAVE_GAME_NUMBER)!=0)
    quit("unable to restart game (unknown error)");
}

void setpal() {
  wsetpalette(0,255,palette);
}

// override packfile functions to allow it to load from our
// custom CLIB datafiles
extern "C" {
PACKFILE*_my_temppack;
extern char* clibgetdatafile(char*);
extern PACKFILE *__old_pack_fopen(char*,char*);

PACKFILE *pack_fopen(char*filnam,char*modd) {
  if (cliboffset(filnam)<1) {
    return __old_pack_fopen(filnam,modd);
  }
  else {
    _my_temppack=__old_pack_fopen(lib_file_name,modd);
    pack_fseek(_my_temppack,cliboffset(filnam));
    _my_temppack->todo=clibfilesize(filnam);
    return _my_temppack;
  }
}

} // end extern "C"

// end packfile functions

volatile int timerloop=0;
int time_between_timers=25;  // in milliseconds
// our timer, used to keep game running at same speed on all systems
void dj_timer_handler(...) {
  timerloop++;
  }
END_OF_FUNCTION(dj_timer_handler);

void set_game_speed(int fps) {
  frames_per_second=fps;
  time_between_timers=1000/fps;
  install_int_ex(dj_timer_handler,MSEC_TO_TIMER(time_between_timers));
}

// writes the virtual screen to the screen
void write_screen() {
  abuf=screen;
  wputblock(0,0,virtual_screen,0);
}

void cremovemenu() {
  wputblock(0,0,virtual_screen,0);
  wsetscreen(virtual_screen);
}

extern char buffer2[60];
int oldmouse;
void setup_for_dialog() {
  write_screen();
  domouse(1);
  oldmouse=cur_cursor; set_mouse_cursor(CURS_ARROW);
}
void restore_after_dialog() {
  set_mouse_cursor(oldmouse);
  domouse(2);
  cremovemenu();
}

void restore_game_dialog() {
  if (inside_script) {
    request_restoregame++;
    return;
  }
  setup_for_dialog();
  int toload=loadgamedialog();
  restore_after_dialog();
  if (toload>=0) {
    int ecret=load_game(toload);
    if (ecret<0) Display("Unable to load game (error code %d).",ecret);
  }
}

void save_game_dialog() {
  setup_for_dialog();
  int toload=savegamedialog();
  restore_after_dialog();
  if (toload>=0)
    save_game(toload,buffer2);
  }

#define Register(inst) ((int*)(inst)->data_stack)
#define regIP 244
#define regSP 245
#define regCP 248
int prepare_text_script(scInstance*sci,char*tsname) {
  scErrorNo=0;
  if (sci==NULL) return -1;
  ccSymOffset=scGet_Symbol(sci,tsname);
  if (ccSymOffset<0) return -2;
  if (Register(sci)[regCP]!=0) {
    return -3;
  }
  request_newroom=-1;
  request_invscreen=0;
  request_restoregame=0;
  request_dialog=-1;
  request_func[0]=0;
  scmouse.x=mousex/sxmult;
  scmouse.y=mousey/symult;
  inside_script++;
  aborted_ip=0;
  abort_executor=0;
  return 0;
  }

void post_script_cleanup() {
  // should do any post-script stuff here, like go to new room
  if (scErrorNo) quit(scErrorMsg);
  if (abort_executor) user_disabled_data2=aborted_ip;
  if (request_invscreen) {
    invscreen();
    request_invscreen=0;
  }
  if (request_newroom>=0) {
    new_room(request_newroom,playerchar);
    request_newroom=-1;
  }
  if (request_func[0]!=0) {
    run_text_script(gameinst,request_func);
  }
  inside_script--;
  if (request_dialog>=0) {
    int dlgnum=request_dialog;
    request_dialog=-1;
    do_conversation(dlgnum);
  }
  if (request_restoregame) {
    request_restoregame--;
    restore_game_dialog();
  }
}

int run_text_script(scInstance*sci,char*tsname) {
  int toret=prepare_text_script(sci,tsname);
  if (toret!=0) return toret;
  toret=scCall_Instance(sci,ccSymOffset);
  post_script_cleanup();
  return toret;
}

int run_text_script_iparam(scInstance*sci,char*tsname,int iparam) {
  int toret=prepare_text_script(sci,tsname);
  if (toret!=0) return toret;
  toret=scCall_Instance(sci,ccSymOffset,iparam);
  post_script_cleanup();
  return toret;
}

int run_text_script_2iparam(scInstance*sci,char*tsname,int iparam,int param2) {
  int toret=prepare_text_script(sci,tsname);
  if (toret!=0) return toret;
  toret=scCall_Instance(sci,ccSymOffset,iparam,param2);
  post_script_cleanup();
  return toret;
}

void my_fade_from_range(PALLETE source,PALLETE dest,int speed,int from,int to) {
  PALLETE temp;
  int c;

  for (c=0;c<PAL_SIZE;c++)
    temp[c]=source[c];
  for (c=0;c<64;c+=speed) {
    fade_interpolate(source,dest,temp,c,from,to);
    set_pallete_range(temp,from,to,TRUE);
    if (libamp_loaded) {
      if (amp_decode()<0) {
        unload_amp();
        libamp_loaded=0;
      }
    }
    set_pallete_range(temp,from,to,TRUE);
  }
  set_pallete_range(dest,from,to,TRUE);
}

void my_fade_out(int spdd,int from,int to) {
  PALETTE temp;

  get_pallete(temp);
  my_fade_from_range(temp,black_pallete,spdd,from,to);
}

void FadeOut(int spdd) {
  my_fade_out(spdd,0,255);
}

void my_fade_in(PALLETE p, int speed) {
  my_fade_from_range(black_pallete,p,speed,0,255);
}

block fix_bitmap_size(block todubl) {
  if (todubl->w==scrnwid) return todubl;
  int oldw=todubl->w, oldh=todubl->h;
  block tempb=create_bitmap(thisroom.width*sxmult,thisroom.height*symult);
  set_clip(tempb,0,0,tempb->w-1,tempb->h-1);
  set_clip(todubl,0,0,oldw-1,oldh-1);
  clear(tempb);
  stretch_blit(todubl,tempb,0,0,oldw,oldh,0,0,tempb->w,tempb->h);
  destroy_bitmap(todubl); todubl=tempb;
  return todubl;
}

#define ToCodeINT(inst) ((int*)(inst)->code)
#define hdrDataSize 10
#define hdrStackSize 12
void save_room_data_segment () {
  FadeOut(5);
  if (croom==NULL) return;
  if (roominst!=NULL) {
    croom->tsdatasize=ToCodeINT(roominst)[hdrDataSize];
    if (croom->tsdata!=NULL) free(croom->tsdata);
    croom->tsdata=(char*)malloc(croom->tsdatasize+10);
    memcpy(croom->tsdata,roominst->data_stack+256*4,croom->tsdatasize);
    scFree_Instance(roominst);
    roominst=NULL;
  }
  else croom->tsdatasize=0;
}

// forchar = playerchar on NewRoom, or NULL if restore saved game
void load_new_room(int newnum,CharacterInfo*forchar) {
  char rmfile[20];
  int cc,ee;
  displayed_room=newnum;
  sprintf(rmfile,"room%d.crm",newnum);
  if (newnum==0) {
    strcpy(rmfile,"intro.crm");
  }
  destroy_bitmap(thisroom.bscene);
  destroy_bitmap(thisroom.object);
  thisroom.bscene=create_bitmap(320,200);
  thisroom.object=create_bitmap(320,200);
  // load the room from disk
  load_room(rmfile,&thisroom);
  in_new_room=1;
  for (cc=0;cc<thisroom.walls->w;cc++) {
    for (ee=0;ee<thisroom.walls->h;ee++) {
      if (_getpixel(thisroom.walls,cc,ee)!=253) ;
      else _putpixel(thisroom.walls,cc,ee,0);
    }
  }
  // convert background to current res
  if (sxmult==thisroom.resolution) ;
  else thisroom.bscene=fix_bitmap_size(thisroom.bscene);
    // fix walk-behinds to current screen resolution
  thisroom.object=fix_bitmap_size(thisroom.object);
  troom.beenhere=0;
  if ((newnum>=0) & (newnum<MAX_ROOMS))
    croom=&roomstats[newnum];
  else croom=&troom;
  if (croom->beenhere==0) {
    croom->numobj=thisroom.numsprs;
    for (cc=0;cc<croom->numobj;cc++) {
      croom->obj[cc].x=thisroom.sprs[cc].x;
      croom->obj[cc].y=thisroom.sprs[cc].y+spriteheight[thisroom.sprs[cc].sprnum]/symult;
      croom->obj[cc].num=thisroom.sprs[cc].sprnum;
      croom->obj[cc].on=thisroom.sprs[cc].on;
      croom->obj[cc].view=-1;
      croom->obj[cc].loop=0;
      croom->obj[cc].frame=0;
      croom->obj[cc].wait=0;
      croom->obj[cc].baseline=-1;
      if (thisroom.objbaseline[cc]>=0)
//        croom->obj[cc].baseoffs=thisroom.objbaseline[cc]-thisroom.sprs[cc].y;
        croom->obj[cc].baseline=thisroom.objbaseline[cc];
    }
    for (cc=0;cc<MAX_FLAGS;cc++) croom->flagstates[cc]=0;
    // we copy these structs for the Score column to work
    croom->misccond=thisroom.misccond;
    for (cc=0;cc<MAX_HOTSPOTS;cc++)
      croom->hscond[cc]=thisroom.hscond[cc];
    for (cc=0;cc<MAX_INIT_SPR;cc++)
      croom->objcond[cc]=thisroom.objcond[cc];
    croom->beenhere=1;
    in_new_room=2;
  }
  objs=&croom->obj[0];
  for (cc=0;cc<PAL_SIZE;cc++) {
    if (game.paluses[cc]==PAL_BACKGROUND) palette[cc]=thisroom.pal[cc];
  }
  offsetx=0;
  offsety=0;
  if (forchar!=NULL) {
    forchar->prevroom=forchar->room;
    forchar->room=newnum;
    forchar->walking=0;
  }
  roominst=NULL;
  if (thisroom.compiled_script!=NULL) {
    roominst=scCreate_Instance(thisroom.compiled_script,"");
    if (croom->tsdatasize>0) {
      if (croom->tsdatasize!=ToCodeINT(roominst)[hdrDataSize])
        quit("room script data segment size has changed");
      memcpy(roominst->data_stack+256*4,croom->tsdata,croom->tsdatasize);
      }
    }
  if ((new_room_pos>0) & (forchar!=NULL)) {
    if (new_room_pos>=4000) {
      forchar->y=thisroom.top+1;
      forchar->x=new_room_pos%1000;
      if (forchar->x==0) forchar->x=thisroom.width/2;
      forchar->loop=0;
      }
    else if (new_room_pos>=3000) {
      forchar->y=thisroom.bottom-1;
      forchar->x=new_room_pos%1000;
      if (forchar->x==0) forchar->x=thisroom.width/2;
      forchar->loop=3;
      }
    else if (new_room_pos>=2000) {
      forchar->x=thisroom.right-1;
      forchar->y=new_room_pos%1000;
      if (forchar->y==0) forchar->y=thisroom.height/2;
      forchar->loop=1;
      }
    else if (new_room_pos>=1000) {
      forchar->x=thisroom.left+1;
      forchar->y=new_room_pos%1000;
      if (forchar->y==0) forchar->y=thisroom.height/2;
      forchar->loop=2;
      }
    // if starts on un-walkable area
    if (getpixel(thisroom.walls,forchar->x,forchar->y)==0) {
      if (new_room_pos>=3000) { // bottom or top of screen
        int tryleft=forchar->x-1,tryright=forchar->x+1;
        while (1) {
          if (getpixel(thisroom.walls,tryleft,forchar->y)>0) {
            forchar->x=tryleft; break; }
          if (getpixel(thisroom.walls,tryright,forchar->y)>0) {
            forchar->x=tryright; break; }
          int nowhere=0;
          if (tryleft>thisroom.left) { tryleft--; nowhere++; }
          if (tryright<thisroom.right) { tryright++; nowhere++; }
          if (nowhere==0) break;  // no place to go, so leave him
          }
        }
      else if (new_room_pos>=1000) { // left or right
        int tryleft=forchar->y-1,tryright=forchar->y+1;
        while (1) {
          if (getpixel(thisroom.walls,forchar->x,tryleft)>0) {
            forchar->y=tryleft; break; }
          if (getpixel(thisroom.walls,forchar->x,tryright)>0) {
            forchar->y=tryright; break; }
          int nowhere=0;
          if (tryleft>thisroom.top) { tryleft--; nowhere++; }
          if (tryright<thisroom.bottom) { tryright++; nowhere++; }
          if (nowhere==0) break;  // no place to go, so leave him
          }
        }
      }
    new_room_pos=0;
    }
  if (thisroom.options[ST_TUNE]>0)
    newmusic(thisroom.options[ST_TUNE]);
  if (forchar!=NULL) {
    if (thisroom.options[ST_MANDISABLED]==0) { forchar->on=1;
      enable_cursor_mode(0); }
    else {
      forchar->on=0;
      disable_cursor_mode(0);
    }
    if (forchar->flags & CHF_FIXVIEW) ;
    else if (thisroom.options[ST_MANVIEW]==0) forchar->view=forchar->defview;
    else forchar->view=thisroom.options[ST_MANVIEW]-1;
    forchar->frame=0;   // make him standing
    }
  update_music_volume();
  new_room_flags=0;
  play.gscript_timer=-1;  // avoid screw-ups with changing screens
  // trash any input which they might have done while it was loading
  while (kbhit()) { getch(); }
  while (mgetbutton()!=NONE) ;
}

void new_room(int newnum,CharacterInfo*forchar) {
  save_room_data_segment();
  load_new_room(newnum,forchar);
}

void main_loop_until(int untilwhat,int udata,int mousestuff) {
  play.disabled_user_interface++;
  set_mouse_cursor(CURS_WAIT);
  restrict_until=untilwhat;
  user_disabled_data=udata;
  return;
}

// action  1: set object view
//         2: animate
//         3: move to
//         4: move to (ignore walls)
//         5: place at
void run_animation_stage(AnimationStruct*stage) {
  char buf[200];
  if (stage->action==0) quit("!undefined animation command");
  if (stage->object==MANOBJNUM) ;
  else if (stage->object>9)
    quit("!invalid object number specified for animation. must be 0-9 or 99.");
  if (stage->action==1) { // set object view
    if (stage->data==0) stage->data=playerchar->defview;
    if (stage->object==MANOBJNUM) {
      playerchar->view=stage->data-1;
      playerchar->loop=0;
      playerchar->frame=0;
    }
    else SetObjectView(stage->object,stage->data);
  }
  else if (stage->action==2) { // animate
    if (stage->object==MANOBJNUM) {
      playerchar->animating=1;
      if ((stage->y & 1)!=0) playerchar->animating|=CHANIM_REPEAT;
      playerchar->animating|=stage->speed<<8;
      playerchar->loop=stage->data;
      playerchar->frame=0;
      playerchar->wait=views[playerchar->view].frames[playerchar->loop][0].speed+playerchar->animspeed;
      if (stage->wait)
        do_main_cycle(UNTIL_CHARIS0,(int)&playerchar->animating);
    }
    else {
      AnimateObject(stage->object,stage->data,stage->speed,stage->y & 1);
      if (stage->wait)
        do_main_cycle(UNTIL_CHARIS0,(int)&objs[stage->object].cycling);
    }
  }
  else if ((stage->action==3) | (stage->action==4)) { // move to
    if (stage->object!=MANOBJNUM) quit("only the player object can move");
    int ignwal=0;
    if (stage->action==4) ignwal=1;
    walk_character(game.playercharacter,stage->x,stage->y,ignwal,false);
    if (stage->wait) do_main_cycle(UNTIL_MOVEEND,(int)&playerchar->walking);
  }
  else if (stage->action==5) { // place at
    if (stage->object==MANOBJNUM) {
      playerchar->x=stage->x;
      playerchar->y=stage->y;
    }
    else {
      objs[stage->object].x=stage->x;
      objs[stage->object].y=stage->y;
    }
  }
  else quit("unknown animation encountered");
}

void run_animation(FullAnimation*anim,int stage) {
  int ee;

  if (restrict_until==0) {
    for (ee=stage;ee<anim->numstages;ee++) {
      run_animation_stage(&anim->stage[ee]);
      if (restrict_until) {
        user_disabled_for=UNTIL_ANIMEND;
        user_disabled_data=(int)anim;
        user_disabled_data2=ee+1;
        break;
      }
      if (/* ??? */ 0) break;
    }
  }
}

// event list functions
void setevent(int evtyp,int ev1,int ev2,int ev3) {
  event[numevents].type=evtyp;
  event[numevents].data1=ev1;
  event[numevents].data2=ev2;
  event[numevents].data3=ev3;
  event[numevents].player=game.playercharacter;
  numevents++;
  if (numevents>=MAXEVENTS) quit("too many events posted");
}

void process_event(EventHappened*evp) {
  EventBlock*evpt;
  if (evp->type==EV_TEXTSCRIPT) {
    evpt=NULL;
    scErrorNo=0;
    if (evp->data2>-1000)
      evpt=(EventBlock*)run_text_script_iparam(gameinst,tsnames[evp->data1],evp->data2);
    else
      evpt=(EventBlock*)run_text_script(gameinst,tsnames[evp->data1]);
  }
  else if (evp->type==EV_RUNEVBLOCK) {
    evpt=NULL;
    if (evp->data1==EVB_HOTSPOT) {
      evpt=&croom->hscond[evp->data2];
      evblockbasename="hotspot%d";
      evblocknum=evp->data2;
    }
    else if (evp->data1==EVB_ROOM) {
      evpt=&croom->misccond;
      evblockbasename="room";
    }
    if (evpt==NULL)
      quit("process_event: RunEvBlock: unknown evb type");
    run_event_block(evpt,evp->data3,-1);
  }
  else if (evp->type==EV_FADEIN)
    my_fade_in(palette,5);
  else if (evp->type==EV_IFACECLICK)
    process_interface_click(evp->data1,evp->data2);
  else quit("process_event: unknown event to process");
}

int inside_processevent=0;
void processallevents(int numev,EventHappened*evlist) {
  int dd;
  if (inside_processevent) return;
  inside_processevent++;
  for (dd=0;dd<numev;dd++) {
    process_event(&evlist[dd]);
    if (in_new_room!=0) continue;
  }
  inside_processevent--;
}

// mouse cursor functions:
// set_mouse_cursor: changes visual appearance to specified cursor
void set_mouse_cursor(int newcurs) {
  mousecurs[0]=images[game.mcurs[newcurs].pic];
  msethotspot(game.mcurs[newcurs].hotx,game.mcurs[newcurs].hoty);
  cur_cursor=newcurs;
  mouse_frame=0;
  mouse_delay=0;
}

// set_default_cursor: resets visual appearance to current mode (walk, look, etc)
void set_default_cursor() {
  set_mouse_cursor(cur_mode);
  }

void find_next_enabled_cursor(int startwith) {
  int testing=startwith;
  do {
    if ((game.mcurs[testing].flags & MCF_STANDARD)==0) ;
    else if ((game.mcurs[testing].flags & MCF_DISABLED)==0)
      break;
    testing++;
    if (testing>=MAX_CURSOR) testing=0;
  } while (testing!=startwith);
  if (testing!=startwith)
    set_cursor_mode(testing);
}

// set_cursor_mode: changes mode and appearance
void set_cursor_mode(int newmode) {
  if (game.mcurs[newmode].flags & MCF_DISABLED) {
    find_next_enabled_cursor(newmode);
    return; }
  cur_mode=newmode;
  set_default_cursor();
}

void enable_cursor_mode(int modd) {
  game.mcurs[modd].flags&=~MCF_DISABLED;
  // now search the interfaces for related buttons to re-enable
  int uu,ww;
  int mod=modd | (IBACT_SETMODE << 16);
  for (uu=0;uu<game.numiface;uu++) {
    for (ww=0;ww<game.iface[uu].numbuttons;ww++) {
      if ((game.iface[uu].button[ww].leftclick)!=mod) continue;
      game.iface[uu].button[ww].flags=IBFLG_ENABLED;
      }
    }
  }

void disable_cursor_mode(int modd) {
  game.mcurs[modd].flags|=MCF_DISABLED;
  // now search the interfaces for related buttons to kill
  int uu,ww;
  int mod=modd | (IBACT_SETMODE << 16);
  for (uu=0;uu<game.numiface;uu++) {
    for (ww=0;ww<game.iface[uu].numbuttons;ww++) {
      if ((game.iface[uu].button[ww].leftclick)!=mod) continue;
      game.iface[uu].button[ww].flags=0;
      }
    }
  if (cur_mode==modd) find_next_enabled_cursor(modd);
  }

void remove_popup_interface(int ifacenum) {
  ifacepopped=-1;
  set_default_cursor();
  game_paused--;
  if (mousey<=game.iface[ifacenum].popupyp)
    msetgraphpos(mousex,game.iface[ifacenum].popupyp+2);
  if (play.disabled_user_interface!=0)
    set_mouse_cursor(CURS_WAIT);
  if (ifacenum==mouse_on_iface) mouse_on_iface=-1;
  }

void process_interface_click(int ifce,int btn) {
  int mod=game.iface[ifce].button[btn].leftclick;
  int rtype=(mod>>16) & 0x000ffff;
  int rdata=mod & 0x000ffff;
  if (rtype==0) ;
  else if (rtype==IBACT_SETMODE)
    set_cursor_mode(rdata);
  else if (rtype==IBACT_SCRIPT)
    run_text_script_2iparam(gameinst,"interface_click",ifce+1,btn);
}

// check_controls: checks mouse & keyboard interface
void check_controls() {
  int ll;
  for (ll=0;ll<game.numiface;ll++) {
    if (game.iface[ll].popup!=POPUP_MOUSEY) continue;
    if (screen_state==2) break;  // interfaces disabled
    if (game.iface[ll].on==0) continue;
    if (ifacepopped==ll) continue;
    if (mousey < game.iface[ll].popupyp) {
      set_mouse_cursor(CURS_ARROW);
      ifacepopped=ll; game_paused++;
      break;
    }
  }
  if ((mouse_pushed_iface>=0) && (mouse_on_iface_button!=mouse_pushed_iface))
    mouse_pushed_iface=-1;
  if (mouse_pushed_iface>=0) {
    if (misbuttondown(LEFT)==0) {
      int wasonie=mouse_on_iface, wasbutdown=mouse_pushed_iface;
      mouse_pushed_iface=-1;
      if ((game.iface[wasonie].popup==POPUP_MOUSEY) && (ifacepopped==wasonie))
        remove_popup_interface(wasonie);
      setevent(EV_IFACECLICK,wasonie,wasbutdown,0);
    }
  }
  ll=mgetbutton();
  if (ll>NONE) {
    if (mouse_on_iface>=0) {
      InterfaceElement*iep=&game.iface[mouse_on_iface];
      int pushedie=-1;
      for (ll=0;ll<iep->numbuttons;ll++) {
        if (iep->button[ll].flags==0) continue;
        int xoffs=iep->x+iep->button[ll].x;
        int yoffs=iep->y+iep->button[ll].y;
        if (ismouseinbox(xoffs,yoffs,xoffs+spritewidth[iep->button[ll].pic],
              yoffs+spriteheight[iep->button[ll].pic])==-1) {
          pushedie=ll;
          break;
        }
      }
      if (pushedie>=0)
        mouse_pushed_iface=pushedie;
    }
    else {
      if (screen_state==1) {
        screen_state=0;
        _global_x_offset=-1;
        wfreeblock(screenop);
        screenop=NULL;
      }
      else if (play.disabled_user_interface!=0) ;
      else setevent(EV_TEXTSCRIPT,TS_MCLICK,ll+1,0);
    }
  }
  // check keypresses
  if (kbhit()) {
    int kgn = getch();
    if (kgn==0) kgn=getch()+300;
//    if (kgn==2) { Display("Crashing"); strcpy(NULL, NULL); }
    //if (kgn==2) Display("Some for�ign text");
    //if (kgn==2) do_conversation(5);
    if ((kgn>='a') & (kgn<='z')) kgn-=32;
    if (kgn==367) restart_game();
    if (screen_state==1) {
      screen_state=0;
      wfreeblock(screenop);
      screenop=NULL;
    }
    else setevent(EV_TEXTSCRIPT,TS_KEYPRESS,kgn);
  }
  int edge=-1;
  if (playerchar->x<=thisroom.left) edge=0;
  else if (playerchar->x>=thisroom.right) edge=1;
  if (playerchar->y>=thisroom.bottom) edge=2;
  else if (playerchar->y<=thisroom.top) edge=3;
  if (edge>=0)
    setevent(EV_RUNEVBLOCK,EVB_ROOM,0,edge);
}  // end check_controls

MoveList mls[60];
int fix_player_sprite(MoveList*cmls) {
  int want_horiz=1,useloop=-1;
  if (abs(fixtoi(cmls->ypermove[cmls->onstage]))>abs(fixtoi(cmls->xpermove[cmls->onstage])))
    want_horiz=0;
  if ((want_horiz==1) & (fixtoi(cmls->xpermove[cmls->onstage])>0))
    useloop=2;
  else if ((want_horiz==1) & (fixtoi(cmls->xpermove[cmls->onstage])<0))
    useloop=1;
  else if (fixtoi(cmls->ypermove[cmls->onstage])<0)
    useloop=3;
  else useloop=0;
  return useloop;
}

void update_stuff() {
  int aa;
  // update graphics for object if cycling view
  for (aa=0;aa<croom->numobj;aa++) {
    if (objs[aa].on==0) continue;
    if (objs[aa].cycling==0) continue;
    if (objs[aa].view<0) continue;
    if (objs[aa].wait>0) { objs[aa].wait--; continue; }
    objs[aa].frame++;
    if (objs[aa].frame>=views[objs[aa].view].numframes[objs[aa].loop]) {
      if (objs[aa].cycling==ANIM_ONCE) {
        // leave it on the last frame
        objs[aa].cycling=0;
        objs[aa].frame--;
        }
      else if (objs[aa].cycling==ANIM_ONCERESET) {
        objs[aa].cycling=0;
        objs[aa].frame=0;
        }
      else objs[aa].frame=0;
    }
    ViewFrame*vfptr=&views[objs[aa].view].frames[objs[aa].loop][objs[aa].frame];
    objs[aa].num=vfptr->pic;
    objs[aa].wait=vfptr->speed+objs[aa].overall_speed;
  }
  // shadow areas
  int onwalkarea=getpixel(thisroom.walls,playerchar->x,playerchar->y);
  if (onwalkarea<0) ;
  else if (playerchar->flags & CHF_FIXVIEW) ;
  else { onwalkarea=thisroom.shadinginfo[onwalkarea];
    if (onwalkarea>0) playerchar->view=onwalkarea-1;
    else if (thisroom.options[ST_MANVIEW]==0) playerchar->view=playerchar->defview;
    else playerchar->view=thisroom.options[ST_MANVIEW]-1;
  }
  // move & animate characters
  for (aa=0;aa<game.numcharacters;aa++) {
    if (game.chars[aa].on==0) continue;
    CharacterInfo*chi=&game.chars[aa];
    if (chi->walking>0) {
      MoveList*mlp=&mls[chi->walking]; int walk;
      if ((chi->wait>0) & (game.options[OPT_ANTIGLIDE]>0)) ;
      else { walk=0;
        if (do_movelist_move(&chi->walking,&chi->x,&chi->y)==2) { walk++;
          if (walk) chi->loop=fix_player_sprite(&mls[chi->walking]);
        }
      }
      if (chi->frame>views[chi->view].numframes[chi->loop])
        chi->frame=1;
      if (chi->walking<1) {
        chi->wait=0;
        chi->frame=0;
      }
      else if (chi->wait>0) chi->wait--;
      else {
        chi->frame++;
        if (chi->frame>=views[chi->view].numframes[chi->loop]) {
          // end of loop, so loop back round skipping the standing frame
          chi->frame=1;
        }
        chi->wait=views[chi->view].frames[chi->loop][chi->frame].speed+chi->animspeed;
      }
    }
    else {
      if (chi->animating!=0) {
        if (chi->wait>0) { chi->wait--; continue; }
        chi->frame++;
        if (chi->frame>=views[chi->view].numframes[chi->loop]) {
          if ((chi->animating & CHANIM_REPEAT)==0) {
            chi->animating=0;
            chi->frame--;
          }
          else chi->frame=0;
        }
        chi->wait=views[chi->view].frames[chi->loop][chi->frame].speed;
        chi->wait+=(chi->animating>>8) & 0x00ff;
      }
    }
  }
  // determine if speech text should be removed
  if (play.messagetime>=0) {
    play.messagetime--;
    if (play.messagetime<1)
    {
      wfreeblock(screenop);
      screenop=NULL;
      screen_state=0;
      _global_x_offset=-1;
    }
  }
  if (play.gscript_timer>0) play.gscript_timer--;
}

char oritext[50];
void printtext(int xx,int yy,int ww, char*text) {
  char*curptr=text; char tmpm[3];
  oritext[0]=0;
  int unk=2;
  int align=0;
  char*endat=curptr+strlen(text);
  char defalign[3];
  wgtfont font=cbuttfont;

  while (1) {
    if (curptr>=endat) {
      strcpy(defalign,"$l");
      curptr=defalign;
    }
    if (curptr[0]=='@') {
      char macroname[20]; int idd=0; curptr++;
      for (idd=0;idd<20;idd++) {
        if (curptr[0]=='@') {
          macroname[idd]=0;
          curptr++;
          break;
        }
        macroname[idd]=curptr[0];
        curptr++;
      }
      macroname[idd]=0; 
      char tempo[50];
      tempo[0]=0;
      if (stricmp(macroname,"score")==0)
        sprintf(tempo,"%d",play.score);
      else if (stricmp(macroname,"totalscore")==0)
        sprintf(tempo,"%d",MAXSCORE);
      else if (stricmp(macroname,"scoretext")==0)
        sprintf(tempo,get_language_text(29),play.score,MAXSCORE);
      else if (stricmp(macroname,"gamename")==0)
        strcpy(tempo, game.gamename);
      else if (stricmp(macroname,"overhotspot")==0) {
        int loc=getpixel(thisroom.lookat,(mousex+offsetx)/sxmult,(mousey+offsety)/symult);
        if (loc<1) tempo[0]=0;
        else strcpy(tempo,thisroom.hotspotnames[loc]);
      }
      strcat(oritext,tempo);
    }
    else if (curptr[0]=='$') {
      int ctrlcode;
      int outxp;
      ctrlcode=tolower(curptr[1]); curptr+=2;
      int outyp=2;
      if (align==0)
        outxp=2;
      else if (align==1)
        outxp=ww/2-wgettextwidth(oritext,font)/2;
      else
        outxp=ww-1-wgettextwidth(oritext,font);
      wouttextxy(outxp+xx,outyp+yy,font,oritext);
      oritext[0]=0;
      if (curptr==&defalign[2]) break;
      if (ctrlcode=='c')
        align=1; // centre
      else if (ctrlcode=='l')
        align=0; // left
      else
        align=2; // right
    }
    else {
      tmpm[0]=curptr[0]; tmpm[1]=0;
      strcat(oritext,tmpm);
      curptr++;
    }
  }
}

void draw_interface(InterfaceElement*iep,int ienum) {
  if (iep->on==0) return;
  if (iep->bgcol<0)
    wputblock(iep->x,iep->y,images[-iep->bgcol],0);
  else {
    currentcolor=iep->bgcol;
    wbar(iep->x,iep->y,iep->x2,iep->y2);
  }
  currentcolor=iep->bordercol;
  wrectangle(iep->x,iep->y,iep->x2,iep->y2);
  if (ismouseinbox(iep->x,iep->y,iep->x2,iep->y2)==-1)
    mouse_on_iface=ienum;

  int ee,pic;
  int tdxp,tdyp;
  for (ee=0;ee<iep->numbuttons;ee++) {
    pic=iep->button[ee].pic;
    tdxp=iep->button[ee].x+iep->x;
    tdyp=iep->button[ee].y+iep->y;
    if (mouse_on_iface!=ienum) ;
    else if (iep->button[ee].flags==0) ;
    else if (play.disabled_user_interface!=0) ;
    else {
      if (ismouseinbox(tdxp,tdyp,tdxp+spritewidth[iep->button[ee].pic],tdyp+spriteheight[iep->button[ee].pic])==-1) {
        mouse_on_iface_button=ee;
        if ((mouse_pushed_iface==ee) && (iep->button[ee].pushpic>0))
          pic=iep->button[ee].pushpic;
        else if (iep->button[ee].overpic>0)
          pic=iep->button[ee].overpic;
      }
    }
    wputblock(tdxp,tdyp,images[pic],0);
    if ((iep->button[ee].flags==0) | (play.disabled_user_interface>0)) {
      int wid,hit;
      for (wid=0;wid<images[pic]->w;wid++) { // grid pattern
        for (hit=wid%2;hit<images[pic]->h;hit+=2)
          _putpixel(abuf,tdxp+wid,tdyp+hit,16);
      }
    }
  }

  wgtfont fnt;
  int outxp,outyp;
  if (iep->vtext[0]!=0) {
    fnt=cbuttfont;
    outxp=iep->vtextxp+iep->x;
    outyp=iep->vtextyp+iep->y;
    int textwid=wgettextwidth(iep->vtext,fnt);
    if (iep->vtextalign==VTA_CENTRE)
      outxp-=textwid/2;
    if (iep->vtextalign==VTA_RIGHT)
      outxp-=textwid;
    wtextcolor(iep->fgcol);
    wtexttransparent(TEXTFG);
    printtext(outxp,outyp,iep->x2-iep->x,iep->vtext);
  }
}

void sort_out_walk_behinds(block sprit,int xx,int yy,int basel) {
  int ee,rr,tmm;
  for (ee=0;ee<sprit->w;ee++) {
    for (rr=0;rr<sprit->h;rr++) {
      tmm = getpixel(thisroom.object,ee+xx,rr+yy);
      if (tmm<1) continue;
      if (thisroom.objyval[tmm]<=basel) continue;
      _putpixel(sprit,ee,rr,_getpixel(thisroom.bscene,ee+xx,rr+yy));
    }
  }
}

void clear_sprite_list() {
  sprlistsize=0;
}

void add_to_sprite_list(block spp,int xx,int yy,int basel) {
  sprlist[sprlistsize].pic=spp;
  sprlist[sprlistsize].baseline=basel;
  sprlist[sprlistsize].x=xx;
  sprlist[sprlistsize].y=yy;
  sprlistsize++;
  if (sprlistsize>=MAX_SPRITES_ON_SCREEN-1)
    quit("ad_to_sprite_list: roo many sprite added");
}

void draw_sprite_list() {
  char sprs[40];
  int baselines[40];
  int ee,spr,ii,rr;
  for (ee=0;ee<40;ee++) {
    sprs[ee]=-1;
    baselines[ee]=9999;
  }
  for (ee=0;ee<sprlistsize;ee++) {
    for (ii=0;ii<sprlistsize;ii++) {
      if (sprlist[ee].baseline<baselines[ii]) {
        for (rr=sprlistsize-2;rr>=ii;rr--) {
          sprs[rr+1]=sprs[rr];
          baselines[rr+1]=baselines[rr];
        }
        sprs[ii]=ee;
        baselines[ii]=sprlist[ee].baseline;
        break;
      }
    }
  }
  for (ee=0;ee<sprlistsize;ee++) {
    spr=sprs[ee];
    wputblock(sprlist[spr].x,sprlist[spr].y,sprlist[spr].pic,1);
  }
}

// draw_screen_background: draws the background scene, all the interfaces
// and objects; basically, the entire screen
void draw_screen_background() {
  int aa,zoom_level,widd,hitt;
  int onarea,sppic;
  int atxp,atyp;
  if (screen_state==2) return;
  if ((thisroom.width>320) | (thisroom.height>200)) {
    offsetx=playerchar->x*sxmult-scrnwid/2;
    offsety=playerchar->y*symult-scrnhit/2;
    if (offsetx<0) offsetx=0;
    if (offsety<0) offsety=0;
    if (offsetx+scrnwid>thisroom.width*sxmult)
      offsetx=thisroom.width*sxmult-scrnwid;
    if (offsety+scrnhit>thisroom.height*symult)
      offsety=thisroom.height*symult-scrnhit;
  }
  wputblock(-offsetx,-offsety,thisroom.bscene,0);
  clear_sprite_list();
  // draw objects
  for (aa=0;aa<croom->numobj;aa++) {
    if (objs[aa].on==0) continue;
    int useindx=aa;
    if (actsps[useindx]!=NULL) wfreeblock(actsps[useindx]);
    atxp=objs[aa].x*sxmult-offsetx;
    atyp=objs[aa].y*symult-spriteheight[objs[aa].num]-offsety;
    actsps[useindx]=wallocblock(spritewidth[objs[aa].num],spriteheight[objs[aa].num]);
    clear(actsps[useindx]);
    draw_sprite(actsps[useindx],images[objs[aa].num],0,0);
    sort_out_walk_behinds(actsps[useindx],atxp+offsetx,atyp+offsety,objs[aa].y);
    add_to_sprite_list(actsps[useindx],atxp,atyp,(thisroom.objbaseline[aa]<1)?objs[aa].y:thisroom.objbaseline[aa]);
  }
  // draw characters
  for (aa=0;aa<game.numcharacters;aa++) {
    if (game.chars[aa].on==0) continue;
    if (game.chars[aa].room!=displayed_room) continue;
    int useindx=aa+MAX_INIT_SPR;
    if (actsps[useindx]!=NULL) wfreeblock(actsps[useindx]);
    CharacterInfo*chin=&game.chars[aa];
    sppic=views[chin->view].frames[chin->loop][chin->frame].pic;
    onarea=getpixel(thisroom.walls,chin->x,chin->y);
    if (onarea==0) {
      // the path finder sometimes slightly goes into non-walkable areas;
      // so check for scaling in adjacent pixels
      const int TRYGAP=2;
      onarea=getpixel(thisroom.walls,chin->x+2,chin->y);
      if (onarea==0) onarea=getpixel(thisroom.walls,chin->x-2,chin->y);
      if (onarea==0) onarea=getpixel(thisroom.walls,chin->x,chin->y+2);
      if (onarea==0) onarea=getpixel(thisroom.walls,chin->x,chin->y-2);
    }
    zoom_level=100;
    if (chin->flags & CHF_MANUALSCALING) ;
    else if ((onarea>=0) & (onarea<MAX_WALK_AREAS)) {
      zoom_level=thisroom.walk_area_zoom[onarea]+100;
    }
    if (zoom_level!=100) {
      // it needs to be stretched, so calculate the new dimensions
      widd=(spritewidth[sppic]*zoom_level)/100;
      hitt=(spriteheight[sppic]*zoom_level)/100;
      atxp=(chin->x*sxmult)-offsetx-(widd/2);
      atyp=(chin->y*symult)-hitt-offsety;
      actsps[useindx]=wallocblock(widd,hitt);
      clear(actsps[useindx]);
      stretch_sprite(actsps[useindx],images[sppic],0,0,widd,hitt);
      sort_out_walk_behinds(actsps[useindx],atxp+offsetx,atyp+offsety,chin->y);
    }
    else {
      // draw at original size, so just use the sprite width and height
      atxp=(chin->x*sxmult)-offsetx-(spritewidth[sppic]/2);
      atyp=(chin->y*symult)-spriteheight[sppic]-offsety;
      actsps[useindx]=wallocblock(spritewidth[sppic],spriteheight[sppic]);
      clear(actsps[useindx]);
      draw_sprite(actsps[useindx],images[sppic],0,0);
      sort_out_walk_behinds(actsps[useindx],atxp+offsetx,atyp+offsety,chin->y);
    }
    add_to_sprite_list(actsps[useindx],atxp,atyp,chin->y);
    chin->actx=atxp+offsetx;
    chin->acty=atyp+offsety;
  }
  draw_sprite_list();
  // draw interfaces
  mouse_on_iface_button=-1;
  for (aa=0;aa<game.numiface;aa++) {
    if (game.iface[aa].popup==POPUP_MOUSEY) continue;
    draw_interface(&game.iface[aa],aa);
  }
}

void draw_topbar() {
  if ((ifacepopped>=0) & (screen_state!=2)) {
    draw_interface(&game.iface[ifacepopped],ifacepopped);
    int wasoniface=ifacepopped;
    if (mousey>game.iface[ifacepopped].y2) ifacepopped=-1;
    if (ifacepopped==-1) remove_popup_interface(wasoniface);
  }

  if (_global_x_offset>=0)
    wputblock(_global_x_offset,_global_y_offset,screenop,1);
}

// update_screen: copies the contents of the virtual screen to the actual
// screen, and draws the mouse cursor on.
void update_screen() {
  // update animating mouse cursor
  if (game.mcurs[cur_cursor].view>=0) {
    mgetgraphpos();
    // only on mousemove, and it's not moving
    if (((game.mcurs[cur_cursor].flags & MCF_ANIMMOVE)!=0) &&
      (mousex==lastmx) && (mousey==lastmy)) ;
    else if (mouse_delay>0) mouse_delay--;
    else {
      int viewnum=game.mcurs[cur_cursor].view;
      int frame=0;
      mouse_frame++;
      if (mouse_frame>=views[viewnum].numframes[frame])
        mouse_frame=0;
      mousecurs[0]=images[views[viewnum].frames[frame][mouse_frame].pic];
      mouse_delay=views[viewnum].frames[frame][mouse_frame].speed+5;
    }
    lastmx=mousex; lastmy=mousey;
  }

  int mwasaty=mousey, mwasatx=mousex;
  domouse(1);
  abuf=screen;
  wputblock(0,0,virtual_screen,0);
  wsetscreen(virtual_screen);
  domouse(2);
  moffsx=offsetx+mousex;
  moffsy=offsety+mousey;
  if ((mwasaty!=mousey) | (mwasatx!=mousex)) {
    int onhs=getpixel(thisroom.lookat,moffsx/sxmult,moffsy/symult);
    setevent(EV_RUNEVBLOCK,EVB_HOTSPOT,onhs,6);
  }
}

void atexit_handler() {
  if (proper_exit==0) {
    printf("\nError: the program has exited without requesting it.\n"
      "If you see a list of numbers above, please write them down and contact\n"
      "Chris Jones. Otherwise, note down any other information displayed.\n");
  }
}

// quit - exits the engine, shutting down everything gracefully
// The parameter is the message to print. If this message begins with
// an '!' character, then it is printed as a "contact game author" error.
// If it begins with a '|' then it is treated as a "thanks for playing" type
// message. If it begins with anything else, it is treated as an internal
// error.
void quit(char*quitmsg) {
  set_gfx_mode(GFX_TEXT,80,25,0,0);
  allegro_exit();

  if (quitmsg[0]=='|') ; //quitmsg++;
  else if (quitmsg[0]=='!') { 
    quitmsg++;
    printf(
      "An error has occured. Please contact the game author for support, as this\n"
      "problem is caused by the game rather than the interpreter.\n"
      "(ACI version " ACI_VERSION_TEXT ")\n\nError: "
    );
  }
  else {
    printf(
      "An internal error has occured. Please note down the following information.\n"
      "If the problem persists, contact Chris Jones.\n"
      "(ACI version " ACI_VERSION_TEXT ")\n\nError: "
    );
  }

  if (quitmsg[0]=='|') ;
  else printf("%s\n",quitmsg);

  if (play.debug_mode!=0)
    printf("Average fps: %d\n",fps);

  remove("agssave.999");
  system("del ~ac*.tmp");
  proper_exit=1;
  exit(3);
}

void setup_sierra_interface() {
  int rr; InterfaceElement*iep;
  game.numiface=2;
  iep=&game.iface[0];
  // top status bar
  iep->numbuttons=0;
  strcpy(iep->vtext,"@SCORETEXT@$r$@GAMENAME@");
  iep->vtextxp=2; iep->vtextyp=1;
  iep->vtextalign=VTA_LEFT;
  iep->x=0; iep->y=0;
  iep->x2=319; iep->y2=13;
  iep->popup=POPUP_NONE;
  iep->bordercol=0;
  iep->fgcol=0; iep->bgcol=8;
  // top popup panel
  iep=&game.iface[1];
  iep->popup=POPUP_MOUSEY; iep->popupyp=13;
  iep->x=0; iep->y=14;
  iep->x2=319; iep->y2=iep->y+32;
  iep->vtext[0]=0;
  iep->bordercol=8;
  iep->bgcol=8; iep->fgcol=15;
  iep->numbuttons=10;
  for (rr=0;rr<iep->numbuttons;rr++)
    iep->button[rr].set(rr*32,0,rr+2043,0,0);
  // default palette usage
  for (rr=0;rr<42;rr++) game.paluses[rr]=PAL_GAMEWIDE;
  for (rr=42;rr<256;rr++) game.paluses[rr]=PAL_BACKGROUND;
}

int load_game_file() {
  int ee;

  FILE*iii=clibfopen("ac2game.dta","rb");
  if (iii==NULL) return -1;

  char teststr[31];
  teststr[30]=0;
  fread(&teststr[0],30,1,iii);
  int filever=getw(iii);
  if (filever!=6) {
    fclose(iii);
    return -2;
  }
  fread(&game,sizeof(GameStruct),1,iii);

  // skip script source
  int cscriptsize=getw(iii);
  fseek(iii,cscriptsize,SEEK_CUR);

  cscriptsize=getw(iii);
  compiled_script=(scScript)malloc(cscriptsize+5);
  fread(compiled_script,cscriptsize,1,iii);

  views=(ViewStruct*)malloc(sizeof(ViewStruct)*game.numviews+20);
  fread(&views[0],sizeof(ViewStruct),game.numviews,iii);

  // skip sprites folders data
  cscriptsize=getw(iii);
  fseek(iii,cscriptsize*sizeof(SpritesFolder),SEEK_CUR);

  game.chars=(CharacterInfo*)calloc(1,sizeof(CharacterInfo)*game.numcharacters+5);
  fread(&game.chars[0],sizeof(CharacterInfo),game.numcharacters,iii);

  for (ee=0;ee<MAXGLOBALMES;ee++) {
    if (game.messages[ee]==NULL) continue;
    game.messages[ee]=(char*)malloc(500);
    fgetstring(game.messages[ee],iii);
  }

  dialog=(DialogTopic*)malloc(sizeof(DialogTopic)*game.numdialog+5);
  fread(&dialog[0],sizeof(DialogTopic),game.numdialog,iii);
  for (ee=0;ee<game.numdialog;ee++) {
    if (dialog[ee].optionscripts!=NULL) {
      dialog[ee].optionscripts=(unsigned char*)malloc(dialog[ee].codesize+10);
      fread(&dialog[ee].optionscripts[0],dialog[ee].codesize,1,iii);
    }
    // skip dialog source
    fseek(iii,getw(iii),SEEK_CUR);
  }

  for (ee=0;ee<game.numdlgmessage;ee++) {
    messages[ee]=(char *)malloc(200);
    fgetstring(messages[ee],iii);
  }

  fclose(iii);

  if (game.numfonts==0) return -2;

  for (ee=0;ee<game.numcharacters;ee++) {
    game.chars[ee].walking=0;
    game.chars[ee].animating=0;
  }

  for (ee=0;ee<game.numiface;ee++) {
    if (game.iface[ee].popup==POPUP_SCRIPT) game.iface[ee].on=0;
    else game.iface[ee].on=1;
  }

  scAdd_External_Symbol("character",&game.chars[0]);
  playerchar=&game.chars[game.playercharacter];
  scAdd_External_Symbol("player",playerchar);
  gameinst = (scInstance*)scCreate_Instance(compiled_script,"");
  if (gameinst == NULL) return -3;

  char filnm[20];
  for (ee=0;ee<game.numfonts;ee++) {
    sprintf(filnm,"agsfnt%d.wfn",ee);
    fonts[ee]=wloadfont(filnm);
  }
  usetup.textheight=wgettextheight("ZHwypqhkilIK",fonts[0])+1;

  return 0;
}

void poll_mp3() {
  if (libamp_loaded!=0) {
    if (amp_decode()<0) {
      unload_amp();
      libamp_loaded=0;
    }
  }
}

void do_corner(int sprn,int xx1,int yy1,int typx,int typy) {
  if (sprn<0) return;
  block pic=images[sprn];
  wputblock(xx1+typx*spritewidth[sprn],yy1+typy*spriteheight[sprn],pic,0);
}

void draw_button_background(int xx1,int yy1,int xx2,int yy2,InterfaceElement*iep) {
  if (iep==NULL) {  // standard window
    rectfill(abuf,xx1,yy1,xx2,yy2,15);
    rect(abuf,xx1,yy1,xx2,yy2,0);
    }
  else {
    wsetcolor(iep->bgcol);
    wbar(xx1,yy1,xx2,yy2);
    int uu;
    for (uu=yy1;uu<=yy2;uu+=spriteheight[iep->button[4].pic]) {
      do_corner(iep->button[4].pic,xx1,uu,-1,0);   // left side
      do_corner(iep->button[5].pic,xx2+1,uu,0,0);  // right side
      }
    for (uu=xx1;uu<=xx2;uu+=spritewidth[iep->button[6].pic]) {
      do_corner(iep->button[6].pic,uu,yy1,0,-1);  // top side
      do_corner(iep->button[7].pic,uu,yy2+1,0,0); // bottom side
      }
    do_corner(iep->button[0].pic,xx1,yy1,-1,-1);  // top left
    do_corner(iep->button[1].pic,xx1,yy2+1,-1,0);  // bottom left
    do_corner(iep->button[2].pic,xx2+1,yy1,0,-1);  //  top right
    do_corner(iep->button[3].pic,xx2+1,yy2+1,0,0);  // bottom right
    }
  }

char lines[MAXLINE][100];
int numlines=0,longestline=0;
void break_up_text_into_lines(int wii,int fonnt,char *todis) {
  char*theline=todis;
  char*win=todis;
  numlines=0;
  int done=0;
  while (done==0) {
    if (win[0]=='[') {
      win[0]=0; strcpy(lines[numlines],theline);
      win[0]='['; numlines++;
      if (numlines>=MAXLINE)
        quit("!Display: message takes up too many lines");
      theline=&win[1];
    }
    if ((win[0]==0) | (win[0]==' ')) {
      if (win[0]==0) done=1;
      int chr=win[0]; win[0]=0;
      int cutline=0;
      if (wii<=wgettextwidth(theline,fonts[fonnt])) cutline=1;
      if (chr=='[') cutline=1;
      if (cutline) {
        if (!done) win[0]=' ';
        int rr=-1;
        while (win[rr]!=' ') {
          rr--;
          if (rr<-strlen(theline))
            quit("!Display: single word longer than window width");
        }
        win[rr]=0; strcpy(lines[numlines],theline);
        numlines++;
        if (numlines>=MAXLINE)
          quit("!Display: message takes up too many lines");
        win[rr]=' ';
        theline=&win[rr+1];
        win=&theline[-1];
      }
      if (done==0) win[0]=chr;
      if (done==1) {
        strcpy(lines[numlines],theline);
        numlines++;
      }
    }
    win++;
  }
  longestline=0;
  int rr;
  for (rr=0;rr<numlines;rr++) {
    if (longestline<wgettextwidth(lines[rr],fonts[fonnt]))
      longestline=wgettextwidth(lines[rr],fonts[fonnt]);
  }
}

void ensure_text_valid(char*text) {
  while (text[0]!=0) {
    if ((unsigned char)text[0]>126)
      quit("!Display: Message contains invalid characters (ASCII >= 127)");
    text++;
  }
}

void _display_at(int xx,int yy,int wii,char*todis,int blocking,int asspch) {
  int usingfont=FONT_NORMAL;
  if (asspch) usingfont=FONT_SPEECHBACK;
  ensure_text_valid(todis);
  break_up_text_into_lines(wii-6,usingfont,todis);
  int texthit = wgettextheight("ZHwypqhkilIK",fonts[usingfont]);
  if (yy<0) yy=(scrnhit/2-(numlines*texthit)/2)-3;
  if (longestline<wii-6) wii=longestline+6;
  if (xx<-1) { xx=(-xx)-wii/2;
    if (xx<0) xx = 0;
    if (xx+wii>=scrnwid)
      xx=scrnwid-wii;
  }
  else if (xx<0) xx=scrnwid/2-wii/2;
  wtextcolor(15);
  if (screenop!=NULL) wfreeblock(screenop);
  screenop=create_bitmap(wii,numlines*texthit+6);
  wsetscreen(screenop); clear(screenop);
  clear_to_color(abuf,0);
  int ee;
  if (asspch) {
    for (ee=0;ee<numlines;ee++) {
      int ttxp=wii/2 - wgettextwidth(lines[ee],fonts[usingfont])/2;
      int ttyp=ee*texthit+3;
      if (game.fontoutline[usingfont]>=0) {
        wtextcolor(16);
        wgtprintf(ttxp,ttyp,fonts[game.fontoutline[usingfont]],lines[ee]);
      }
      wtextcolor(asspch);
      wgtprintf(ttxp,ttyp,fonts[usingfont],lines[ee]);
    }
  }
  else {
    if (game.options[OPT_TWCUSTOM]==0) {
      draw_button_background(0,0,abuf->w-1,abuf->h-1,NULL);
      wtextcolor(0);
      for (ee=0;ee<numlines;ee++)
        wgtprintf(3,ee*texthit+3,fonts[usingfont],lines[ee]);
    }
    else {
      int ifnum=game.options[OPT_TWCUSTOM]-1;
      int tbnum=game.iface[ifnum].button[0].pic;
      wii+=spritewidth[tbnum]*2;
      xx-=spritewidth[tbnum];
      yy-=spriteheight[tbnum];
      wfreeblock(screenop);
      screenop=create_bitmap(wii,numlines*texthit+6+spriteheight[tbnum]*2);
      clear(screenop); wsetscreen(screenop);
      int xoffs=spritewidth[tbnum];
      int yoffs=spriteheight[tbnum];
      draw_button_background(xoffs,yoffs,(abuf->w-xoffs)-1,(abuf->h-yoffs)-1,&game.iface[ifnum]);
      wtextcolor(game.iface[ifnum].fgcol);
      for (ee=0;ee<numlines;ee++)
        wgtprintf(xoffs+3,yoffs+3+ee*texthit,fonts[usingfont],lines[ee]);
    }
  }
  _global_x_offset=xx;
  _global_y_offset=yy;
  screen_state=1;
  wsetscreen(virtual_screen);
  if (blocking) {
    write_screen();
    wputblock(xx,yy,screenop,0);
    screen_state=0;
    _global_x_offset=-1;
    domouse(1);
    while (1) {
      domouse(0);
      poll_mp3();
      if (mgetbutton()>NONE) break;
      if (kbhit()) {
        getch();
        break;
      }
    }
    domouse(2);
    cremovemenu();
  }
  else {
    do_main_cycle(UNTIL_NOOVERLAY,0);
  }
  play.messagetime=-1;
}

void DisplayAt(int xxp,int yyp,int widd,char*texx, ...) {
  char displbuf[STD_BUFFER_SIZE];
  va_list ap;
  va_start(ap,texx);
  vsprintf(displbuf,texx,ap);
  va_end(ap);
  xxp*=sxmult;
  yyp*=symult;
  widd*=sxmult;
  if (widd<1) widd=scrnwid/2;
  if (xxp<0) xxp=scrnwid/2-widd/2;
  _display_at(xxp,yyp,widd,displbuf,1,0);
  }

void Display(char*texx, ...) {
  char displbuf[STD_BUFFER_SIZE];
  va_list ap;
  va_start(ap,texx);
  vsprintf(displbuf,texx,ap);
  va_end(ap);
  _display_at(-1,-1,scrnwid/2+scrnwid/4,displbuf,1,0);
}

#define CHANIM_SPEED 5
void DisplaySpeech(char*texx,int askip,int aschar) {
  if (askip!=0)
    play.messagetime=(strlen(texx)/15+1)*frames_per_second;
  int textcol=(game.chars[aschar].flags & OCHF_SPEECHCOL) >> OCHF_SPEECHCOLSHIFT;
  textcol&=0xff;
  int widd=scrnwid/2+scrnwid/4;
  int tdxp=-1,tdyp=-1;
  int oldview=-1;
  if (game.chars[aschar].room==displayed_room) {
    tdxp=game.chars[aschar].x*sxmult-offsetx;
    if (tdxp<2) tdxp=2;
    int sppic=views[game.chars[aschar].view].frames[game.chars[aschar].loop][game.chars[aschar].frame].pic;
    tdyp=game.chars[aschar].y*symult-spriteheight[sppic]-offsety-25;
    if (tdyp<5) tdyp=5;
    tdxp=-tdxp;  // tell it to centre it
    if (game.chars[aschar].talkview>=0) {
      oldview=game.chars[aschar].view;
      game.chars[aschar].animating=(1 | CHANIM_REPEAT | (CHANIM_SPEED << 8));
      game.chars[aschar].view=game.chars[aschar].talkview;
      game.chars[aschar].frame=0;
      game.chars[aschar].flags|=CHF_FIXVIEW;
    }
  }
  _display_at(tdxp,tdyp,widd,texx,0,textcol);
  if (oldview>=0) {
    game.chars[aschar].flags&=(~CHF_FIXVIEW);
    game.chars[aschar].view=oldview;
    game.chars[aschar].animating=0;
    game.chars[aschar].frame=0;
  }
}

int display_message=0;
void DisplayMessage(int msnum) {
  display_message=-1;
  if (msnum>=500) { //quit("global message requseted, nto yet supported");
    if (game.messages[msnum-500]==NULL)
      quit("!DisplayGlobalMessage: message not exist");
    Display(game.messages[msnum-500]);
    return;
  }
  else {
    if (msnum>=thisroom.nummes)
      quit("!DisplayMessage: Invalid message number to display");
    int repeatloop=1;
    while (repeatloop) {
      display_message=msnum;
      if (thisroom.msgi[msnum].displayas>0) {
        DisplaySpeech(thisroom.message[msnum],
          thisroom.msgi[msnum].flags & MSG_TIMELIMIT,
          thisroom.msgi[msnum].displayas-1);
      }
      else Display(thisroom.message[msnum]);
      if (thisroom.msgi[msnum].flags & MSG_DISPLAYNEXT) {
        msnum++;
        repeatloop=1;
      }
      else repeatloop=0;
    }
  }
}

void CaptureScreen(int clr) {
  if (screenop!=NULL) wfreeblock(screenop);
  screenop=wallocblock(scrnwid,scrnhit);
  if (clr==0) clr=16;
  clear_to_color(screenop,clr);
  _global_x_offset=0;
  _global_y_offset=0;
  screen_state=2;
}

void EndCapture() {
  if (screen_state!=2)
    quit("!EndCapture: The screen is not currently captured.");
  wfreeblock(screenop);
  screenop=NULL;
  _global_x_offset=-1;
  screen_state=0;
}

void CapturedPrint(int xx,int yy,char*texx,...) {
  char displbuf[STD_BUFFER_SIZE];
  if (screen_state!=2)
    quit("!CapturedPrint: The screen is not currently captured.");
  xx*=sxmult; yy*=symult;
  va_list ap;
  va_start(ap,texx);
  vsprintf(displbuf,texx,ap);
  va_end(ap);
  wsetscreen(screenop);
  wtexttransparent(TEXTFG);
  wtextcolor(16);
  wouttextxy(xx,yy,cbuttfont,displbuf);
  wsetscreen(virtual_screen);
}

void SetObjectView(int obn,int vii) {
  if (!is_valid_object(obn)) quit("!SetObjectView: invalid object number specified");
  vii--;
  if (vii>=game.numviews) quit("!SetObjectView: invalid view number used");
  objs[obn].view=vii;
  objs[obn].frame=0;
  objs[obn].loop=0;
  objs[obn].cycling=0;
  objs[obn].num=views[vii].frames[0][0].pic;
  }

void SetObjectFrame(int obn,int viw,int lop,int fra) {
  if (!is_valid_object(obn)) quit("!SetObjectFrame: invalid object number specified");
  viw--;
  if (viw>=game.numviews) quit("!SetObjectFrame: invalid view number used");
  if (lop>=views[viw].numloops) quit("!SetObjectFrame: invalid loop number used");
  objs[obn].view=viw;
  objs[obn].frame=fra;
  objs[obn].loop=lop;
  objs[obn].cycling=0;
  objs[obn].num=views[viw].frames[lop][fra].pic;
  }

void AnimateCharacter(int chh,int loopn,int sppd,int rept) {
  CharacterInfo*ci;
  if (chh==MANOBJNUM) ci=playerchar;
  else ci=&game.chars[chh-100];
  ci->walking=0;
  ci->animating=1;
  if (rept) ci->animating|=CHANIM_REPEAT;
  ci->animating|=((sppd << 8) & 0xff00);
  ci->loop=loopn;
  ci->frame=0;
  ci->wait=sppd+views[ci->view].frames[loopn][0].speed;
  }

void AnimateObject(int obn,int loopn,int spdd,int rept) {
  if (objs[obn].view<0) quit("!AnimateObject: object has not been assigned a view");
  if (obn>=MANOBJNUM) {
    AnimateCharacter(obn,loopn,spdd,rept);
    return;
  }
  if (!is_valid_object(obn))
    quit("!AnimateObject: invalid object number specified");
  objs[obn].cycling=rept+1;
  objs[obn].loop=loopn;
  objs[obn].frame=0;
  objs[obn].overall_speed=spdd;
  objs[obn].wait=spdd+views[objs[obn].view].frames[loopn][0].speed;
  }

void ObjectOff(int obn) {
  if (!is_valid_object(obn)) quit("!ObjectOff: invalid object specified");
  objs[obn].on=0;
  }

void ObjectOn(int obn) {
  if (!is_valid_object(obn)) quit("!ObjectOn: invalid object specified");
  objs[obn].on=1;
  }

void SetObjectGraphic(int obn,int slott) {
  if (!is_valid_object(obn)) quit("!SetObjectGraphic: invalid object specified");
  objs[obn].num=slott;
  objs[obn].cycling=0;
  }

char bname[40],bne[40];
char* make_ts_func_name(char*base,int iii,int subd) {
  sprintf(bname,base,iii);
  sprintf(bne,"%s_%c",bname,subd+'a');
  return &bne[0];
}

// FLIC player start
block fli_buffer;
short fliwidth,fliheight;
int canabort=0;
int fli_callback() {
  stretch_blit(fli_buffer,screen,0,0,fliwidth,fliheight,0,0,scrnwid,scrnhit);
  if (kbhit()) {
    if ((getch()==27) && (canabort==1))
      return 1;
  }
  return 0;
}

void play_flc_file(int numb,int playflags) { canabort=playflags;
  char flicnam[20]; sprintf(flicnam,"flic%d.flc",numb);
  FILE*iii=clibfopen(flicnam,"rb");
  if (iii==NULL) { sprintf(flicnam,"flic%d.fli",numb);
    iii=clibfopen(flicnam,"rb"); }
  if (iii==NULL) {
    quit("!FLIC animation not found");
    }
  fseek(iii,8,SEEK_CUR);
  fread(&fliwidth,2,1,iii);
  fread(&fliheight,2,1,iii);
  fclose(iii);
  fli_buffer=create_bitmap(640,400); //scrnwid,scrnhit);
  if (fli_buffer==NULL) quit("Not enough memory to play animation");
  clear(screen);
  if (play_fli(flicnam,fli_buffer,0,fli_callback)==FLI_ERROR)
    quit("FLI/FLC animation play error");
  wfreeblock(fli_buffer);
  setpal();
}
// FLIC player end

SAMPLE*soundfx=NULL;
void play_sound(char*val1) {
  if (soundfx!=NULL) destroy_sample(soundfx);
  soundfx=load_sample(val1);
  if (soundfx==NULL) {
    char err[50];
    sprintf(err,"!Sound sample load failure: '%s'",val1);
    quit(err);
  }
  play_sample(soundfx,255,128,1000,0);
}

void play_audio_clip_by_index(int indx) {
  if (indx>=1000) {
    newmusic(indx-1000);
    return;
  }
  char sndnam[20];
  sprintf(sndnam,"sound%d.wav",indx);
  PACKFILE*sndfil=pack_fopen(sndnam,"rb");
  if (sndfil==NULL) {
    sprintf(sndnam,"sound%d.voc",indx);
    sndfil=pack_fopen(sndnam,"rb");
  }
  if (sndfil!=NULL) pack_fclose(sndfil);
  play_sound(sndnam);
}

void add_inventory(int inum) {
  if ((inum<0) | (inum>=MAX_INV))
    quit("!AddInventory: invalid invnetory number");
  playerchar->inv[inum]++;
}

void RunDialog(int tum) {
  if ((tum<0) | (tum>=game.numdialog))
    quit("!RunDialog: invalid topic number specified");
  if (inside_script)  request_dialog=tum;
  else do_conversation(tum);
}

void run_event_block(EventBlock*evpt,int evnt,int invnum) {
  int ee;
  for (ee=0;ee<evpt->numcmd;ee++) {
    if (evpt->list[ee]!=evnt) continue;
    if (evpt->data[ee]==99) ;
    else if ((invnum>=0) && (evpt->data[ee]==0))
      quit("!ProcessEvent: An inventory event was used, but the Data column\n"
        "was not set. See section 6.1 in the documentation for information.");
    else ;
    if ((invnum>=0) && (evpt->data[ee]!=invnum)) continue;
    if ((invnum>=0) & (invnum<MAX_INV-1)) {
      playerchar->inv[invnum]--;
      if (playerchar->inv[invnum]==0) {
        playerchar->activeinv=-1;
        if (GetCursorMode()==MODE_USE)
          set_cursor_mode(MODE_WALK);
      }
    }
    if (evpt->score[ee]!=0) {
      GiveScore(evpt->score[ee]);
      evpt->score[ee]=0;
    }
    if (evpt->respond[ee]==SHOW_MESSAGE)
      DisplayMessage(evpt->respondval[ee]);
    else if (evpt->respond[ee]==NO_ACTION) ;
    else if (evpt->respond[ee]==NO_WALK)
      StopMoving(game.playercharacter);
    else if (evpt->respond[ee]==GO_TO_SCREEN) {
      new_room_pos=evpt->data[ee];
      // auto place on other side of screen
      if ((new_room_pos==0) & (evpt==&croom->misccond) & (evpt->list[ee]<4)) {
        int edge=evpt->list[ee]+1;
        if (edge==1) edge=2; // left->right
        else if (edge==2) edge=1; // right->left
        else if (edge==3) edge=4; // bottom->top
        else edge=3; // top->bottom
        new_room_pos=edge*1000;
        if (edge<=2) new_room_pos+=playerchar->y;
        else new_room_pos+=playerchar->x;
      }
      NewRoom(evpt->respondval[ee]);
      break;
    }
    else if (evpt->respond[ee]==RUNSCRIPT) {
      if ((strstr(evblockbasename,"character")!=0) | (strstr(evblockbasename,"inventory")!=0)) {
        // Character or Inventory (global script)
        char*torun=make_ts_func_name(evblockbasename,evblocknum,evpt->respondval[ee]);
        // we are already inside the mouseclick event of the script, can't nest calls
        if (inside_script) 
          strcpy(request_func,torun);
        else run_text_script(gameinst,torun);
      }
      else {
        // Other (room script)
        run_text_script(roominst,make_ts_func_name(evblockbasename,evblocknum,evpt->respondval[ee]));
      }
    }
    else if (evpt->respond[ee]==RUN_ANIMATE) {
      if (evpt->respondval[ee]>=MAXANIMSTAGES)
        quit("!run_animate: undefined animation was run");
      if (thisroom.anims[evpt->respondval[ee]].numstages==0)
        quit("!Run_animate: empty animation was run");
      run_animation(&thisroom.anims[evpt->respondval[ee]],0);
    }
    else if (evpt->respond[ee]==ADD_INV)
      add_inventory(evpt->data[ee]);
    else if (evpt->respond[ee]==OBJECT_INV) {
      ObjectOff(evpt->respondval[ee]);
      add_inventory(evpt->data[ee]);
    }
    else if (evpt->respond[ee]==OBJECT_OFF)
      ObjectOff(evpt->respondval[ee]);
    else if (evpt->respond[ee]==OBJECT_ON)
      ObjectOn(evpt->respondval[ee]);
    else if (evpt->respond[ee]==PLAY_SOUND)
      play_audio_clip_by_index(evpt->respondval[ee]);
    else if (evpt->respond[ee]==PLAY_FLI)
      play_flc_file(evpt->respondval[ee],evpt->data[ee]);
    else if (evpt->respond[ee]==GRAPHSCRIPT)
      run_graph_script(evpt->respondval[ee]);
    else if (evpt->respond[ee]==RUN_DIALOG)
      RunDialog(evpt->respondval[ee]);
    else quit("unknown response encountered in block");
  }
}

int isposinbox(int mmx,int mmy,int lf,int tp,int rt,int bt) {
  if ((mmx>=lf) & (mmx<=rt) & (mmy>=tp) & (mmy<=bt)) return TRUE;
  else return FALSE;
  }

// X and Y co-ordinates must be in 320x200 format
int check_click_on_object(int xx,int yy,int mood) {
  int aa;
  // Iterate through all objects in the room
  for (aa=0;aa<croom->numobj;aa++) {
    if (objs[aa].on==0) continue;
    int xxx=objs[aa].x,yyy=objs[aa].y;
    if (isposinbox(xx,yy,xxx,yyy-spriteheight[objs[aa].num]/symult,
      spritewidth[objs[aa].num]/sxmult+xxx,yyy)==0) ;
    else {
      int passon=-1,cdata=-1;
      if (mood==MODE_LOOK) passon=0;
      else if (mood==MODE_HAND) passon=1;
      else if (mood==MODE_TALK) passon=2;
      else if (mood==MODE_USE) { passon=3;
        cdata=playerchar->activeinv;
        play.usedinv=cdata; }
      if (passon>=0) { evblockbasename="object%d"; evblocknum=aa;
        run_event_block(&croom->objcond[aa],passon,cdata);
        return 1; }
    }
  }
  return 0;
  }

int is_pos_on_character(int xx,int yy) {
  int cc,sppic,lowestyp=0,lowestwas=-1;
  for (cc=0;cc<game.numcharacters;cc++) {
    if (game.chars[cc].room!=displayed_room) continue;
    if (game.chars[cc].on==0) continue;
    CharacterInfo*chin=&game.chars[cc];
    sppic=views[chin->view].frames[chin->loop][chin->frame].pic;
    int xxx=chin->x-spritewidth[sppic]/sxmult/2;
    int yyy=chin->y-spriteheight[sppic]/symult;
    if (isposinbox(xx,yy,xxx,yyy,xxx+spritewidth[sppic]/sxmult,yyy+spriteheight[sppic]/symult)==0) continue;
    if (chin->y<lowestyp) continue;
    lowestyp=chin->y;
    lowestwas=cc;
  }
  return lowestwas;
}

int check_click_on_character(int xx,int yy,int mood) {
  int lowestwas=is_pos_on_character(xx,yy);
  if (lowestwas>=0) {
    int cc=lowestwas;
    int passon=-1,cdata=-1;
    if (mood==MODE_LOOK) passon=0;
    else if (mood==MODE_HAND) passon=1;
    else if (mood==MODE_TALK) passon=2;
    else if (mood==MODE_USE) { passon=3;
      cdata=playerchar->activeinv;
      play.usedinv=cdata;
      }
    evblockbasename="character%d"; evblocknum=cc;
    if (passon>=0)
      run_event_block(&game.charcond[cc],passon,cdata);
    run_event_block(&game.charcond[cc],4);  // any click on char
    return 1;
  }
  return 0;
}

void break_out_seer() {
  if (user_disabled_for!=0)
    quit("break_out_seer: user_disabled_For is set");
  user_disabled_data2=Register(scActual_Instance)[regIP];
  user_disabled_data3=(int)scActual_Instance;
  user_disabled_for=FOR_SCRIPT;
  scaddr=Register(scActual_Instance)[regCP];
  abort_executor=1;
}

void walk_character(int chac,int tox,int toy,int ignwal,int blk) {
  CharacterInfo*chin=&game.chars[chac];
  if (chin->room!=displayed_room)
    quit("!MoveCharacter: character not in current room");
  move_speed=chin->walkspeed;
  int mslot=find_route(chin->x,chin->y,tox,toy,thisroom.walls,chac+5,1,ignwal);
  if (mslot>0) {
    chin->walking=mslot;
    chin->loop=fix_player_sprite(&mls[mslot]);
  }
  if (blk!=0) {
    break_out_seer();
    main_loop_until(UNTIL_MOVEEND,(int)&chin->walking,0);
  }
}

void ProcessClick(int xx,int yy,int mood) {
  xx+=offsetx/sxmult;
  yy+=offsety/symult;
  if (mood==MODE_WALK) {
    walk_character(game.playercharacter,xx,yy,0,0);
    return;
  }
  play.usedmode=mood;
  if (check_click_on_character(xx,yy,mood)) return;
  if (check_click_on_object(xx,yy,mood)) return;
  int hotspothere=getpixel(thisroom.lookat,xx,yy);
  int passon=-1,cdata=-1;
  if (mood==MODE_TALK) passon=4;
  else if (mood==MODE_LOOK) passon=1;
  else if (mood==MODE_HAND) passon=2;
  else if (mood==MODE_USE) { passon=3;
    cdata=playerchar->activeinv;
    play.usedinv=cdata;
  }
  if ((game.options[OPT_WALKONLOOK]==0) & (mood==MODE_LOOK)) ;
  else if (mood!=MODE_WALK)
    MoveCharacterToHotspot(game.playercharacter,hotspothere);
  evblockbasename="hotspot%d";
  evblocknum=hotspothere;
  if (passon>=0)
    run_event_block(&croom->hscond[hotspothere],passon,cdata);
  run_event_block(&croom->hscond[hotspothere],5);
}

int do_movelist_move(short*mlnum,int*xx,int*yy) {
  int need_to_fix_sprite=0;
  if (mlnum[0]<1) quit("movelist_move: attempted to move on a non-exist movelist");
  MoveList*cmls; cmls=&mls[mlnum[0]];

  int xps=xx[0],yps=yy[0];
  if (cmls->doneflag & 1) {
    // if the X-movement has finished, and the Y-per-move is < 1, finish
    // This can cause jump at the end, but without it the character will
    // walk on the spot for a while if the Y-per-move is for example 0.2
    int ypmm=(cmls->ypermove[cmls->onstage] >> 16) & 0x0000ffff;
    if ((ypmm==0) | (ypmm==0xffff)) cmls->doneflag|=2;
  }
  else xps=cmls->fromx+(int)(fixtof(cmls->xpermove[cmls->onstage])*(float)cmls->onpart);

  if (cmls->doneflag & 2) {
    // Y-movement has finished
    int xpmm=(cmls->xpermove[cmls->onstage] >> 16) & 0x0000ffff;
    if ((xpmm==0) | (xpmm==0xffff)) cmls->doneflag|=1;
  }
  else yps=cmls->fromy+(int)(fixtof(cmls->ypermove[cmls->onstage])*(float)cmls->onpart);

  // check if finished horizontal movement
  if ((cmls->xpermove[cmls->onstage]>0) & (xps>=(cmls->pos[cmls->onstage+1] >> 16)))
    cmls->doneflag|=1;
  else if ((cmls->xpermove[cmls->onstage]<0) & (xps<=(cmls->pos[cmls->onstage+1] >> 16)))
    cmls->doneflag|=1;

  // check if finished vertical movement
  if ((cmls->ypermove[cmls->onstage]>0) & (yps>=(cmls->pos[cmls->onstage+1] & 0x00ffff)))
    cmls->doneflag|=2;
  else if ((cmls->ypermove[cmls->onstage]<0) & (yps<=(cmls->pos[cmls->onstage+1] & 0x00ffff)))
    cmls->doneflag|=2;

  if (cmls->xpermove[cmls->onstage]==0)
    cmls->doneflag|=1;
  if (cmls->ypermove[cmls->onstage]==0)
    cmls->doneflag|=2;

  if ((cmls->doneflag & 0x03)==3) {
    // this stage is done, go on to the next stage
    cmls->fromx=((cmls->pos[cmls->onstage+1] >> 16) & 0x000ffff);
    cmls->fromy=(cmls->pos[cmls->onstage+1] & 0x000ffff);
    cmls->onstage++; cmls->onpart=-1; cmls->doneflag&=0xf0;
    cmls->lastx=-1;
    if (cmls->onstage < cmls->numstage) {
      xps=cmls->fromx; yps=cmls->fromy; }
    if (cmls->onstage>=cmls->numstage-1) {  // last stage is just dest pos
      cmls->numstage=0;
      mlnum[0]=0;
      need_to_fix_sprite=1;
      }
    else need_to_fix_sprite=2;
    }
  cmls->onpart++;
  xx[0]=xps; yy[0]=yps;
  return need_to_fix_sprite;
  }

void stopmusic() {
  if (libamp_loaded!=0) {
    libamp_loaded=0;
    unload_amp();
  }
  if (opts.mod_player!=0) {
    if (is_mod_playing()!=0) {
      stop_mod();
      if (modptr!=NULL) destroy_mod(modptr);
      modptr=NULL;
    }
  }
  if (midiptr!=NULL) {
    stop_midi();
    destroy_midi(midiptr);
    midiptr=NULL;
  }
}

void update_music_volume() {
  int newvol=160+((int)thisroom.options[ST_VOLUME])*30;
  if (libamp_loaded)
    amp_setvolume(newvol);
  else if ((opts.mod_player!=0) && (is_mod_playing()!=0))
    set_mod_volume(newvol);
  else set_volume(-1,newvol);
}

void newmusic(int mnum) {
  if (mnum<0) {
    stopmusic();
    return;
  }
  char fname[20];
  sprintf(fname,"music%d.mid",mnum);

  if (libamp_loaded!=0) unload_amp();
  libamp_loaded=0;
  if (opts.mod_player!=0) {
    if (is_mod_playing()!=0) stop_mod();
    if (modptr!=NULL) {
      destroy_mod(modptr);
      modptr=NULL;
    }
  }
  if (midiptr!=NULL) destroy_midi(midiptr);

  // now that all the previous music is unloaded, load in the new one
  midiptr=load_midi(fname);
  if(midiptr!=NULL) {
    cur_music_number=mnum;
    if (play_midi(midiptr,-1)!=0) quit("!Couldn't play MIDI file");
  }
  else {
    if (opts.mp3_player!=0) {
      sprintf(fname,"music%d.mp3",mnum);
      if (load_amp(fname,1)!=0)
        libamp_loaded=1;
    }
  }

  if (libamp_loaded!=0) ;
  else if (midiptr!=NULL) ;
  else {
    if (opts.mod_player==0) cur_music_number=0;
    else {
      cur_music_number=0;
      sprintf(fname,"music%d.mod",mnum);
      modptr=load_mod(fname);
      if(modptr==NULL) {
        sprintf(fname,"music%d.xm",mnum);
        modptr=load_mod(fname);
      }
      if (modptr!=NULL) play_mod(modptr,-1);
    }
  }

  update_music_volume();
}

void SetPlayerCharacter(int newchar) {
  if ((newchar<0) | (newchar>=game.numcharacters))
    quit("!SetPlayerCharacter: Invalid character specified");
  game.playercharacter=newchar;
  playerchar=&game.chars[newchar];
  NewRoom(playerchar->room);
}

void QuitGame(int dialog) {
  if (dialog) {
    int rcode;
    setup_for_dialog();
    rcode=quitdialog();
    restore_after_dialog();
    if (rcode==0) return;
    }
  quit("|You have exited.");
  }

FILE* FileOpen(const char*fnmm, const char* mode) {
  return fopen(fnmm,mode);
  }
void FileClose(FILE*hha) {
  fclose(hha);
  }
void FileWrite(FILE*haa, const char *towrite) {
  putw(strlen(towrite)+1,haa);
  fwrite(towrite,strlen(towrite)+1,1,haa);
  }
void FileRead(FILE*haa,char*toread) {
  int lle=getw(haa);
  if ((lle>=200) | (lle<1)) quit("!FileRead: file was not written by FileWrite");
  fread(toread,lle,1,haa);
  }

void InterfaceOn(int ifn) {
  if (ifn<=0)
    quit("!InterfaceOn: invalid interface specified");
  ifn--;
  game.iface[ifn].on=1;
  // modal interface
  if (game.iface[ifn].popup==POPUP_SCRIPT) game_paused++;
}

void InterfaceOff(int ifn) {
  if (ifn<=0)
    quit("!InterfaceOff: invalid interface specified");
  ifn--;
  game.iface[ifn].on=0;
  // modal interface
  if (game.iface[ifn].popup==POPUP_SCRIPT) game_paused--;
}

int IsGamePaused() {
  if (game_paused>0) return 1;
  return 0;
  }

void NewRoom(int nrnum) {
  if ((inside_script==0) & (in_graph_script==0)) {
    new_room(nrnum,playerchar);
    return;
  }
  if (request_newroom>=0)
    quit("!NewRoom: requested 2 room changes within one script");
  request_newroom=nrnum;
}

void SetGameSpeed(int newspd) {
  if (newspd>100) newspd=100;
  if (newspd<10) newspd=10;
  set_game_speed(newspd);
}

void SetDialogOption(int dlg,int opt,int onoroff) {
  if ((dlg<0) | (dlg>=game.numdialog))
    quit("!SetDialogOption: Invalid topic number specified");
  if ((opt<1) | (opt>dialog[dlg].numoptions))
    quit("!SetDialogOption: Invalid option number specified");
  opt--;

  dialog[dlg].optionflags[opt]&=~DFLG_ON;
  if ((onoroff==1) & ((dialog[dlg].optionflags[opt] & DFLG_OFFPERM)==0))
    dialog[dlg].optionflags[opt]|=DFLG_ON;
  else if (onoroff==2)
    dialog[dlg].optionflags[opt]|=DFLG_OFFPERM;
}

void CyclePalette(int strt,int eend) {
  wcolrotate(strt,eend,0,palette);
  wsetpalette(strt,eend,palette);
}
void SetPalRGB(int inndx,int rr,int gg,int bb) {
  wsetrgb(inndx,rr,gg,bb,palette);
  wsetpalette(inndx,inndx,palette);
}
/*void scSetPal(color*pptr) {
  wsetpalette(0,255,pptr);
  }
void scGetPal(color*pptr) {
  get_palette(pptr);
  }*/
void FadeIn(int sppd) {
  my_fade_in(palette,sppd);
}

int __Rand(int upto) {
  return rand()%upto;
  }

void RefreshMouse() {
  mgetgraphpos();
  scmouse.x=mousex/sxmult;
  scmouse.y=mousey/symult;
}

int GetCursorMode() {
  return cur_mode;
}

void GiveScore(int amnt) {
  play.score+=amnt;
  if ((amnt>0) && (game.options[OPT_SCORESOUND]>0))
    play_audio_clip_by_index(game.options[OPT_SCORESOUND]);
}

void GetLocationName(int xxx,int yyy,char*tempo) {
  xxx+=offsetx/sxmult;
  yyy+=offsety/symult;
  if ((xxx>=thisroom.width) | (xxx<0) | (yyy<0) | (yyy>=thisroom.height)) {
    tempo[0]=0;
    return;
  }
  tempo[0]=0;
  int onhs,aa;
  // on character
  onhs=is_pos_on_character(xxx,yyy);
  if (onhs>=0) {
    strcpy(tempo,game.chars[onhs].name);
    return;
  }
  // on object
  for (aa=0;aa<croom->numobj;aa++) {
    if (objs[aa].on==0) continue;
    int xx=objs[aa].x,yy=objs[aa].y;
    if (isposinbox(xxx,yyy,xx,yy-spriteheight[objs[aa].num]/symult,xx+spritewidth[objs[aa].num]/sxmult,yy)==0) continue;
    sprintf(tempo,"OBJECT %d",aa);
    return;
  }
  onhs=getpixel(thisroom.lookat,xxx,yyy);
  if (onhs>0) strcpy(tempo,thisroom.hotspotnames[onhs]);
}

void MoveCharacter(int cc,int xx,int yy) {
  walk_character(cc,xx,yy,0,0);
  }

int GetPlayerCharacter() {
  return game.playercharacter;
  }

void SetCharacterView(int chaa,int vii) {
  if (!is_valid_character(chaa))
    quit("!SetCharacterView: invalid character specified");
  vii--;
  CharacterInfo*chap=&game.chars[chaa];
  chap->view=vii;
  chap->walking=0;
  chap->animating=0;
  chap->loop=0;
  chap->frame=0;
  chap->wait=0;
  chap->flags|=CHF_FIXVIEW;
}

void ReleaseCharacterView(int chat) {
  if (!is_valid_character(chat))
    quit("!ReleaseCahracterView: invalid character supplied");
  game.chars[chat].flags&=~CHF_FIXVIEW;
}

void FaceLocation(int cha,int xx,int yy) {
  if (!is_valid_character(cha))
    quit("!FaceLocation: Invalid character specified");
  CharacterInfo*chap=&game.chars[cha];
  int diffrx=xx-chap->x;
  int diffry=yy-chap->y;
  int useloop=-1,wanthoriz=0;
  if (abs(diffry)<abs(diffrx)) wanthoriz=1;
  if ((wanthoriz==1) && (diffrx>0)) useloop=2;
  else if ((wanthoriz==1) && (diffrx<0)) useloop=1;
  else if (diffry>0) useloop=0;
  else if (diffry<0) useloop=3;
  chap->loop=useloop;
  chap->frame=0;
}

void FaceCharacter(int cha,int toface) {
  if (!is_valid_character(cha) || !is_valid_character(toface))
    quit("!FaceCharacter: Invalid character specified");
  CharacterInfo*char1=&game.chars[cha];
  CharacterInfo*char2=&game.chars[toface];
  if (char1->room!=char2->room)
    quit("!FaceCharacter: characters are in different rooms");
  FaceLocation(cha,char2->x,char2->y);
}

void MoveCharacterToObject(int chaa,int obbj) {
  walk_character(chaa,objs[obbj].x+5,objs[obbj].y+6,0,0);
  do_main_cycle(UNTIL_MOVEEND,(int)&game.chars[chaa].walking);
}

void MoveCharacterToHotspot(int chaa,int hotsp) {
  if (thisroom.hswalkto[hotsp].x<1) return;
  walk_character(chaa,thisroom.hswalkto[hotsp].x,thisroom.hswalkto[hotsp].y,0,0);
  do_main_cycle(UNTIL_MOVEEND,(int)&game.chars[chaa].walking);
  }

void StopMoving(int chaa) {
  game.chars[chaa].walking=0;
  game.chars[chaa].frame=0;
}

void GetLanguageString(int indxx,char*buffr) {
  char*bptr=get_language_text(indxx);
  if (bptr==NULL) strcpy(buffr,"[language string error]");
  else strncpy(buffr,bptr,199);
  buffr[199]=0;
  }

void script_debug(int cmdd) {
  if (play.debug_mode==0) return;
  int rr;
  if (cmdd==0) {
    for (rr=0;rr<game.numinvitems;rr++)
      playerchar->inv[rr]=1;
  }
  else if (cmdd==1) {
    Display("Adventure Game Studio run-time engine[ACI version " ACI_VERSION_TEXT
      "[Compiled on " __DATE__ " at " __TIME__);
  }
  else if (cmdd==2) { // show walkable areas from here
    block tempw=create_bitmap(thisroom.walls->w,thisroom.walls->h);
    blit(thisroom.walls,tempw,0,0,0,0,tempw->w,tempw->h);
    if (getpixel(tempw,playerchar->x,playerchar->y)==0)
      destroy_bitmap(tempw);
    else {
      int ww,hh;
      for (ww=0;ww<tempw->w;ww++) {
        for (hh=0;hh<tempw->h;hh++) {
          if (_getpixel(tempw,ww,hh)!=0)
            _putpixel(tempw,ww,hh,1);
        }
      }
      floodfill(tempw,playerchar->x,playerchar->y,14);
      stretch_sprite(screen,tempw,-offsetx,-offsety,tempw->w*sxmult,tempw->h*symult);
      destroy_bitmap(tempw);
      while (!kbhit()) ;
      getch();
    }
  }
  else if (cmdd==3) {
    setup_for_dialog();
    int rnum=enternumberwindow("Enter room to teleport to:");
    restore_after_dialog();
    if (rnum>=0)
      NewRoom(rnum);
  }
  else quit("!Debug: unknown command code");
}

int cd_player_control(int cmdd,int datt) {
  if (cmdd==0) return use_cdplayer;
  if (use_cdplayer==0) return 0;
  if (cmdd==1) return cd_isplayingaudio(cddrive);
  else if (cmdd==2) cd_playtrack(cddrive,datt);
  else if (cmdd==3) cd_stopmusic(cddrive);
  else if (cmdd==4) cd_resumemusic(cddrive);
  else if (cmdd==5) {
    if ((cd_getstatus(cddrive) & (CDS_DRIVEOPEN | CDS_DRIVEEMPTY))!=0)
      return 0;
    return cd_getlasttrack(cddrive);
  }
  else if (cmdd==6) cd_eject(cddrive);
  else if (cmdd==7) cd_uneject(cddrive);
  else if (cmdd==8) return numcddrives;
  else if (cmdd==9) {
    if ((datt<1) | (datt>numcddrives)) return 0;
    cddrive=cd_driveletters[datt-1];
  }
  else quit("!CDAudio: Unknown command code");
  return 0;
}

int get_graph_variable(int indx) {
  if (indx>=100) return play.globalvars[indx-100];
  return croom->flagstates[indx];
}

void set_graph_variable(int indx,int val) {
  if (indx>=100) play.globalvars[indx-100]=val;
  else croom->flagstates[indx]=val;
}

int run_graph_commandlist(int ct) {
  int ee;
  if (restrict_until!=0) return 0;
  for (ee=0;ee<scnf[ct].numevents;ee++) {
    if (restrict_until!=0) return 0;
    ScriptEvent*gse=&scnf[ct].events[ee];
    if (gse->type==0) continue;
    switch (gse->type) {
      case 1: // go to screen
        new_room_pos=gse->with;
        NewRoom(gse->_using);
        return 0;
      case 2: // give score
        GiveScore(gse->_using);
        break;
      case 3: // stop man
        StopMoving(game.playercharacter);
        break;
      /*case 4: // lose game
        do_condition(3,0,gse->with,0);
        break;*/
      case 5: // run animation
        run_animation(&thisroom.anims[gse->_using],0);
        break;
      case 6: // display message
        DisplayMessage(gse->_using);
        break;
      case 7: // remove object
        ObjectOff(gse->_using);
        break;
      /*case 8: // reserved
        break;*/
      case 16: // play sound effect
        play_audio_clip_by_index(gse->_using);
        break;
      case 17: // play fli/flc animation
        play_flc_file(gse->_using,gse->with);
        break;
      case 18: // turn object on
        ObjectOn(gse->_using);
        break;
      case 9: // add object inv
        add_inventory(gse->_using);
        break;
      /*case 10: // run text script
        break;*/
      case 11: // set flag
        if ((gse->_using>14) & (gse->_using<100))
          quit("!graph_script: SET_FLAG used with invalid flag number");
        set_graph_variable(gse->_using,1);
        break;
      case 12: // clear flag
        if ((gse->_using>14) & (gse->_using<100))
          quit("!graph_script: CLEAR_FLAG used with invalid flag number");
        set_graph_variable(gse->_using,0);
        break;
      case 14: // if flag is clear
        if (get_graph_variable(gse->_using)!=0) break;
        if (run_graph_commandlist(gse->branchto)==0)
          return 0;
        break;
      case 15: // if flag is set
        if (get_graph_variable(gse->_using)==0) break;
        if (run_graph_commandlist(gse->branchto)==0)
          return 0;
        break;
      case 19: // if player has inv
        if (playerchar->inv[gse->_using]==0) break;
        if (run_graph_commandlist(gse->branchto)==0)
          return 0;
        break;
      case 22: // random chance 1 in %d
        if ((rand() % gse->_using)!=0) break;
        if (run_graph_commandlist(gse->branchto)==0)
          return 0;
        break;
      case 21: // every %d loops
        if (((int)loopcounter % gse->_using)!=0) break;
        if (run_graph_commandlist(gse->branchto)==0)
          return 0;
        break;
      case 20: // lose inventory
        if (playerchar->inv[gse->_using]<1)
          quit("!lose_inventory: doesn\'t have the object");
        playerchar->inv[gse->_using]--;
        if ((playerchar->activeinv==gse->_using) & (playerchar->inv[gse->_using]<1))
          playerchar->activeinv=-1;
        break;
      case 13: // stop script
        return 0;
      case 23: // set timer
        play.gscript_timer=gse->_using;
        if (gse->_using==0) play.gscript_timer=-1;
        break;
      case 24: // if timer expired
        if (play.gscript_timer!=0) break;
        play.gscript_timer=-1;
        if (run_graph_commandlist(gse->branchto)==0)
          return 0;
        break;
      case 26: // if inventory %d was used
        if (play.usedinv!=gse->_using) break;
        if (run_graph_commandlist(gse->branchto)==0)
          return 0;
        break;
      case 25: // move man to obj
        MoveCharacterToObject(game.playercharacter,gse->_using);
        break;
      default:
        char msg[54]; int tmp;
        sprintf(msg,"run_graph_script: unknown evnt %d",(int)gse->type-1);
        quit(msg);
    }
  }
  return 1;
}

long fget_long(FILE*iii) { long tmpp;
  fread(&tmpp,4,1,iii);
  return tmpp;
}

void run_graph_script(int ct) {
  char thisscn[50];
  sprintf(thisscn,scripttempn,ct);
  FILE*iii=fopen(thisscn,"rb");
  if (iii==NULL) {
    quit(thisscn);
    quit("Run_Graph_script: temp file not found");
  }
  if (fget_long(iii)!=2)
    quit("run_graph_script: invalid script version");
  if (fget_long(iii)!=sizeof(ScriptBlock))
    quit("run_graph_script: invalid block version");
  long cnt=fget_long(iii);
  scnf=(ScriptBlock*)malloc(cnt*sizeof(ScriptBlock));
  fread(scnf,cnt,sizeof(ScriptBlock),iii);
  fclose(iii);
  in_graph_script++;
  run_graph_commandlist(0);
  in_graph_script--;
  if (inside_script>0) ;
  else if (request_newroom>=0) {
    new_room(request_newroom,playerchar);
    request_newroom=-1;
  }
  free(scnf);
}

#define RUN_DIALOG_STOP_DIALOG -2

int run_dialog_request(DialogTopic*dtop,int topic) {
  if (topic<0) return -1;
  unsigned char*optn=&dtop->optionscripts[topic];
  while (1) {
    unsigned char dlgcmd=optn[0];
    short*opt=(short*)&optn[1];
    if (dlgcmd==DCMD_SAY) {
      DisplaySpeech(messages[opt[1]],1,opt[0]);
      optn+=5;
    }
    else if (dlgcmd==DCMD_OPTOFF) {
      dtop->optionflags[opt[0]]&=~DFLG_ON;
      optn+=3;
    }
    else if (dlgcmd==DCMD_OPTON) {
      if ((dtop->optionflags[opt[0]] & DFLG_OFFPERM)!=0) ;
      else dtop->optionflags[opt[0]]|=DFLG_ON;
      optn+=3;
    }
    else if (dlgcmd==DCMD_OPTOFFFOREVER) {
      dtop->optionflags[opt[0]]&=~DFLG_ON;
      dtop->optionflags[opt[0]]|=DFLG_OFFPERM;
      optn+=3;
    }
    else if (dlgcmd==DCMD_RUNTEXTSCRIPT) {
      run_text_script_iparam(gameinst,"dialog_request",opt[0]);
      optn+=3;
    }
    else if (dlgcmd==DCMD_GOTODIALOG)
      return opt[0];
    else if (dlgcmd==DCMD_RETURN)
      return -1;
    else if (dlgcmd==DCMD_STOPDIALOG)
      return RUN_DIALOG_STOP_DIALOG;
    else if (dlgcmd==DCMD_ENDSCRIPT)
      return -1;
    else quit("unknown dialog command");
  }
}

void do_conversation(int dlgnum) {
  int dlgxp=1,dlgyp=symult*160,usingfont=FONT_NORMAL;
  int txthit=wgettextheight("ZHwypqhkilIK",fonts[usingfont]);
  int dlgnumwas=-1,curwas=cur_cursor;
  DialogTopic*dtop=&dialog[dlgnum];
  // run the startup script
  int tocar=run_dialog_request(dtop,dtop->startupentrypoint);
  if (tocar==RUN_DIALOG_STOP_DIALOG) return;
  else if (tocar>=0) dlgnum=tocar;
  while (dlgnum>=0) {
    set_mouse_cursor(CURS_ARROW);
    dtop=&dialog[dlgnum];
    int ww,chose=-1,numdisp=0;
    if (dlgnumwas!=dlgnum) dlgnumwas=dlgnum;
    write_screen();
    wtexttransparent(TEXTFG);
    char disporder[MAXTOPICOPTIONS];
    short dispyp[MAXTOPICOPTIONS];
    int areawid;
    numdisp=0;
    for (ww=0;ww<dtop->numoptions;ww++) {
      if ((dtop->optionflags[ww] & DFLG_ON)==0) ;
      else {
        disporder[numdisp]=ww;
        numdisp++;
      }
    }
    if (numdisp<1) quit("!DoDialog: all options have been turned off");
    if (numdisp>1) {
      currentcolor=0;
      if (game.options[OPT_DIALOGIFACE]>0) {
        InterfaceElement*iep=&game.iface[game.options[OPT_DIALOGIFACE]-1];
        dlgxp=iep->x; dlgyp=iep->y;
        rectfill(abuf,dlgxp,dlgyp,iep->x2,iep->y2,currentcolor);
        areawid=iep->x2-dlgxp-5;
      }
      else {
        dlgyp=scrnhit-numdisp*txthit-1;
        rectfill(abuf,0,dlgyp-1,scrnwid-1,scrnhit-1,currentcolor);
        areawid=scrnwid-5;
      }
      int mouseison=-1,curyp;
      int cc;
      int mousewason=-10; mouseison=-10;
redraw_options:
      curyp=dlgyp+1;
      for (ww=0;ww<numdisp;ww++) {
        wtextcolor((playerchar->flags & OCHF_SPEECHCOL) >> OCHF_SPEECHCOLSHIFT);
        if (mouseison==ww) wtextcolor(14);
        break_up_text_into_lines(areawid-5,1,dtop->optionnames[disporder[ww]]);
        dispyp[ww]=curyp;
        for (cc=0;cc<numlines;cc++) {
          wouttextxy(dlgxp+((cc==0)?0:6),curyp,fonts[usingfont],lines[cc]);
          curyp+=txthit;
        }
        curyp+=game.options[OPT_DIALOGGAP];
      }
      domouse(1);
      while (1) {
        domouse(0);
        poll_mp3();
        mousewason=mouseison;
        mouseison=-1;
        if ((mousey<=dlgyp) | (mousey>curyp)) ;
        else {
          mouseison=numdisp-1;
          for (ww=0;ww<numdisp;ww++) {
            if (mousey<dispyp[ww]) { mouseison=ww-1; break; }
          }
          if ((mouseison<0) | (mouseison>=numdisp)) mouseison=-1;
        }
        if (mgetbutton()!=NONE) {
          if (mouseison<0) continue;
          chose=disporder[mouseison];
          break;
        }
        if (mousewason!=mouseison) {
          domouse(2);
          goto redraw_options;
        }
      }
      domouse(2);
    }
    else chose=disporder[0]; // only one choice, so select it
    while (kbhit()) getch(); // empty keyboard buffer
    cremovemenu();
    DisplaySpeech(dtop->optionnames[chose],1,game.playercharacter);
    tocar=run_dialog_request(dtop,dtop->entrypoints[chose]);
    if (tocar==RUN_DIALOG_STOP_DIALOG) break;
    else if (tocar>=0) dlgnum=tocar;
  }
  set_mouse_cursor(curwas);
}

// save game functions
#define SGVERSION 7
const char* sgnametemplate = "agssave.%03d";
char*sgsig="Adventure Game Studio saved game";
int sgsiglen=32;
int find_highest_room_entered() {
  int qq,fndas=-1;
  for (qq=0;qq<MAX_ROOMS;qq++) {
    if (roomstats[qq].beenhere!=0) fndas=qq;
  }
  if (fndas<0) quit("find_highest_room: been in no rooms?");
  return fndas;
}

void save_game(int slotn,char*descript) {
  char nametouse[20];
  int bb;
  sprintf(nametouse,sgnametemplate,slotn);
  FILE*ooo=fopen(nametouse,"wb");
  fwrite(sgsig,sgsiglen,1,ooo);
  fputstring(descript,ooo);
  putw(SGVERSION,ooo);
  fwrite(ACI_VERSION_TEXT,8,1,ooo);
  putw(scrnhit,ooo);
  putw(frames_per_second,ooo);
  putw(cur_mode,ooo);
  putw(cur_cursor,ooo);
  putw(offsetx,ooo); putw(offsety,ooo);
  putw(loopcounter,ooo);
  // write the data segment of the global script
  int gdatasize=ToCodeINT(gameinst)[hdrDataSize];
  putw(gdatasize,ooo);
  fwrite(&scVar(gameinst,0,void*),gdatasize,1,ooo);
  if (roominst!=NULL) {
    // update the current room script's data segment copy
    croom->tsdatasize=ToCodeINT(roominst)[hdrDataSize];
    if (croom->tsdata!=NULL)
      free(croom->tsdata);
    croom->tsdata=(char*)malloc(croom->tsdatasize+10);
    memcpy(croom->tsdata,&scVar(roominst,0,void*),croom->tsdatasize);
  }
  int roomnum=find_highest_room_entered()+1;
  putw(roomnum,ooo);
  fwrite(roomstats,sizeof(RoomStatus),roomnum,ooo);
  for (bb=0;bb<roomnum;bb++) {
    if (roomstats[bb].tsdatasize>0)
      fwrite(&roomstats[bb].tsdata[0],roomstats[bb].tsdatasize,1,ooo);
  }
  fwrite(&play,sizeof(GameState),1,ooo);
  fwrite(&mls[0],sizeof(MoveList),60,ooo);
  fwrite(&game,sizeof(GameStruct),1,ooo);
  fwrite(&game.chars[0],sizeof(CharacterInfo),game.numcharacters,ooo);
  putw(displayed_room,ooo);
  fwrite(&palette[0],sizeof(color),256,ooo);
  for (bb=0;bb<game.numdialog;bb++)
    fwrite(&dialog[bb].optionflags[0],sizeof(int),MAXTOPICOPTIONS,ooo);
  fclose(ooo);
}

int load_game(int slotn) {
  char nametouse[20];
  char rbuffer[100];
  int vv;
  sprintf(nametouse,sgnametemplate,slotn);
  FILE*ooo=fopen(nametouse,"rb");
  if (ooo==NULL)
    return -1;
  fread(rbuffer,sgsiglen,1,ooo);
  rbuffer[sgsiglen]=0;
  if (strcmp(rbuffer,sgsig)!=0) {
    // not a save game
    fclose(ooo);
    return -2;
  }
  fgetstring(rbuffer,ooo);
  if (getw(ooo)!=SGVERSION) {
    fclose(ooo);
    return -3;
  }
  fread(rbuffer,8,1,ooo);
  rbuffer[8]=0;
  if (strcmp(rbuffer,ACI_VERSION_TEXT)!=0) {
    fclose(ooo);
    return -4;
  }
  if (getw(ooo)!=scrnhit)
    quit("This game was saved with the interpreter running at a different\n"
      "resolution. It can not be restored.\n");
  set_game_speed(getw(ooo));
  cur_mode=getw(ooo);
  cur_cursor=getw(ooo);
  offsetx=getw(ooo); offsety=getw(ooo);
  loopcounter=getw(ooo);
  int gdatasize=ToCodeINT(gameinst)[hdrDataSize];
  if (getw(ooo)!=gdatasize)
    quit("!Restore_game: Global script changed, cannot restore game");
  fread(&scVar(gameinst,0,void*),gdatasize,1,ooo);
  // now the rooms
  for (vv=0;vv<MAX_ROOMS;vv++) {
    if (roomstats[vv].tsdata==NULL) ;
    else if (roomstats[vv].tsdatasize>0) {
      free(roomstats[vv].tsdata);
      roomstats[vv].tsdatasize=0; roomstats[vv].tsdata=NULL;
      }
    roomstats[vv].beenhere=0;
    }
  save_room_data_segment();
  int numtoread=getw(ooo);
  if (numtoread>MAX_ROOMS)
    quit("too many roomsentered: bug?");
  for (vv=0;vv<MAX_ROOMS;vv++) {
    if ((roomstats[vv].tsdatasize>0) & (roomstats[vv].tsdata!=NULL))
      free(roomstats[vv].tsdata);
    roomstats[vv].tsdatasize=0;
    roomstats[vv].tsdata=NULL;
  }
  fread(&roomstats[0],sizeof(RoomStatus),numtoread,ooo);
  for (vv=0;vv<numtoread;vv++) {
    if (roomstats[vv].tsdatasize>0) {
      roomstats[vv].tsdata=(char*)malloc(roomstats[vv].tsdatasize+5);
      fread(&roomstats[vv].tsdata[0],roomstats[vv].tsdatasize,1,ooo);
      }
    else roomstats[vv].tsdata=NULL;
    }
  fread(&play,sizeof(GameState),1,ooo);
  fread(&mls[0],sizeof(MoveList),60,ooo);
  // save pointer members before reading
  char*gswas=game.globalscript;
  scScript compsc=game.compiled_script;
  CharacterInfo*chwas=game.chars;
  char*mesbk[MAXGLOBALMES];
  int numchwas=game.numcharacters;
  for (vv=0;vv<MAXGLOBALMES;vv++) mesbk[vv]=game.messages[vv];
  int numdiwas=game.numdialog;
  fread(&game,sizeof(GameStruct),1,ooo);
  if (game.numdialog!=numdiwas)
    quit("!Restore_Game: Game has changed (dlg), unable to restore");
  // restore pointer members
  game.globalscript=gswas;
  game.compiled_script=compsc;
  game.chars=chwas;
  for (vv=0;vv<MAXGLOBALMES;vv++) game.messages[vv]=mesbk[vv];
  if (numchwas!=game.numcharacters)
    quit("!Restore_Game: Game has changed, unable to restore position");
  fread(&game.chars[0],sizeof(CharacterInfo),game.numcharacters,ooo);
  if (roominst!=NULL) {  // so it doesn't overwrite the tsdata
    scFree_Instance(roominst);
    roominst=NULL;
  }
  displayed_room=getw(ooo);
  fread(&palette[0],sizeof(color),256,ooo);
  for (vv=0;vv<game.numdialog;vv++)
    fread(&dialog[vv].optionflags[0],sizeof(int),MAXTOPICOPTIONS,ooo);
  fclose(ooo);
  int curwas=cur_cursor;
  set_cursor_mode(cur_mode);
  set_mouse_cursor(curwas);
  int gstimer=play.gscript_timer;
  load_new_room(displayed_room,NULL);
  play.gscript_timer=gstimer;
  in_new_room=3;
  return 0;
}

void run_event_block_inv(EventBlock*evpt,int evnt,int invn=-1) {
  domouse(2);
  abuf=virtual_screen;
  wputblock(0,0,screen,0);
  evblockbasename="inventory%d";
  run_event_block(evpt,evnt,invn);
  abuf=screen;
  domouse(1);
}

#define ICONSPERLINE 4

struct DisplayInvItem {
  int num;
  int sprnum;
  };
int __actual_invscreen() {
  int iconsperline;
  write_screen();
start_actinv:
  DisplayInvItem dii[MAX_INV];
  int numitems=0,ww,widest=0,highest=0;
  for (ww=0;ww<game.numinvitems;ww++) {
    if ((playerchar->inv[ww]>0) && (game.invinfo[ww].name[0]!=0)) {
      dii[numitems].num=ww;
      dii[numitems].sprnum=game.invinfo[ww].pic;
      int snn=dii[numitems].sprnum;
      if (spritewidth[snn]>widest) widest=spritewidth[snn];
      if (spriteheight[snn]>highest) highest=spriteheight[snn];
      numitems++;
      }
    }
  if (numitems<1) {
    char msg[60];
    strcpy(msg,get_language_text(28));
    strcat(msg," ");
    strcat(msg,get_language_text(24));
    Display(msg);
    cremovemenu();
    return -1;
  }
  widest+=sxmult*4;
  highest+=symult*4;
  int windowhit=highest*(numitems/ICONSPERLINE)+symult*30+symult*4;
  if ((numitems%ICONSPERLINE) !=0) windowhit+=highest;
  int windowwid=widest*ICONSPERLINE + sxmult*4;
  if (windowwid<(sxmult*105)) windowwid=sxmult*105;
  int windowxp=scrnwid/2-windowwid/2;
  int windowyp=scrnhit/2-windowhit/2;
  int buttonyp=windowyp+windowhit-(symult*30);
  wbutt(windowxp,windowyp,windowxp+windowwid,windowyp+windowhit);
  wsetcolor(0);
  int bartop=windowyp+symult*2;
  int barxp=windowxp+sxmult*2;
  wbar(barxp,bartop,windowxp+windowwid-sxmult*2,buttonyp-1);
  for (ww=0;ww<numitems;ww++) {
    block spof=images[dii[ww].sprnum];
    wputblock(barxp+1+(ww%4)*widest+widest/2-wgetblockwidth(spof)/2,
      bartop+1+(ww/4)*highest+highest/2-wgetblockheight(spof)/2,spof,1);
    }
  #define BUTTONWID spritewidth[2042]
  // Draw select, look and OK buttons
  wputblock(windowxp+2,buttonyp+symult*2,images[2041],0);
  wputblock(windowxp+3+BUTTONWID,buttonyp+symult*2,images[2042],0);
  wputblock(windowxp+4+BUTTONWID*2,buttonyp+symult*2,images[2043],0);
  domouse(1);
  set_mouse_cursor(CURS_ARROW);
  int toret=-1,wasonitem=-1;
  int cmode=CURS_ARROW;
  while (!kbhit()) {
    domouse(0);
    poll_mp3();
    int isonitem=((mousey-bartop)/highest)*ICONSPERLINE+(mousex-barxp)/widest;
    if (mousey<=bartop) isonitem=-1;
    if ((isonitem<0) | (isonitem>=numitems)) isonitem=-1;
    if (mgetbutton()==LEFT) {
      if ((mousey<windowyp) | (mousey>windowyp+windowhit) | (mousex<windowxp) | (mousex>windowxp+windowwid))
        continue;
      if (mousey<buttonyp) {
        int clickedon=isonitem;
        if (clickedon<0) continue;
        evblocknum=dii[clickedon].num;
        if (cmode==MODE_LOOK) {
          run_event_block_inv(&game.invcond[dii[clickedon].num],0);
          continue;
          }
        else if (cmode==MODE_USE) {
          // use objects on each other
          run_event_block_inv(&game.invcond[dii[clickedon].num],3,toret);
          domouse(2);
          goto start_actinv;
//          continue;
          }
        toret=dii[clickedon].num;
        game.mcurs[MODE_USE].pic=game.invinfo[toret].pic;
        game.mcurs[MODE_USE].hotx=spritewidth[game.invinfo[toret].pic]/2;
        game.mcurs[MODE_USE].hoty=spriteheight[game.invinfo[toret].pic]/2;
        set_mouse_cursor(MODE_USE);
        cmode=MODE_USE;
//        break
        continue;
        }
      else {
        int buton=(mousex-windowxp)-2;
        if (buton<0) continue;
        buton/=BUTTONWID;
        if (buton>=3) continue;
        if (buton==0) { toret=-1; cmode=MODE_LOOK; }
        else if (buton==1) { cmode=CURS_ARROW; toret=-1; }
        else break;
        set_mouse_cursor(cmode);
        }
      }
    else if (isonitem!=wasonitem) { domouse(2);
      int rectxp=barxp+1+(wasonitem%4)*widest;
      int rectyp=bartop+1+(wasonitem/4)*highest;
      if (wasonitem>=0) {
        wsetcolor(0);
        wrectangle(rectxp,rectyp,rectxp+widest-1,rectyp+highest-1);
        }
      if (isonitem>=0) { wsetcolor(14);//opts.invrectcol);
        rectxp=barxp+1+(isonitem%4)*widest;
        rectyp=bartop+1+(isonitem/4)*highest;
        wrectangle(rectxp,rectyp,rectxp+widest-1,rectyp+highest-1);
        }
      domouse(1);
      }
    wasonitem=isonitem;
  }
  while (kbhit()) getch();
  set_default_cursor();
  domouse(2);
  cremovemenu();
  return toret;
}

int invscreen() {
  int selt=__actual_invscreen();
  if (selt<0) return -1;
  playerchar->activeinv=selt;
  set_cursor_mode(MODE_USE);
  return selt;
  }

void sc_invscreen() {
  request_invscreen=1;
  }

void setup_script_exports() {
  scAdd_External_Symbol("AddInventory",(void *)add_inventory);
  scAdd_External_Symbol("AnimateObject",(void *)AnimateObject);
  scAdd_External_Symbol("CapturedPrint",(void *)CapturedPrint);
  scAdd_External_Symbol("CaptureScreen",(void *)CaptureScreen);
  scAdd_External_Symbol("CDAudio",(void *)cd_player_control);
  scAdd_External_Symbol("CyclePalette",(void *)CyclePalette);
  scAdd_External_Symbol("Debug",(void *)script_debug);
  scAdd_External_Symbol("DisableCursorMode",(void *)disable_cursor_mode);
  scAdd_External_Symbol("Display",(void *)Display);
  scAdd_External_Symbol("DisplayAt",(void *)DisplayAt);
  scAdd_External_Symbol("DisplayMessage",(void *)DisplayMessage);
  scAdd_External_Symbol("EnableCursorMode",(void *)enable_cursor_mode);
  scAdd_External_Symbol("EndCapture",(void *)EndCapture);
  scAdd_External_Symbol("FaceCharacter",(void *)FaceCharacter);
  scAdd_External_Symbol("FaceLocation",(void *)FaceLocation);
  scAdd_External_Symbol("FadeIn",(void *)FadeIn);
  scAdd_External_Symbol("FadeOut",(void *)FadeOut);
  scAdd_External_Symbol("FileClose",(void *)FileClose);
  scAdd_External_Symbol("FileOpen",(void *)FileOpen);
  scAdd_External_Symbol("FileRead",(void *)FileRead);
  scAdd_External_Symbol("FileWrite",(void *)FileWrite);
  scAdd_External_Symbol("GetCursorMode",(void *)GetCursorMode);
  scAdd_External_Symbol("GetLanguageString",(void *)GetLanguageString);
  scAdd_External_Symbol("GetLocationName",(void *)GetLocationName);
  scAdd_External_Symbol("GetPlayerCharacter",(void *)GetPlayerCharacter);
  scAdd_External_Symbol("GiveScore",(void *)GiveScore);
  scAdd_External_Symbol("InterfaceOff",(void *)InterfaceOff);
  scAdd_External_Symbol("InterfaceOn",(void *)InterfaceOn);
  scAdd_External_Symbol("InventoryScreen",(void *)sc_invscreen);
  scAdd_External_Symbol("IsGamePaused",(void *)IsGamePaused);
  scAdd_External_Symbol("MoveCharacter",(void *)MoveCharacter);
  scAdd_External_Symbol("MoveCharacterToObject",(void *)MoveCharacterToObject);
  scAdd_External_Symbol("NewRoom",(void *)NewRoom);
  scAdd_External_Symbol("ObjectOff",(void *)ObjectOff);
  scAdd_External_Symbol("ObjectOn",(void *)ObjectOn);
  scAdd_External_Symbol("PlayFlic",(void *)play_flc_file);
  scAdd_External_Symbol("PlayMusic",(void *)newmusic);
  scAdd_External_Symbol("PlaySound",(void *)play_audio_clip_by_index);
  scAdd_External_Symbol("ProcessClick",(void *)ProcessClick);
  scAdd_External_Symbol("QuitGame",(void *)QuitGame);
  scAdd_External_Symbol("Random",(void *)__Rand);
  scAdd_External_Symbol("RefreshMouse",(void *)RefreshMouse);
  scAdd_External_Symbol("ReleaseCharacterView",(void *)ReleaseCharacterView);
  scAdd_External_Symbol("RestoreGameDialog",(void *)restore_game_dialog);
  scAdd_External_Symbol("RunDialog",(void *)RunDialog);
  scAdd_External_Symbol("SaveGameDialog",(void *)save_game_dialog);
  scAdd_External_Symbol("SetCharacterView",(void *)SetCharacterView);
  scAdd_External_Symbol("SetCursorMode",(void *)set_cursor_mode);
  scAdd_External_Symbol("SetDefaultCursor",(void *)set_default_cursor);
  scAdd_External_Symbol("SetDialogOption",(void *)SetDialogOption);
  scAdd_External_Symbol("SetGameSpeed",(void *)SetGameSpeed);
  scAdd_External_Symbol("SetMouseCursor",(void *)set_mouse_cursor);
  scAdd_External_Symbol("SetObjectFrame",(void *)SetObjectFrame);
  scAdd_External_Symbol("SetObjectGraphic",(void *)SetObjectGraphic);
  scAdd_External_Symbol("SetObjectView",(void *)SetObjectView);
  scAdd_External_Symbol("SetPalRGB",(void *)SetPalRGB);
  scAdd_External_Symbol("SetPlayerCharacter",(void *)SetPlayerCharacter);
  scAdd_External_Symbol("StopMoving",(void *)StopMoving);
  scAdd_External_Symbol("StrCat",(void *)strcat);
  scAdd_External_Symbol("StrComp",(void *)strcmp);
  scAdd_External_Symbol("StrCopy",(void *)strcpy);
  scAdd_External_Symbol("StrLen",(void *)strlen);
  scAdd_External_Symbol("interface",&game.iface[0]-1);
  scAdd_External_Symbol("game",&play);
  scAdd_External_Symbol("gs_globals",&play.globalvars[0]);
  scAdd_External_Symbol("mouse",&scmouse);
  scAdd_External_Symbol("system",&scsystem);
}

void mainloop() {
  timerloop=0;
  setevent(EV_TEXTSCRIPT,TS_REPEAT);
  setevent(EV_RUNEVBLOCK,EVB_ROOM,0,6);
  // run this immediately to make sure it gets done before fade-in
  // (player enters screen)
  if ((in_new_room>0) & (in_new_room!=3))
    setevent(EV_RUNEVBLOCK,EVB_ROOM,0,5);
  // check if he's standing on a hotspot
  int onhs=getpixel(thisroom.lookat,playerchar->x,playerchar->y);
  // run Stands on Hotspot event
  setevent(EV_RUNEVBLOCK,EVB_HOTSPOT,onhs,0);
  check_controls();
  if (game_paused==0) update_stuff();
  mouse_on_iface=-1;
  draw_screen_background();
  if (libamp_loaded!=0) {
    if (amp_decode()<0) {
      unload_amp();
      libamp_loaded=0;
    }
  }
  draw_topbar();
  update_screen();
  if (in_new_room>0) {
    setevent(EV_FADEIN,0,0,0);
    if (in_new_room==2)  // first time enters screen
      setevent(EV_RUNEVBLOCK,EVB_ROOM,0,4);
  }
  in_new_room=0;
  processallevents(numevents,event);
  numevents=0;
  poll_mp3();
  gettime(&t2);
  loopcounter++;
  if (wtimer(t1,t2)>100) {
    gettime(&t1);
    fps=loopcounter-lastcounter;
    lastcounter=loopcounter;
  }
  while (timerloop==0) ;
}

int check_write_access() {
  struct diskfree_t df;
  unsigned long freebytes=0;
  if (!_dos_getdiskfree(0,&df)) {
    freebytes = (unsigned long)df.avail_clusters *
                (unsigned long)df.bytes_per_sector *
                (unsigned long)df.sectors_per_cluster;
  }
  if (freebytes<200000) return 0;
  FILE*yy=fopen("~tmptest.tmp","wb");
  if (yy==NULL)
    return 0;
  fwrite("just to test the drive free space",30,1,yy);
  fclose(yy);
  if (remove("~tmptest.tmp"))
    return 0;
  return 1;
}

void run_setup() {
  setupmain();
  }

int main_game_loop() {
  mainloop();
  if (restrict_until==0) ;
  else {
    if (restrict_until==UNTIL_MOVEEND) {
      short*wkptr=(short*)user_disabled_data;
      if (wkptr[0]<1) restrict_until=0; }
    else if (restrict_until==UNTIL_CHARIS0) {
      char*chptr=(char*)user_disabled_data;
      if (chptr[0]==0) restrict_until=0; }
    else if (restrict_until==UNTIL_NOOVERLAY) {
      if (screen_state==0) restrict_until=0; }
    else quit("loop_until: unknown until event");
    if (restrict_until==0) {
      set_default_cursor();
      play.disabled_user_interface--;
      if (user_disabled_for==FOR_ANIMATION)
        run_animation((FullAnimation*)user_disabled_data2,user_disabled_data3);
      else if (user_disabled_for==FOR_EXITLOOP) {
        user_disabled_for=0; return -1; }
      else if (user_disabled_for==FOR_SCRIPT) {
        user_disabled_for=-2;
        scActual_Instance=(scInstance*)user_disabled_data3;
        Register(scActual_Instance)[regIP]=user_disabled_data2;
        scInstance*inst=scActual_Instance;
        Register(inst)[regCP]=Register(inst)[regSP]=ToCodeINT(inst)[hdrStackSize];
      }
      if (user_disabled_for<0) {
        user_disabled_for=0;
        if (Executor(scActual_Instance,-1)<0)
          quit(scErrorMsg);
      }
      else user_disabled_for=0;
    }
  }
  return 0;
}

void do_main_cycle(int untilwhat,int daaa) {
  main_loop_until(untilwhat,daaa,0);
  user_disabled_for=FOR_EXITLOOP;
  while (main_game_loop()==0) ;
}

int init_cd_player()
{
  use_cdplayer=0;
  numcddrives=cd_installed();
  if (numcddrives==0) return -1;
  if (cd_getversion()<0x020A) return -2;
  cd_getdriveletters(cd_driveletters);
  cddrive=cd_driveletters[0];
  use_cdplayer=1;
  return 0;
}

//char datname[80]="ac.clb";
char*ac_config_file="acsetup.cfg";
char conffilebuf[80];

int main(int argc,char**argv)
{
  int ee,bb;
  cfopenpriority=2;
  printf("Adventure Creator v" AC_VERSION_TEXT "Interpreter\n"
         "Copyright (c) 1999 Chris Jones\n"
         "ACI version " ACI_VERSION_TEXT "\n");
  if ((argc>1) && (argv[1][1]=='?'))
    return 0;
  strcpy(conffilebuf,argv[0]);
  for (ee=0;ee<strlen(conffilebuf);ee++) {
    if (conffilebuf[ee]=='/') conffilebuf[ee]='\\';
  }
  INIgetdirec(conffilebuf,ac_config_file);
  ac_config_file=&conffilebuf[0];
  FILE*ppp=fopen(ac_config_file,"rt");
  if (ppp!=NULL) {
    strcpy(filetouse,ac_config_file);
    fclose(ppp);
    usetup.digicard=INIreadint("sound","digiid");
    usetup.midicard=INIreadint("sound","midiid");
    usetup.screenres=INIreadint("misc","screenres");
  }
  int errcod=csetlib(argv[0],"");  // assume it's appended to exe
  if (errcod!=0) { // it's not
    errcod = csetlib("ac2game.dat","");
  }
  if (errcod!=0) {  // there's a problem
    printf("Error ST02: could not load game file 'AC2GAME.DAT'.\n");
    if (errcod==-1) { // file not found
      printf("You must create and save a game first in the Room Editor before you\n"
             "can use this engine.\n");
    } else {
      printf("The file is corrupt. Make sure you have the correct version of the\n"
             "Room Editor.\n");
    }
    return 8;
  }
  if (init_cd_player()==0) {
    printf("CD-ROM Audio support enabled.\n");
  }
  check_cpu();
  const char*cpu_families[4]={
    "386",
    "486",
    "Pentium",
    "Pentium Pro"
  };
  if (cpu_family>6) cpu_family=6;
  printf("%s CPU detected.\n", cpu_families[cpu_family-3]);
  if (cpu_family<5) usetup.mp3_player=0;
  if (cpu_family<4) usetup.mod_player=0;
  if ((argc>1) && (stricmp(argv[1],"--setup")==0)) {
    run_setup();
    return 0;
  }
  init_language_text("en");
  if (check_write_access()==0) {
    printf("Unable to write to the current directory. Do not run this game off a\n"
      "network or CD-ROM drive. Also check drive free space (you need 1 Mb free).\n");
    return 6;
  }
  if (minstalled()==0) {
    printf("This game requires a mouse. You need to load a DOS mouse driver to\n"
      "play this game. Try typing 'MOUSE'.\n");
    return 3;
  }
  char*memcheck=(char*)malloc(4000000);
  if (memcheck==NULL) {
    printf("There is not enough memory available to run this game. You need 4 Mb free\n"
      "extended memory to run the game.\n"
      "If you are running from Windows, check the 'DPMI memory' setting on the DOS box\n"
      "properties.\n");
    return 9;
    }
  free(memcheck);
  roomstats=(RoomStatus*)calloc(sizeof(RoomStatus),MAX_ROOMS);
  for (ee=0;ee<MAX_ROOMS;ee++) {
    roomstats[ee].beenhere=0;
    roomstats[ee].numobj=0;
    roomstats[ee].tsdatasize=0;
    roomstats[ee].tsdata=NULL;
    }
  printf("Checking sound inits.\n");
  allegro_init();
  if (usetup.mod_player) reserve_voices(NUM_MOD_DIGI_VOICES+2,-1);
  if (install_sound(usetup.digicard,usetup.midicard,NULL)!=0) {
    reserve_voices(-1,-1);
    opts.mod_player=0;
    opts.mp3_player=0;
    if (install_sound(usetup.digicard,usetup.midicard,NULL)!=0) {
      printf("\nUnable to initialize your audio hardware.\n");
      printf("[Problem: %s]\n",allegro_error);
      proper_exit=1;
      return 7;
    }
  }
  else {
    if (usetup.mp3_player==0) ;
    else install_amp();
  }
  atexit(atexit_handler);
  srand(time(NULL) % 2000);
  init_pathfinder();
  set_gfx_mode(GFX_VGA,320,200,320,200);
  abuf=screen; vesa_xres=320; vesa_yres=200;
  scrnwid=320;
  scrnhit=200;
  if (usetup.screenres==1) {
    scrnwid=640;
    scrnhit=400;
  }
  if (usetup.screenres==2) {
    scrnwid=960;
    scrnhit=600;
  }
  vesa_xres=scrnwid;vesa_yres=scrnhit;
  scrnwto=scrnwid-1;scrnhto=scrnhit-1;
  sxmult=scrnwid/320;
  symult=scrnhit/200;
  color temppal[256];
  int initasx=scrnwid,initasy=scrnhit;
  if (scrnwid==960) { initasx=1024;initasy=768; }
  if (set_gfx_mode(GFX_AUTODETECT,initasx,initasy,0,0)!=0) {
    set_gfx_mode(GFX_TEXT,80,25,0,0);
    proper_exit=1;
    printf("There was a problem initializing graphics mode.\n");
    printf("(Problem: \'%s\')\n",allegro_error);
    printf("Try to correct the problem, or seek help from the Adventure Creator homepage.\n");
    return 4;
  }
  if (scrnwid==960) {
    _old_screen=screen;
    clear(screen);
    screen=create_sub_bitmap(_old_screen,32,84,960,600);
  }
  block splashsc=load_pcx("preload.pcx",temppal);
  if (splashsc!=NULL) {
    wsetpalette(0,255,temppal);
    stretch_blit(splashsc,screen,0,0,splashsc->w,splashsc->h,0,0,scrnwid,scrnhit);
    wfreeblock(splashsc);
  }
  install_timer();
  LOCK_VARIABLE(timerloop);
  LOCK_FUNCTION((void*)dj_timer_handler);
  set_game_speed(40);
  if (usetup.mod_player) {
    if (install_mod(NUM_MOD_DIGI_VOICES+1)<0) {
      quit("install_mod: MOD player initalize failed.");
    }
  }
  virtual_screen=create_bitmap(scrnwid,scrnhit);
  clear(virtual_screen);
  if (wloadsprites(temppal,"acsprset.spr",images,0,MAX_SPRITES)) {
    if (wloadsprites(temppal,"acdefset.spr",images,0,MAX_SPRITES)) {
      set_gfx_mode(GFX_TEXT,80,25,0,0);
      allegro_exit();
      proper_exit=1;
      printf("Could not load sprite set file ACSPRSET.SPR\n");
      printf("(Error ST01).\n");
      return 5;
    }
  }
  for (ee=0;ee<MAX_SPRITES;ee++) {
    if (images[ee]==NULL) {
      spritewidth[ee]=0;
      spriteheight[ee]=0;
      continue;
    }
    else {
      block tmpdbl=create_bitmap(images[ee]->w*sxmult,images[ee]->h*symult);
      clear(tmpdbl);
      stretch_sprite(tmpdbl,images[ee],0,0,tmpdbl->w,tmpdbl->h);
      destroy_bitmap(images[ee]);
      images[ee]=tmpdbl;
      spritewidth[ee]=images[ee]->w;
      spriteheight[ee]=images[ee]->h;
    }
  }
  for (ee=0;ee<MAX_INIT_SPR+MAX_CHARACTERS;ee++) {
    actsps[ee]=NULL;
  }
  thisroom.allocall();
  setup_sierra_interface();
  setup_script_exports();
  if ((ee=load_game_file())!=0) {
    set_gfx_mode(GFX_TEXT,80,25,0,0);
    proper_exit=1;
    printf("Could not load game file.\n(Reason: ");
    if (ee==-1)
      printf("File not found. Please run the Room Editor to create a game first).\n");
    else if (ee==-2)
      printf("Invalid file format. The file may be corrupt, or from a different\n"
        "version of Creator).\n");
    else if (ee==-3)
      printf("Script link failed [error %d]\n-- %s --)\n",scErrorNo,scErrorMsg);
    return 6;
  }
  init_language_text(game.langcodes[0]);
  for (ee=0;ee<256;ee++) {
    if (game.paluses[ee]!=PAL_BACKGROUND) palette[ee]=game.defpal[ee];
  }
  for (ee=0;ee<game.numcharacters;ee++) {
    memset(&game.chars[ee].inv[0],0,MAX_INV*sizeof(short));
  }
  for (ee=0;ee<game.numiface;ee++) {
    InterfaceElement*iep=&game.iface[ee];
    iep->x*=sxmult;
    iep->y*=symult;
    iep->x2=iep->x2*sxmult+(sxmult-1);
    iep->y2=iep->y2*symult+(symult-1);
    iep->vtextxp*=sxmult;
    iep->vtextyp*=symult;
    iep->popupyp*=symult;
    for (bb=0;bb<iep->numbuttons;bb++) {
      iep->button[bb].x*=sxmult;
      iep->button[bb].y*=symult;
    }
  }
  for (ee=0;ee<MAX_CURSOR;ee++) {
    game.mcurs[ee].hotx*=sxmult;
    game.mcurs[ee].hoty*=symult;
  }
  for (ee=0;ee<game.numinvitems;ee++) {
    if (game.invinfo[ee].flags & IFLG_STARTWITH) playerchar->inv[ee]=1;
    else playerchar->inv[ee]=0;
  }
  play.score=0;
  play.messagetime=-1;
  play.disabled_user_interface=0;
  play.gscript_timer=-1;
  play.debug_mode=game.options[OPT_DEBUGMODE];
  scsystem.width=scrnwid;
  scsystem.height=scrnhit;
  scsystem.coldepth=8;
  scsystem.os=1;
  mousecurs[0]=images[2054];
  currentcursor=0;
  set_cursor_mode(MODE_WALK);
  mgraphconfine(0,0,319,199);
  wsetscreen(virtual_screen);
  newmusic(0);
  run_text_script(gameinst,"game_start");
  new_room(playerchar->room,playerchar);
  gettime(&t1);
  lastcounter=0;
  loopcounter=0;
  main_game_loop();
  save_game(RESTART_POINT_SAVE_GAME_NUMBER,"restart game position");
  while (true) {
    main_game_loop();
  }
  quit("|bye!");
  return 0;
}
