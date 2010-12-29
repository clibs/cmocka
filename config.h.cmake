/* Name of package */
#cmakedefine PACKAGE "${APPLICATION_NAME}"

/* Version number of package */
#cmakedefine VERSION "${APPLICATION_VERSION}"

#cmakedefine LOCALEDIR "${LOCALE_INSTALL_DIR}"
#cmakedefine DATADIR "${DATADIR}"
#cmakedefine LIBDIR "${LIBDIR}"
#cmakedefine PLUGINDIR "${PLUGINDIR}"
#cmakedefine SYSCONFDIR "${SYSCONFDIR}"
#cmakedefine BINARYDIR "${BINARYDIR}"
#cmakedefine SOURCEDIR "${SOURCEDIR}"

/************************** HEADER FILES *************************/

/* Define to 1 if you have the <assert.h> header file. */
#cmakedefine HAVE_ASSERT_H 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#cmakedefine HAVE_DLFCN_H 1

/* Define to 1 if you have the <inttypes.h> header file. */
#cmakedefine HAVE_INTTYPES_H 1

/* Define to 1 if you have the <malloc.h> header file. */
#cmakedefine HAVE_MALLOC_H 1

/* Define to 1 if you have the <memory.h> header file. */
#cmakedefine HAVE_MEMORY_H 1

/* Define to 1 if you have the <setjmp.h> header file. */
#cmakedefine HAVE_SETJMP_H 1

/* Define to 1 if you have the <signal.h> header file. */
#cmakedefine HAVE_SIGNAL_H 1

/* Define to 1 if you have the <stdarg.h> header file. */
#cmakedefine HAVE_STDARG_H 1

/* Define to 1 if you have the <stddef.h> header file. */
#cmakedefine HAVE_STDDEF_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#cmakedefine HAVE_STDINT_H 1

/* Define to 1 if you have the <stdio.h> header file. */
#cmakedefine HAVE_STDIO_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#cmakedefine HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#cmakedefine HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#cmakedefine HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#cmakedefine HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#cmakedefine HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#cmakedefine HAVE_UNISTD_H 1

/*************************** FUNCTIONS ***************************/

/* Define to 1 if you have the `calloc' function. */
#cmakedefine HAVE_CALLOC 1

/* Define to 1 if you have the `exit' function. */
#cmakedefine HAVE_EXIT 1

/* Define to 1 if you have the `fprintf' function. */
#cmakedefine HAVE_FPRINTF 1

/* Define to 1 if you have the `free' function. */
#cmakedefine HAVE_FREE 1

/* Define to 1 if you have the `longjmp' function. */
#cmakedefine HAVE_LONGJMP 1

/* Define to 1 if you have the `malloc' function. */
#cmakedefine HAVE_MALLOC 1

/* Define to 1 if you have the `memcpy' function. */
#cmakedefine HAVE_MEMCPY 1

/* Define to 1 if you have the `memset' function. */
#cmakedefine HAVE_MEMSET 1

/* Define to 1 if you have the `printf' function. */
#cmakedefine HAVE_PRINTF 1

/* Define to 1 if you have the `setjmp' function. */
#cmakedefine HAVE_SETJMP 1

/* Define to 1 if you have the `signal' function. */
#cmakedefine HAVE_SIGNAL 1

/* Define to 1 if you have the `snprintf' function. */
#cmakedefine HAVE_SNPRINTF 1

/* Define to 1 if you have the `strcmp' function. */
#cmakedefine HAVE_STRCMP 1

/* Define to 1 if you have the `strcpy' function. */
#cmakedefine HAVE_STRCPY 1

/* Define to 1 if you have the `vsnprintf' function. */
#cmakedefine HAVE_VSNPRINTF 1

/**************************** OPTIONS ****************************/

/* define if the compiler implements namespaces */
#cmakedefine HAVE_NAMESPACES

/* Namespace for Google classes */
#cmakedefine GOOGLE_NAMESPACE

/* the namespace where STL code like vector<> is defined */
#cmakedefine STL_NAMESPACE

/* Stops putting the code inside the Google namespace */
#cmakedefine _END_GOOGLE_NAMESPACE_

/* Puts following code inside the Google namespace */
#cmakedefine _START_GOOGLE_NAMESPACE_

/*************************** ENDIAN *****************************/

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#cmakedefine WORDS_BIGENDIAN 1
