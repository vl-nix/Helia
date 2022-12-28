subdir('icons')

desktop = meson.project_name() + '.desktop'

run_command('sh', '-c', 'echo "[Desktop Entry]\nName=Helia\nComment=Digital TV\nExec=helia\nIcon=helia\nTerminal=false\nType=Application\nCategories=GTK;AudioVideo;Player;Video;\n" > desktop', check: true)
configure_file(input: 'desktop', output: 'helia.desktop', copy: true, install: true, install_dir: join_paths('share', 'applications'))

run_command('sh', '-c', 'echo \'<?xml version="1.0" encoding="UTF-8"?>\n<schemalist gettext-domain="helia">\n  <schema id="org.gnome.helia" path="/org/gnome/helia/">\n    <key name="dark" type="b">\n      <default>true</default>\n    </key>\n    <key name="opacity" type="u">\n      <default>100</default>\n    </key>\n    <key name="width" type="u">\n      <default>900</default>\n    </key>\n    <key name="height" type="u">\n      <default>400</default>\n    </key>\n    <key name="theme" type="s">\n      <default>"none"</default>\n    </key>\n  </schema>\n</schemalist>\' > gschema', check: true)
configure_file(input: 'gschema', output: 'org.gnome.helia.gschema.xml', copy: true, install: true, install_dir: join_paths('share', 'glib-2.0/schemas'))

meson.add_install_script('sh', '-c', 'glib-compile-schemas /usr/share/glib-2.0/schemas')
meson.add_install_script('sh', '-c', 'gtk-update-icon-cache -qtf /usr/share/icons/hicolor')
meson.add_install_script('sh', '-c', 'update-desktop-database -q /usr/share/applications')
