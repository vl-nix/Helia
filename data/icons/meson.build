gnome = import('gnome')

res = gnome.compile_resources('res', 'gresource.xml', source_dir: '.')

dvb_datadir = join_paths(get_option('prefix'), get_option('datadir'))
install_data('helia.svg', install_dir: join_paths(dvb_datadir, 'icons', 'hicolor', 'scalable', 'apps'))
