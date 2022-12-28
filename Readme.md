### [Helia](https://github.com/vl-nix/helia)

* Digital TV
  * Record
  * Multi-Windows
  * Scan: DVB, ATSC, DTMB
  * Drag and Drop: gtv-channel.conf

#### Channels ( scan initial file )

* [Dvbv5-Gtk](https://github.com/vl-nix/dvbv5-gtk)
* Convert ( DVB, ATSC, DTMB, ISDB ): [DVBv5](https://www.linuxtv.org/docs/libdvbv5/index.html) ⇨ [GstDvbSrc](https://gstreamer.freedesktop.org/documentation/dvb/dvbsrc.html#dvbsrc)

#### Dependencies

* gcc
* meson
* libgtk 3.0 ( & dev )
* gstreamer 1.0 ( & dev )
* all gst-plugins 1.0 ( & dev )
* gst-libav

#### Build

1. Clone: git clone git@github.com:vl-nix/helia.git

2. Configure: meson build --prefix /usr --strip

3. Build: ninja -C build

4. Install: sudo ninja -C build install

5. Uninstall: sudo ninja -C build uninstall

#### Ver. 22.10

* Add:
  * Multi-Windows ( max 4 ): open channel ( double-clicked ) -> select channel ( current transponder ) -> add window ( right-click )

* Delete:
  * Media Player
  * Encoding
  * Equalizers

