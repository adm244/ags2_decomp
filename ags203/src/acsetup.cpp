#include <allegro.h>
#include <conio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern char*get_language_text(int);
extern void init_language_text(char*);
extern char*ac_config_file;
extern int INIreadint(char*,char*,int=-1);

char border_top_left[]=" \xDA";
char border_horizontal[]="\xC4";
char border_top_right[]="\xBF ";
char border_left[]=" \xB3";
char space[]=" ";
char border_right[]="\xB3 ";
char shadow_vertical[]="\xDB\xDB";
char border_bottom_left[]=" \xC0";
char border_bottom_right[]="\xD9 ";
char shadow_horizontal[]="\xDB";

char conpos=0;

char*backup[4];
char mesboxx[4];
char mesboxy[4];
char mesboxw[4];
char mesboxh[4];

int scrline=0;
char bgline[80+1];

char optionsbuf[5][50];
char autodetect_sndbuf[50];
char no_sndbuf[50];
char autodetect_musbuf[50];
char no_musbuf[50];

char textcolo=0,barcolor=10,backcol=0;
int bkpnum=0;
int ac_desc_xpos=25;
int ac_setup_key=0;
char ac_opts_enabled[10]={
  TRUE, TRUE, TRUE, TRUE, TRUE,
  TRUE, TRUE, TRUE, TRUE, TRUE
};
int fillscrn=0;
int ac_cursel=0;

void draw_box(int xx,int yy,int ww,int hh,int bkp) {
  if (bkp!=0) {
    // save area that will be occupied by a box
    backup[bkpnum]=(char*)malloc((ww+2)*(hh+1)*2+2);
    gettext(xx,yy,xx+ww+1,yy+hh,backup[bkpnum]);
    bkpnum++;
  }
  // draw top border
  textbackground(barcolor);
  textcolor(textcolo);
  gotoxy(xx,yy);
  cprintf(border_top_left);
  for (conpos=0;conpos<ww-4;conpos++) cprintf(border_horizontal);
  cprintf(border_top_right);
  // draw middle part
  for (conpos=0;conpos<hh-2;conpos++) {
    gotoxy(xx,conpos+yy+1);
    cprintf(border_left);
    int cc;
    for (cc=0;cc<ww-4;cc++) cprintf(" ");
    cprintf(border_right);
    textcolor(backcol);
    cprintf(shadow_vertical);
    textcolor(textcolo);
  }
  // draw botom border
  gotoxy(xx,yy+hh-1);
  cprintf(border_bottom_left);
  for (conpos=0;conpos<ww-4;conpos++) cprintf(border_horizontal);
  cprintf(border_bottom_right);
  // draw bottom shadow
  textcolor(backcol);
  cprintf(shadow_vertical);
  gotoxy(xx+2,yy+hh);
  for (conpos=0;conpos<ww;conpos++) cprintf(shadow_horizontal);
  textbackground(1);
  if(bkp!=0) {
    mesboxx[bkpnum-1]=xx;
    mesboxy[bkpnum-1]=yy;
    mesboxw[bkpnum-1]=ww;
    mesboxh[bkpnum-1]=hh;
  }
}

void restore_backup_line() {
  puttext(mesboxx[bkpnum-1],mesboxy[bkpnum-1],mesboxx[bkpnum-1]+mesboxw[bkpnum-1]+1,
    mesboxy[bkpnum-1]+mesboxh[bkpnum-1],backup[bkpnum-1]);
  free(backup[bkpnum-1]);
  bkpnum--;
}

void set_text_color(int fgcol,int bgcol) {
  textbackground(bgcol); textcolor(fgcol);
  barcolor=bgcol; textcolo=fgcol;
}

void draw_text_centre(int yy,char *txt) {
  gotoxy(40-strlen(txt)/2,yy);
  cprintf(txt);
}

int cgetch() {
  int chr=getch();
  if (chr==0) chr=getch()+300;
  return chr;
}

void draw_statusbar(char *txt,int xx) {
  textcolor(0);
  textbackground(7);
  int ii=0;
  if (txt==NULL) return;
  gotoxy(xx,25);
  for (;txt[ii]!=0;ii++) {
    if (txt[ii]=='|') { textcolor(4); ii++; }
    putch(txt[ii]);
    textcolor(0);
  }
  if (fillscrn!=0) { int xp=79-wherex();
    cprintf("%*s",xp," ");
  }
}

char*actitle="Adventure Creator Setup";
char*accopyright="(c) 1999 Chris Jones";
void setup_clearscr() {
  set_text_color(0,7);
  clrscr();
  gotoxy(2,1);
  cprintf(actitle);
  gotoxy(80-strlen(accopyright),1);
  cprintf(accopyright);
  gotoxy(1,2);
  textcolor(1);
  for (scrline=1;scrline<24;scrline++) cprintf("%s",bgline);
}

