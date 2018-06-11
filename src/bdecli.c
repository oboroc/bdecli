/* bdecli - Borland Database Engine command-line interface.
 * 
 * Copyright (C) 2010, 2016 Adrian Oboroc
 *  
 * This file is part of bdecli project
 *
 *     https://github.com/oboroc/bdecli/
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


#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN

#include <memory.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "compat_s.h"

#define BDECLI_VER "1.0.4"
#define MAX_BUFFER 1000
#define BDE_NUL 0
#define BDE_VAR 1
#define BDE_EOB 2
#define BDE_NEW 3
#define BDE_EQU 4


typedef enum bde_entry_type
{
	BDE_UNDEFINED = 0,
	BDE_CONTAINER = 1,
	BDE_VARIABLE = 2
} bde_entry_type;

typedef struct bde_entry_t
{
	bde_entry_type entry_type;							/* entry type */
	char *name, *value;									/* entry name and value */
	struct bde_entry_t *next, *previous, *container;	/* linked list and container pointers */
} bde_entry_t;


char *bde_fgets(FILE *f)
{
	char *str;
	int ch;
	char buf[MAX_BUFFER];
	int r_len;

	ch = fgetc(f);

	/* we expect byte with value 0 */
	if (BDE_NUL != ch)
	{
		fprintf_s(stderr, "Error in bde_fgets(): leading %i expected, %i found\n", BDE_NUL, ch);
		exit(1);	/* fail */
	}

	/* initial length of return string */
	r_len = 0;

	do
	{
		/* build string in heap buffer */
		ch = fgetc(f);
		buf[r_len] = (char)ch;

		r_len++;

		/* check for potential buffer overflow */
		if (MAX_BUFFER == r_len)
		{
			fprintf_s(stderr, "Error in bde_fgets(): buffer overflow in string parser\n");
			exit(1);	/* fail */
		}

		/* check for unexpected end of file */
		if (feof(f))
		{
			fprintf_s(stderr, "Error in bde_fgets(): unexpected end of file in string parser\n");
			exit(1);
		}

	} while ((BDE_NUL != ch));

	/* allocate memory for the return string */
	str = (char *)malloc(r_len);

	if (!str)
	{
		fprintf_s(stderr, "Error in bde_fgets(): failed to allocate %i bytes of memory for return string\n", r_len);
		exit(1);
	}

	strcpy_s(str, r_len, buf);	/* copy heap buffer to return string */

	return str;
}

bde_entry_t* bde_new_entry(bde_entry_t *prev, bde_entry_t *cont)
{
	bde_entry_t *entry;

	entry = (bde_entry_t*)malloc(sizeof(bde_entry_t));

	if (!entry)
	{
		fprintf_s(stderr, "Error in bde_new_entry(): failed to allocate %lu bytes of memory for linked list entry\n", (unsigned long)sizeof(bde_entry_t));
		exit(1);
	}

	memset(entry, 0, sizeof(bde_entry_t));	/* set everything to zero/null */

	entry->previous = prev;
	entry->container = cont;	/* by default, use container from previous linked list entry */

	if (prev)
		prev->next = entry;	/* set previous entry's next field to new entry */

	return entry;
}

