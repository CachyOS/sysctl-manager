project('cachyos-sysctl-manager', 'cpp',
        version: '0.0.1',
        license: 'GPLv3',
        meson_version: '>=0.59.0',
        default_options: ['cpp_std=c++20',
                          'buildtype=debugoptimized',
                          'warning_level=3',
                          'werror=false',
                          'b_ndebug=if-release'])

is_debug_build     = get_option('buildtype').startswith('debug')
cc = meson.get_compiler('cpp')
if cc.get_id() == 'clang'
    specific_cc_flags = [
        '-nostdlib++',
        #'-stdlib=libc++',
        '-nodefaultlibs',
    ]
    specific_link_flags = [
        '-fuse-ld=lld',
    ]
    add_global_arguments(cc.get_supported_arguments(specific_cc_flags), language : 'cpp')
    add_global_link_arguments(cc.get_supported_link_arguments(specific_link_flags), language : 'cpp')
endif

if is_debug_build
    add_global_arguments('-D_GLIBCXX_ASSERTIONS', language : 'cpp')
endif
add_global_arguments('-DQT_DISABLE_DEPRECATED_BEFORE=0x050F00', language : 'cpp')

qt5 = import('qt5')
qt5_dep = dependency('qt5', modules: ['Widgets'])

# Common dependencies
fmt = dependency('fmt', version : ['>=8.0.0'], fallback : ['fmt', 'fmt_dep'])
ranges = dependency('range-v3', version : ['>=0.11.0'], fallback : ['range-v3', 'range_dep'])

src_files = files(
    'src/utils.hpp', 'src/utils.cpp',
    'src/sysctl_option.hpp', 'src/sysctl_option.cpp',
    'src/sm-window.hpp', 'src/sm-window.cpp',
    'src/main.cpp',
)

possible_cc_flags = [
    '-Wshadow',

    '-Wnon-virtual-dtor',

    '-Wold-style-cast',
    '-Wcast-align',
    '-Wunused',
    '-Woverloaded-virtual',

    '-Wpedantic', # non-standard C++
    '-Wconversion', # type conversion that may lose data
    '-Wsign-conversion',
    '-Wnull-dereference',
    '-Wdouble-promotion', # float to double

    '-Wformat=2',
    '-Wimplicit-fallthrough', # fallthrough without an explicit annotation
]

if cc.get_id() == 'gcc'
    possible_cc_flags += [
        '-Wmisleading-indentation',

        '-Wduplicated-cond',
        '-Wduplicated-branches',
        '-Wlogical-op',
        '-Wuseless-cast',

        '-Wsuggest-attribute=cold',
        '-Wsuggest-attribute=format',
        '-Wsuggest-attribute=malloc',
        '-Wsuggest-attribute=noreturn',
        '-Wsuggest-attribute=pure',
        '-Wsuggest-final-methods',
        '-Wsuggest-final-types',
        '-Wdiv-by-zero',
        '-Wanalyzer-double-fclose',
        '-Wanalyzer-double-free',
        '-Wanalyzer-malloc-leak',
        '-Wanalyzer-use-after-free',
    ]
endif

if not is_debug_build
   if cc.get_id() == 'gcc'
      possible_cc_flags += [
         '-flto',
         '-fwhole-program',
      ]
   else
      possible_cc_flags += [
         '-flto=thin',
      ]
   endif

   possible_cc_flags += ['-fdata-sections', '-ffunction-sections']
   possible_link_flags = ['-Wl,--gc-sections']
   add_project_link_arguments(cc.get_supported_link_arguments(possible_link_flags), language : 'cpp')
endif

add_project_arguments(cc.get_supported_arguments(possible_cc_flags), language : 'cpp')

deps = [qt5_dep, fmt, ranges]

prep = qt5.preprocess(
  moc_headers : ['src/sm-window.hpp'] # These need to be fed through the moc tool before use.
)
# XML files that need to be compiled with the uic tol.
prep += qt5.compile_ui(sources : ['src/sm-window.ui'])

src_files += [prep]
executable(
  'cachyos-sysctl-manager',
  src_files,
  dependencies: deps,
  include_directories: [include_directories('src')],
  install: true)

summary(
  {
    'Build type': get_option('buildtype'),
  },
  bool_yn: true
)
