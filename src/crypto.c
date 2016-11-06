/************************************************************************
* encrypt.c
* nodau console note taker
* Copyright (C) Lisa Milne 2010-2013 <lisa@ltmnet.com>
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

#include "nodau.h"
#include <termios.h>

#if OPENSSL_VERSION_NUMBER < 0x10100000L
#include <string.h>
#include <openssl/engine.h>

static void *OPENSSL_zalloc(size_t num)
{
	void *ret = OPENSSL_malloc(num);

	if (ret != NULL)
		memset(ret, 0, num);
	return ret;
}

EVP_MD_CTX *EVP_MD_CTX_new(void)
{
	return OPENSSL_zalloc(sizeof(EVP_MD_CTX));
}

void EVP_MD_CTX_free(EVP_MD_CTX *ctx)
{
	EVP_MD_CTX_cleanup(ctx);
	OPENSSL_free(ctx);
}

#endif

char* crypt_key = NULL;

static char* md5(const void *content, int len)
{
	EVP_MD_CTX *mdctx;
	unsigned char md_value[EVP_MAX_MD_SIZE];
	unsigned int md_len;
	int i;
	static char r[33];
	char tmp[5];

	mdctx = EVP_MD_CTX_new();
	EVP_DigestInit(mdctx, EVP_md5());
	EVP_DigestUpdate(mdctx, content, (size_t) len);
	EVP_DigestFinal_ex(mdctx, md_value, &md_len);
	EVP_MD_CTX_free(mdctx);

	/* turn the hash into a hex string */
	r[0] = 0;
	for (i=0; i<md_len; i++) {
		sprintf(tmp,"%02X",md_value[i]);
		strcat(r,tmp);
	}

	return r;
}

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
	 * these next few lines are essentially getpass() - which we
	 * don't use because it's deprecated */
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
		crypt_key[i-1] = 0;
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

/* encrypt a string using a key - DES */
char* note_encrypt(char* data, char* key)
{
	char* r;
	char* d;
	char* m;
	int n = 0;
	int l = strlen(data);
	DES_cblock k;
	DES_key_schedule schedule;

	d = alloca(l+36);

	l += 4;
	r = alloca(l+36);

	m = md5(data,l);

	memcpy(k, key, 8);
	DES_set_odd_parity(&k);
	DES_set_key_checked(&k, &schedule);

	DES_cfb64_encrypt(
		(unsigned char *)data,
		(unsigned char *)r,
		l+4,
		&schedule,
		&k,
		&n,
		DES_ENCRYPT
	);

	/* here we store the length of the encrypted data,
	 * the md5 hash of the raw string
	 * and the encrypted data */
	memcpy(d,&l,4);
	memcpy(d+4,m,32);
	memcpy(d+36,r,l);

	return b64_encode((unsigned char *)d,l+36);
}

/* decrypt a string using a key - DES */
char* note_decrypt(char* data, char* key)
{
	char* r;
	char* d;
	char m[33];
	char* c;
	int n = 0;
	int l;
	DES_cblock k;
	DES_key_schedule schedule;

	d = b64_decode(data);

	/* extract the length of the encrypted data,
	 * and the decrypted string's md5 hash */
	l = *((int*)d);
	d += 4;
	memcpy(m,d,32);
	m[32] = 0;
	d += 32;

	r = alloca(l);

	memcpy(k, key, 8);
	DES_set_odd_parity(&k);
	DES_set_key_checked(&k, &schedule);

	DES_cfb64_encrypt(
		(unsigned char *)d,
		(unsigned char *)r,
		l,
		&schedule,
		&k,
		&n,
		DES_DECRYPT
	);

	memcpy(data, r, l);
	data[l] = 0;

	/* get a md5 hash of the decrypted data, if it doesn't match
	 * the original then the passphrase was wrong */
	c = md5(data,l);

	if (strcmp(m,c)) {
		fprintf(stderr,"Incorrect passphrase\n");
		return NULL;
	}

	return data;
}
