// TODO - need to define mlist
/*
  PathFinder v2.00 (AC2 customized version)
  (c) 1998-99 Chris Jones

  This is UNPUBLISHED PROPRIETARY SOURCE CODE;
  the contents of this file may not be disclosed to third parties,
  copied or duplicated in any form, in whole or in part, without
  prior express permission from Chris Jones.
*/

#define WGT2ALLEGRO_NOFUNCTIONS
#include "wgt2allg.h"
#define CROOM_NOFUNCTIONS
#include "acroom.h"
#include <string.h>
#include <math.h>

#define MAXPATHBACK 1000
int *pathbackx, *pathbacky;
int waspossible = 1;
int routex1, routey1;
int suggestx, suggesty;

extern void Display(char *, ...);

extern MoveList mls[60];
extern int move_speed;

void init_pathfinder()
{
  pathbackx = (int *)malloc(sizeof(int) * MAXPATHBACK);
  pathbacky = (int *)malloc(sizeof(int) * MAXPATHBACK);
}

block wallscreen;
//#define DEBUG_PATHFINDER
char *movelibcopyright = "PathFinder library v2.0 (c) 1998, 1999 Chris Jones.";
int line_failed = 0;
int lastcx, lastcy;

void line_callback(block bmpp, int x, int y, int d)
{
/*  if ((x>=320) | (y>=200) | (x<0) | (y<0)) line_failed=1;
  else */ if (getpixel(bmpp, x, y) == 0)
    line_failed = 1;
  else if (line_failed == 0) {
    lastcx = x;
    lastcy = y;
  }
}

int can_see_from(int x1, int y1, int x2, int y2)
{
  line_failed = 0;
  lastcx = -1;
  lastcy = -1;

  do_line(wallscreen, x1, y1, x2, y2, 0, line_callback);
  if (line_failed == 0)
    return 1;

  return 0;
}

#define MAX_GRANULARITY 3
int walk_area_granularity[MAX_WALK_AREAS + 1];
int is_route_possible(int fromx, int fromy, int tox, int toy, block wss)
{
  wallscreen = wss;
  suggestx = -1;

  block tempw = create_bitmap(wallscreen->w, wallscreen->h);
  blit(wallscreen, tempw, 0, 0, 0, 0, tempw->w, tempw->h);

  if (getpixel(tempw, fromx, fromy) == 0) {
    wfreeblock(tempw);
    return 0;
  }

  int dd, ff;
  for (dd = 0; dd < tempw->w; dd++) {
    for (ff = 0; ff < tempw->h; ff++) {
      if (_getpixel(tempw, dd, ff) != 0)
        _putpixel(tempw, dd, ff, 1);
    }
  }

  floodfill(tempw, fromx, fromy, 232);
  if (getpixel(tempw, tox, toy) != 232) {
    // Destination pixel is not walkable
    int ex, ey, nearest = 99999, thisis, nearx, neary;

    for (ex = 0; ex < tempw->w; ex += 5) {
      for (ey = 14; ey < tempw->h; ey += 5) {
        if (getpixel(tempw, ex, ey) != 232)
          continue;

        thisis = sqrt(fix((double)((ex - tox) * (ex - tox) + (ey - toy) * (ey - toy))));
        if (thisis < nearest)
        {
          nearest = thisis;
          nearx = ex;
          neary = ey;
        }
      }
    }

    if (nearest < 90000) {
      suggestx = nearx;
      suggesty = neary;
    }

    wfreeblock(tempw);
    return 0;
  }
  wfreeblock(tempw);

  return 1;
}

extern block mousecurs[10];
int leftorright = 0;
int nesting = 0;
int pathbackstage = 0;
int finalpartx = 0, finalparty = 0;
unsigned char *beenhere[600];

#define DIR_LEFT  0
#define DIR_RIGHT 2
#define DIR_UP    1
#define DIR_DOWN  3

