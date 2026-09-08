/*         ______   ___    ___ 
 *        /\  _  \ /\_ \  /\_ \ 
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *      By Shawn Hargreaves
 *      shawn@talula.demon.co.uk
 *      http://www.talula.demon.co.uk/allegro/
 *
 *      List of available sound drivers, kept in a seperate file so that
 *      they can be overriden by user programs.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"



BEGIN_DIGI_DRIVER_LIST
   DIGI_DRIVER_GUS
   DIGI_DRIVER_AUDIODRIVE
   DIGI_DRIVER_SB
END_DIGI_DRIVER_LIST


BEGIN_MIDI_DRIVER_LIST
   MIDI_DRIVER_AWE32
   MIDI_DRIVER_DIGMID
   MIDI_DRIVER_ADLIB
   MIDI_DRIVER_MPU
   MIDI_DRIVER_SB_OUT
END_MIDI_DRIVER_LIST