int dotextmenu(int optcnt,char*optname[],char*optdesc[],int optsel,int yoffs) {
  int aa=0;
  _setcursortype(_NOCURSOR);
  int maxopt=18,curopt=0,yy=11;
  if (optcnt<maxopt) maxopt=optcnt;
  yy=13-maxopt/2; yy+=yoffs;
  for (scrline=0;scrline<optcnt;scrline++) {
    if (aa<strlen(optname[scrline])) aa=strlen(optname[scrline]);
  }
  aa+=8;
  set_text_color(15,1);
  draw_box(39-aa/2,yy-1,aa-2,maxopt+2,1);
  int xx=42-aa/2,ww=aa;
  aa=0;
  textbackground(7);
  int drawbar=1;
  if (optdesc[0]==NULL) drawbar=0;
  if (drawbar)
    draw_statusbar("                                              ",ac_desc_xpos);
  set_text_color(15,1);
  int drawtxt=0;
  for (;;) {
    if (aa==27) break;
    set_text_color(0,7);
    if (drawbar) draw_statusbar(optdesc[optsel],ac_desc_xpos);
    for (scrline=curopt;scrline<curopt+maxopt;scrline++) {
      gotoxy(xx,yy+scrline-curopt);
      textbackground(1); textcolor(15);
      if (drawtxt) { cprintf("%-*s",ww-7,optname[scrline]);
        gotoxy(xx,yy+scrline-curopt); }
      if (optsel==scrline) { textbackground(7); textcolor(1); }
      if ((!ac_opts_enabled[scrline]) & (scrline<10)) textcolor(8);
      cprintf("%s",optname[scrline]);
    }
    aa=cgetch(); drawtxt=0; ac_cursel=optsel;
    if (aa>=500) { optsel=aa; break; }
    if ((aa==13) & (!ac_opts_enabled[optsel]) & (scrline<10)) aa=0; // enter
    if (aa==372) optsel--; // up arrow
    if (aa==380) optsel++; // down arrow
    // page up
    if (aa==373) { optsel=optsel-(maxopt-1);
      if (optsel<0) optsel=0; }
    // page down
    if (aa==381) { optsel=optsel+(maxopt-1);
      if (optsel>=optcnt) optsel=optcnt-1; }
    if (aa==371) optsel=0; // home
    if (aa==379) optsel=optcnt-1; // end
    if (optsel<0) optsel=0;
    if (optsel>=optcnt) optsel=optcnt-1;
    if ((optsel-curopt)>=maxopt) { curopt=optsel-maxopt+1; drawtxt=1; }
    if (optsel<curopt) { curopt=optsel; drawtxt=1; }
    if (aa==13) break;
    if (aa>370) { textbackground(7);
      if (drawbar)
        draw_statusbar("                                                  ",ac_desc_xpos);
    }
    if ((aa==ac_setup_key) & (ac_setup_key>0)) break;
  }
  restore_backup_line();
  if (aa==27) return -1; // escape
  if (aa==ac_setup_key) return -2;
  return optsel;
}

void domessagebox(char*ll1,char*ll2,int col) {
  int yy=11,hh=4;
  if (ll2[0]==0) {yy++;hh=3;}
  barcolor=col;
  int xx=18,ww=44;
  if (strlen(ll1)>=ww) ww=strlen(ll1)+4;
  xx=40-ww/2;
  draw_box(xx,yy,ww,hh,1);
  set_text_color(15,col);
  if (ll2[0]!=0) {
    draw_text_centre(12,ll1);
    draw_text_centre(13,ll2);
  }
  else draw_text_centre(13,ll1);
}

#define bool __bool

#define OPTSNO 5
char*ac_setup_options[OPTSNO]={
  "Select digital sound driver",
  "Select MIDI music driver",
  "Select screen resolution",
  "Exit and save configuration",
  NULL
};
char*ac_setup_descs[OPTSNO]={
  "Specify which digital sound card you have.",
  "Specify which sound card you use for Midi music",
  "Choose whether to use VGA or SVGA",
  "Saves your settings and exits Setup",
  NULL
};

#define DIGIDRVNO 6
char*digi_options[DIGIDRVNO]={
  "Autodetect sound card",
  "No digital sound",
  "Sound Blaster",
  "Sound Blaster Pro",
  "Sound Blaster 16/AWE32",
  "ESS Audiodrive"
};
int digi_drivers[DIGIDRVNO]={
  DIGI_AUTODETECT,
  DIGI_NONE,
  DIGI_SB,
  DIGI_SBPRO,
  DIGI_SB16,
  DIGI_AUDIODRIVE
};

#define MIDIDRVNO 8
char*midi_options[MIDIDRVNO]={
  "Autodetect music card",
  "No music",
  "Adlib",
  "Sound Blaster",
  "Sound Blaster Pro-II/SB16",
  "Sound Blaster AWE32",
  "SB MIDI out",
  "General MIDI (SB Live!/MPU-401)"
};
int midi_drivers[MIDIDRVNO]={
  MIDI_AUTODETECT,
  MIDI_NONE,
  MIDI_OPL2,
  MIDI_ADLIB,
  MIDI_OPL3,
  MIDI_AWE32,
  MIDI_SB_OUT,
  MIDI_MPU
};

