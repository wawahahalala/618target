/*-
 * Copyright 2003-2005 Colin Percival
 * Copyright 2012 Matthew Endsley
 * All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted providing that the following conditions 
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <limits.h>
#include "bspatch.h"
#include <stdio.h>
#include <log.h>
#include <string.h>

static int64_t offtin(uint8_t *buf) {
	int64_t y;

	y = buf[7] & 0x7F;
	y = y * 256;
	y += buf[6];
	y = y * 256;
	y += buf[5];
	y = y * 256;
	y += buf[4];
	y = y * 256;
	y += buf[3];
	y = y * 256;
	y += buf[2];
	y = y * 256;
	y += buf[1];
	y = y * 256;
	y += buf[0];

	if (buf[7] & 0x80)
		y = -y;

	return y;
}

int bspatch(const uint8_t* old, int64_t oldsize, uint8_t* new1, int64_t newsize, struct bspatch_stream* stream,
		FILE *pf) {
	int num=0;
//	char mem[200];
//	int r = fread(mem, 1, 108, pf);
//	log_debug("mem %d", r);
	uint8_t buf[8];
	int64_t oldpos, newpos;
	//unused variable
	//int64_t yy[3];
	int64_t y, y1, y2;
	//unused variable
	//int64_t ctrlTmp = 0;
	int64_t i;
	//unused variable
	//FILE *tmp;
	oldpos = 0;
	newpos = 0;
//	memset((void *)ctrl, 0, sizeof(int64_t) * 3);
	while (newpos < newsize) {

		/* Read control data */
		//for(i=0;i<=2;i++) {
		if (fread(buf, 1, 8, pf) != 8) {
			log_debug("buf err");
			return -1;
		}

		y = buf[7] & 0x7F;
		y = y * 256;
		y += buf[6];
		y = y * 256;
		y += buf[5];
		y = y * 256;
		y += buf[4];
		y = y * 256;
		y += buf[3];
		y = y * 256;
		y += buf[2];
		y = y * 256;
		y += buf[1];
		y = y * 256;
		y += buf[0];

		if (buf[7] & 0x80)
			y = -y;

//		log_debug("y = %lld", y);

		if (fread(buf, 1, 8, pf) != 8) {
			log_debug("buf err");
			return -1;
		}

		y1 = buf[7] & 0x7F;
		y1 = y1 * 256;
		y1 += buf[6];
		y1 = y1 * 256;
		y1 += buf[5];
		y1 = y1 * 256;
		y1 += buf[4];
		y1 = y1 * 256;
		y1 += buf[3];
		y1 = y1 * 256;
		y1 += buf[2];
		y1 = y1 * 256;
		y1 += buf[1];
		y1 = y1 * 256;
		y1 += buf[0];

		if (buf[7] & 0x80)
			y1 = -y1;

//		log_debug("y = %lld", y);
//		log_debug("y1 = %lld", y1);

		if (fread(buf, 1, 8, pf) != 8) {
			log_debug("buf err");
			return -1;
		}

		y2 = buf[7] & 0x7F;
		y2 = y2 * 256;
		y2 += buf[6];
		y2 = y2 * 256;
		y2 += buf[5];
		y2 = y2 * 256;
		y2 += buf[4];
		y2 = y2 * 256;
		y2 += buf[3];
		y2 = y2 * 256;
		y2 += buf[2];
		y2 = y2 * 256;
		y2 += buf[1];
		y2 = y2 * 256;
		y2 += buf[0];

		if (buf[7] & 0x80)
			y2 = -y2;

		//log_debug("y = %lld", y);
		//log_debug("y2 = %lld", y2);

