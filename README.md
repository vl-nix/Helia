[Helia](https://gitlab.com/vl-nix/Helia)
-------

* Digital TV
  * DVB-T2/S2/C, ATSC, DTMB, ISDB
* Media Player
  * IPTV


Requirements
------------

* Graphical user interface - [Gtk+3](https://developer.gnome.org/gtk3)
* Audio & Video & Digital TV - [Gstreamer 1.0](https://gstreamer.freedesktop.org)
* [GNU Lesser General Public License](http://www.gnu.org/licenses/lgpl.html)


Drag and Drop
-------------

* files, folders
* playlists - [M3U, M3U8](https://en.wikipedia.org/wiki/M3U)


Channels
--------

* scan channels manually
* convert - dvb_channel.conf ( [dvbv5-scan { OPTION... } --output-format=DVBV5 initial file](https://www.linuxtv.org/downloads/v4l-utils) )


Design
------

* [Dark Theme](https://github.com/GNOME/gnome-themes-extra) ( recommended )


Build ( two variants )
---------------------

#### 1. Makefile ( by default, the files will be installed in - $HOME/.local )
  
  * make help
  * make info
  * make
  * make install
  
#### 2. Autogen ( by default, the files will be installed in - /usr/local )

  * sh autogen.sh
  * sh configure
  * make
  * make install


Digital TV
----------
![alt text](https://cn.pling.com/img/5/c/1/a/6ba1cb4b73cdb3cf17db44325829e315c4d0.png)

Media Player
------------
![alt text](https://cn.pling.com/img/7/5/6/5/b22d69f2611e558b25377b127bcbbd169f61.png)

