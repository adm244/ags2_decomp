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
 *      Open Sound System driver.  Supports for /dev/dsp and /dev/audio.
 *
 *      By Joshua Heyer.
 *
 *      Modified by Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"

#ifdef DIGI_OSS

#include "allegro/aintern.h"
#include "allegro/aintunix.h"

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#if defined(HAVE_SOUNDCARD_H)
   #include <soundcard.h>
#elif defined(HAVE_SYS_SOUNDCARD_H)
   #include <sys/soundcard.h>
#elif defined(HAVE_MACHINE_SOUNDCARD_H)
   #include <machine/soundcard.h>
#elif defined(HAVE_LINUX_SOUNDCARD_H)
   #include <linux/soundcard.h>
#endif
#include <sys/ioctl.h>


#ifndef AFMT_S16_NE
   #ifdef ALLEGRO_BIG_ENDIAN
      #define AFMT_S16_NE AFMT_S16_BE
   #else
      #define AFMT_S16_NE AFMT_S16_LE
   #endif
#endif
#ifndef AFMT_U16_NE
   #ifdef ALLEGRO_BIG_ENDIAN
      #define AFMT_U16_NE AFMT_U16_BE
   #else
      #define AFMT_U16_NE AFMT_U16_LE
   #endif
#endif


#define OSS_DEFAULT_FRAGBITS 12
#define OSS_DEFAULT_NUMFRAGS 2

static int oss_fd;
static int oss_bufsize;
static unsigned char *oss_bufdata;
static int oss_bits, oss_signed, oss_rate, oss_stereo, oss_format;

static int oss_detect(int input);
static int oss_init(int input, int voices);
static void oss_exit(int input);
static int oss_mixer_volume(int volume);

static char oss_desc[320] = EMPTY_STRING;

DIGI_DRIVER digi_oss =
{
   DIGI_OSS,
   empty_string,
   empty_string,
   "Open Sound System",
   0,
   0,
   MIXER_MAX_SFX,
   MIXER_DEF_SFX,

   oss_detect,
   oss_init,
   oss_exit,
   oss_mixer_volume,

   NULL,
   NULL,
   _mixer_init_voice,
   _mixer_release_voice,
   _mixer_start_voice,
   _mixer_stop_voice,
   _mixer_loop_voice,

   _mixer_get_position,
   _mixer_set_position,

   _mixer_get_volume,
   _mixer_set_volume,
   _mixer_ramp_volume,
   _mixer_stop_volume_ramp,

   _mixer_get_frequency,
   _mixer_set_frequency,
   _mixer_sweep_frequency,
   _mixer_stop_frequency_sweep,

   _mixer_get_pan,
   _mixer_set_pan,
   _mixer_sweep_pan,
   _mixer_stop_pan_sweep,

   _mixer_set_echo,
   _mixer_set_tremolo,
   _mixer_set_vibrato,
   0, 0,
   0,
   0,
   0,
   0,
   0,
   0
};



/* oss_update:
 *  Update data.
 */
static void oss_update(unsigned long interval)
{
   int i;
   audio_buf_info bufinfo;

   DISABLE();

   if (ioctl(oss_fd, SNDCTL_DSP_GETOSPACE, &bufinfo) != -1) {
      /* Write fragments.  */
      for (i = 0; i < bufinfo.fragments; i++) {
	 write(oss_fd, oss_bufdata, oss_bufsize);
	 _mix_some_samples((unsigned long) oss_bufdata, 0, oss_signed);
      }
   }

   ENABLE();
}



/* oss_detect:
 *  Detect driver presence.
 */
static int oss_detect(int input)
{
   int fd;
   char *filename;
   char tmp1[80], tmp2[80], tmp3[80];
   char s[256];

   if (input) {
      usprintf(allegro_error, get_config_text("Input is not supported"));
      return FALSE;
   }

   /* Get OSS driver name.  */
   filename = get_config_string(uconvert_ascii("sound", tmp1),
				uconvert_ascii("ossdigi_driver", tmp2),
				uconvert_ascii("/dev/dsp", tmp3));

   /* Try to open OSS driver for output.  */
   fd = open(uconvert_toascii(filename, s), O_WRONLY);
   if (fd < 0) {
      usprintf(allegro_error, get_config_text("%s: %s"),
	       filename, ustrerror(errno));
      return FALSE;
   }

   close(fd);
   return TRUE;
}



/* oss_init:
 *  OSS init routine.
 */
