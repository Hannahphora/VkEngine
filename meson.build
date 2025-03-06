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
  add_project_arguments('-DDEBUG', language : 'cpp')
endif

inc = include_directories(
  'dep/include/',
  'dep/include/imgui/',
  'dep/include/imgui/backends/'
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
res = run_command(python3, '-c', 'import os; print(os.environ["VULKAN_SDK"])')
if res.returncode() == 0
  vk_sdk = res.stdout().strip()
else
  # vk_sdk = 'C:/VulkanSDK/1.3.296.0/'
  vk_sdk = 'C:/VulkanSDK/1.3.283.0/'
endif

dxc = find_program(join_paths(vk_sdk, 'Bin/dxc.exe'), required: true)

# shaders
shaders_src = [
  { 'name': 'vert', 'src': 'shaders/vert.hlsl', 'profile': 'vs_6_0' },
  { 'name': 'frag', 'src': 'shaders/frag.hlsl', 'profile': 'ps_6_0' }
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
  'src/renderer/vk_renderer.cpp',
  'src/renderer/vk_initialisers.cpp',
  'src/renderer/vk_images.cpp',
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
  'dep/include/vkbootstrap/VkBootstrap.cpp',
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