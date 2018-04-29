[Helia ( Gtv-Dvb )](https://github.com/vl-nix/helia)
----------------------------

* Digital TV
  * DVB-T2/S2/C, ATSC, DTMB
* Media Player
  * IPTV


Requirements
------------

* Graphical user interface - [Gtk+3](https://developer.gnome.org/gtk3)
* Audio & Video & Digital TV - [Gstreamer 1.0](https://gstreamer.freedesktop.org)
* [GNU Lesser General Public License](http://www.gnu.org/licenses/lgpl.html)


Digital TV
----------
![alt text](https://static.wixstatic.com/media/650ea5_f6aa02cf376f40588bcca961f6c3ec45~mv2.png)

Media Player
------------
![alt text](https://static.wixstatic.com/media/650ea5_ecca24dc614349d18c89f0c3a0e72c01~mv2.png)


Drag and Drop
-------------
* folders
* files
* playlists - [M3U, M3U8](https://ru.wikipedia.org/wiki/M3U)


Channels
--------

* scan channels manually
* scan [initial file](https://www.linuxtv.org/downloads/dtv-scan-tables)
* convert - dvb_channel.conf ( [dvbv5-scan {OPTION...} --output-format=DVBV5 initial file](https://www.linuxtv.org/downloads/v4l-utils) )


Design
------

* [Dark Theme](https://github.com/GNOME/gnome-themes-extra) ( recommended )


Makefile
--------

* make [target]:
  * help
  * info
  * ...
  
  
Depends
-------

* gcc
* make
* gettext  ( gettext-tools )
* libgtk 3 ( & dev )
* gstreamer 1.0 ( & dev )
* gst-plugins 1.0 ( & dev )
  * base, good, ugly, bad ( & dev )
* gst-libav
