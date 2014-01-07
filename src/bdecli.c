#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN

#include <memory.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>


#define	PROGRAM_VERSION	"1.0.0"
#define MAX_BUFFER	10000
#define BDE_NUL	0
#define	BDE_VAR	1
#define	BDE_EOB	2
#define	BDE_NEW	3
#define BDE_EQU	4


typedef enum e_BDEEntryType {BDE_Undefined = 0, BDE_Container = 1, BDE_Variable = 2} tBDEEntryType;

typedef struct s_BDEConfigEntry
{
	tBDEEntryType entry_type;	/* entry type */
	char *name, *value;		/* entry name and value */
	struct s_BDEConfigEntry *next, *previous, *container;	/* linked list and container pointers */
} tBDEConfigEntry, *pBDEConfigEntry;


char *bde_fgets(FILE *stream)
{
	char *str;
	int ch;
	char buf[MAX_BUFFER];
	rsize_t iStrSize;

	ch = fgetc(stream);

	/* we expect byte with value 0 */
	if (BDE_NUL != ch)
	{
		printf_s("Error in bde_fgets(): leading %i expected, %i found\n", BDE_NUL, ch);
		return NULL;	/* fail */
	}

	/* initial length of return string */
	iStrSize = 0;

	do
	{
		/* build string in heap buffer */
		ch = fgetc(stream);
		buf[iStrSize] = ch;

		iStrSize++;

		/* check for potential buffer overflow */
		if (MAX_BUFFER == iStrSize)
		{
			printf_s("Error in bde_fgets(): buffer overflow in string parser\n");
			return NULL;	/* fail */
		}

		/* check for unexpected end of file */
		if (feof(stream))
		{
			printf_s("Error in bde_fgets(): unexpected end of file in string parser\n");
			return NULL;
		}

	} while ((BDE_NUL != ch));

	/* allocate memory for the return string */
	str = (char *)malloc(iStrSize);

	if (!str)
	{
		printf_s("Error in bde_fgets(): failed to allocate %i bytes of memory for return string\n", iStrSize);
		return NULL;
	}

	strcpy_s(str, iStrSize, buf);	/* copy heap buffer to return string */

	return str;
}

pBDEConfigEntry bde_new_entry(pBDEConfigEntry prev, pBDEConfigEntry cont)
{
	pBDEConfigEntry entry;

	entry = (pBDEConfigEntry)malloc(sizeof(tBDEConfigEntry));

	if (!entry)
	{
		printf_s("Error in bde_new_entry(): failed to allocate %i bytes of memory for linked list entry\n", sizeof(tBDEConfigEntry));
		return NULL;
	}

	memset(entry, 0, sizeof(tBDEConfigEntry));	/* set everything to zero/null */

	entry->previous = prev;
	entry->container = cont;	/* by default, use container from previous linked list entry */

	if (prev)
		prev->next = entry;	/* set previous entry's next field to new entry */

	return entry;
}

