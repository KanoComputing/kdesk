//
// config.cpp
//
// Copyright (C) 2013-2014 Kano Computing Ltd.
// License: http://www.gnu.org/licenses/gpl-2.0.txt GNU General Public License v2
//
// An app to show and bring life to Kano-Make Desktop Icons.
//

#include <dirent.h>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "main.h"
#include "configuration.h"
#include "logging.h"

// Name of the reserved icon filename which will always
// be positioned at the last cell of the grid
char *kdesk_plus_icon_filename=NULL;


Configuration::Configuration()
{
  numicons=0;
}

Configuration::~Configuration()
{
}

bool Configuration::load_conf(const char *filename)
{
  string token, value;
  struct stat file_status;

  stat (filename, &file_status);
  if (!S_ISREG(file_status.st_mode)) {
      return false;
    }

  ifile.open (filename, std::ifstream::in);
  if (ifile.fail()) {
    return false;
  }

  // separate parameter - value tokens
  // in the form "Parameter: Value", one per line
  while (ifile >> token)
    {
      if (token == "FontName:") {
	ifile >> value;
	configuration["fontname"] = value;
      }
      
      if (token == "FontSize:") {
	ifile >> value;
	configuration["fontsize"] = value;
      }

      if (token == "SubtitleFontSize:") {
	ifile >> value;
	configuration["subtitlefontsize"] = value;
      }
      
      if (token == "Bold:") {
	ifile >> value;
	configuration["bold"] = value;
      }

      if (token == "Shadow:") {
	ifile >> value;
	configuration["shadow"] = value;
      }

      if (token == "ShadowX:") {
	ifile >> value;
	configuration["shadowx"] = value;
      }
      
      if (token == "ShadowY:") {
	ifile >> value;
	configuration["shadowy"] = value;
      }

      if (token == "Background.File-4-3:") {
          value = get_spaced_value();
          configuration["background.file-4-3"] = value;
      }

      if (token == "Background.File-16-9:") {
          value = get_spaced_value();
          configuration["background.file-16-9"] = value;
      }

      if (token == "ClickDelay:") {
	ifile >> value;
	configuration["clickdelay"] = value;
      }

      if (token == "IconStartDelay:") {
	ifile >> value;
	configuration["iconstartdelay"] = value;
      }

      if (token == "IconTitleGap:") {
	ifile >> value;
	configuration["icontitlegap"] = value;
      }

      if (token == "IconGapHorz:") {
	ifile >> value;
	configuration["icongaphorz"] = value;
      }

      if (token == "IconGapVert:") {
	ifile >> value;
	configuration["icongapvert"] = value;
      }

      if (token == "Transparency:") {
	ifile >> value;
	configuration["transparency"] = value;
      }

      if (token == "EnableSound:") {
	ifile >> value;
	configuration["enablesound"] = value;
      }

      if (token == "SoundWelcome:") {
	ifile >> value;
	configuration["soundwelcome"] = value;
      }

      if (token == "SoundLaunchApp:") {
	ifile >> value;
	configuration["soundlaunchapp"] = value;
      }

      if (token == "SoundDisabledIcon:") {
	ifile >> value;
	configuration["sounddisabledicon"] = value;
      }

      if (token == "Background.Delay:") {
	ifile >> value;
	configuration["background.delay"] = value;
      }

      if (token == "ScreenSaverTimeout:") {
	ifile >> value;
	configuration["screensavertimeout"] = value;
      }

      if (token == "ScreenSaverProgram:") {
	ifile >> value;
	configuration["screensaverprogram"] = value;
      }

      if (token == "OneClick:") {
	ifile >> value;
	configuration["oneclick"] = value;
      }

      if (token == "ScreenMedResWidth:") {
	ifile >> value;
	configuration["screenmedreswidth"] = value;
      }

      if (token == "Background.File-medium:") {
          value = get_spaced_value();
          configuration["background.file-medium"] = value;
      }

      if (token == "MouseHoverIcon:") {
	ifile >> value;
	configuration["mousehovericon"] = value;
      }

      if (token == "IconHook:") {
	ifile >> value;
	configuration["iconhook"] = value;
      }

      if (token == "GridWidth:") {
	ifile >> value;
	configuration["gridwidth"] = value;
      }

      if (token == "GridHeight:") {
	ifile >> value;
	configuration["gridheight"] = value;
      }

      if (token == "GridIconWidth:") {
	ifile >> value;
	configuration["gridiconwidth"] = value;
      }

      if (token == "GridIconHeight:") {
	ifile >> value;
	configuration["gridiconheight"] = value;
      }

      if (token == "MaximizeSingleton:") {
	ifile >> value;
	configuration["maximizesingleton"] = value;
      }

      if (token == "ImageCacheSize:") {
	ifile >> value;
	configuration["imagecachesize"] = value;
      }

      if (token == "LastGridIcon:") {
	ifile >> value;
	configuration["lastgridicon"] = value;

        // This icon filename will be used to make sure
        // it is the last icon to position in the grid
        kdesk_plus_icon_filename = (char *) configuration["lastgridicon"].c_str();
      }

    }

  ifile.close();
  return true;
}

