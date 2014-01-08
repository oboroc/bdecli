
#ifndef VCFIX_H
#define VCFIX_H

#if _MSC_VER < 1400	/* Check if Visual C++ version is below 2005 */

#define fopen_s(FPP, FNAME, FMODE)	((int)((void *)(NULL) == (void *)((*FPP = fopen(FNAME, FMODE)))))

#define strcpy_s(DESTSTR, MAXLEN, SRCSTR)	(strcpy(DESTSTR, SRCSTR))

#define strncpy_s(DESTSTR, MAXLEN, SRCSTR, NUM)	(strncpy(DESTSTR, SRCSTR, NUM))

#define strcat_s(DESTSTR, MAXLEN, SRCSTR)	(strcat(DESTSTR, SRCSTR))

#define fprintf_s	fprintf
#define printf_s	printf

#endif	/* _MSC_VER < 1400 */

#endif	/* VCFIX_H */
