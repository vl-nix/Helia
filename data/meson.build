subdir('icons')

bindir = join_paths(get_option('prefix'), get_option('bindir'))
exec_path = bindir + '/' + meson.project_name()

desktop_conf = configuration_data()

set_desktop = [
  ['NAME', 'Helia'],
  ['COMMENT', 'Digital TV & Media Player'],
  ['EXEC', exec_path],
  ['ICON', 'helia'],
  ['TERMINAL', 'false'],
  ['TYPE', 'Application'],
  ['CATEGORIES', 'GTK;AudioVideo;Player;Video;']
]

foreach props: set_desktop
  desktop_conf.set(props[0], props[1])
endforeach

desktop = meson.project_name() + '.desktop'

configure_file(
  input: 'desktop',
  output: desktop,
  configuration: desktop_conf,
  install: true,
  install_dir: join_paths('share', 'applications')
)

path_gschema = join_paths(get_option('prefix'), get_option('datadir'), 'glib-2.0/schemas')
gschema_file = configure_file(input: 'gschema', output: 'org.gnome.helia.gschema.xml', copy: true)
install_data(gschema_file, install_dir: path_gschema)

cmd_compile_schema = 'glib-compile-schemas ' + path_gschema
meson.add_install_script('sh', '-c', cmd_compile_schema)
