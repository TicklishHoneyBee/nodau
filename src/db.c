/************************************************************************
* db.c
* nodau console note taker
* Copyright (C) Lisa Milne 2010-2012 <lisa@ltmnet.com>
*
* db.c is free software: you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* db.c is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program.  If not, see <http://www.gnu.org/licenses/>
************************************************************************/

#include <stdarg.h>
/* so that getdate() works */
#define _XOPEN_SOURCE 500
#include <time.h>

#include "nodau.h"

/* convert a db string to a date string */
static char* db_gettime(char* d)
{
	time_t date = (time_t)atoi(d);
	struct tm *timeinfo = localtime(&date);
	char* tmp = asctime(timeinfo);
	tmp[strlen(tmp)-1] = '\0';
	return tmp;
}

/* convert a date string to a stamp */
static unsigned int db_getstamp(char* d)
{
	struct tm *timeinfo;
	/* if string is now, get current time */
	if (strcmp(d,"now") == 0) {
		return (unsigned int)time(NULL);
	}

	/* check datmask is set, if not create a temporary mask file */
	if (getenv("DATEMSK") == 0) {
		create_datemask();
	}

	/* get the stamp from the string */
	timeinfo = getdate(d);

	/* null means something went wrong, so print an error and return 'now' */
	if (timeinfo == NULL) {
		fprintf(stderr,"invalid date format\n");
		return db_getstamp("now");
	}

	/* convert the tm struct to a time_t */
	return mktime(timeinfo);
}

/* create the nodau table if it doesn't exist */
static int db_check()
{
	sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS nodau(name VARCHAR(10), date INTEGER UNSIGNED, text VARCHAR(255), encrypted BOOLEAN DEFAULT 'false')", NULL, 0, &error_msg);

	return 0;
}

/* connect to the database */
int db_connect()
{
	int c;
	char* f;
	char* xdh;
	char fl[PATH_MAX];
	error_msg = NULL;

	f = getenv("HOME");
	xdh = getenv("XDG_DATA_HOME");

	/* use XDG data directory for storing the database */
	if (!xdh || !xdh[0]) {
		sprintf(fl,"%s/.local/share/nodau",f);
	}else{
		sprintf(fl,"%s/nodau",xdh);
	}

	dir_create(fl);

	strcat(fl,"/nodau.db");

	/* connect */
	c = sqlite3_open_v2(fl, &db, SQLITE_OPEN_READWRITE  | SQLITE_OPEN_CREATE, NULL);

	/* TODO: import from old database file */

	/* check for an error */
	if (c)
		return 1;

	/* check the table exists and return */
	return db_check();
}

/* create a result struct */
sql_result *db_result_alloc()
{
	/* malloc space */
	sql_result *res = malloc(sizeof(sql_result));

	/* null means error and return */
	if (res == NULL) {
		fprintf(stderr,"allocation failure\n");
		return NULL;
	}

	/* set some default values */
	res->num_cols = 0;
	res->num_rows = 0;
	res->data = NULL;

	/* return the struct */
	return res;
}

/* free a result struct */
int db_result_free(sql_result *result)
{
	/* if null do nothing */
	if (result == NULL)
		return 1;

	/* if there's data free it */
	if (result->data != NULL) {
		sqlite3_free_table(result->data);
	}

	/* free the struct */
	free(result);

	/* done */
	return 0;
}

/* get results from the database */
static sql_result *db_get(char* sql,...)
{
	/* temp storage area */
	sql_result *result;
	char dtmp[512];

	/* insert variable args to the sql statement */
	va_list ap;
	va_start(ap, sql);
	vsnprintf(dtmp, 512, sql, ap);
	va_end(ap);

	/* get a result struct */
	result = db_result_alloc();

	/* null result, return null */
	if (result == NULL)
		return NULL;

	/* run the query, store the results in the result struct */
	sqlite3_get_table(db, dtmp, &result->data, &result->num_rows, &result->num_cols, &error_msg);

	/* return the struct */
	return result;
}

/* insert a new note */
static int db_insert(char* name, char* value)
{
	/* somewhere to put the sql */
	char sql[512];

	/* get the current time */
	unsigned int date = (unsigned int)time(NULL);

	/* create the sql statement using the name/date/text for this note */
	sprintf(sql, "INSERT INTO nodau values('%s','%u','%s', 'false')", name, date, value);

	/* do it */
	return sqlite3_exec(db, sql, NULL, 0, &error_msg);
}

/* update an existing note */
int db_update(char* name, char* value)
{
	/* create the sql statement using the name/text for this note */
	char sql[512];
	sprintf(sql, "UPDATE nodau set text='%s' WHERE name='%s'", value, name);

	/* do it */
	return sqlite3_exec(db, sql, NULL, 0, &error_msg);
}