pBDEConfigEntry bde_config_parse(char *szFileName)
{
	FILE *stream;
	errno_t err;
	int ch;
	char *str, *value;
	pBDEConfigEntry start, current;

	/* Initialize start and current pointers */
	start = bde_new_entry(NULL, NULL);
	current = start;
	
	err = fopen_s(&stream, szFileName, "r");	/* open for read (will fail if file does not exist) */

	if (0 != err)
	{
		printf_s("Error in bde_config_parse(): file \"%s\" was not opened\n", szFileName);
		return NULL;
	}

	ch = fgetc(stream);

	while (!feof(stream))
	{
		if (BDE_NEW != ch)
		{
			printf_s("Error in bde_config_parse(): unexpected byte at the beginning of line. Expected %i, actual value is %i\n", BDE_NEW, ch);
			return NULL;
		}

		ch = fgetc(stream);
		if (BDE_NUL != ch)
		{
			printf_s("Error in bde_config_parse(): unexpected second byte at the beginning of line. Expected %i, actual value %i\n", BDE_NUL, ch); 
			return NULL;
		}


		ch = fgetc(stream);	/* read block id from file */

		switch(ch)
		{

		case BDE_NUL:	/* 0x0000 - contrainer */

			str = bde_fgets(stream);

			ch = fgetc(stream);
			if (BDE_VAR != ch)
			{
				printf_s("Error in bde_config_parse(): unexpected byte at the end of contrainer name. Expected %i, actual value is %i\n", BDE_VAR, ch);
				return NULL;
			}
					
			ch = fgetc(stream);
			if (BDE_NUL != ch)
			{
				printf_s("Error in bde_config_parse(): unexpected second byte after the end of contrainer name. Expected %i, actual value is %i\n", BDE_NUL, ch);
				return NULL;
			}

			current->entry_type = BDE_Container;
			current->name = str;
			current = bde_new_entry(current, current);	/* spawn a new linked list entry */

			break;

		case BDE_VAR:	/* 0x0001 - value */

			str = bde_fgets(stream);

			ch = fgetc(stream);
			if (BDE_EQU != ch)
			{
				printf_s("Error in bde_config_parse(): unexpected byte between variable name and value. Expected %i, actual value %i\n", BDE_EQU, ch); 
				return NULL;
			}

			value = bde_fgets(stream);

			if (!value)
			{
				printf_s("Error in bde_config_parse(): failed to parse the value of variable\n");
				return NULL;
			}

			current->entry_type = BDE_Variable;
			current->name = str;
			current->value = value;

			current = bde_new_entry(current, current->container);	/* spawn a new linked list entry */

			break;

		default:

			printf_s("Error in bde_config_parse(): unexpected byte in main parser loop: %i", ch);
			return NULL;

		}

		/* parse the end of line */
		do {
			ch = fgetc(stream);

			if (BDE_EOB == ch)
			{
				/* end of contrainer, go to upper level container */
				if (current->container)
					current->container = current->container->container;	

				/* check for padding zero byte */
				ch = fgetc(stream);

				if (BDE_NUL != ch)
				{
					printf_s("Error in bde_config_parse(): unexpected byte at the end of line. Expected %i, actual value is %i\n", BDE_NUL, ch);
					return NULL;
				}
			}
		} while ((!feof(stream)) && (BDE_NEW != ch));
	};

	if (!current->name)	/* sanitize last preallocated linked list entry */
	{
		current->previous->next = NULL;	/* set the next field of previous linked list entry to null */
		free(current);
	}

	/* close stream */
	if ((stream) && (fclose(stream)))
		printf_s("Error in bde_config_parse(): the file was not closed\n");

	return start;
}

/* build and return a fully qualified hierarchical name of entry */
char *bde_fqn(pBDEConfigEntry entry)
{
	pBDEConfigEntry current;
	char tmp[MAX_BUFFER], *str;
	char buf[MAX_BUFFER] = "";	/* initialize buffer with empty string */

	current = entry;
	while (current)
	{
		strcpy_s(tmp, MAX_BUFFER, buf);	/* save work in progress name in temporary buffer */

		strcpy_s(buf, MAX_BUFFER, current->name);	/* start building new name prefixed with container name */

		if (current != entry)
			strcat_s(buf, MAX_BUFFER, "\\");	/* and registry or folder style delimiter */
		
		strcat_s(buf, MAX_BUFFER, tmp);	/* append previous name */

		current = current->container;
	}

	str = (char *)malloc(strlen(buf) + 1);

	strcpy_s(str, strlen(buf) + 1, buf);

	return str;
}


int bde_containers(pBDEConfigEntry entry)
{
	int count;

	count = 0;

	while (entry)
	{
		entry = entry->container;
		count++;
	}

	return count;
}


void bde_config_export(pBDEConfigEntry list, char *szFileName)
{
	FILE *stream;
	errno_t err;
	pBDEConfigEntry current;
	char *str;

	/* Initialize current pointer */
	current = list;
	
	err = fopen_s(&stream, szFileName, "w");	/* open for writing (will overwrite if file exists) */

	if (0 != err)
	{
		printf_s("Error in bde_config_export(): file \"%s\" was not opened\n", szFileName);
		return;
	}

	while (current)
	{
		if (BDE_Variable == current->entry_type)
		{
			str = bde_fqn(current);

			fprintf_s(stream, "%s = %s\n", str, current->value);

			free(str);
		}

		current = current->next;
	}

	/* close stream */
	if ((stream) && (fclose(stream)))
		printf_s("Error in bde_config_export(): the file was not closed\n");
}


void bde_config_free(pBDEConfigEntry list)	/* free all memory allocated for linked list */
{
	pBDEConfigEntry item, tmp;

	item = list;

	while (item)
	{
		tmp = item;
		item = item->next;

		if (tmp->name)
			free(tmp->name);

		if (tmp->value)
			free(tmp->value);

		free(tmp);
	}
}


