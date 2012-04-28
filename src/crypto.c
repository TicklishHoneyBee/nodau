/************************************************************************
* encrypt.c
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
#include <termios.h>

char* crypt_key = NULL;

/* get the encryption key, either from the user, or from crypt_key */
char* crypt_get_key()
{
	struct termios old;
	struct termios new;
	size_t i;
	if (crypt_key)
		return crypt_key;

	printf("Passphrase: ");
	fflush(stdout);

	/* Don't echo the password to the console
	 * this is essentially getpass() - which we don't use because
	 * it's deprecated */
	if (tcgetattr (fileno (stdin), &old) != 0)
		return NULL;

	new = old;
	new.c_lflag &= ~ECHO;

	if (tcsetattr (fileno(stdin), TCSAFLUSH, &new) != 0)
		return NULL;

	i = getline(&crypt_key, &i, stdin);

	(void)tcsetattr(fileno(stdin), TCSAFLUSH, &old);

	if (i < 0) {
		crypt_key = NULL;
	}else if (crypt_key[i-1] == '\n') {
		crypt_key[i-1] = '\0';
	}

	printf("\n");

	return crypt_key;
}

/* base64 encode a binary value of length */
char* b64_encode(const unsigned char* input, int length)
{
	BIO *bmem, *b64;
	BUF_MEM *bptr;
	char* buff;

	b64 = BIO_new(BIO_f_base64());
	BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
	bmem = BIO_new(BIO_s_mem());
	b64 = BIO_push(b64, bmem);
	BIO_write(b64, input, length);
	(void)BIO_flush(b64);
	BIO_get_mem_ptr(b64, &bptr);

	buff = malloc(bptr->length+1);
	memcpy(buff, bptr->data, bptr->length);
	buff[bptr->length] = 0;

	BIO_free_all(b64);

	return buff;
}

/* decode a base64 string to a binary value */
char* b64_decode(char* str)
{
	BIO *b64, *bmem;
	char *buff;
	int l = strlen(str);

	buff = malloc(l);
	memset(buff,0,l);

	b64 = BIO_new(BIO_f_base64());
	BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
	bmem = BIO_new_mem_buf(str, l);
	bmem = BIO_push(b64, bmem);

	BIO_read(bmem, buff, l);

	BIO_free_all(bmem);

	return buff;
}

/* TODO: use hashing or something to check for a correctly decrypted string */

/* encrypt a string using a key - DES */
char* note_encrypt(char* data, char* key)
{
	char* r;
	char* d;
	int n = 0;
	int l = strlen(data);
	DES_cblock k;
	DES_key_schedule schedule;

	d = alloca(l+4);

	l += 4;
	r = alloca(l+4);

	memcpy(k, key, 8);
	DES_set_odd_parity(&k);
	DES_set_key_checked(&k, &schedule);

	DES_cfb64_encrypt((unsigned char *)data, (unsigned char *)r, l+4, &schedule, &k, &n, DES_ENCRYPT);

	memcpy(d,&l,4);
	memcpy(d+4,r,l);

	return b64_encode((unsigned char *)d,l+4);
}

/* decrypt a string using a key - DES */
char* note_decrypt(char* data, char* key)
{
	char* r;
	char* d;
	int n = 0;
	int l;
	DES_cblock k;
	DES_key_schedule schedule;

	d = b64_decode(data);

	l = *((int*)d);
	d += 4;

	r = alloca(l);

	memcpy(k, key, 8);
	DES_set_odd_parity(&k);
	DES_set_key_checked(&k, &schedule);

	DES_cfb64_encrypt((unsigned char *)d, (unsigned char *)r, l, &schedule, &k, &n, DES_DECRYPT);

	memcpy(data, r, l);
	data[l] = 0;

	return data;
}
