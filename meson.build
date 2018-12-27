project('Stress_RSR', 'c',
        version: '1',
        default_options: ['c_std=c99', 'buildtype=release'],
        meson_version: '>=0.46')

warnings = [
  '-Wall',
  '-Wextra',
  '-Wno-unused-parameter',
]

cflags = [
  warnings,
]

ldflags = [
]

host_system = host_machine.system()

host_cpu_family = host_machine.cpu_family()

if host_system == 'windows' or host_system == 'cygwin'
  if host_cpu_family == 'x86'
    ldflags += '-Wl,--kill-at'
  endif
endif

cc = meson.get_compiler('c')


sources = [
  'src/stress.c',
]

deps = [
  dependency('vapoursynth').partial_dependency(
      includes: true,
      compile_args: true),
]

shared_module('stress',
              sources,
              dependencies: deps,
              link_args: ldflags,
              c_args: cflags,
              install: true)
