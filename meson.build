project('VkEngine', 'cpp',
  version : '0.1',
  default_options : [
    'cpp_std=c++20',
    'buildtype=debug',
    'warning_level=3'
  ]
)

cpp = meson.get_compiler('cpp')

if get_option('buildtype') == 'debug' or get_option('buildtype') == 'debugoptimized'
  add_project_arguments('-D_DEBUG', language : 'cpp')
else
  add_project_arguments('-DNDEBUG', language : 'cpp')
endif

inc = include_directories(
  'dep/include/',
  'dep/include/imgui/',
  'dep/include/imgui/backends/',
)

# get deps
libdir = join_paths(meson.current_source_dir(), 'dep/lib')
deps = [
  dependency('vulkan', required: true),
  cpp.find_library('glfw3', dirs: libdir, static: true, required: true)
]

dlls = [
  'fmt',
  'assimp-vc143-mt'
]

foreach dll : dlls
  deps += cpp.find_library(dll, dirs: libdir, required: true)
endforeach

# get vulkan sdk path
pymodule = import('python3')
python3 = pymodule.find_python()
vk_sdk = run_command(python3, '-c', 'import os; print(os.environ["VULKAN_SDK"])').stdout().strip()

dxc = find_program(join_paths(vk_sdk, 'Bin/dxc.exe'), required: true)

# shaders
shaders_src = [
  { 'name': 'gradient', 'src': 'shaders/gradient.comp.hlsl', 'profile': 'cs_6_0' },
]

shaders = []
foreach shader : shaders_src
  header = custom_target(
    shader['name'] + '_spv',
    input : shader['src'],
    output : shader['name'] + '.spv',
    command : [
      dxc,
      '-spirv',
      '-T', shader['profile'],
      '-E', 'main',
      '@INPUT@',
      '-Fo', '@OUTPUT@',
      '-fspv-target-env=vulkan1.3',
    ]
  )
  shaders += header
endforeach

# src/executable
src = [
  # src
  'src/main.cpp',
  'src/engine.cpp',
  'src/input.cpp',
  'src/renderer/vk_renderer.cpp',
  'src/renderer/vk_initialisers.cpp',
  'src/renderer/vk_images.cpp',
  'src/renderer/vk_descriptors.cpp',
  'src/renderer/vk_pipelines.cpp',
  # imgui
  'dep/include/imgui/imgui.cpp',
  'dep/include/imgui/imgui_demo.cpp',
  'dep/include/imgui/imgui_draw.cpp',
  'dep/include/imgui/imgui_tables.cpp',
  'dep/include/imgui/imgui_widgets.cpp',
  'dep/include/imgui/backends/imgui_impl_glfw.cpp',
  'dep/include/imgui/backends/imgui_impl_vulkan.cpp',
  # meshoptimizer
  'dep/include/meshoptimizer/allocator.cpp',
  'dep/include/meshoptimizer/clusterizer.cpp',
  'dep/include/meshoptimizer/indexcodec.cpp',
  'dep/include/meshoptimizer/indexgenerator.cpp',
  'dep/include/meshoptimizer/overdrawanalyzer.cpp',
  'dep/include/meshoptimizer/overdrawoptimizer.cpp',
  'dep/include/meshoptimizer/partition.cpp',
  'dep/include/meshoptimizer/quantization.cpp',
  'dep/include/meshoptimizer/simplifier.cpp',
  'dep/include/meshoptimizer/spatialorder.cpp',
  'dep/include/meshoptimizer/stripifier.cpp',
  'dep/include/meshoptimizer/vcacheanalyzer.cpp',
  'dep/include/meshoptimizer/vcacheoptimizer.cpp',
  'dep/include/meshoptimizer/vertexcodec.cpp',
  'dep/include/meshoptimizer/vertexfilter.cpp',
  'dep/include/meshoptimizer/vfetchanalyzer.cpp',
  'dep/include/meshoptimizer/vfetchoptimizer.cpp',
  # other
  'dep/include/vkb/VkBootstrap.cpp',
  shaders
]

exe = executable('engine',
  sources : src,
  include_directories : inc,
  dependencies : deps,
  install : true
)

# copy dlls to output dir
if host_machine.system() == 'windows'
  foreach dll : dlls
    name = dll + '.dll'
    configure_file(
      input : join_paths(libdir, 'native', name),
      output : name,
      copy : true
    )
  endforeach
endif