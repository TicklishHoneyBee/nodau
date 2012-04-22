/************************************************************************
* edit.c
* nodau console note taker
* Copyright (C) Lisa Milne 2010-2012 <lisa@ltmnet.com>
*
* edit.c is free software: you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* edit.c is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program.  If not, see <http://www.gnu.org/licenses/>
************************************************************************/

#include <unistd.h>
#include <ncurses.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

#include "nodau.h"

/* local storage for note name and date */
static char* bname;
static char* bdate;

/* draw to the screen */
static void draw(char* data)
{
	/* clear the screen */
	clear();
	/* print the name and date in bold */
	attron(A_BOLD);
	printw("%s (%s):\n",bname,bdate);
	attroff(A_BOLD);
	/* print the note body */
	printw("%s",data);
	/* refresh the screen */
	refresh();
}

/* edit a note with the builtin editor */
static void edit_builtin(char* name, char* date, char* data)
{
	char buffer[256];
	int bl;
	/* still editing? */
	int quit = 0;
	/* character storage */
	int plch = 0;
	int lch = 0;
	int ch;

	/* set the local data */
	bname = name;
	bdate = date;
	/* find the buffer length */
	bl = strlen(data);
	/* create the buffer */
	/* fill the buffer with 0's */
	memset(&buffer,0,256);

	/* if the note is too long, shorten it */
	if (bl > 255) {
		data[255] = 0;
		bl = 255;
	}

	/* put the note into the buffer */
	sprintf(buffer, "%s", data);

	/* init ncurses */
	initscr();
	/* no line buffering */
	cbreak();
	/* get all the keys */
	keypad(stdscr, TRUE);
	/* don't echo keypresses */
	noecho();

	/* while we are editing */
	while (!quit) {
		/* draw the screen */
		draw(buffer);
		/* set previous last char to last char */
		plch = lch;
		/* set last char to char */
		lch = ch;
		/* get char */
		ch = getch();
		/* if it's printable or newline */
		if (isprint(ch) || ch == '\n') {
			bl++;
			/* if the note is under 255 chars, add the char */
			if (bl < 255) {
				buffer[bl-1] = ch;
				buffer[bl] = 0;
			}
		/* backspace means delete a char */
		}else if (ch == 127 || ch == KEY_BACKSPACE) {
			/* if we've got one to delete */
			if (bl > 0) {
				bl--;
				buffer[bl] = 0;
			}
		}

		/* check for newline dot exit */
		if (plch == '\n' && lch == '.' && ch == '\n') {
			/* don't include the dot in the note */
			bl -= 3;
			buffer[bl] = 0;
			quit = 1;
		/* check for escape exit */
		}else if (ch == 27) {
			quit = 1;
		}
	}

	/* exit curses */
	endwin();

	/* save the note */
	db_update(name,buffer);

	/* let the user know */
	printf("%s saved\n",name);
}

/* edit with an external editor */
int edit_ext(char* editor, char* name, char* date, char* data)
{
	FILE *f;
	int st;
	int sz;
	char* b;
	char* l;
	char buff[512];
	pid_t pid;
	sprintf(buff,"/tmp/nodau.%d",(int)time(NULL));

	pid = fork();

	if (pid < 0) {
		return 1;
	}else if (pid) {
		waitpid(pid,&st,0);
		if (!st) {
			if ((f = fopen(buff,"r")) == NULL)
				return 1;
			/* find the file length */
			fseek(f,0,SEEK_END);
			sz = ftell(f);
			fseek(f,0,SEEK_SET);
			if (sz) {
				/* load the note into memory */
				b = alloca(sz+1);
				fread(b,1,sz,f);
				fclose(f);
				/* delete the file */
				remove(buff);
				b[sz] = 0;
				/* find the note data */
				l = strstr(b,"-----");
				if (l) {
					/* save the note */
					l += 6;
					db_update(name,l);

					/* let the user know */
					printf("%s saved\n",name);
				}
			}
		}
		return st;
	}

	if ((f = fopen(buff,"w+")) == NULL)
		exit(1);

	/* insert data into file */
	fprintf(
		f,
		"%s (%s)\nText above this line is ignored\n-----\n%s",
		name,
		date,
		data
	);
	fflush(f);
	fclose(f);

	st = execl(editor,editor,buff,(char*)NULL);

	/* we should only ever get here if something goes wrong with exec */
	exit(st);

	/* and we shouldn't ever get here, but it stops the compiler complaining */
	return 1;
}

/* edit a note */
void edit(char* name, char* date, char* data)
{
	char* ed = getenv("EDITOR");
	char* pt = getenv("PATH");
	char editor[1024];
	char* p;
	struct stat st;

	/* no editor or no path, use builtin */
	if (!ed || !pt) {
		edit_builtin(name,date,data);
		return;
	}

	/* find the executable */
	p = strtok(pt,":");
	while (p) {
		p = strtok(NULL,":");
		sprintf(editor,"%s/%s",p,ed);
		stat(editor,&st);
		/* check it exists */
		if (S_ISREG(st.st_mode))
			break;
	}

	/* no executable, or fails to run, use builtin */
	if (!p || edit_ext(editor,name,date,data)) {
		edit_builtin(name,date,data);
		return;
	}
}