//		for(i=0;i<=2;i++) {
//					if (stream->read(stream, buf, 8))
//						return -1;
//					yy[i]=offtin(buf);
//					log_debug("yy [%d] = %d",i,yy[i]);
//				};

		/* Sanity-check */
		//log_debug("y = %lld", y);
		if (y < 0 || y > INT_MAX || y1 < 0 || y1 > INT_MAX || newpos + y > newsize)
			return -2;

		//log_debug("y = %lld", y);

		/* Read diff string */
		//log_debug("newpos = %lld , y = %lld", newpos, y);
		int ret;
		ret = fread(new1 + newpos, 1, y, pf);
		//log_debug("ret = %d", ret);
		if (ret != y)
		{

			if (feof(pf)) {
				log_debug("READ ERROR");
			} else if (ferror(pf)) {
				log_debug("reading error");
			} else {
				log_debug("data error");
			}
			return -3;
		}

		/* Add old data to diff string */
		for (i = 0; i < y; i++)
			if ((oldpos + i >= 0) && (oldpos + i < oldsize))
				new1[newpos + i] += old[oldpos + i];

		/* Adjust pointers */
		newpos += y;
		oldpos += y;

		/* Sanity-check */
		if (newpos + y2 > newsize)
			return -4;

		/* Read extra string */
//		if (stream->read(stream, new1 + newpos, y1))
//			return -5;
		ret = fread(new1+newpos,1,y1,pf);
		if (ret != y1)
		{
			if (feof(pf)) {
				log_debug("READ ERROR");
			} else if (ferror(pf)) {
				log_debug("reading error");
			} else {
				log_debug("data error");
			}
			return -3;
		}


		/* Adjust pointers */
		newpos += y1;
		//log_debug("newpos = %d",newpos);
		oldpos += y2;
		if(num == 3)
		{
			break;
		}
//		num++;
	};

	return 0;
}

//#include <bzlib.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
//#include <err.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "log.h"
#include "zlib.h"

static int bz2_read(const struct bspatch_stream* stream, void* buffer, int64_t length) {
	int n;
//	FILE* bz2;
	gzFile bz2;

	bz2 = (gzFile) stream->opaque;
	log_debug("length = %lld", length);
//	n = fread(buffer, 1, length, bz2);
	n = gzread(bz2,buffer,length);
	if (n != length) {
		log_error("gzread error");
		return -1;
	}

	return 0;
}

