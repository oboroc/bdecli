
#ifndef VC6FIX_H
#define VC6FIX_H

#if _MSC_VER < 1400
//#define min(X, Y)  ((X) < (Y) ? (X) : (Y))
#define fopen_s(f, filename, mode)	(!(f = fopen(filename, mode)))
#endif

#endif	/* VC6FIX_H */
