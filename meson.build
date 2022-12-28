project('helia', 'c', version: '22.10', meson_version: '>= 0.50.0', default_options: ['warning_level=2', 'c_std=gnu11'])

c_args = ['-DVERSION="' + meson.project_version() + '"']

res = []
subdir('data')

c = run_command('sh', '-c', 'for file in src/*.h src/*.c; do echo $file; done', check: true)
src = c.stdout().strip().split('\n')

src += res

deps = [dependency('gtk+-3.0'), dependency('gstreamer-video-1.0'), dependency('gstreamer-mpegts-1.0')]

executable(meson.project_name(), src, dependencies: deps, c_args: c_args, install: true)
