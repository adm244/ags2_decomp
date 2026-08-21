#ifndef __ACRUNTIME_H
#define __ACRUNTIME_H

#include <libamp.h>
#include <jgmod.h>

#define MAX_MAXSTRLEN 200
#define MAXGLOBALVARS 50

// This struct is only used in save games and by plugins
struct RoomObject {
  short x,y;
  short num;            // sprite slot number
  short baseline;       // <=0 to use Y co-ordinate; >0 for specific baseline
  short view,loop,frame; // only used to track animation - 'num' holds the current sprite
  short wait;
  char  cycling;        // is it currently animating?
  char  overall_speed;
  char  on;
};

// This struct is saved in the save games - it contains everything about
// a room that could change
struct RoomStatus {
  int   beenhere;
  int   numobj;
  RoomObject obj[MAX_INIT_SPR];
  short flagstates[MAX_FLAGS];
  int   tsdatasize;
  char* tsdata;
  EventBlock hscond[MAX_HOTSPOTS];
  EventBlock objcond[MAX_INIT_SPR];
  EventBlock misccond;
  RoomStatus() { beenhere=0; numobj=0; tsdatasize=0; tsdata=NULL; }
};

// The text script's "mouse" struct
struct ScriptMouse {
  int x,y;
};

// The text script's "system" struct
struct ScriptSystem {
  int width,height;
  int coldepth;
  int os;
};

struct EventHappened {
  int type;
  int data1,data2,data3;
  int player;
};

struct SpriteListEntry {
  block pic;
  int baseline;
  int x,y;
};

// game setup, read in from CFG file
// this struct is redefined in acdialog.cpp, any changes might
// need to be reflected there
struct GameSetup
{
  int digicard, midicard;
  int screenres;
  int mod_player;
  int textheight;
  int mp3_player;
  GameSetup() {
    digicard=DIGI_AUTODETECT; midicard=MIDI_AUTODETECT;
    screenres=0;
    mod_player=1; mp3_player=1;
  }
};

struct GameState {
  int  score;      // player's current score
  int  usedmode;   // set by ProcessClick to last cursor mode used
  int  disabled_user_interface;  // >0 while in cutscene/etc
  int  gscript_timer;    // graphical script timer
  int  debug_mode;       // whether we're in debug mode
  int  globalvars[MAXGLOBALVARS];  // graphical script variables
  int  messagetime;      // time left for auto-remove messages
  int  usedinv;          // inventory item last used
  int  inv_top,inv_numdisp,inv_numorder,inv_numinline;
  short inv_order[100];
};

#define NUM_DIGI_VOICES     16
#define NUM_MOD_DIGI_VOICES 12

#define MAXSCORE game.totalscore
#define CHANIM_REPEAT    2
#define ANIM_ONCE      1
#define ANIM_REPEAT    2
#define ANIM_ONCERESET 3
#define FONT_NORMAL     0
#define FONT_SPEECHBACK 1
#define FONT_SPEECH     2
#define MODE_WALK 0
#define MODE_LOOK 1
#define MODE_HAND 2
#define MODE_TALK 3
#define MODE_USE  4
#define MODE_PICKUP 5
#define CURS_ARROW  6
#define CURS_WAIT   7
#define MODE_CUSTOM1 8

#define FOR_ANIMATION 1
#define FOR_SCRIPT    2
#define FOR_EXITLOOP  3
#define opts usetup

#define UNTIL_ANIMEND   1
#define UNTIL_MOVEEND   2
#define UNTIL_CHARIS0   3
#define UNTIL_NOOVERLAY 4
#define UNTIL_NEGATIVE  5
#define UNTIL_INTIS0    6
#define UNTIL_SHORTIS0  7
#define UNTIL_INTISNEG  8
#define MANOBJNUM 99

#define STD_BUFFER_SIZE 300

#define MAXLINE 15

extern GameStruct game;
extern RoomStatus *croom;

#define NONE -1
#define LEFT  0
#define RIGHT 1
extern int  mousex,mousey;
extern void domouse(int);
extern int  mgetbutton();

extern int  find_route(int,int,int,int,block,int,int=0,int=0);
extern void init_pathfinder();
extern void do_main_cycle(int,int);
extern void Display(char*, ...);
extern int  do_movelist_move(short*,int*,int*);
extern void disable_cursor_mode(int);
extern void enable_cursor_mode(int);
extern void set_cursor_mode(int);
extern void newmusic(int);

inline int is_valid_character(int newchar) {
  if ((newchar<0) | (newchar>=game.numcharacters)) return 0;
  return 1;
}

inline int is_valid_object(int obtest) {
  if ((obtest<0) | (obtest>=croom->numobj)) return 0;
  return 1;
}

#endif
