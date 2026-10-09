/*         ______   ___    ___ 
 *        /\  _  \ /\_ \  /\_ \ 
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Linux joystick driver.
 *
 *      By George Foot.
 *
 *      See readme.txt for copyright information.
 */

#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>


#include "allegro.h"
#include "allegro/aintunix.h"


#ifdef HAVE_LINUX_JOYSTICK_H

#include <linux/joystick.h>


static int joy_fd = -1;
static int num_buttons, num_axes;
static char description[100];

static JOYSTICK_AXIS_INFO *axis[8];
static JOYSTICK_BUTTON_INFO *button[8];


static int joy_init (void)
{
	char tmp[80];
	int version;
	int i;

	joy_fd = open ("/dev/js0", O_RDONLY);
	if (joy_fd == -1) {
		usprintf (allegro_error, get_config_text ("Unable to open %s: %s"), uconvert_ascii ("/dev/js0", tmp), ustrerror (errno));
		return -1;
	}

	ioctl (joy_fd, JSIOCGVERSION, &version);
	/* TODO: Check version? */

	ioctl (joy_fd, JSIOCGAXES, &num_axes);
	ioctl (joy_fd, JSIOCGBUTTONS, &num_buttons);
	ioctl (joy_fd, JSIOCGNAME(sizeof description), description);

	/* Now assume that each pair of axes is a separate stick.
	 * If we have an odd number of axes, the last is the throttle
	 * for the last stick. */

	/* We only support up to eight axes and eight buttons */
	if (num_axes > 8) num_axes = 8;
	if (num_buttons > 8) num_buttons = 8;

	/* fill in the joystick structure */
	num_joysticks = num_axes / 2;

	for (i = 0; i < num_joysticks; i++) {
		joy[i].flags = JOYFLAG_ANALOGUE;
		joy[i].stick[0].flags = JOYFLAG_ANALOGUE | JOYFLAG_SIGNED;
		joy[i].stick[0].num_axis = 2;
		joy[i].stick[0].axis[0].name = get_config_text("X");
		joy[i].stick[0].axis[1].name = get_config_text("Y");
		joy[i].stick[0].name = malloc (32);
		usprintf (joy[i].stick[0].name, get_config_text("Stick %d"), i+1);
		joy[i].num_sticks = 1;
		axis[i*2] = &joy[i].stick[0].axis[0];
		axis[i*2+1] = &joy[i].stick[0].axis[1];
	}

	if (num_axes & 1) {
		int s = joy[i-1].num_sticks++;
		joy[i-1].stick[s].flags = JOYFLAG_ANALOGUE | JOYFLAG_UNSIGNED;
		joy[i-1].stick[s].num_axis = 1;
		joy[i-1].stick[s].axis[0].name = get_config_text("Throttle");
		joy[i-1].stick[s].name = joy[i-1].stick[s].axis[0].name;
		axis[num_axes-1] = &joy[i-1].stick[s].axis[0];
	}

	/* Now share out the buttons.  It's not possible to decide 
	 * which belong on which joystick, so I assume that if there
	 * are multiple sticks each has just two buttons. */
	if (num_joysticks > 1) {
		int j,k;
		j = 0;
		for (i = 0; i < num_joysticks; i++) {
			for (k = 0; k < 2; k++)
				if (j < num_buttons) {
					joy[i].button[k].name = malloc (16);
					usprintf (joy[i].button[k].name, uconvert_ascii("%c", tmp), 'A' + k);
					button[j] = &joy[i].button[k];
					j++;
				}
			joy[i].num_buttons = k;
		}
	} else {
		for (i = 0; i < num_buttons; i++) {
			joy[0].button[i].name = malloc (16);
			usprintf (joy[0].button[i].name, uconvert_ascii("%c", tmp), 'A' + i);
			button[i] = &joy[0].button[i];
		}
		joy[0].num_buttons = num_buttons;
	}

	return 0;
}


static void joy_exit (void)
{
	close (joy_fd);
}


static void set_axis (JOYSTICK_AXIS_INFO *axis, int value)
{
	axis->pos = value * 127 / 32767;
	axis->d1 = (value < 0);
	axis->d2 = (value > 0);
}

static int joy_poll (void)
{
	fd_set set;
	struct timeval tv;
	struct js_event e;
	int ready;

	while (1) {
		tv.tv_sec = tv.tv_usec = 0;
		FD_ZERO (&set);
		FD_SET (joy_fd, &set);
		ready = select (FD_SETSIZE, &set, NULL, NULL, &tv);
		if (ready <= 0) break;
		read (joy_fd, &e, sizeof e);
		if (e.type & JS_EVENT_BUTTON) {
			if (e.number < 8)
				button[e.number]->b = e.value;
		} else if (e.type & JS_EVENT_AXIS) {
			if (e.number < 8)
				set_axis (axis[e.number], e.value);
		}
	}

	return 0;
}


static int joy_save (void)
{
	return 0;
}

static int joy_load (void)
{
	return 0;
}


static char *joy_calib_name (int n)
{
	return NULL;
}

static int joy_calib (int n)
{
	return -1;
}


JOYSTICK_DRIVER joystick_linux_analogue = {
	JOY_TYPE_LINUX_ANALOGUE,
	empty_string,
	empty_string,
	"Linux analogue joystick(s)",
	joy_init,
	joy_exit,
	joy_poll,
	joy_save,
	joy_load,
	joy_calib_name,
	joy_calib
};

#endif

/* list the available drivers */
_DRIVER_INFO _linux_joystick_driver_list[] = {
#ifdef HAVE_LINUX_JOYSTICK_H
	{    JOY_TYPE_LINUX_ANALOGUE,  &joystick_linux_analogue,  TRUE   },
#endif
	{    JOY_TYPE_NONE,            &joystick_none,            TRUE   },
	{    0,                        0,                         0      }
};