int try_this_square(int srcx, int srcy, int tox, int toy)
{
  if (beenhere[srcy][srcx] & 0x80)
    return 0;

  if (nesting > 20000)
    return 0;

  nesting++;
  if (can_see_from(srcx, srcy, tox, toy)) {
    finalpartx = srcx;
    finalparty = srcy;
    nesting--;
    pathbackstage = 0;
    return 2;
  }

#ifdef DEBUG_PATHFINDER
  wputblock(lastcx, lastcy, mousecurs[C_CROSS], 1);
#endif

  int trydir = DIR_UP;
  int xdiff = abs(srcx - tox), ydiff = abs(srcy - toy);
  if (ydiff > xdiff) {
    if (srcy > toy)
      trydir = DIR_UP;
    else
      trydir = DIR_DOWN;
  } else if (srcx > tox)
    trydir = DIR_LEFT;
  else if (srcx < tox)
    trydir = DIR_RIGHT;

  int iterations = 0;

try_again:
  int nextx = srcx, nexty = srcy;
  if (trydir == DIR_LEFT)
    nextx--;
  else if (trydir == DIR_RIGHT)
    nextx++;
  else if (trydir == DIR_DOWN)
    nexty++;
  else if (trydir == DIR_UP)
    nexty--;

  iterations++;
  if (iterations > 5) {
//    fprintf(stderr,"not found: %d,%d  beenhere 0x%X\n",srcx,srcy,beenhere[srcy][srcx]);
    nesting--;
    return 0;
  }

  if (((nextx < 0) | (nextx >= wallscreen->w) | (nexty < 0) | (nexty >= wallscreen->h)) ||
      (getpixel(wallscreen, nextx, nexty) == 0) || ((beenhere[srcy][srcx] & (1 << trydir)) != 0)) {

    if (leftorright == 0) {
      trydir++;
      if (trydir > 3)
        trydir = 0;
    } else {
      trydir--;
      if (trydir < 0)
        trydir = 3;
    }
    goto try_again;
  }
  beenhere[srcy][srcx] |= (1 << trydir);
//  srcx=nextx; srcy=nexty;
  beenhere[srcy][srcx] |= 0x80; // being processed

  int retcod = try_this_square(nextx, nexty, tox, toy);
  if (retcod == 0)
    goto try_again;

  nesting--;
  beenhere[srcy][srcx] &= 0x7f;
  if (retcod == 2) {
    pathbackx[pathbackstage] = srcx;
    pathbacky[pathbackstage] = srcy;
    pathbackstage++;
    if (pathbackstage >= MAXPATHBACK - 1)
      return 0;

    return 2;
  }
  return 1;
}

int __find_route(int srcx, int srcy, int *tox, int *toy, int noredx)
{
  if ((noredx == 0) && (getpixel(wallscreen, tox[0], toy[0]) == 0))
    return 0; // clicked on a wall

  memset(&beenhere[0][0], 0, wallscreen->w * wallscreen->h);
  pathbackstage = 0;

  if (leftorright == 0) {
    waspossible = 1;

findroutebk:
    if ((waspossible = is_route_possible(srcx, srcy, tox[0], toy[0], wallscreen)) == 0) {
      if (suggestx >= 0) {
        tox[0] = suggestx;
        toy[0] = suggesty;
        goto findroutebk;
      }
      return 0;
    }
  }

  if (leftorright == 1) {
    if (waspossible == 0)
      return 0;

    memset(&beenhere[0][0], 0, wallscreen->w * wallscreen->h);
  }

  if (try_this_square(srcx, srcy, tox[0], toy[0]) == 0)
    return 0;

  return 1;
}

#define MAKE_INTCOORD(x,y) ((x << 16) + (y))

