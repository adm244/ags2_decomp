/* LIBAMP.CPP

  Win32 compatability layer
*/

extern "C" {

int install_amp(void)
{
  return 0;
}

int load_amp(char*filename,int loop)
{
  return 0;
}

#ifdef AMP_POLL
int poll_amp(void)
{
  return -1;
}

int run_amp(void)
{
  return -1;
}

#else
int amp_decode(void)
{
  return -1;
}

void amp_interrupt(void)
{
}

#endif
void amp_pause(void)
{
}

void amp_setvolume(int vol)
{
}

void amp_resume(void)
{
}

int replay_amp(void)
{
  return -1;
}

int seek_amp_abs(int frame)
{
  return -1;
}

int seek_amp_rel(int framecnt)
{
  return -1;
}

// NOTE: should return 'int', but then it won't match
void unload_amp(void)
{
}

int amp_downmix(int mix)
{
  return 0;
}

}
