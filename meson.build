project(
    'martel-print-mech-analyser-firmware',
    'cpp',
    version: '1.0',
    default_options: ['warning_level=3', 'b_staticpic=false', 'cpp_std=c++20'],
)

add_project_arguments(
    [
        '-Wundef',
        '-Wshadow',
        '-Wconversion',
        '-Wno-volatile',
        '-Wsign-conversion',
        '-Wdouble-promotion',

        '-ffunction-sections',
        '-fdata-sections',
        '-funsigned-char',
    ],
    language: ['cpp'],
)

linkscript = files('src/lscript.ld')

add_project_link_arguments(
    [
        '-Wl,-T,@0@/@1@'.format(meson.current_source_dir(), linkscript[0]),
        '-Wl,--gc-sections',
        '-Wl,--print-memory-usage',
        '-Wl,--no-warn-rwx-segment',
    ],
    language: ['cpp'],
)

subdir('lib/bsp')
subdir('src')

executable(
    'martel-print-mech-analyser-firmware',
    name_suffix: 'elf',
    # link_args: [
    #     '-nostartfiles',
    #     '--specs=nano.specs',
    #     '--specs=nosys.specs',
    # ],
    dependencies: [project_src_dep, bsp_dep.as_system()],
)
