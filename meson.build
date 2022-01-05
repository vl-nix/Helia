project('helia', 'c', version: '22.2', meson_version: '>= 0.50.0', default_options: ['warning_level=2', 'c_std=gnu11'])

cc = meson.get_compiler('c')

c_args = ['-DVERSION="' + meson.project_version() + '"']

res = []
subdir('data')

c = run_command('sh', '-c', 'for file in src/*.h src/*.c; do echo $file; done')
src = c.stdout().strip().split('\n')

src += res

libgstpbutils = cc.find_library('libgstpbutils-1.0', required: true)

helia_deps  = [dependency('gtk+-3.0', version: '>= 3.22')]
helia_deps += [dependency('gstreamer-video-1.0'), dependency('gstreamer-mpegts-1.0'), libgstpbutils]

executable(meson.project_name(), src, dependencies: helia_deps, c_args: c_args, install: true)
