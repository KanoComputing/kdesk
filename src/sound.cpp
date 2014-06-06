//
// sound.cpp
//
// Copyright (C) 2013-2014 Kano Computing Ltd.
// License: http://www.gnu.org/licenses/gpl-2.0.txt GNU General Public License v2
//
// An app to show and bring life to Kano-Make Desktop Icons.
//

#include <ao/ao.h>
#include <mpg123.h>

#include <pthread.h>

#include "configuration.h"
#include "logging.h"
#include "sound.h"

Sound::Sound (Configuration *loaded_conf)
{
  configuration = loaded_conf;
  playing = false;
  driver = 0;
  dev = NULL;
  mh = NULL;
  set_enabled(false);
  mpg123_outblock_buffer = (unsigned char *) NULL;
  load_chimes();
}

Sound::~Sound (void)
{
  if (playing) {
    pthread_join(t, NULL);
  }
  
  if (get_enabled() == true) {
    terminate();
  }
}

bool Sound::set_enabled (bool benabled)
{
  enabled = benabled;
}

bool Sound::get_enabled (void)
{
  return enabled;
}

bool Sound::load_chimes(void)
{
  bool bsuccess=false;

  if (configuration->get_config_string ("enablesound") == "true") {
    // TODO: Preload chimes in memory for faster response times
    set_enabled(true);
    bsuccess = true;
  }

  return bsuccess;
}

bool Sound::init(void)
{
  bool bsuccess=false;
  int err=0;

  if (get_enabled() == true) {
    ao_initialize();
    driver = ao_default_driver_id();
    mpg123_init();
    mh = mpg123_new(NULL, &err);
    buffer_size = mpg123_outblock(mh);
    mpg123_outblock_buffer = (unsigned char*) calloc (buffer_size, sizeof (unsigned char));

    initialized = true;
    bsuccess = true;
    log1 ("initialized audio device (driver)", driver);

  }
  return bsuccess;
}

bool Sound::terminate(void)
{
  free (mpg123_outblock_buffer);
  ao_close (dev);
  mpg123_close (mh);
  mpg123_delete (mh);
  mpg123_exit ();
  ao_shutdown ();

  dev = NULL;
  mh = NULL;
  mpg123_outblock_buffer = (unsigned char *) NULL;
}

bool Sound::play(void)
{
  bool bsuccess = false;
  int channels, encoding;
  size_t done;
  long rate;

  if (!initialized || tune->size() == 0 || (playing == true)) {
    log3 ("error initializing sound (initialized, tune, playing)", initialized, tune, playing);
  }
  else {
    playing = true;
    mpg123_open(mh, tune->c_str());
    mpg123_getformat(mh, &rate, &channels, &encoding);

    // set the output format and open the output device
    format.bits = mpg123_encsize(encoding) * 8;
    format.channels = channels;
    format.rate = rate;
    format.byte_format = AO_FMT_NATIVE;
    format.matrix = 0;

    log4 ("decoding mp3 file (file, bits, rate, channels)", tune->c_str(), format.bits, format.rate, format.channels);

    // decode and play
    dev = ao_open_live(driver, &format, NULL);
    log2 ("sound device open (driver, device)", driver, dev);
    int rc=0;
    do {
      rc = mpg123_read(mh, mpg123_outblock_buffer, buffer_size, &done);
      log1 ("mpg123_read operation rc", rc);
      if (rc == MPG123_OK) {
	ao_play(dev, (char *) mpg123_outblock_buffer, done);
      }
    } while (rc == MPG123_OK);

    ao_close(dev);
    mpg123_close(mh);
    bsuccess = true;
    playing = false;
  }
  free (tune);

  log ("finished playing tune");
  return bsuccess;
}

void Sound::play_sound(string sound_name)
{
  int rc;
  string sound_cmdline;

  // FIXME: In order to provide for Analog / HDMI Sound output
  // we momentarily rely on the external tool kano-sound-cli
  // until a faster method is in place

  sound_cmdline  = "/usr/bin/kano-sound-cli ";
  sound_cmdline += configuration->get_config_string (sound_name);
  sound_cmdline += " &";
  log1 ("Playing sound cmdline:", sound_cmdline);
  rc = system (sound_cmdline.c_str());

  // sound name is the key name specified in the configuration file
  /*
  if (playing == false && get_enabled() == true) {
    tune = new std::string (configuration->get_config_string (sound_name));
    pthread_create(&t, NULL, InternalThreadEntryFunc, this);
  }
  */
}
