/************************************************************************
* nodau.c
* nodau console note taker
* Copyright (C) Lisa Milne 2010-2012 <lisa@ltmnet.com>
*
* nodau.c is free software: you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* nodau.c is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program.  If not, see <http://www.gnu.org/licenses/>
************************************************************************/

#include "nodau.h"

/* print help and usage */
static void print_help(char* appname)
{
	printf("usage: %s <option> [data]\n",appname);
	puts("simple console notetaking\n\n"
	   "OPTIONS:\n"
	   " help            print this message\n"
	   " list [search]   list notes, accepts optional search term\n"
	   " new <name>      create new note, name must be unique\n"
	   " edit <name>     open an existing note for editing\n"
	   " show <name>     display an existing note\n"
	   " del <search>    accepts name or search term\n");
	puts("SEARCH TERM:\n"
	   " Search terms with spaces do not need to be inside \"quotes\"\n"
	   " <name>          name of a note, list will search for names\n"
	   "  similar to the term, del will delete only an exact match\n"
	   " t@<datestring>  matches notes created at a given date/time\n"
	   " t-<datestring>  matches notes created before a given date/time\n"
	   " t+<datestring>  matches notes created after a given date/time\n");
	puts( "DATE STRING:\n"
	   " datestring can be made of any typical date format such as:\n"
	   " dd/mm/yy\n"
	   " dd, mm, yyyy hh:mm\n");
	puts( "EDITING:\n"
	   " By default nodau will use the EDITOR environment variable to\n"
	   " edit notes, simply edit the file as usual, nodau will save the\n"
	   " note to it's database when you exit the editor. If EDITOR is not\n"
	   " set though the nodau builtin editor will be used.\n");
	puts(" The current editor accepts standard printable characters, enter,\n"
	   " and backspace. There is no support for moving the cursor with the\n"
	   " arrow keys or mouse. To exit the editor and save the note, create\n"
	   " a new line with only a dot (.) on it, or press escape.\n\n");

}

/* assemble arguments into a single argument, undoes the space splitting */
static char* get_args(int argc, char** argv)
{
	int i;
	char* args;
	int l = 0;

	/* get the compined length of the arguments */
	for (i=2; i<argc; i++) {
		l += strlen(argv[i]);
	}

	/* no arguments, return null */
	if (l == 0)
		return NULL;

	/* add spaces to the argument length */
	l += (i-1);

	/* create space for the argument */
	args = (char*)malloc(sizeof(char)*(l));
	/* if null throw an error */
	if (args == NULL) {
		fprintf(stderr,"an error occured in argument compilation\n");
		return NULL;
	}

	/* set up for joining */
	args[0] = 0;

	/* join the arguments */
	for (i=2; i<argc; i++) {
		strcat(args," ");
		strcat(args,argv[i]);
	}

	/* return the string */
	return args;
}

int main(int argc, char** argv)
{
	char* args;
	/* no option, print usage */
	if (argc < 2) {
		print_help(argv[0]);
		return 0;
	}

	/* connect to the db or error */
	if (db_connect()) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return 0;
	}

	/* compile the arguments */
	args = get_args(argc,argv);

	/* if listing notes */
	if (strcmp(argv[1],"list") == 0) {
		db_list(args);
	}else{
		/* if null argument print usage */
		if (args == NULL || argc <3) {
			print_help(argv[0]);
		/* if creating a new note */
		}else if (strcmp(argv[1],"new") == 0) {
			db_new(args);
		/* if opening/editing an existing note */
		}else if (strcmp(argv[1],"open") == 0 || strcmp(argv[1],"edit") == 0) {
			db_edit(args);
		/* display an existing note */
		}else if (strcmp(argv[1],"show") == 0) {
			db_show(args);
		/* if deleting note/s */
		}else if (strcmp(argv[1],"del") == 0) {
			db_del(args);
		/* unknown option, print usage */
		}else{
			print_help(argv[0]);
		}
	}

	/* free args if we can */
	if (args != NULL)
		free(args);

	/* close the database */
	sqlite3_close(db);

	return 0;
}
