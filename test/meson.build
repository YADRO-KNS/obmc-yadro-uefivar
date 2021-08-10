# Rules for building tests

test(
  'uefivar',
  executable(
    'uefivar_test',
    [
      'nvram_test.cpp',
      'storage_test.cpp',
      'variable_test.cpp',
      '../src/nvram.cpp',
      '../src/storage.cpp',
      '../src/variable.cpp',
    ],
    dependencies: [
      dependency('gtest', main: true, disabler: true, required: true),
      dependency('json-c'),
      dependency('phosphor-logging'),
      dependency('uuid'),
    ],
    include_directories: '../src',
    cpp_args : '-DTEST_DATA_DIR="' + meson.current_source_dir() + '"',
  )
)
