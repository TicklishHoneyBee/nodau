/************************************************************************
* config.c
* nodau console note taker
* Copyright (C) Lisa Milne 2012 <lisa@ltmnet.com>
*
* lib.c is free software: you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* lib.c is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program.  If not, see <http://www.gnu.org/licenses/>
************************************************************************/

#include "nodau.h"

typedef struct config_s {
	char* name;
	char* value;
	struct config_s *next;
} config_t;

static int config_written = 0;
static config_t *config = NULL;

static config_t *config_get(char* name)
{
	config_t *c = config;
	config_t *l = NULL;
	while (c) {
		if (!strcmp(c->name,name))
			return c;
		l = c;
		c = c->next;
	}

	c = malloc(sizeof(config_t));
	c->name = strdup(name);
	c->value = NULL;
	c->next = NULL;

	if (l) {
		l->next = c;
	}else{
		config = c;
	}

	return c;
}

/* get the file pointer for the nodau config file */
static FILE *config_file(char* mode)
{
	char* f;
	char* xch;
	char fl[PATH_MAX];

	f = getenv("HOME");
	xch = getenv("XDG_CONFIG_HOME");

	/* use XDG config directory */
	if (!xch || !xch[0]) {
		sprintf(fl,"%s/.config/nodau",f);
	}else{
		sprintf(fl,"%s/nodau",xch);
	}

	dir_create(fl);

	strcat(fl,"/nodau.conf");

	return fopen(fl,mode);
}

/* load configuration */
void config_load()
{
	char* fb;
	int l;
	char* n;
	char* v;
	char* e;
	char* t;
	FILE *f = config_file("r");
	if (!f)
		return;

	fseek(f, 0, SEEK_END);
	l = ftell(f);
	fseek(f, 0, SEEK_SET);

	fb = alloca(l+1);
	if (l != fread(fb, 1, l, f)) {
		fputs("Failed to read config file\n",stderr);
		fclose(f);
		return;
	}

	fb[l] = 0;

	n = fb;
	while (n) {
		e = strchr(n,'\n');
		if (e)
			*e = 0;
		if (n[0] != '#' && (!e || e+1 > n)) {
			v = strchr(n,'=');
			if (v) {
				t = v;
				*v = 0;
				v++;
				while (isspace(*v)) {
					v++;
				}
				t--;
				while (isspace(*t)) {
					t--;
				}
				*(t+1) = 0;
				if (e) {
					t = e;
					t--;
					while (isspace(*t)) {
						t--;
					}
					*(t+1) = 0;
				}

				config_write(n,v);
			}
		}
		n = e;
		if (n)
			n++;
	}

	config_written = 0;

	fclose(f);
}

/* save configuration */
void config_save()
{
	FILE *f;
	config_t *c;
	if (!config_written)
		return;

	f = config_file("w");
	if (!f)
		return;

	fputs(
		"# Nodau config file\n"
		"# Autowritten by Nodau\n\n",
		f
	);

	c = config;
	while (c) {
		if (c->value) {
			fprintf(f,"%s = %s\n",c->name,c->value);
		}
		c = c->next;
	}

	fclose(f);
}

/* read a config setting
 * if value is not NULL, compare with the set value, return value on
 *  match or NULL
 * if value is NULL, return the set value */
char* config_read(char* name, char* value)
{
	config_t *c = config_get(name);
	if (!value)
		return c->value;

	if (c->value && !strcmp(c->value,value))
		return value;

	return NULL;
}

/* write a config setting */
char* config_write(char* name, char* value)
{
	config_t *c = config_get(name);

	if (c->value)
		free(c->value);

	if (value) {
		c->value = strdup(value);
	}else{
		c->value = NULL;
	}

	config_written = 1;

	return c->value;
}
