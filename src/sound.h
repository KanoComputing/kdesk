//
// sound.h
//
// Copyright (C) 2013-2014 Kano Computing Ltd.
// License: http://www.gnu.org/licenses/gpl-2.0.txt GNU General Public License v2
//
// An app to show and bring life to Kano-Make Desktop Icons.
//

#include <string>

#include <ao/ao.h>
#include <mpg123.h>

class Sound
{
 private:
  Configuration *configuration;
  int num_bits;
  int initialized;
  int driver;
  ao_sample_format format;
  ao_device *dev;
  mpg123_handle *mh;
  size_t buffer_size;
  unsigned char *mpg123_outblock_buffer;
  std::string *tune;
  pthread_t t;
  bool playing;

 public:
  int enabled;

  Sound (Configuration *loaded_conf);
  virtual ~Sound (void);

  bool load_chimes(void);
  bool init(void);

  static void * InternalThreadEntryFunc(void * This)
  {
    ((Sound *)This)->play(); return NULL;
  }

  bool play(void);
  void play_sound(std::string sound_name);
  bool terminate(void);
  bool set_enabled (bool benabled);
  bool get_enabled (void);

};