bde_entry_t* bde_cfg_parse(char *szFileName)
{
	FILE *f;
	int err, ch;
	char *str, *value;
	bde_entry_t *start, *current;

	/* Initialize start and current pointers */
	start = bde_new_entry(NULL, NULL);
	current = start;
	
	err = fopen_s(&f, szFileName, "r");	/* open for read (will fail if file does not exist) */

	if ((0 != err) || (0 == f))
	{
		fprintf_s(stderr, "Error in bde_cfg_parse(): file \"%s\" was not opened\n", szFileName);
		exit(1);
	}

	ch = fgetc(f);

	while (!feof(f))
	{
		if (BDE_NEW != ch)
		{
			fprintf_s(stderr, "Error in bde_cfg_parse(): unexpected byte at the beginning of line. Expected %i, actual value is %i\n", BDE_NEW, ch);
			exit(1);
		}

		ch = fgetc(f);
		if (BDE_NUL != ch)
		{
			fprintf_s(stderr, "Error in bde_cfg_parse(): unexpected second byte at the beginning of line. Expected %i, actual value %i\n", BDE_NUL, ch); 
			exit(1);
		}


		ch = fgetc(f);	/* read block id from file */

		switch(ch)
		{

		case BDE_NUL:	/* 0x0000 - contrainer */

			str = bde_fgets(f);

			ch = fgetc(f);
			if (BDE_VAR != ch)
			{
				fprintf_s(stderr, "Error in bde_cfg_parse(): unexpected byte at the end of contrainer name. Expected %i, actual value is %i\n", BDE_VAR, ch);
				exit(1);
			}
					
			ch = fgetc(f);
			if (BDE_NUL != ch)
			{
				fprintf_s(stderr, "Error in bde_cfg_parse(): unexpected second byte after the end of contrainer name. Expected %i, actual value is %i\n", BDE_NUL, ch);
				exit(1);
			}

			current->entry_type = BDE_CONTAINER;
			current->name = str;
			current = bde_new_entry(current, current);	/* spawn a new linked list entry */

			break;

		case BDE_VAR:	/* 0x0001 - value */

			str = bde_fgets(f);

			ch = fgetc(f);
			if (BDE_EQU != ch)
			{
				fprintf_s(stderr, "Error in bde_cfg_parse(): unexpected byte between variable name and value. Expected %i, actual value %i\n", BDE_EQU, ch); 
				exit(1);
			}

			value = bde_fgets(f);

			if (!value)
			{
				fprintf_s(stderr, "Error in bde_cfg_parse(): failed to parse the value of variable\n");
				exit(1);
			}

			current->entry_type = BDE_VARIABLE;
			current->name = str;
			current->value = value;

			current = bde_new_entry(current, current->container);	/* spawn a new linked list entry */

			break;

		default:

			fprintf_s(stderr, "Error in bde_cfg_parse(): unexpected byte in main parser loop: %i", ch);
			exit(1);

		}

		/* parse the end of line */
		do {
			ch = fgetc(f);

			if (BDE_EOB == ch)
			{
				/* end of contrainer, go to upper level container */
				if (current->container)
					current->container = current->container->container;	

				/* check for padding zero byte */
				ch = fgetc(f);

				if (BDE_NUL != ch)
				{
					fprintf_s(stderr, "Error in bde_cfg_parse(): unexpected byte at the end of line. Expected %i, actual value is %i\n", BDE_NUL, ch);
					exit(1);
				}
			}
		} while ((!feof(f)) && (BDE_NEW != ch));
	};

	if (!current->name)	/* sanitize last preallocated linked list entry */
	{
		current->previous->next = NULL;	/* set the next field of previous linked list entry to null */
		free(current);
	}

	/* close stream */
	if ((f) && (fclose(f)))
		fprintf_s(stderr, "Error in bde_cfg_parse(): the file was not closed\n");

	return start;
}

/* build and return a fully qualified hierarchical name of entry */
char* bde_fqn(bde_entry_t *entry)
{
	bde_entry_t *current;
	char *str;
	size_t str_len;
	int str_pos, cur_pos;

	current = entry;
	str_len = 0;
	while (current)
	{
		str_len += strlen(current->name);

		if (current != entry)
			str_len++;	/* add '\\' length */

		current = current->container;
	}

	str = (char *)malloc(str_len + 1);

	if (NULL == str)
	{
		fprintf_s(stderr, "Error in bde_fqn(): failed to allocate %lu bytes for return string\n", (unsigned long)str_len);
		exit(1);
	}

	str[str_len] = 0;	/* terminate string with \0 */

	/* build fully qualified path string for entry */
	current = entry;
	str_pos = (int)(str_len - 1);
	while (current)
	{
		if (current != entry)
		{
			str[str_pos] = '\\';	/* add delimiter */
			str_pos--;
		}

		cur_pos = (int)(strlen(current->name) - 1);

		while (0 <= cur_pos)
		{
			str[str_pos] = current->name[cur_pos];
			str_pos--;
			cur_pos--;
		}
		
		current = current->container;
	}

	return str;
}

int bde_containers(bde_entry_t *entry)
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


void bde_cfg_export(bde_entry_t *list, char *szFileName)
{
	FILE *f;
	int err;
	bde_entry_t *current;
	char *str;

	/* Initialize current pointer */
	current = list;
	
	err = fopen_s(&f, szFileName, "w");	/* open for writing (will overwrite if file exists) */

	if ((0 != err) || (NULL == f))
	{
		fprintf_s(stderr, "Error in bde_cfg_export(): file \"%s\" was not opened\n", szFileName);
		exit(1);
	}

	while (current)
	{
		if (BDE_VARIABLE == current->entry_type)
		{
			str = bde_fqn(current);

			fprintf_s(f, "%s = %s\n", str, current->value);

			free(str);
		}

		current = current->next;
	}

	/* close stream */
	if ((f) && (fclose(f)))
		fprintf_s(stderr, "Error in bde_cfg_export(): the file was not closed\n");
}