/* write string to file, BDE way */
void bde_fputs(char *str, FILE *stream)
{
	if (!str)
	{
		printf_s("Error in bde_fputs(): string pointer is NULL\n");
		return;
	}

	fputc(BDE_NUL, stream);	/* write lead zero */
	fputs(str, stream);	/* write the string */
	fputc(BDE_NUL, stream);	/* write trail zero */
}


/* write from linked list to BDE configuration file */
void bde_config_write(pBDEConfigEntry list, char *szFileName)
{
	FILE *stream;
	errno_t err;
	int ld, eob;
	pBDEConfigEntry current, tmp_c, tmp_n;

	/* Initialize current pointer */
	current = list;
	
	err = fopen_s(&stream, szFileName, "w");	/* open for writing (will overwrite if file exists) */

	if (0 != err)
	{
		printf_s("Error in bde_config_write(): file \"%s\" was not opened\n", szFileName);
		return;
	}

	while (current)
	{
		/* start new line */
		fputc(BDE_NEW, stream);
		fputc(BDE_NUL, stream);

		switch(current->entry_type)
		{
		case BDE_Container:
			/* leading zero */
			fputc(BDE_NUL, stream);

			/* put container name */
			bde_fputs(current->name, stream);

			/* trail with 1 and 0 */
			fputc(BDE_VAR, stream);
			fputc(BDE_NUL, stream);

			break;

		case BDE_Variable:
			/* leading 1 */
			fputc(BDE_VAR, stream);

			/* put variable name */
			bde_fputs(current->name, stream);

			/* put 'equals' */
			fputc(BDE_EQU, stream);

			/* put variable value */
			bde_fputs(current->value, stream);

			break;

		default:
			printf_s("Error in bde_config_write(): invalid list entry. Expected %i or %i, actual value is %i\n",
				BDE_Container, BDE_Variable, current->entry_type);
			return;
		}

		/* check if we should put the 'end of container' byte sequence */
		if ((current->next) &&						/* there is a next entry */
			(current->entry_type = BDE_Variable) &&			/* current entry is a variable */
			(current->container != current->next->container))	/* and it has a different container */
		{
			eob = 0;

			tmp_c = current->container;
			tmp_n = current->next->container;

			ld = bde_containers(current) - bde_containers(current->next);	/* next entry belongs to fewer containers */

			if (0 < ld)
			{
				eob = ld;
				while (0 < ld)
				{
					tmp_c = tmp_c->container;
					ld--;
				}
			}
			else
				if (0 > ld)
					while (0 > ld)
					{
						tmp_n = tmp_n->container;
						ld++;
					}

			while (tmp_c != tmp_n)
			{
				tmp_c = tmp_c->container;
				tmp_n = tmp_n->container;
				eob++;
			}
				
			while (eob > 0)
			{
				fputc(BDE_EOB, stream);
				fputc(BDE_NUL, stream);
				eob--;
			}
		}

		/* see if this was the last entry in list */
		if (!current->next)
		{
			/* write final end of block sequence */
			eob = bde_containers(current) - 1;

			while (eob > 0)
			{
				fputc(BDE_EOB, stream);
				fputc(BDE_NUL, stream);
				eob--;
			}
		}

		/* iterate to next item in list */
		current = current->next;
	}

	/* close stream */
	if ((stream) && (fclose(stream)))
		printf_s("Error in bde_config_write(): the file was not closed\n");
}


/* bde_conains() returns 1 if container is a parent of entry, 0 otherwise */
int bde_contains(pBDEConfigEntry co, pBDEConfigEntry entry)
{
	while (entry)
	{
		entry = entry->container;
		if (entry == co)
			return 1;
	}
	return 0;
}