bool Configuration::parse_icon (const char *directory, string fname, int iconid)
{
  bool bsuccess=false;
  string lnk_extension = ".lnk";
  string fpath;

  // Only read kano-desktop LNK files.
  if (! (std::equal(lnk_extension.rbegin(), lnk_extension.rend(), fname.rbegin()))) {
    return false;
  }

  // open the icon file and parse arguments
  fpath = directory;
  fpath += "/";
  fpath += fname.c_str();
  ifile.open (fpath.c_str(), std::ifstream::in);
  if (!ifile.is_open()) {
    log1 ("could not open icon file", fpath);
  }
  else {
    log1 ("parsing icon file", fpath);
	
    // Process line-by-line tokens
    // In the form "Parameter: some values"
    icons[iconid]["filename"] = fname;
    std::string line;
    while (std::getline(ifile, line))
      {
	std::istringstream iss(line);
	std::string temp, value, token;
	    
	// Collect the key name aka "token"
	iss >> token;
	
	// Then collect the token's value, up to EOL
	while (!iss.eof()) {
	  iss >> temp;
	  value += temp;
	  if (!iss.eof()) value += " ";
	}
	
	if (token == "AppID:") {
	  icons[iconid]["appid"] = value;
	}
	
	if (token == "Command:") {
	  icons[iconid]["command"] = value;
	}
	
	if (token == "Icon:") {
	  icons[iconid]["icon"] = value;
	}

	if (token == "IconHover:") {
	  icons[iconid]["iconhover"] = value;
	}

	if (token == "IconStamp:") {
	  icons[iconid]["iconstamp"] = value;
	}
	
	if (token == "HoverTransparent:") {
	  icons[iconid]["hovertransparent"] = value;
	}

	if (token == "HoverXOffset:") {
	  icons[iconid]["hoverxoffset"] = value;
	}

	if (token == "HoverYOffset:") {
	  icons[iconid]["hoveryoffset"] = value;
	}

	if (token == "Caption:") {
	  
	  // caption supports environment variable expansion
	  if (value[0] == '$') {
	    value = getenv (&value[1]);
	  }
	  
	  icons[iconid]["caption"] = value;
	}

	if (token == "Message:") {
	  icons[iconid]["message"] = value;
	}

	if (token == "HAlign:") {
	  icons[iconid]["halign"] = value;
	}
	
	if (token == "X:") {
	  icons[iconid]["x"] = value;
	}
	
	if (token == "Y:") {
	  icons[iconid]["y"] = value;
	}
	
	if (token == "Width:") {
	  icons[iconid]["width"] = value;
	}
	
	if (token == "Height:") {
	  icons[iconid]["height"] = value;
	}
	
	if (token == "Singleton:") {
	  icons[iconid]["singleton"] = value;
	}
	
	if (token == "Relative-To:") {
	  icons[iconid]["relative-to"] = value;
	}
      }

    ifile.close();
    bsuccess = true;
  }

  return bsuccess;
}

