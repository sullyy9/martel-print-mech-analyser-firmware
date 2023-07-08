current_dir = meson.source_root() + '/lib/bsp/'
bsp_lib_path = current_dir + 'microblaze_0/lib/libxil.a'

# Build the BSP library and install it into buildir.
bsp_build_lib = custom_target('bsp',
    command: ['make', '-C', current_dir, '&&', 'cp', bsp_lib_path, './lib/bsp/'],
    output: 'libxil.a'
)

bsp_dep = declare_dependency(
    sources: bsp_build_lib,
    link_args: '-L' + './lib/bsp/',
    include_directories: './microblaze_0/include'
)
