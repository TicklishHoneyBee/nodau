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
* You should have received a copy of the GNU General Public License along
* with this program.  If not, see <http://www.gnu.org/licenses/>
************************************************************************/

#include "nodau.h"

/* create a temporary datemask file and set DATEMSK so getdate() works */
void create_datemask()
{
	FILE *dm;
	char dmfn[PATH_MAX];
	/* get the users home directory */
	char* home = getenv("HOME");
	/* create the filename */
	sprintf(dmfn,"%s/.datemask",home);

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
}