int find_route(int srcx, int srcy, int xx, int yy, block onscreen, int movlst, int nocross =
               0, int ignore_walls = 0)
{
#ifdef DEBUG_PATHFINDER
  __wnormscreen();
#endif
  wallscreen = onscreen;
  leftorright = 0;
  int aaa;

  beenhere[0] = (unsigned char *)malloc((wallscreen->w) * (wallscreen->h));

  for (aaa = 1; aaa < wallscreen->h; aaa++)
    beenhere[aaa] = beenhere[0] + aaa * (wallscreen->w);

  int orisrcx = srcx, orisrcy = srcy, x, y;
  finalpartx = -1;

  if (ignore_walls) {
    pathbackstage = 0;
  }
  else {
    if (__find_route(srcx, srcy, &xx, &yy, nocross) == 0) {
      leftorright = 1;
      if (__find_route(srcx, srcy, &xx, &yy, nocross) == 0)
        pathbackstage = -1;
    }
  }

  free(beenhere[0]);

  if (pathbackstage >= 0) {
    int nearestpos = 0, nearestindx;
    int reallyneed[MAXNEEDSTAGES], numstages = 0;
    reallyneed[numstages] = MAKE_INTCOORD(srcx,srcy);
    numstages++;

stage_again:
    nearestpos = 0;
    nearestindx = -1;
    // find the furthest point that can be seen from this stage
    for (aaa = pathbackstage-1; aaa >= 0; aaa--) {
//      fprintf(stderr,"stage %2d: %2d,%2d\n",aaa,pathbackx[aaa],pathbacky[aaa]);
      if (can_see_from(srcx, srcy, pathbackx[aaa], pathbacky[aaa])) {
        nearestpos = MAKE_INTCOORD(pathbackx[aaa], pathbacky[aaa]);
        nearestindx = aaa;
      }
    }

    if (nearestpos > 0) {
      reallyneed[numstages] = nearestpos;
      numstages++;
      if (numstages >= MAXNEEDSTAGES - 1)
        quit("too many stages for auto-walk");
      srcx = (nearestpos >> 16) & 0x000ffff;
      srcy = nearestpos & 0x000ffff;
//      Display("Added: %d, %d pbs:%d",srcx,srcy,pathbackstage);
      pathbackstage = nearestindx;
      goto stage_again;
    }

    if (finalpartx >= 0) {
      reallyneed[numstages] = MAKE_INTCOORD(finalpartx, finalparty);
      numstages++;
    }

    reallyneed[numstages] = MAKE_INTCOORD(xx, yy);
    numstages++;

    //Display("Route from %d,%d to %d,%d - %d stage, %d stages", orisrcx,orisrcy,xx,yy,pathbackstage,numstages);

    int mlist = movlst;
    mls[mlist].numstage = numstages;
    memcpy(&mls[mlist].pos[0], &reallyneed[0], sizeof(int) * numstages);
//    fprintf(stderr,"stages: %d\n",numstages);

    for (aaa = 0; aaa < numstages; aaa++) {
      int ourx = (mls[mlist].pos[aaa] >> 16) & 0xffff;
      int oury = mls[mlist].pos[aaa] & 0xffff;
      mls[mlist].pos[aaa] = mls[mlist].pos[aaa] & 0xffff;
      mls[mlist].pos[aaa] = mls[mlist].pos[aaa] | (ourx << 16);
    }

    for (aaa = 0; aaa < numstages-1; aaa++) {
      int ourx = (mls[mlist].pos[aaa] >> 16) & 0x000ffff;
      int destx = ((mls[mlist].pos[aaa + 1] >> 16) & 0x000ffff);
      int oury = (mls[mlist].pos[aaa] & 0x000ffff);
      int desty = (mls[mlist].pos[aaa + 1] & 0x000ffff);

      fixed xdist = itofix(abs(ourx - destx));
      fixed ydist = itofix(abs(oury - desty));
      fixed angl = fatan(fdiv(ydist, xdist));
      
      // now, since new opp=hyp*sin, work out the Y step size
      fixed newymove = move_speed * fsin(angl);
      // since adj=hyp*cos, work out X step size
      fixed newxmove = move_speed * fcos(angl);

      if (destx < ourx)
        newxmove = -newxmove;
      if (desty < oury)
        newymove = -newymove;

      mls[mlist].xpermove[aaa] = newxmove;
      mls[mlist].ypermove[aaa] = newymove;
      mls[mlist].fromx = orisrcx;
      mls[mlist].fromy = orisrcy;
    }

    mls[mlist].onstage = 0;
    mls[mlist].onpart = 0;
    mls[mlist].doneflag = 0;
    mls[mlist].lastx = -1;
    mls[mlist].lasty = -1;
#ifdef DEBUG_PATHFINDER
    getch();
#endif
    return mlist;
  } else {
    return 0;
  }

#ifdef DEBUG_PATHFINDER
  __unnormscreen();
#endif
}
