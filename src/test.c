
#include <stdio.h>

#include "vc6fix.h"

//#if _MSC_VER < 1400
//#define fopen_s(FPP, FNAME, FMODE)	((int)((void *)(NULL) == (void *)((*FPP = fopen(FNAME, FMODE)))))
//#endif

void test(char *filename)
{
	FILE *f;
	int err;

	printf("Attempting to write to file \"%s\"\n", filename);

	err = fopen_s(&f, filename, "wt");

	printf("err = %d\n", err);

	if (0 != err)
	{
		printf("Error: can't create file \"%s\"\n\n", filename);
		return;
	}

	fprintf(f, "This is file \"%s\"\n", filename);
	fclose(f);
	printf("Success!\n\n");
}

void main()
{
	test("blah.tmp");		// positive

	test("\\noserver\\bleh.tmp");	// negative
}