bool Configuration::load_icons(const char *directory)
{
  struct dirent **files;
  int numfiles, count;
  string last_grid_icon_file;
  char last_grid_icon_dir[256]={0};

  // Read kano-desktop distributed icons first
  log1 ("Loading icons from directory", directory);
  numfiles = scandir (directory, &files, 0, 0);
  for (count=0; count < numfiles && numicons <= MAX_ICONS; count++)
    {
      string f = files[count]->d_name;
      if (kdesk_plus_icon_filename && !strcmp(f.c_str(), kdesk_plus_icon_filename)) {
          last_grid_icon_file=f;
          strcpy(last_grid_icon_dir, directory);
          continue;
      }

      if (parse_icon (directory, f, numicons) == true) {
	numicons++;
      }
    }

  // Read icons located at the user's home directory
  char *env_display = getenv ("HOME");
  string kdesk_homedir = env_display + string("/");
  kdesk_homedir += DIR_KDESKTOP_USER;
  log1 ("Loading icons from homedir", kdesk_homedir);
  numfiles = scandir (kdesk_homedir.c_str(), &files, 0, 0);
  for (count=0; count < numfiles && numicons <= MAX_ICONS; count++)
    {
      string f = files[count]->d_name;
      if (kdesk_plus_icon_filename && !strcmp(f.c_str(), kdesk_plus_icon_filename)) {
          last_grid_icon_file=f;
          strcpy(last_grid_icon_dir, kdesk_homedir.c_str());
          continue;
      }

      if (parse_icon (kdesk_homedir.c_str(), f, numicons) == true) {
	numicons++;
      }
    }

  if (last_grid_icon_file.length()) {
      log1 ("Adding a last_grid_icon: ", last_grid_icon_file);

      // add the last grid icon at the end of the list
      if (parse_icon (last_grid_icon_dir, last_grid_icon_file, numicons) == true) {
          numicons++;
      }      
  }

  log2 ("Number of icons loaded, max permitted", numicons, MAX_ICONS);
  return (bool) (count > 0);
}

string Configuration::get_config_string(string item)
{
  string value = configuration[item];
  return value;
}

unsigned int Configuration::get_config_int(string item)
{
  string value = configuration[item];
  return atoi(value.c_str());
}

std::string Configuration::get_icon_string(int iconid, std::string key)
{
  string value = icons[iconid][key];
  return value;
}

int Configuration::get_icon_int(int iconid, std::string key)
{
  string value = icons[iconid][key];
  return atoi(value.c_str());
}

int Configuration::get_numicons(void)
{
  return numicons;
}

std::string Configuration::get_spaced_value(void)
{
    std::string value;

    // get the value until end of line
    std::getline (ifile, value);

    // trim leading and trailing spaces
    value.erase (0, value.find_first_not_of (' '));
    value.erase (value.find_last_not_of (' ') + 1);

    return value;
}

void Configuration::dump()
{
  log ("dumping MAP configuration values");
  std::map<string,string>::iterator it;
  for (it=configuration.begin(); it != configuration.end(); ++it)
    {
      log2 ("configuration item", it->first, it->second);
    }

  log1 ("dumping all loaded icons:", numicons);
  for (int c=0; c < numicons; c++) {
    log1 ("dumping icon file:", icons[c]["filename"]);
    for (it=icons[c].begin(); it != icons[c].end(); ++it)
      {
	log2 ("icon key", it->first, it->second);
      }
  }
}

void Configuration::reset(void)
{
  std::map<string,string>::iterator it;
  for (it=configuration.begin(); it != configuration.end(); ++it)
    {
      configuration.erase(it);
    }

  configuration.clear();
  reset_icons();
}

void Configuration::reset_icons(void)
{
  std::map<string,string>::iterator it;
  for (int c=0; c < numicons; c++) {
    for (it=icons[c].begin(); it != icons[c].end(); ++it)
      {
	icons[c].erase (it);
      }
  }

  icons.clear();
  numicons = 0;
}

