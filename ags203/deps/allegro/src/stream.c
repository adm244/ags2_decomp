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
 *      Audio stream functions (original version by Andrew Ellem).
 *
 *      See readme.txt for copyright information.
 */


#include <stdlib.h>

#include "internal.h"



/* play_audio_stream:
 *  Creates a new audio stream and starts it playing. The length is the
 *  size of each transfer buffer, which should be at least 2k.
 */
AUDIOSTREAM *play_audio_stream(int len, int bits, int stereo, int freq, int vol, int pan)
{
   AUDIOSTREAM *stream;
   int i;

   stream = malloc(sizeof(AUDIOSTREAM));
   if (!stream)
      return NULL;

   stream->len = len;

   stream->samp = create_sample(bits, stereo, freq, len*2);
   if (!stream->samp) {
      free(stream);
      return NULL;
   }

   stream->b1 = stream->samp->data;
   stream->b2 = stream->samp->data + len * ((bits==8) ? 1 : sizeof(short)) * ((stereo) ? 2 : 1);
   stream->bufnum = 0;

   if (bits == 16) {
      unsigned short *p = stream->samp->data;
      for (i=0; i < len*2 * ((stereo) ? 2 : 1); i++)
	 p[i] = 0x8000;
   }
   else {
      unsigned char *p = stream->samp->data;
      for (i=0; i < len*2 * ((stereo) ? 2 : 1); i++)
	 p[i] = 0x80;
   }

   LOCK_DATA(stream, sizeof(AUDIOSTREAM));

   stream->voice = allocate_voice(stream->samp);
   if (stream->voice < 0) {
      destroy_sample(stream->samp);
      UNLOCK_DATA(stream, sizeof(AUDIOSTREAM));
      free(stream);
      return NULL;
   }

   voice_set_playmode(stream->voice, PLAYMODE_LOOP);
   voice_set_volume(stream->voice, vol);
   voice_set_pan(stream->voice, pan);
   voice_start(stream->voice);

   return stream;
}



/* stop_audio_stream:
 *  Destroys an audio stream when it is no longer required.
 */
void stop_audio_stream(AUDIOSTREAM *stream)
{
   voice_stop(stream->voice);
   deallocate_voice(stream->voice);

   destroy_sample(stream->samp);

   UNLOCK_DATA(stream, sizeof(AUDIOSTREAM));
   free(stream); 
}



/* get_audio_stream_buffer:
 *  Returns a pointer to the next audio buffer, or NULL if the previous 
 *  data is still playing. This must be called at regular intervals while
 *  the stream is playing, and you must fill the return address with the
 *  appropriate number (the same length that you specified when you create
 *  the stream) of samples. Call free_audio_stream_buffer() after loading
 *  the new samples, to indicate that the data is now valid.
 */
void *get_audio_stream_buffer(AUDIOSTREAM *stream)
{
   int pos = voice_get_position(stream->voice);

   if (stream->bufnum == 0) {
      if (pos >= stream->len)
	 return stream->b1;
   }
   else {
      if (pos < stream->len)
	 return stream->b2;
   }

   return NULL;
}



/* free_audio_stream_buffer:
 *  Indicates that a sample buffer previously returned by a call to
 *  get_audio_stream_buffer() has now been filled with valid data.
 */
void free_audio_stream_buffer(AUDIOSTREAM *stream)
{
   stream->bufnum = 1-stream->bufnum;
}