/*
 *	bde_config_add_entry() checks if proper container exists, if not, creates it.
 *	within that container, it looks for variable with supplied name, if it exists and
 *	has different value, bde_config_add_entry() will update it. If variable doesn't exist
 *	bde_config_add_entry() will create it. Return 1 if variable was created or modified,
 *	0 otherwise.
*/
int bde_config_add_entry(pBDEConfigEntry list, char *szFQNPath, char *szName, char *szValue)
{
	pBDEConfigEntry current, parcon, co, tmp_e;
	char buf[MAX_BUFFER], *tmp;
	int i;
	size_t buflen;

	if (list->entry_type != BDE_Container)	/* sanitize list */
	{
		printf_s("Error in bde_config_add_entry(): first entry in linked list is not a container\n");
		return 0;
	}

	tmp = szFQNPath;
	i = 0;
	co = NULL;
	parcon = NULL;

	/* iterate through containers in szFQNPath */
	while (0 != tmp[i])
	{
		if (('\\' == tmp[i]) || (0 == tmp[i + 1]))
		{
			if (i > MAX_BUFFER)	/* sanitize possible buffer overflow */
			{
				printf_s("Error in bde_config_add_entry(): string buffer is too small");
				return 0;
			}

			if ('\\' == tmp[i])
				strncpy_s(buf, MAX_BUFFER, tmp, i);
			else
				strcpy_s(buf, MAX_BUFFER, tmp);

			co = NULL;
			current = parcon ? parcon->next : list;

			/* iterate through the linked list, looking for a container with
			specific name and same parent container */
			while (current)
			{
				if ((BDE_Container == current->entry_type) &&
					(current->container == parcon))
					if (0 == strcmp(current->name, buf))	/* case-sensitive comparison */
					{
						/* container found */
						co = current;
						parcon = co;
						break;
					}
				current = current->next;
			}

			if (!co)	/* container not found, we should create a new one */
			{
				current = parcon ? parcon : list;
				while ((current->next) && (bde_contains(parcon, current->next)))
					current = current->next;
				tmp_e = current->next;

				co = bde_new_entry(current, parcon);	/* create new entry */
				if (!co)
				{
					printf_s("Error in bde_config_add_entry(): malloc() failed while creating new container\n");
					return 0;
				}

				co->entry_type = BDE_Container;

				co->name = (char *)malloc(strlen(buf) + 1);	/* allocate memory for name */
				if (!co->name)
				{
					printf_s("Error in bde_config_add_entry(): malloc() failed while creating new container name\n");
					return 0;
				}
				strcpy_s(co->name, strlen(buf) + 1, buf);

				/* relink the list */
				co->next = tmp_e;
				if (tmp_e)
					tmp_e->previous = co;

				/* new container created, use it as parent in next iteration */
				parcon = co;
			}
			tmp += i + 1;
			i = 0;
		} else
		i++;
	}

	/* we have proper container, now we should modify or add variable */
	current = co->next;
	while ((current) && (current->container == co))
	{
		if ((BDE_Variable == current->entry_type) &&
			(0 == strcmp(current->name, szName)))	/* case-sensitive comparison */
		{
			/* found variable, compare it's value to new value */
			if (0 == strcmp(current->value, szValue))	/* case-sensitive comparison */
				return 0;
			else
			{
				buflen = strlen(szValue) + 1;

				if (buflen != strlen(current->value) + 1)
				{
					free(current->value);
					current->value = (char *)malloc(buflen);	/* realloc didn't work */
					if (!current->value)	/* sanitize for realloc failure */
					{
						printf_s("Error in bde_config_add_entry(): realloc() failed\n");
						return 0;
					}
				}
				strcpy_s(current->value, buflen, szValue);
				return 1;
			}
			break;
		}
		current = current->next;
	}

	/* variable not found, add new entry to list
	iterate until the last entry within container */
	current = co;
	while ((current->next) && (bde_contains(co, current->next)))
		current = current->next;
	tmp_e = current->next;

	current = bde_new_entry(current, co);	/* create new entry */
	if (!current)
	{
		printf_s("Error in bde_config_add_entry(): malloc() failed while creating new variable\n");
		return 0;
	}

	current->entry_type = BDE_Variable;

	current->name = (char *)malloc(strlen(szName) + 1);	/* allocate memory for name */
	if (!current->name)
	{
		printf_s("Error in bde_config_add_entry(): malloc() failed while creating new variable name\n");
		return 0;
	}
	strcpy_s(current->name, strlen(szName) + 1, szName);

	current->value = (char *)malloc(strlen(szValue) + 1);	/* allocate memory for value */
	if (!current->value)
	{
		printf_s("Error in bde_config_add_entry(): malloc() failed while creating new variable value\n");
		return 0;
	}
	strcpy_s(current->value, strlen(szValue) + 1, szValue);

	/* relink the list */
	current->next = tmp_e;
	if (tmp_e)
		tmp_e->previous = current;

	return 1;
}

/*
 *	bde_config_update() parses text file one line at a time,
 *	extracts FQN path, variable name and variable value and
 *	calls bde_config_add_entry() to update linked list, if necessary.
 *	 bde_config_update() returns a total number of updated and added entries.
 */
