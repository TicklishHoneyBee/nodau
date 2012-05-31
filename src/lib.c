/************************************************************************
* lib.c
* nodau console note taker
* Copyright (C) Lisa Milne 2010-2012 <lisa@ltmnet.com>
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
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>
*
* In addition, as a special exception, the copyright holder (Lisa Milne)
* gives permission to link the code of this release of nodau with the
* OpenSSL project's "OpenSSL" library (or with modified versions of it
* that use the same license as the "OpenSSL" library), and distribute
* the linked executables. You must obey the GNU General Public License
* in all respects for all of the code used other than "OpenSSL". If you
* modify this file, you may extend this exception to your version of the
* file, but you are not obligated to do so. If you do not wish to do so,
* delete this exception statement from your version.
************************************************************************/

/* so that asprintf works */
#define _GNU_SOURCE
#include "nodau.h"

/* create a temporary datemask file and set DATEMSK so getdate() works */
void create_datemask()
{
	FILE *dm;
	char* dmfn;
	/* get the users home directory */
	char* home = getenv("HOME");
	/* create the filename */
	asprintf(&dmfn,"%s/.datemask",home);

	/* try to open the file */
	dm = fopen(dmfn,"r");

	/* if it didn't open, we need to create the file */
	if (dm == NULL) {
		/* let the user know about it */
		printf(
			"\nSet environment variable DATEMSK to the file\n"
			"containing the date templates\n"
			"see 'man 3 getdate'.\n"
			"A temporary datemask file will be installed in %s\n\n",
			dmfn
		);

		/* create the file */
		dm = fopen(dmfn,"w+");

		/* or error */
		if (dm == NULL) {
			fprintf(stderr,"could not create temporary datemask\n");
			return;
		}

		/* insert some datemasks */
		fprintf(dm,
			"%%m\n"
			"%%A %%B %%d, %%Y, %%H:%%M:%%S\n"
			"%%A\n"
			"%%B\n"
			"%%m/%%d/%%y %%I %%p\n"
			"%%d/%%m/%%y\n"
			"%%d, %%m, %%Y %%H:%%M\n"
			"at %%A the %%dst of %%B in %%Y\n"
			"run job at %%I %%p, %%B %%dnd\n"
			"&A den %%d. %%B %%Y %%H.%%M Uhr\n"
		);
	}

	/* close the file */
	fclose(dm);

	/* set the DATEMSK environment variable */
	setenv("DATEMSK",dmfn,1);

	free(dmfn);
}

int dir_create(char* p)
{
	mode_t process_mask = umask(0);
	mode_t mode = S_IRWXU;
        char *q, *r = NULL, *path = NULL, *up = NULL;
        int ret = 1;
        if (!strcmp(p, ".") || !strcmp(p, "/")) {
		umask(process_mask);
		return 0;
	}

        if ((path = strdup(p)) == NULL)
                goto fail;

        if ((q = strdup(p)) == NULL)
                goto fail;

        if ((r = dirname(q)) == NULL)
                goto out;

        if ((up = strdup(r)) == NULL)
               goto fail;

        if ((dir_create(up) == -1) && (errno != EEXIST))
                goto out;

        if ((mkdir(path, mode) == -1) && (errno != EEXIST))
                ret = 1;
        else
                ret = 0;

out:
	umask(process_mask);
        if (up != NULL)
                free(up);
        free(q);
        free(path);
        return ret;

fail:
	umask(process_mask);
	return 1;
}
