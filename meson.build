project('VkEngine', 'cpp',
  version : '0.1',
  default_options : [
    'cpp_std=c++20',
    'buildtype=debug',
    'warning_level=3'
  ]
)

cpp = meson.get_compiler('cpp')

inc = include_directories('dep/include/')

libdir = join_paths(meson.current_source_dir(), 'dep/lib')
deps = [
  dependency('vulkan', required: true),
  cpp.find_library('fmt', dirs: libdir),
  cpp.find_library('assimp-vc143-mt', dirs: libdir),
  cpp.find_library('glfw3', dirs: libdir, static: true)
]

src = [
  'src/main.cpp',
  'src/engine.cpp',
  'src/window.cpp'
]

exe = executable('VkEngine',
  sources : src,
  include_directories : inc,
  dependencies : deps,
  install : true
)

dll_files = ['fmt.dll', 'assimp-vc143-mt.dll']
foreach dll : dll_files
  configure_file(
    input : join_paths(libdir, 'native', dll),
    output : dll,
    copy : true
  )
endforeach