/* list notes according to search criteria */
void db_list(char* search)
{
	sql_result *result;
	int i;
	char* pref = "match";

	/* if search is null, list all */
	if (search == NULL) {
		pref = "note";
		result = db_get("SELECT * FROM nodau");

		/* nothing there */
		if (result->num_rows == 0) {
			printf("No notes to list\n");
			db_result_free(result);
			return;
		}
	}else{
		/* first try a name search */
		result = db_get("SELECT * FROM nodau WHERE name LIKE '%%%s%%'",search);

		/* if there's nothing then try a time search */
		if (result->num_rows == 0) {
			unsigned int idate;
			db_result_free(result);
			/* at time */
			if (strncmp(search,"t@",2) == 0) {
				idate = db_getstamp(search+2);
				result = db_get("SELECT * FROM nodau WHERE date = %u", idate);
			/* after time */
			}else if (strncmp(search,"t+",2) == 0) {
				idate = db_getstamp(search+2);
				result = db_get("SELECT * FROM nodau WHERE date > %u", idate);
			/* before time */
			}else if (strncmp(search,"t-",2) == 0) {
				idate = db_getstamp(search+2);
				result = db_get("SELECT * FROM nodau WHERE date < %u", idate);
			}
		}
		/* nothing there */
		if (result->num_rows == 0) {
			printf("No notes match '%s'\n",search);
			db_result_free(result);
			return;
		}
	}

	/* print the list */
	for (i=0; i<result->num_rows; i++) {
		printf("%s %d: %s\n",pref,i+1,result->data[COLUMN(i,COL_NAME)]);
	}

	/* free the result */
	db_result_free(result);
}

/* open an existing note */
void db_edit(char* search)
{
	char* date;
	char* name;
	/* get the note by name */
	sql_result *result;
	result = db_get("SELECT * FROM nodau WHERE name = '%s'",search);

	/* nothing there */
	if (result->num_rows == 0) {
		printf("No notes match '%s'\n",search);
		db_result_free(result);
		return;
	}

	/* get the data */
	date = db_gettime(result->data[COLUMN(0,COL_DATE)]);
	name = result->data[COLUMN(0,COL_NAME)];

	/* edit the note */
	edit(name, date, result->data[COLUMN(0,COL_TEXT)]);

	/* free the result */
	db_result_free(result);
}

/* show an existing note */
void db_show(char* search)
{
	char* date;
	char* name;
	char* text;
	/* get the note by name */
	sql_result *result;
	result = db_get("SELECT * FROM nodau WHERE name = '%s'",search);

	/* nothing there */
	if (result->num_rows == 0) {
		printf("No notes match '%s'\n",search);
		db_result_free(result);
		return;
	}

	/* get the data */
	date = db_gettime(result->data[COLUMN(0,COL_DATE)]);
	name = result->data[COLUMN(0,COL_NAME)];
	text = result->data[COLUMN(0,COL_TEXT)];

	/* display the note */
	printf("%s (%s):\n%s\n",name,date,text);

	/* free the result */
	db_result_free(result);
}

/* delete notes */
void db_del(char* search)
{
	char sql[512];
	unsigned int date = 0;
	/* try a name search */
	sql_result *result;
	result = db_get("SELECT * FROM nodau WHERE name = '%s'",search);

	/* if we got something, delete it */
	if (result->num_rows) {
		sprintf(sql, "DELETE FROM nodau WHERE name = '%s'", search);
	/* or try a delete by time at */
	}else if (strncmp(search,"t@",2) == 0) {
		date = db_getstamp(search+2);
		sprintf(sql, "DELETE FROM nodau WHERE date = %u", date);
	/* or try a delete by later than */
	}else if (strncmp(search,"t+",2) == 0) {
		date = db_getstamp(search+2);
		sprintf(sql, "DELETE FROM nodau WHERE date > %u", date);
	/* or try a delete by earlier than */
	}else if (strncmp(search,"t-",2) == 0) {
		date = db_getstamp(search+2);
		sprintf(sql, "DELETE FROM nodau WHERE date < %u", date);
	/* or print an error */
	}else{
		printf("No notes matches '%s'\n",search);
		return;
	}

	/* run the statement */
	sqlite3_exec(db, sql, NULL, 0, &error_msg);

	/* free the earlier result */
	db_result_free(result);
}

/* create a new note */
void db_new(char* search)
{
	/* search by name */
	sql_result *result;
	result = db_get("SELECT * FROM nodau WHERE name = '%s'",search);

	/* there's already a note with that name, so error and return */
	if (result->num_rows) {
		printf("There is already a note called '%s'\n",search);
		db_result_free(result);
		return;
	}

	/* free the search result */
	db_result_free(result);

	/* create the new entry */
	db_insert(search,"new entry");

	if (error_msg)
		printf("%s\n",error_msg);

	/* open for editing */
	db_edit(search);
}
