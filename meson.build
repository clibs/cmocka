project('cmocka', 'c',
	version : '1.1.5',
	license : 'Apache-2.0')

cc = meson.get_compiler('c')

conf = configuration_data()

foreach hdr : ['assert.h', 'inttypes.h', 'io.h', 'malloc.h', 'memory.h',
	       'setjmp.h', 'signal.h', 'stdarg.h', 'stddef.h', 'stdint.h',
	       'stdio.h', 'stdlib.h', 'string.h', 'strings.h', 'sys/stat.h',
	       'sys/types.h', 'time.h', 'unistd.h']
  conf.set('HAVE_@0@'.format(hdr.underscorify().to_upper()), cc.has_header(hdr))
endforeach

code = '''#include <time.h>
int a = sizeof(struct timespec);
'''
conf.set('HAVE_STRUCT_TIMESPEC', cc.compiles(code, name : 'struct timepec'))

foreach func: ['calloc', 'exit', 'fprintf', 'free', 'longjmp', 'siglongjmp',
	       'malloc', 'memcpy', 'memset', 'printf', 'setjmp', 'signal',
	       'strsignal', 'strcmp', 'clock_gettime']
  conf.set('HAVE_@0@'.format(func.to_upper()), cc.has_function(func))
endforeach

code = '__thread int tls;'
conf.set('HAVE_GCC_THREAD_LOCAL_STORAGE', cc.compiles(code, name : '__thread'))

code = '''#include <time.h>
clockid_t t = CLOCK_REALTIME;'''
conf.set('HAVE_CLOCK_REALTIME', cc.compiles(code, name : 'CLOCK_REALTIME'))

configure_file(output : 'config.h', configuration : conf)

cmocka_includes = [include_directories('.'), include_directories('include')]
libcmocka = library('cmocka', 'src/cmocka.c',
                    c_args : ['-DHAVE_CONFIG_H'],
                    include_directories : cmocka_includes,
                    install : meson.is_subproject(),
                    override_options : ['c_std=gnu99'],
                    dependencies : [cc.find_library('rt', required : false)])

if meson.is_subproject()
  cmocka_dep = declare_dependency(include_directories : cmocka_includes,
                                  link_with : libcmocka)
else
  install_headers('include/cmocka.h')

  pkgconfig = import('pkgconfig')
  pkgconfig.generate(libraries : [libcmocka],
                     version : meson.project_version(),
                     name : 'cmocka',
                     filebase : 'cmocka',
                     description : 'The cmocka unit testing library')
endif

if get_option('unit_testing')
	subdir('tests')
endif
