/*-
 *
 */

#ifndef _BSDIFF_H_
#define _BSDIFF_H_

//# include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

struct bsdiff_stream
{
	void* opaque;

	void* (*malloc)(size_t size);
	void (*free)(void* ptr);
	int (*write)(struct bsdiff_stream* stream, const void* buffer, int size);
};

struct bsdiff_request
{
	const uint8_t* old;
	int64_t oldsize;
	const uint8_t* new1;
	int64_t newsize;
	struct bsdiff_stream* stream;
	int64_t *I;
	uint8_t *buffer;
};

int bsdiff(const uint8_t* old, int64_t oldsize, const uint8_t* new1, int64_t newsize, struct bsdiff_stream* stream);
int bsdiffImp(char *oldFileName, char *diffFileName, char *newFileName);

void split(int64_t *I,int64_t *V,int64_t start,int64_t len,int64_t h);
void qsufsort(int64_t *I,int64_t *V,const uint8_t *old,int64_t oldsize);
int64_t matchlen(const uint8_t *old,int64_t oldsize,const uint8_t *new1,int64_t newsize);
int64_t search(const int64_t *I,const uint8_t *old,int64_t oldsize,const uint8_t *new1,int64_t newsize,int64_t st,int64_t en,int64_t *pos);
void offtout(int64_t x,uint8_t *buf);
int64_t writedata(struct bsdiff_stream* stream, const void* buffer, int64_t length);
int bsdiff_internal(const struct bsdiff_request req);
int bsd_write(struct bsdiff_stream* stream, const void* buffer, int size);
static int64_t offtin(uint8_t *buf)
{
	int64_t y;

	y=buf[7]&0x7F;
	y=y*256;y+=buf[6];
	y=y*256;y+=buf[5];
	y=y*256;y+=buf[4];
	y=y*256;y+=buf[3];
	y=y*256;y+=buf[2];
	y=y*256;y+=buf[1];
	y=y*256;y+=buf[0];

	if(buf[7]&0x80) y=-y;

	return y;
}
#endif