void bde_cfg_free(bde_entry_t *list)	/* free all memory allocated for linked list */
{
	bde_entry_t *item;

	item = list;

	while (item)
	{
		bde_entry_t *tmp;

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
void bde_fputs(char *str, FILE *f)
{
	if (!str)
	{
		fprintf_s(stderr, "Error in bde_fputs(): string pointer is NULL\n");
		return;
	}

	fputc(BDE_NUL, f);	/* write lead zero */
	fputs(str, f);		/* write the string */
	fputc(BDE_NUL, f);	/* write trail zero */
}


/* write from linked list to BDE configuration file */
void bde_cfg_write(bde_entry_t *list, char *szFileName)
{
	FILE *f;
	int err, ld, eob;
	bde_entry_t *current, *tmp_c, *tmp_n;

	/* Initialize current pointer */
	current = list;
	
	err = fopen_s(&f, szFileName, "w");	/* open for writing (will overwrite if file exists) */

	if (0 != err)
	{
		fprintf_s(stderr, "Error in bde_cfg_write(): file \"%s\" was not opened\n", szFileName);
		exit(1);
	}

	while (current)
	{
		/* start new line */
		fputc(BDE_NEW, f);
		fputc(BDE_NUL, f);

		switch(current->entry_type)
		{
		case BDE_CONTAINER:
			/* leading zero */
			fputc(BDE_NUL, f);

			/* put container name */
			bde_fputs(current->name, f);

			/* trail with 1 and 0 */
			fputc(BDE_VAR, f);
			fputc(BDE_NUL, f);

			break;

		case BDE_VARIABLE:
			/* leading 1 */
			fputc(BDE_VAR, f);

			/* put variable name */
			bde_fputs(current->name, f);

			/* put 'equals' */
			fputc(BDE_EQU, f);

			/* put variable value */
			bde_fputs(current->value, f);

			break;

		default:
			fprintf_s(stderr, "Error in bde_cfg_write(): invalid list entry. Expected %i or %i, actual value is %i\n",
				BDE_CONTAINER, BDE_VARIABLE, current->entry_type);
			exit(1);
		}

		/* check if we should put the 'end of container' byte sequence */
		if ((current->next) &&						/* there is a next entry */
			(BDE_VARIABLE == current->entry_type) &&		/* current entry is a variable */
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
				fputc(BDE_EOB, f);
				fputc(BDE_NUL, f);
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
				fputc(BDE_EOB, f);
				fputc(BDE_NUL, f);
				eob--;
			}
		}

		/* iterate to next item in list */
		current = current->next;
	}

	/* close stream */
	if ((f) && (fclose(f)))
		fprintf_s(stderr, "Error in bde_cfg_write(): the file was not closed\n");
}


/* bde_contains() returns 1 if container is a parent of entry, 0 otherwise */
int bde_contains(bde_entry_t *co, bde_entry_t *entry)
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
 *	bde_cfg_add_entry() checks if proper container exists, if not, creates it.
 *	within that container, it looks for variable with supplied name, if it exists and
 *	has different value, bde_cfg_add_entry() will update it. If variable doesn't exist
 *	bde_cfg_add_entry() will create it. Return 1 if variable was created or modified,
 *	0 otherwise.
*/
int bde_cfg_add_entry(bde_entry_t *list, char *szFQNPath, char *szName, char *szValue)
{
	bde_entry_t *current, *parcon, *co, *tmp_e;
	char buf[MAX_BUFFER], *tmp;
	size_t i, buflen;

	if (BDE_CONTAINER != list->entry_type)	/* sanitize list */
	{
		fprintf_s(stderr, "Error in bde_cfg_add_entry(): first entry in linked list is not a container\n");
		exit(1);
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
				fprintf_s(stderr, "Error in bde_cfg_add_entry(): string buffer is too small");
				exit(1);
			}

			if ('\\' == tmp[i])
			{
				strncpy_s(buf, MAX_BUFFER, tmp, i);
				buf[i] = 0;
			}
			else
				strcpy_s(buf, MAX_BUFFER, tmp);

			co = NULL;
			current = parcon ? parcon->next : list;

			/* iterate through the linked list, looking for a container with
			specific name and same parent container */
			while (current)
			{
				if ((BDE_CONTAINER == current->entry_type) &&
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
				if (!current)
				{
					fprintf_s(stderr, "Error in bde_cfg_add_entry(): parent container is NULL\n");
					exit(1);
				}
				while ((current->next) && (bde_contains(parcon, current->next)))
					current = current->next;
				tmp_e = current->next;

				co = bde_new_entry(current, parcon);	/* create new entry */
				if (!co)
				{
					fprintf_s(stderr, "Error in bde_cfg_add_entry(): malloc() failed while creating new container\n");
					exit(1);
				}

				co->entry_type = BDE_CONTAINER;

				co->name = (char *)malloc(strlen(buf) + 1);	/* allocate memory for name */
				if (!co->name)
				{
					fprintf_s(stderr, "Error in bde_cfg_add_entry(): malloc() failed while creating new container name\n");
					exit(1);
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

	/* validate that container is not NULL */
	if (NULL == co)
	{
		fprintf_s(stderr, "Error in bde_cfg_add_entry(): container is NULL\n");
		exit(1);
	}

	/* we have proper container, now we should modify or add variable */
	current = co->next;
	while ((current) && (current->container == co))
	{
		if ((BDE_VARIABLE == current->entry_type) &&
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
						fprintf_s(stderr, "Error in bde_cfg_add_entry(): realloc() failed\n");
						exit(1);
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
		fprintf_s(stderr, "Error in bde_cfg_add_entry(): malloc() failed while creating new variable\n");
		exit(1);
	}

	current->entry_type = BDE_VARIABLE;

	current->name = (char *)malloc(strlen(szName) + 1);	/* allocate memory for name */
	if (!current->name)
	{
		fprintf_s(stderr, "Error in bde_cfg_add_entry(): malloc() failed while creating new variable name\n");
		exit(1);
	}
	strcpy_s(current->name, strlen(szName) + 1, szName);

	current->value = (char *)malloc(strlen(szValue) + 1);	/* allocate memory for value */
	if (!current->value)
	{
		fprintf_s(stderr, "Error in bde_cfg_add_entry(): malloc() failed while creating new variable value\n");
		exit(1);
	}
	strcpy_s(current->value, strlen(szValue) + 1, szValue);

	/* relink the list */
	current->next = tmp_e;
	if (tmp_e)
		tmp_e->previous = current;

	return 1;
}

/*
 *	bde_cfg_update() parses text file one line at a time,
 *	extracts FQN path, variable name and variable value and
 *	calls bde_cfg_add_entry() to update linked list, if necessary.
 *	 bde_cfg_update() returns a total number of updated and added entries.
 */
int bde_cfg_update(bde_entry_t *list, char *szFileName)
{	
	FILE *f;
	int err, i, change_no = 0;
	char buf[MAX_BUFFER], *szFQNPath, *szName, *szValue;

	err = fopen_s(&f, szFileName, "r");	/* open for reading */

	if (0 != err)
	{
		fprintf_s(stderr, "Error in bde_cfg_export(): file \"%s\" was not opened\n", szFileName);
		exit(1);
	}

	while (!feof(f))
	{
		if (!fgets(buf, MAX_BUFFER - 1, f))
			/* we get here if fgets failed due to error or end of file */
			if (ferror(f))	/* check for error */
			{
				fprintf_s(stderr, "Error in bde_cfg_export(): fgets() failed while reading from \"%s\"\n", szFileName);
				exit(1);
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
			fprintf_s(stderr, "Error in bde_cfg_export(): no '=' in line %s from file %s\n", buf, szFileName);
			exit(1);
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
			fprintf_s(stderr, "Error in bde_cfg_export(): failed to find '\\' in '%s'\n", szFQNPath);
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
		change_no += bde_cfg_add_entry(list, szFQNPath, szName, szValue);
	}

	/* close stream */
	if ((f) && (fclose(f)))
		fprintf_s(stderr, "Error in bde_cfg_export(): the file was not closed\n");

	return change_no;
}


int main(int argc, char *argv[])
{
	bde_entry_t *cfg;
	int change_no;
	cfg = NULL;

	if (1 == argc)
	{
		printf_s("bdecli %s (c) 2010, 2016 Adrian Oboroc https://github.com/oboroc/bdecli/ (%s)\n\n", BDECLI_VER, __DATE__);
		printf_s(
			"Export settings from idapi32.cfg: bdecli -e idapi32.cfg output.txt\n"
			"Edit the text file, leaving only settings relevant to deployed application.\n"
			"Import settings from edited text file: bdecli -i idapi32.cfg input.txt\n"
			);
		return 1;
	}

	if (4 != argc)
	{
		fprintf_s(stderr, "Error in main(): wrong number of parameters. Type bdecli for usage.\n");
		return 2;
	}

	if (argv[1][0] != '-')
	{
		fprintf_s(stderr, "Error in main(): invalid switch. Type bdecli for usage.\n");
		return 2;
	}

	switch(argv[1][1])
	{
	case 'i':
	case 'I':
		printf_s("Importing configuration from %s to %s\n", argv[3], argv[2]);
		cfg = bde_cfg_parse(argv[2]);
		change_no = bde_cfg_update(cfg, argv[3]);
		if (change_no)
		{
			bde_cfg_write(cfg, argv[2]);
			printf_s("%i change(s) applied to %s\n", change_no, argv[2]);
		}
		else
			printf_s("No changes required\n");
		break;

	case 'e':
	case 'E':
		printf_s("Exporting configuration from %s to %s\n", argv[2], argv[3]);
		cfg = bde_cfg_parse(argv[2]);
		bde_cfg_export(cfg, argv[3]);
		break;

	default:
		fprintf_s(stderr, "Error in main(): invalid switch. Type bdecli for usage.\n");
		return 2;
	}

	bde_cfg_free(cfg);
	return 0;
}