int bde_config_update(pBDEConfigEntry list, char *szFileName)
{	
	FILE *stream;
	errno_t err;
	char buf[MAX_BUFFER], *szFQNPath, *szName, *szValue;
	int i, change_no = 0;

	err = fopen_s(&stream, szFileName, "r");	/* open for reading */

	if (0 != err)
	{
		printf_s("Error in bde_config_export(): file \"%s\" was not opened\n", szFileName);
		return 0;
	}

	while (!feof(stream))
	{
		if (!fgets(buf, MAX_BUFFER - 1, stream))
			/* we get here if fgets failed due to error or end of file */
			if (ferror(stream))	/* check for error */
			{
				printf_s("Error in bde_config_export(): fgets() failed while reading from \"%s\"\n", szFileName);
				return 0;
			}
			else
				break;	/* no error, exit loop */

		/* next we should split the line from text file into FQN path,
		variable name and variable value */
		szFQNPath = (char *)buf;

		/* skip leading white space */
		while ((*szFQNPath == ' ') || (*szFQNPath == '\t'))
			szFQNPath++;

		/* check for empty line, if found, skip to next */
		if (*szFQNPath == 0)
			continue;

		/* look for '=' */
		i = 0;
		while ((szFQNPath[i] != '=') && (szFQNPath[i] != 0))
			i++;

		/* sanitize for '=' presense */
		if (szFQNPath[i] != '=')
		{
			printf_s("Error in bde_config_export(): no '=' in line %s from file %s\n", buf, szFileName);
			return 0;
		}

		/* point to value memory location after '=' and white space */
		szValue = szFQNPath + i + 1;
		while ((*szValue == ' ') || (*szValue == '\t'))
			szValue++;

		/* back to szFQNPath - pad '=' and trailing white space before it with zeroes */
		while ((0 < i) &&
				(('=' == szFQNPath[i]) || (' ' == szFQNPath[i]) || ('\t' == szFQNPath[i])))
		{
			szFQNPath[i] = 0;
			i--;
		}

		/* search for last '\' in szFQNPath */
		while (0 < i)
		{
			if ('\\' == szFQNPath[i])
				break;	/* found '\', exit loop */
			else
				i--;
		}

		if ('\\' != szFQNPath[i])
		{
			printf_s("Error in bde_config_export(): failed to find '\\' in '%s'\n", szFQNPath);
			return 0;
		}

		szFQNPath[i] = 0;
		szName = szFQNPath + i + 1;

		/* find the end of value string */
		i = 0;
		while (szValue[i] != 0)
		{
			if ((szValue[i] == 10) || (szValue[i] == 13))	/* get rid of cr/lf */
			{
				szValue[i] = 0;
				break;
			}
			i++;
		}

		/* add new entry to linked list, based on FQN path, variable name and variable value */
		change_no += bde_config_add_entry(list, szFQNPath, szName, szValue);
	}

	/* close stream */
	if ((stream) && (fclose(stream)))
		printf_s("Error in bde_config_export(): the file was not closed\n");

	return change_no;
}


int main(int argc, char *argv[])
{
	pBDEConfigEntry cfg;
	int change_no;
	cfg = NULL;

	if (1 == argc)
	{
		printf_s("bdecli %s\thttp://oboroc.com/bdecli\n\n", PROGRAM_VERSION);
		printf_s(
			"How to use:\n\n"	\
			"Export settings from idapi32.cfg: bdecli -e idapi32.cfg output.txt\n\n"
			"Edit the text file, leaving only settings relevant to deployed application.\n\n"
			"Import settings from edited text file: bdecli -i idapi32.cfg input.txt\n"
			);
		return 1;
	}

	if (4 != argc)
	{
		printf_s("Error in main(): wrong number of parameters. Type bdecli for usage.\n");
		return 2;
	}

	if (argv[1][0] != '-')
	{
		printf_s("Error in main(): invalid switch. Type bdecli for usage.\n");
		return 2;
	}

	switch(argv[1][1])
	{
	case 'i':
	case 'I':
		printf_s("Importing configuration from %s to %s\n", argv[3], argv[2]);
		cfg = bde_config_parse(argv[2]);
		change_no = bde_config_update(cfg, argv[3]);
		if (change_no)
		{
			bde_config_write(cfg, argv[2]);
			printf_s("%i change(s) applied to %s\n", change_no, argv[2]);
		}
		else
			printf_s("No changes required\n");
		break;

	case 'e':
	case 'E':
		printf_s("Exporting configuration from %s to %s\n", argv[2], argv[3]);
		cfg = bde_config_parse(argv[2]);
		bde_config_export(cfg, argv[3]);
		break;

	default:
		printf_s("Error in main(): invalid switch. Type bdecli for usage.\n");
		return 2;
	}

	bde_config_free(cfg);
	return 0;
}