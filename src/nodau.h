#ifndef _NODAU_H
#define _NODAU_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sqlite3.h>

#define ROW(x) ((x+1)*3)
#define COLUMN(x,y) (ROW(x)+y)

typedef struct {
	int num_rows;
	int num_cols;
	char** data;
} sql_result;

enum {
	COL_NAME,
	COL_DATE,
	COL_TEXT
};

sqlite3 *db;
char *error_msg;

/* defined in db.c */
int db_connect(void);
int db_update(char* name, char* value);
void db_list(char* search);
void db_edit(char* search);
void db_show(char* search);
void db_del(char* search);
void db_new(char* search);
sql_result *db_result_alloc(void);
int db_result_free(sql_result *result);

/* defined in time.c */
unsigned int gettime(char* str);

/* defined in lib.c */
void create_datemask(void);

/* defined in edit.c */
void edit(char* name, char* date, char* data);

#endif
