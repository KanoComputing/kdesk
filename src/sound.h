//
// sound.h
//
// Copyright (C) 2013-2014 Kano Computing Ltd.
// License: http://www.gnu.org/licenses/gpl-2.0.txt GNU General Public License v2
//
// An app to show and bring life to Kano-Make Desktop Icons.
//

#include <string>

class Sound
{
 private:
  Configuration *configuration;
  std::string * volatile tune;
  std::string *tune_tmp;
  pthread_t t;
  bool playing;

 public:
  Sound (Configuration *loaded_conf);
  virtual ~Sound (void);

  bool init(void);

  static void * InternalThreadEntryFunc(void * This)
  {
    ((Sound *)This)->play(); return NULL;
  }

  bool play(void);
  void play_sound(std::string sound_name);
  bool terminate(void);
};
