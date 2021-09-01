# Rules for building with Meson

project(
  'uefivar',
  'cpp',
  default_options: [
   'warning_level=3',
   'werror=true',
   'cpp_std=c++17',
  ],
  license: 'Apache-2.0',
)

# version info from git
version = vcs_tag(
  command: [ 'git', 'describe', '--always', '--dirty', '--long' ],
  input: 'src/version.hpp.in',
  output: 'version.hpp',
)
# year for copyright title
year = run_command('date', '+%Y').stdout().strip()

# unit tests
if get_option('tests').enabled()
  subdir('test')
endif

# generate source code for D-Bus interface
sdbuspp = find_program('sdbus++', native: true)
sdbus_hpp = custom_target(
  'sdbus_hpp',
  input: 'com/yadro/UefiVar.interface.yaml',
  output: 'server.hpp',
  capture: true,
  command: [sdbuspp, '-r', meson.current_source_dir(),
                     'interface', 'server-header', 'com.yadro.UefiVar'],
)
sdbus_cpp_in = custom_target(
  'sdbus_cpp_in',
  input: 'com/yadro/UefiVar.interface.yaml',
  output: 'server.cpp.in',
  capture: true,
  command: [sdbuspp, '-r', meson.current_source_dir(),
                     'interface', 'server-cpp', 'com.yadro.UefiVar'],
)
sed = find_program('sed', native: true)
sdbus_cpp = custom_target(
  'sdbus_cpp',
  input: sdbus_cpp_in,
  output: 'server.cpp',
  capture: true,
  command: [sed, 's#<com/yadro/UefiVar/server.hpp>#<server.hpp>#',
                 '@INPUT@'],
)

# install systemd unit template file
systemd = dependency('systemd')
systemd_system_unit_dir = systemd.get_pkgconfig_variable(
  'systemdsystemunitdir',
  define_variable: ['prefix', get_option('prefix')],
)
configure_file(
  copy: true,
  input: 'uefivar.service',
  install: true,
  install_dir: systemd_system_unit_dir,
  output: 'uefivar.service',
)

executable(
  'uefivar',
  [
    version,
    sdbus_hpp,
    sdbus_cpp,
    'src/dbus.cpp',
    'src/main.cpp',
    'src/nvram.cpp',
    'src/storage.cpp',
    'src/variable.cpp',
  ],
  dependencies: [
    systemd,
    dependency('json-c'),
    dependency('phosphor-logging'),
    dependency('sdbusplus'),
    dependency('uuid'),
  ],
  install: true,
  cpp_args : '-DUEFIVAR_YEAR="' + year + '"',
)
