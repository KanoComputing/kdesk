## Kano Desktop manager - kdesk


Kano Desktop Manager, aka *kdesk*, is a lightweight desktop manager optimized for Kano OS and other Debian-based systems, including the desktop background image, icons, and egl screensaver. 

We’ve created the utility from scratch, with inspiration taken from other desktop managers, especially [idesk](https://github.com/neagix/idesk) and [vdesk](http://xvnkb.sourceforge.net/?menu=vdesk&lang=en).

Features: 
* optimized connection with the notify library for Openbox window manager and others, so the hourglass start-up feature works seamlessly
* 100% customizable background, icons, screensaver
* efficient screensaver with low CPU usage

Check all the detailed information in the [wiki](https://github.com/KanoComputing/kdesk/wiki)  


[Introduction](https://github.com/KanoComputing/kdesk/wiki/Introduction)  
[Development](https://github.com/KanoComputing/kdesk/wiki/Development)  
[Collaboration](https://github.com/KanoComputing/kdesk/wiki/Collaboration)  


###Localization

kdesk supports localization of two icon file types: `Icon` and `IconHover`.

In your `lnk` icon files, simply set a path to the default international version of the asset,
and kdesk will try to find the localized version of it on the fly, the lnk file will not be modified.
For example, provided your LANG is set to `es_AR.UTF-8`:

```
Icon: /usr/share/my-app/icons/app.png
IconHover: /usr/share/my-app/hovers/app-hover.png
```

Would be translated by kdesk to:

```
Icon: /usr/share/my-app/icons/i18n/es_AR/app.png
IconHover: /usr/share/my-app/hovers/i18n/es_AR/app-hover.png
```

The `es_AR` will be obtained by querying the `LANG` environment variable, you can easily force it to test new locales.
Testing the configuration on the debug version will tell you which icons are localized or missing:

```
LANG=zh_TW.Big5 ./kdesk-dbg -t | grep i18n0
```