#define RESOPTNO 3
char*res_options[RESOPTNO]={
  "320 x 200",
  "640 x 400",
  "960 x 600"
};
char*res_descs[RESOPTNO]={
  "Low-res VGA (fastest game speed)",
  "High-res VESA SVGA 640x400 (Pentium133+ only)",
  "Ultra-hires (at 1024x768), hi-spec only"
};

#define MENUOPTSNO 10
char*menu_descs[MENUOPTSNO]={
  NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL
};

void setupmain() {
  ac_desc_xpos=2;
  init_language_text(NULL);
  allegro_init();
  int curdigi=0,curmidi=0,curscrn=0;
  memset(bgline,-80,80); bgline[80]=0;
  setup_clearscr();
  FILE*iii=fopen(ac_config_file,"rt");
  if (iii!=NULL) { // read settings
    fclose(iii);
    curdigi=INIreadint("sound","digiindx",0);
    curmidi=INIreadint("sound","midiindx",0);
    curscrn=INIreadint("misc","screenres",0);
    if (curdigi<0) curdigi=0;
    if (curmidi<0) curmidi=0;
    if (curscrn<0) curscrn=0;
  }
  int ee;
  for (ee=0;ee<4;ee++) {
    strcpy(optionsbuf[ee],get_language_text(205+ee)); ac_setup_options[ee]=optionsbuf[ee];
  }
  strcpy(&autodetect_sndbuf[0],get_language_text(220)); digi_options[0]=autodetect_sndbuf;
  strcpy(&no_sndbuf[0],get_language_text(221)); digi_options[1]=no_sndbuf;
  strcpy(&autodetect_musbuf[0],get_language_text(222)); midi_options[0]=autodetect_musbuf;
  strcpy(&no_musbuf[0],get_language_text(223)); midi_options[1]=no_musbuf;
start_setup:
  set_text_color(0,3);
  draw_box(12,3,56,5,1);
  set_text_color(0,3);
  gotoxy(14,4); cprintf(get_language_text(200));
  gotoxy(14,5); cprintf(get_language_text(201));
  gotoxy(14,6); cprintf(get_language_text(202));
  int cursel=0;
  while (cursel>=0) {
    set_text_color(0,3);
    gotoxy(34,4); cprintf("%-29s",digi_options[curdigi]);
    gotoxy(34,5); cprintf("%-29s",midi_options[curmidi]);
    gotoxy(34,6); cprintf("%-29s",res_options[curscrn]);
    cursel=dotextmenu(4,&ac_setup_options[0],&ac_setup_descs[0],cursel,0);
    if (cursel==3) break; // exit
    if (cursel==0) { // select digi
repeat_digi:
      int digi=dotextmenu(DIGIDRVNO,&digi_options[0],&menu_descs[0],0,0);
      if (digi>=0) { curdigi=digi;
        if (detect_digi_driver(digi_drivers[curdigi])==0) {
          domessagebox(get_language_text(215),get_language_text(217),4);
          getch();
          restore_backup_line();
          curdigi=0;
          goto repeat_digi;
        }
      }
    }
    if (cursel==1) { // select midi
repeat_midi:
      int midi=dotextmenu(MIDIDRVNO,&midi_options[0],&menu_descs[0],0,0);
      if (midi>=0) { curmidi=midi;
        if (detect_midi_driver(midi_drivers[curmidi])==0) {
          domessagebox(get_language_text(216),get_language_text(217),4);
          getch();
          restore_backup_line();
          curmidi=0;
          goto repeat_midi;
        }
      }
    }
    if (cursel==2) { // select resolution
      int res=dotextmenu(RESOPTNO,&res_options[0],&res_descs[0],0,0);
      if (res>=0) curscrn=res;
    }
  }
  if (cursel<0) { // save settings dialog
    char*bool[2]={"Yes","No"};
    char btnyes[12],btnno[12];
    strcpy(btnyes,get_language_text(211)); bool[0]=btnyes;
    strcpy(btnno,get_language_text(212)); bool[1]=btnno;
    restore_backup_line();
    set_text_color(0,3);
    draw_box(20,5,40,3,1);
    set_text_color(0,3);
    char*exitmsg=get_language_text(210);
    gotoxy(40-strlen(exitmsg)/2,6); cprintf("%s",exitmsg);
    int chose=dotextmenu(2,&bool[0],&menu_descs[0],0,0);
    if (chose<0) { restore_backup_line();
      goto start_setup; }
    if (chose==0) cursel=3;
  }
  if (cursel==3) { // save settings
    FILE*ooo=fopen(ac_config_file,"wt");
    fprintf(ooo,"[sound]\ndigiindx=%d\ndigiid=%d\n",curdigi,digi_drivers[curdigi]);
    fprintf(ooo,"midiindx=%d\nmidiid=%d\n",curmidi,midi_drivers[curmidi]);
    fprintf(ooo,"\n[misc]\nscreenres=%d\n",curscrn);
    fclose(ooo);
  }
  textbackground(0);
  textcolor(7);
  clrscr();
  _setcursortype(_NORMALCURSOR);
}