int bspatchImp(char *oldFileName, char *patchFileName, char *newFileName) {
	int retval;
	int i;
	FILE *pf;
	FILE *of;
	FILE *nf;
	uint8_t header[24];
	uint8_t *old, *new1 = NULL;
	int64_t oldsize, newsize;
	struct bspatch_stream stream;
	pf = fopen(patchFileName, "rb");
	if (pf == NULL) {
		log_debug("open file error");
		return -1;
	}
	if (fread(header, 1, 24, pf) != 24) {
		log_debug("HEADER NESSAGE ERROR");
		return -2;
	}

	if (memcmp(header, "ENDSLEY/BSDIFF43", 16) != 0) {
		log_debug("HEADER STRING ERROR");
		return -2;
	}

	newsize = offtin(header + 16);
	log_debug("newsize = %d", newsize / 1024);
	if (newsize < 0) {
		log_debug("NEWSIZE ERROR");
		return -2;
	}

	if ((of = fopen(oldFileName, "rb")) == NULL) {
		log_debug("OLD FILE %s OPEN ERROR", oldFileName);
		return -1;
	}

	if ((fseek(of, 0, SEEK_END) != 0) || ((oldsize = ftell(of)) < 0))
	//		(oldsize != fread(old,1,oldsize,of)) || fclose(of))
			{
		log_debug("OLD FILE READ ERROR");
		free(old);
		return -3;
	}
	retval = fseek(of, 0, SEEK_SET);
	log_debug("fseek status = %d", retval);
	old = (uint8_t*) malloc(oldsize + 1);
	log_debug("oldsize=%d KB", oldsize / 1024);
	retval = fread(old, sizeof(uint8_t), oldsize, of);
	if (retval != oldsize) {
		if (feof(of)) {
			log_debug("FILE READ OVER");
			return -1;
		} else if (ferror(of)) {
			log_debug("reading error");
			return -1;
		}
		log_debug("retval = %d KB", retval / 1024);
		log_debug("READ OLD FILE ERROR");
		free(old);
		return -3;
	}
	fclose(of);
	new1 = (uint8_t*) malloc(newsize + 1);
	if (new1 == NULL) {
		log_debug("new error");
		return -1;
	}

	stream.read = bz2_read;
	stream.opaque = pf;
//	fseek(pf,0,SEEK_SET);
//	if(fread(header, 1, 30, pf) != 30)
//	{
//		return -1;
//	}
//	for(i=0;i<30;i++)
//	{
//		log_debug("herder[%d] = %c",i,header[i]);
//	}
//	log_debug("headER = %s",header);
//	fseek(pf,0,SEEK_SET);
	retval = bspatch(old, oldsize, new1, newsize, &stream, pf);
	if (retval != 0) {
		log_debug("retval = %d", retval);
		log_debug("patch file parse error");
		return -4;
	}
	nf = fopen(newFileName, "wb+");
	retval= fwrite(new1, 1, newsize, nf);
	if(retval != newsize)
	{
		log_debug("write data error");
		return -5;
	}
	fclose(nf);
	fclose(pf);
	free(old);
	free(new1);

	return 0;
}
#if defined(BSPATCH_EXECUTABLE)
int main(int argc,char * argv[])
{
	FILE * f;
	int fd;
	int bz2err;
	uint8_t header[24];
	uint8_t *old, *new;
	int64_t oldsize, newsize;
	BZFILE* bz2;
	struct bspatch_stream stream;
	struct stat sb;

	if(argc!=4) errx(1,"usage: %s oldfile newfile patchfile\n",argv[0]);

	/* Open patch file */
	if ((f = fopen(argv[3], "r")) == NULL)
	err(1, "fopen(%s)", argv[3]);

	/* Read header */
	if (fread(header, 1, 24, f) != 24) {
		if (feof(f))
		errx(1, "Corrupt patch\n");
		err(1, "fread(%s)", argv[3]);
	}

	/* Check for appropriate magic */
	if (memcmp(header, "ENDSLEY/BSDIFF43", 16) != 0)
	errx(1, "Corrupt patch\n");

	/* Read lengths from header */
	newsize=offtin(header+16);
	if(newsize<0)
	errx(1,"Corrupt patch\n");

	/* Close patch file and re-open it via libbzip2 at the right places */
	if(((fd=open(argv[1],O_RDONLY,0))<0) ||
			((oldsize=lseek(fd,0,SEEK_END))==-1) ||
			((old=malloc(oldsize+1))==NULL) ||
			(lseek(fd,0,SEEK_SET)!=0) ||
			(read(fd,old,oldsize)!=oldsize) ||
			(fstat(fd, &sb)) ||
			(close(fd)==-1)) err(1,"%s",argv[1]);
	if((new=malloc(newsize+1))==NULL) err(1,NULL);

	if (NULL == (bz2 = BZ2_bzReadOpen(&bz2err, f, 0, 0, NULL, 0)))
	errx(1, "BZ2_bzReadOpen, bz2err=%d", bz2err);

	stream.read = bz2_read;
	stream.opaque = bz2;
	if (bspatch(old, oldsize, new, newsize, &stream))
	errx(1, "bspatch");

	/* Clean up the bzip2 reads */
	BZ2_bzReadClose(&bz2err, bz2);
	fclose(f);

	/* Write the new file */
	if(((fd=open(argv[2],O_CREAT|O_TRUNC|O_WRONLY,sb.st_mode))<0) ||
			(write(fd,new,newsize)!=newsize) || (close(fd)==-1))
	err(1,"%s",argv[2]);

	free(new);
	free(old);

	return 0;
}

#endif
