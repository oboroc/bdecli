/* Secure CRT backwards compatibility for outdated compilers.
 * 
 * Copyright (C) 2014 Adrian Oboroc
 * 
 * This file is part of bdecli project: https://github.com/oboroc/bdecli/
 *
 * See LICENSE file for GPLv3 details
 */

#ifndef COMPAT_S_H
#define COMPAT_S_H

/* Check if compiler is Visual C++ version is below 2005, Watcom, GCC or Digital Mars */
#if ((_MSC_VER && (_MSC_VER < 1400)) || (__WATCOMC__) || (__GNUC__) || (__DMC__))

#define COMPAT_S	1

#define fopen_s(FPP, FNAME, FMODE)	((int)((void *)(NULL) == (void *)((*FPP = fopen(FNAME, FMODE)))))

#define strcpy_s(DESTSTR, MAXLEN, SRCSTR)	(strcpy(DESTSTR, SRCSTR))

#define strncpy_s(DESTSTR, MAXLEN, SRCSTR, NUM)	(strncpy(DESTSTR, SRCSTR, NUM))

#define strcat_s(DESTSTR, MAXLEN, SRCSTR)	(strcat(DESTSTR, SRCSTR))

#define strtok_s(TOK, DELIM, NEXT)	(strtok(TOK, DELIM))

/* most likely this won't work for multiple arguments */
#define sprintf_s(DESTSTR, DESTSIZE, FORMAT, ARGS)	(sprintf(DESTSTR, FORMAT, ARGS))

#define fprintf_s	fprintf
#define printf_s	printf

#endif	/* COMPAT_S */


#if (_MSC_VER >= 1400)
#define fileno		_fileno
#endif	/* (_MSC_VER >= 1400) */

#ifdef __DMC__
#define _strdup		strdup
#define fileno(fp)	((fp)->_file)
#define _fileno(fp)	((fp)->_file)
#endif	/* __DMC__ */

#ifdef __clang__
#define _strdup		strdup
#endif /* __clang__ */

#endif	/* COMPAT_S_H */