static int oss_init(int input, int voices)
{
   char *filename;
   char tmp1[80], tmp2[80], tmp3[80];
   char s[256];
   int fragsize, fragbits, numfrags;
   audio_buf_info bufinfo;

   if (input) {
      usprintf(allegro_error, get_config_text("Input is not supported"));
      return -1;
   }

   filename = get_config_string(uconvert_ascii("sound", tmp1),
				uconvert_ascii("ossdigi_driver", tmp2),
				uconvert_ascii("/dev/dsp", tmp3));

   oss_fd = open(uconvert_toascii(filename, s), O_WRONLY);
   if (oss_fd < 0) {
      usprintf(allegro_error, get_config_text("%s: %s"),
	       filename, ustrerror(errno));
      return -1;
   }

   fragsize = get_config_int(uconvert_ascii("sound", tmp1),
			     uconvert_ascii("ossdigi_fragsize", tmp2),
			     1 << OSS_DEFAULT_FRAGBITS);
   numfrags = get_config_int(uconvert_ascii("sound", tmp1),
			     uconvert_ascii("ossdigi_numfrags", tmp2),
			     OSS_DEFAULT_NUMFRAGS);
   oss_bits = get_config_int(uconvert_ascii("sound", tmp1),
			     uconvert_ascii("ossdigi_bits", tmp2),
			     16);
   oss_stereo = get_config_int(uconvert_ascii("sound", tmp1),
			       uconvert_ascii("ossdigi_stereo", tmp2),
			       1);
   oss_rate = get_config_int(uconvert_ascii("sound", tmp1),
			     uconvert_ascii("ossdigi_rate", tmp2),
			     45454);

   /* Fragment size is specified in samples, not in bytes.  */
   fragsize *= ((oss_bits == 16) ? 2 : 1) * (oss_stereo ? 2 : 1);
   fragsize += fragsize - 1;
   for (fragbits = 0; (fragbits < 16) && (fragsize > 1); fragbits++)
      fragsize /= 2;

   fragbits = MID(4, fragbits, 16);
   numfrags = MID(2, numfrags, 0x7FFF);

   fragsize = (numfrags << 16) | fragbits;
   if (ioctl(oss_fd, SNDCTL_DSP_SETFRAGMENT, &fragsize) == -1) {
      usprintf(allegro_error, get_config_text("Setting fragment size: %s"),
	       ustrerror(errno));
      close(oss_fd);
      return -1;
   }

   oss_stereo = (oss_stereo ? 1 : 0);
   oss_format = ((oss_bits == 16) ? AFMT_S16_NE : AFMT_U8);

   if ((ioctl(oss_fd, SNDCTL_DSP_SETFMT, &oss_format) == -1)
       || (ioctl(oss_fd, SNDCTL_DSP_STEREO, &oss_stereo) == -1)
       || (ioctl(oss_fd, SNDCTL_DSP_SPEED, &oss_rate) == -1)) {
      usprintf(allegro_error, get_config_text("Setting DSP parameters: %s"),
	       ustrerror(errno));
      close(oss_fd);
      return -1;
   }

   oss_signed = 0;
   switch(oss_format) {
      case AFMT_S8:
	 oss_signed = 1;
      case AFMT_U8:
	 oss_bits = 8;
	 break;
      case AFMT_S16_NE:
	 oss_signed = 1;
      case AFMT_U16_NE:
	 oss_bits = 16;
	 if (sizeof(short) != 2) {
	    usprintf(allegro_error, get_config_text("Unsupported sample format"));
	    close(oss_fd);
	    return -1;
	 }
	 break;
      default:
	 usprintf(allegro_error, get_config_text("Unsupported sample format"));
	 close(oss_fd);
	 return -1;
   }

   if ((oss_stereo != 0) && (oss_stereo != 1)) {
      usprintf(allegro_error, get_config_text("Not in stereo or mono mode"));
      close(oss_fd);
      return -1;
   }

   if (ioctl(oss_fd, SNDCTL_DSP_GETOSPACE, &bufinfo) == -1) {
      usprintf(allegro_error, get_config_text("Getting buffer size: %s"),
	       ustrerror(errno));
      close(oss_fd);
      return -1;
   }

   oss_bufsize = bufinfo.fragsize;
   oss_bufdata = malloc(oss_bufsize);
   if (oss_bufdata == 0) {
      usprintf(allegro_error, get_config_text("Can not allocate audio buffer"));
      close(oss_fd);
      return -1;
   }

   digi_oss.voices = voices;

   if (_mixer_init(oss_bufsize / (oss_bits / 8), oss_rate,
		   oss_stereo, ((oss_bits == 16) ? 1 : 0),
		   &digi_oss.voices) != 0) {
      usprintf(allegro_error, get_config_text("Can not init software mixer"));
      close(oss_fd);
      return -1;
   }

   _mix_some_samples((unsigned long) oss_bufdata, 0, oss_signed);

   /* Add audio interrupt.  */
   DISABLE();
   _sigalrm_digi_interrupt_handler = oss_update;
   ENABLE();

   usprintf(oss_desc, get_config_text("%s: %d bits, %ssigned, %d bps, %s"),
	    filename, oss_bits,
	    uconvert_ascii((oss_signed ? "" : "un"), tmp1), oss_rate,
	    uconvert_ascii((oss_stereo ? "stereo" : "mono"), tmp2));
   digi_driver->desc = oss_desc;

   return 0;
}



/* oss_exit:
 *  Shutdown OSS driver.
 */
static void oss_exit(int input)
{
   if (input) {
      return;
   }

   DISABLE();
   _sigalrm_digi_interrupt_handler = 0;
   ENABLE();

   free(oss_bufdata);
   oss_bufdata = 0;

   _mixer_exit();

   close(oss_fd);
}



/* oss_mixer_volume:
 *  Set mixer volume.
 */
static int oss_mixer_volume(int volume)
{
   return 0;
}

#endif

