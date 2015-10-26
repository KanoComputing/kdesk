//
// sound.cpp
//
// Copyright (C) 2013-2014 Kano Computing Ltd.
// License: http://www.gnu.org/licenses/gpl-2.0.txt GNU General Public License v2
//
// An app to show and bring life to Kano-Make Desktop Icons.
//

#include <stdlib.h>
#include <pthread.h>

#include "configuration.h"
#include "logging.h"
#include "sound.h"

Sound::Sound (Configuration *loaded_conf)
{
  configuration = loaded_conf;
  playing = false;
}

Sound::~Sound (void)
{
  if (playing) {
    pthread_join(t, NULL);
  }
}

bool Sound::init(void)
{
  return true;
}

bool Sound::terminate(void)
{
  return true;
}

bool Sound::play(void)
{
  bool success=false;
  int rc;

  if (!tune || ! (tune->size() > 0)) {
    log2 ("no tune file set, or a play is in progress (tune, playing)", tune, playing);
  }
  else {
    // Call external tool with the sound file
    playing = true;

    string sound_cmdline;
    sound_cmdline  = "/usr/bin/aplay ";
    sound_cmdline += tune->c_str();
    sound_cmdline += " &";
    log1 ("Playing sound cmdline:", sound_cmdline);

    // protect against eventual race condition
    tune_tmp=tune;
    tune = NULL;
    delete (tune_tmp);

    rc = system (sound_cmdline.c_str());
    log1 ("Sound played (return code)", rc);
    success = (rc == 0 ? true : false);
    playing = false;
  }

  return success;
}

void Sound::play_sound(string sound_name)
{
  int rc;
  string sound_cmdline;

  // Do not play anything if sound is disabled in kdeskrc
  if (!(configuration->get_config_string ("enablesound") == "true")) {
      return;
  }

  if (playing == true) {
    log ("A sound is currently being played");
  }
  else {
    // sound name is the key name specified in the configuration file
    tune = new std::string (configuration->get_config_string (sound_name));
    pthread_create (&t, NULL, InternalThreadEntryFunc, this);
  }

  return;
}
