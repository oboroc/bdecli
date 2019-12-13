/* Secure CRT backwards compatibility for outdated compilers.
 * 
 * Copyright (C) 2014 Adrian Oboroc
 *  
 * This file is part of bdecli project <https://github.com/oboroc/bdecli/>.
 * 
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
