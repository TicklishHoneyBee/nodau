#ifndef _NODAU_H
#define _NODAU_H 1

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* mmm unix */
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <libgen.h>
#include <errno.h>
#include <limits.h>

/* openssl */
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/des.h>

/* database */
#include <sqlite3.h>

/* for accessing old database */
#define OROW(__row) ((__row+1)*3)
#define OCOLUMN(__row,__col) (OROW(__row)+__col)
/* and the new one */
#define ROW(__row) ((__row+1)*4)
#define COLUMN(__row,__col) (ROW(__row)+__col)

typedef struct {
	int num_rows;
	int num_cols;
	char** data;
} sql_result;

enum {
	COL_NAME,
	COL_DATE,
	COL_TEXT,
	COL_CRYPT
};

sqlite3 *db;
char *error_msg;

/* defined in db.c */
int db_connect(void);
int db_update(char* name, char* value);
int db_list(char* search);
int db_edit(char* search);
int db_append(char* search);
int db_show(char* search);
int db_del(char* search);
int db_new(char* search);
int db_encrypt(char* search);
int db_decrypt(char* search);
sql_result *db_result_alloc(void);
int db_result_free(sql_result *result);

/* defined in time.c */
unsigned int gettime(char* str);

/* defined in lib.c */
void create_datemask(void);
int dir_create(char* p);

/* defined in edit.c */
int edit_stdin(char* name, char* date, char* data, int append);
int edit(char* name, char* date, char* data);

/* defined in crypto.c */
extern char* crypt_key;
char* crypt_get_key(void);
char* note_encrypt(char* data, char* key);
char* note_decrypt(char* data, char* key);

/* defined in config.c */
void config_load(void);
void config_save(void);
char* config_read(char* name, char* value);
char* config_write(char* name, char* value);

#endif
