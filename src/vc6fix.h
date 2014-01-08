
#ifndef VC6FIX_H
#define VC6FIX_H

/* Implement fopen_s() using fopen() if Visual C++ version is below 2005 */
#if _MSC_VER < 1400
#define fopen_s(FPP, FNAME, FMODE)	((int)((void *)(NULL) == (void *)((*FPP = fopen(FNAME, FMODE)))))
#endif

#endif	/* VC6FIX_H */
