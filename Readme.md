### [Helia](https://github.com/vl-nix/helia)

* Media Player
  * Record IPTV ( Original / Encoding )
  * Drag and Drop: files, folders, playlists - [M3U](https://en.wikipedia.org/wiki/M3U)
  * Shortcuts: Ctrl + O, Ctrl + D, Ctrl + Z, space, period, F8

* Digital TV
  * Record ( Original / Encoding )
  * Drag and Drop: gtv-channel.conf
  * Scan: DVB, DTMB ( DVB-T/T2, DVB-S/S2, DVB-C )

#### Channels ( scan initial file )

* [Dvbv5-Gtk](https://github.com/vl-nix/dvbv5-gtk)
* Convert ( DVB, ATSC, DTMB, ISDB ): [DVBv5](https://www.linuxtv.org/docs/libdvbv5/index.html) ⇨ [GstDvbSrc](https://gstreamer.freedesktop.org/documentation/dvb/dvbsrc.html#dvbsrc)

#### Dependencies

* gcc, meson
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
