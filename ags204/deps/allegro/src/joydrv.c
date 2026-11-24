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
 *      List of available joystick drivers, kept in a seperate file so that
 *      they can be overriden by user programs.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"



BEGIN_JOYSTICK_DRIVER_LIST
   JOYSTICK_DRIVER_SIDEWINDER
   JOYSTICK_DRIVER_GAMEPAD_PRO
   JOYSTICK_DRIVER_STANDARD
   JOYSTICK_DRIVER_SNESPAD
END_JOYSTICK_DRIVER_LIST
