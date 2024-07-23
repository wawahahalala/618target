#include "tftp.h"
#include "log.h"
#include "global.h"
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <fcntl.h>
extern const unsigned short max_tftpRequest_len = 128;
extern int tftpServerFd;
extern int protocalFileFd;
extern int statusFileFd;
extern const unsigned blksize_default;
extern const unsigned timeout_default;
extern const unsigned max_retransmit_default;
extern const char tftpTypeString[][20];
//extern ULONG flashRead(ADDRESS start, char * buf, ULONG nbytes);
//extern ULONG flashWrite(ADDRESS flashStart, char * buf, ULONG nbytes);


tftpType parseTftpPacketType(char* buf, unsigned short len){
    if(len < 4 || buf[1] > 6) return UNDEFINED;
//    log_debug("buf[1] = %c",buf[1]);
    return buf[1];
}


tftpRWRQ parseTftpReadWriteRquest(char* buf, unsigned short len){
	//log_error("22");
    char s[1024];
    memcpy(s, buf, len);
    for(int i = 0; i < len; i++){
        if(s[i] == '\0') s[i] = '#';
    }
    size_t length;
    char* curp=s+2;
    char *p=curp;
    p=strstr(curp,"#");
    length=p-curp;
    tftpRWRQ RWRQ;
    memset(&RWRQ,0,sizeof(RWRQ));
    strncpy(RWRQ.fileName, curp, length);
//    snprintf(RWRQ.fileName, length + 4, "C:/%s", curp);

    curp=p;
    p=strstr(curp,"#");
    length=p-curp;
    strncpy(RWRQ.mode,curp,length);
    
    //log_error("33");
    char digit[128]={'\0'};
    RWRQ.blksize = 0;
    RWRQ.maxRetransmit = 0;
    RWRQ.timeout = 0;
    if((curp = strstr(s, "blksize")) != NULL){
        curp += sizeof("blksize");
        log_debug("found blksize");
        p=strstr(curp,"#");
        length=p-curp;
        strncpy(digit, curp,length);
        RWRQ.blksize = atoi(digit);
    }
    memset(digit,'\0',sizeof(digit));
    if((curp = strstr(s, "timeout")) != NULL){
        curp += sizeof("timeout");
        p=strstr(curp,"#");
        length=p-curp;
        strncpy(digit, curp,length);
        RWRQ.timeout = atoi(digit);
    }
    memset(digit,'\0',sizeof(digit));
    if((curp = strstr(s, "max-retransmit")) != NULL){
        curp += sizeof("max-retransmit");
        p=strstr(curp,"#");
        length=p-curp;
        strncpy(digit, curp,length);
        RWRQ.maxRetransmit = atoi(digit);
    }
    log_debug("from RWRQ blksize = %hu, timeout = %hu, maxRetransmit = %hu", RWRQ.blksize, RWRQ.timeout, RWRQ.maxRetransmit);
    return RWRQ;
}

tftpAck parseTftpAck(char* buf, unsigned len){
    tftpAck ack;
//    log_debug("parseTftpAck buf[0] = %d buf[1] = %d", buf[2], buf[3]);
    unsigned short high = (unsigned char)buf[2];
    unsigned short low = (unsigned char)buf[3];
    ack.ackNo = (high << 8) + low;
    return ack;
}

tftpOAck parseTftpOAck(char *buf, unsigned short len)
{
    char s[512];
    memset(s, 0, sizeof(s)); //
    memcpy(s, buf, len);
    for(int i = 0; i < len; i++){
        if(s[i] == '\0') s[i] = '#';
    }
    tftpOAck oack;
    char digits[128];
    char* curp;
    //闁告帗绻傞‖濠囧礌閿燂拷
    oack.blksize = 0;
    oack.timeout = 0;
    oack.maxRetransmit = 0;
    if((curp = strstr(s, "blksize")) != NULL){  //valgrind闁圭粯鍔楅妵娆硂nditional jump or move depends on uninitialised value(s)
        curp += sizeof("blksize");              //闁轰礁鎳庨崢娑氭嫬閸愵亝鏆弇emset閻庣數鐣熼弶鈺傜椤㈡垿宕氬┑鍡╃�闁告牭鎷�
        strcpy(digits, curp - s + buf);
        oack.blksize = atoi(digits);
    }
    if((curp = strstr(s, "timeout")) != NULL){
        curp += sizeof("timeout");
        strcpy(digits, curp - s + buf);
        oack.timeout = atoi(digits);
    }
    if((curp = strstr(s, "max-retransmit")) != NULL){
        curp += sizeof("max-retransmit");
        strcpy(digits, curp - s + buf);
        oack.maxRetransmit = atoi(digits);
    }
    //log_debug("from OACK blksize = %hu, timeout = %hu, maxRetransmit = %hu", oack.blksize, oack.timeout, oack.maxRetransmit);
    return oack;
}

size_t getFileSize(char *filename){

	//鑾峰彇鏂囦欢澶у皬
	FILE *file;
	size_t filesize = 0;

	file = fopen(filename, "rb");
	if(NULL == file)
	{
		log_debug("getFileSize open file %s error!", filename);
		return -1;
	}
	if(0 == fseek(file, 0L, SEEK_END))
	{
		filesize = ftell(file);
	}
	rewind(file);

	fclose(file);

	file = NULL;

	return filesize;
}


//size_t flashWriteFile(char *filename, size_t startAddr, size_t infoAddr, size_t maxFileAddr)
//{
//	ADDRESS flashWriteAddr = startAddr;
//	FILE* fp;
//	FILE *op;
//	//每次读取和写入的大小为2048字节
//	int readWriteSize = 2048;
//	fp = fopen(filename, "rb");
//	if (fp == NULL) {
//		log_error("flashWriteFile fp %s open fail\n", filename);
//		return 0;
//	}
//	op = fopen("C:/out.bin","wb");
//	if(op == NULL)
//	{
//		log_error("write error");
//		fclose(fp);
//		return -1;
//	}
//	//先判断一下是否在flash中能保存完
//	size_t fileSize = 0;
//	//获取文件大小
//	if(0 == fseek(fp, 0, SEEK_END))
//	{
//		fileSize = ftell(fp);
//	}
//
//	fseek(fp, 0, SEEK_SET);
//	//判断是否超限
//	if(startAddr + fileSize > maxFileAddr)
//	{
//		log_error("flashWriteFile file size too big, flash not write!");
//		fclose(fp);
//		return 0;
//	}
//
//	char *mem = (char*)malloc(readWriteSize);
//	char temp[2048];
//	memset(mem,0,readWriteSize);
//	memset(temp,0,2048);
//	char infoBuf[128] = {0};
//	size_t rdsize = 0;
//	ULONG fwsize = 0;
//	ULONG owsize = 0;
//	size_t flashtotalsize = 0;
//	size_t readtotalsize = 0;
//	unsigned char fileNameSize = 0;
//	while (!feof(fp)) {
//		log_debug("flashWriteAddr=%p\n", flashWriteAddr);
//		rdsize = fread(mem, 1, readWriteSize, fp);
////		if(rdsize != 2048)
////		{
////			for(int i=0;i<rdsize;i++)
////			{
////				log_debug("mem[%d] = %c ",i,mem[i]);
////			}
////		}
//		fwsize = flashWrite(flashWriteAddr, (char*) mem, rdsize);
////		if(rdsize == 2048)
////		{
////			fwsize = flashWrite(flashWriteAddr, (char*) mem, rdsize);
////		}
//		memcpy(temp,(void*)flashWriteAddr,rdsize);
////		if(rdsize != 2048)
////		{
////			for(int j=0;j<rdsize;j++)
////			{
////				log_debug("temp[%d] = %c ",j,temp[j]);
////			}
////		}
////		if(rdsize == 2048)
////		{
////			memcpy(temp,(void*)flashWriteAddr,rdsize);
////			owsize = fwrite(temp,1,rdsize,op);
////			log_debug("out.bin write size = %ld ",owsize);
////			memset(temp,0,sizeof(temp));
////			flashtotalsize += rdsize;
////	
////			readtotalsize += rdsize;
////			flashWriteAddr = flashWriteAddr + rdsize;
////			log_debug("fileread size=%d\t\tfile read total size=%ld\t\tflashwrite size=%ld\t\tflash total size=%ld\n", rdsize,
////							readtotalsize, fwsize, flashtotalsize);
////		}
//		owsize = fwrite(temp,1,rdsize,op);
//		log_debug("out.bin write size = %ld ",owsize);
//		memset(temp,0,sizeof(temp));
//		flashtotalsize += rdsize;
//
//		readtotalsize += rdsize;
//		flashWriteAddr = flashWriteAddr + rdsize;
//		log_debug("fileread size=%d\t\tfile read total size=%ld\t\tflashwrite size=%ld\t\tflash total size=%ld\n", rdsize,
//				readtotalsize, fwsize, flashtotalsize);
//	}
//	fclose(fp);
//	fclose(op);
//	//再记录一下文件的相关内容
//	//               文件名大小$文件名$文件大小
//
//	fileNameSize=(unsigned char)(strlen(filename)+1);
//	sprintf(infoBuf, "%c$%s$%d", fileNameSize, filename, readtotalsize);
//
//	//写入到flash中
//	fwsize = flashWrite(infoAddr, (char*) infoBuf, 128);
//
//	log_debug("write info : %s OK , ret = %d", infoBuf, fwsize);
//	free(mem);
//
//	return readtotalsize;
//}

int get(tftpRWRQ readRequest, struct sockaddr_in loaderAddr, uint64_t* no ,ADDRESS writeAddr)
{
    //1.闁告瑦鍨块敓绲塕Q
    char buf[512];
    //int tftpClientFd = socket(AF_INET, SOCK_DGRAM, 0);
//    if(tftpClientFd < 0){
//        log_error("tftpclient socket init error");
//        //close(tftpClientFd);
//        return -1;
//    }
    ssize_t count = makeTftpReadRequest(readRequest, buf);
    if(sendto(protocalFileFd, buf, count, 0, (struct sockaddr*)&loaderAddr, sizeof(struct sockaddr_in)) < 0){
        log_error("SEND RRQ TO HOST:%s PORT:%hu ERROR", inet_ntoa(loaderAddr.sin_addr), ntohs(loaderAddr.sin_port));
        return -1;
    }
    //2.閻熸瑱绲鹃悗浠嬪绩鐠哄搫鐓傞柣銊ュ殸ACK闁硅翰鍎查弸锟介悷娆欑稻閻庝粙鐛張鐢电懍缁绢収鍠栭悾楣冨嫉閿熺晫鐭掗悗娑欘殱椤斿矂鎯冮崟顐嫹
    socklen_t addrLen = sizeof(struct sockaddr_in);
    if((count = recvfrom(protocalFileFd, buf, sizeof(buf), 0, (struct sockaddr*)&loaderAddr, &addrLen)) < 0){
        log_error("RECEIVE OACK ERROR errno = %d", errno);
        return -1;
    }

    if(parseTftpPacketType(buf, count) != OACK){
        log_error("EXPECT OACK BUT RECEIVE %s count = %d", tftpTypeString[parseTftpPacketType(buf, count)], count);
        if(parseTftpPacketType(buf, count) == DATA){
            log_error("DATA PACKET NO = %d", parseTftpAck(buf, count).ackNo);
        }
        return -1;
    }

    tftpOAck oAck = parseTftpOAck(buf, count);
    //3. 闁告瑦鍨块敓绱窩K 0
    tftpAck ack;
    ack.ackNo = 0;
    count = makeTftpAck(ack, buf);
    sendto(protocalFileFd, buf, count, 0, (struct sockaddr*)&loaderAddr, sizeof(struct sockaddr_in));
    log_debug("oack blksize = %d timeout = %d maxRetransmit = %d", oAck.blksize, oAck.timeout, oAck.maxRetransmit);
#ifdef OS_HAVE_FILE_SYSTEM
    int res = download(protocalFileFd, readRequest.fileName, loaderAddr, oAck.blksize, oAck.timeout, oAck.maxRetransmit, buf, count, no);
#elif defined(OS_NO_FILE_SYSTEM)
    int res=downloadflash(protocalFileFd, writeAddr, loaderAddr, oAck.blksize, oAck.timeout,oAck.maxRetransmit,buf,count, no);
#endif
//    struct timeval timeout_;
//    timeout_.tv_sec = 0;
//    timeout_.tv_usec = 0;
//    if(setsockopt(protocalFileFd, SOL_SOCKET, SO_RCVTIMEO, &timeout_, sizeof(timeout_)) < 0){
//        log_error("set timeout arg error");
//    }
    return res;
}

int put(tftpRWRQ writeRequest, struct sockaddr_in loaderAddr, size_t len,ADDRESS readAddr){
    //1.闁告瑦鍨块敓绲嶳Q闁硅鎷�
	char buf[512] = {0};
    int fd = 0;
//    int tftpClientFd = socket(AF_INET, SOCK_DGRAM, 0);
//    if(tftpClientFd < 0){
//        log_error("tftpclient socket init error");
//        return -1;
//    }
    int count = makeTftpWriteRequest(writeRequest, buf);
    //log_debug("count = %d", count);
    if(strstr(writeRequest.fileName, ".LCS") || strstr(writeRequest.fileName, ".LUS") || strstr(writeRequest.fileName, ".LNS")){
    	fd = statusFileFd;
    }
    else fd = protocalFileFd;
    struct timeval timeout_;
    timeout_.tv_sec = writeRequest.timeout;
    timeout_.tv_usec = 0;
    if(setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout_, sizeof(timeout_)) < 0){
        log_error("set timeout arg error");
    }
    if(sendto(fd, buf, count, 0, (struct sockaddr*)&loaderAddr, sizeof(struct sockaddr_in)) < 0){
        log_error("SEND WRQ TO HOST:%s PORT:%hu ERROR", inet_ntoa(loaderAddr.sin_addr), ntohs(loaderAddr.sin_port));
        return -1;
    }

    //2.閻熸瑱绲鹃悗浠嬪绩鐠哄搫鐓傞柣銊ュ殸ACK闁硅翰鍎查弸锟介悷娆欑稻閻庝粙鐛張鐢电懍缁绢収鍠栭悾楣冨嫉閿熺晫鐭掗悗娑欘殱椤斿矂鎯冮崟顐嫹
    //unsigned retry = 0;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    int receiveOAckLen;
    //struct sockaddr_in newLoaderAddr;
    if((receiveOAckLen = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr*)&loaderAddr, &addr_len)) < 0)
    {
        log_error("RECEIVE OACK ERROR");
        return -1;
    };
    log_debug("loaderAddr ip:%s port:%hu", inet_ntoa(loaderAddr.sin_addr), ntohs(loaderAddr.sin_port));
    log_debug("receiveOAckLen = %d", receiveOAckLen);
    tftpOAck oack = parseTftpOAck(buf, receiveOAckLen);

    //3.闂佹彃娲ㄩ弫顦丄CK濞戞搩鍘惧▓鎴﹀矗閸屾稒娈堕弶鈺傜椤㈡垿宕ユ惔锝囨暰闁汇劌瀚粭鍌涘閻樿櫕鎯欏ù锝忔嫹    //log_error("PARSE FILE NAME %s",writeRequest.fileName);
//    int res = upload(fd, writeRequest.fileName, loaderAddr, oack.blksize, oack.timeout, oack.maxRetransmit);
#ifdef OS_HAVE_FILE_SYSTEM
    int res = upload(fd, writeRequest.fileName, loaderAddr, oack.blksize, oack.timeout, oack.maxRetransmit);
#elif defined(OS_NO_FILE_SYSTEM)
    int res =uploadflash(fd, readAddr, loaderAddr, oack.blksize, oack.timeout, oack.maxRetransmit,len);
#endif
//    timeout_.tv_sec = 0;
//    timeout_.tv_usec = 0;
//    if(setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout_, sizeof(timeout_)) < 0){
//        log_error("set timeout arg error");
//    }
    return res;
}

int handleGet(tftpRWRQ readRuquest, struct sockaddr_in loaderAddr, size_t len,ADDRESS readAddr){
    //1.consulting stage
    unsigned short blksize = readRuquest.blksize == 0 ? blksize_default: min(readRuquest.blksize, blksize_default);
    unsigned short timeout = readRuquest.timeout == 0 ? timeout_default: min(readRuquest.timeout, timeout_default);
    unsigned short max_retransmit = readRuquest.maxRetransmit == 0 ? max_retransmit_default : min(readRuquest.maxRetransmit, max_retransmit_default);
    // make OAck
    tftpOAck oAck;
    // log_debug("blksize = %hu blksize_default = %hu blksizeInRequest = %hu", blksize, blksize_default, readRuquest.blksize);
    oAck.blksize = blksize;
    oAck.timeout = timeout;
    oAck.maxRetransmit = max_retransmit;
    char buf[512];
    int count = makeTftpOAck(oAck, buf);
    // log_debug("count = %d", count);
    // send OAck
    if(sendto(protocalFileFd, buf, count, 0, (struct sockaddr*)&loaderAddr, sizeof(struct sockaddr_in)) < 0){
        log_error("SEND OACK ERROR");
        return -1;
    }
    //wait for Ack 0
    socklen_t addrLen = sizeof(struct sockaddr_in);
    ssize_t recvLen;
    if((recvLen = recvfrom(protocalFileFd, buf, 512, 0, (struct sockaddr*)&loaderAddr, &addrLen)) < 0){
        log_error("RECV ACK 0 ERROR");
        return -1;
    }
    else{
    	tftpType type = parseTftpPacketType(buf, recvLen);
    	if(type != ACK){
    		log_error("EXPECT ACK BUT OPCODE = %d", type + 1);
    		return -1;
    	}
        tftpAck ack = parseTftpAck(buf, recvLen);
        if(ack.ackNo != 0){
            log_error("EXPECT ACK 0 BUT RECV %hu", ack.ackNo);
            return -1;
        }
    }
    log_debug("ACK RECV SUCCESS");

#ifdef OS_HAVE_FILE_SYSTEM
    int res = upload(protocalFileFd, readRuquest.fileName, loaderAddr, oAck.blksize, oAck.timeout, oAck.maxRetransmit);
#elif defined(OS_NO_FILE_SYSTEM)
    int res =uploadflash(protocalFileFd, readAddr, loaderAddr, oAck.blksize, oAck.timeout, oAck.maxRetransmit,len);
#endif
//    struct timeval timeout_;
//    timeout_.tv_sec = 0;
//    timeout_.tv_usec = 0;
//    if(setsockopt(protocalFileFd, SOL_SOCKET, SO_RCVTIMEO, &timeout_, sizeof(timeout_)) < 0){
//        log_error("set timeout arg error");
//    }
    return res;
}

int handlePut(tftpRWRQ writeRuquest, struct sockaddr_in loaderAddr,ADDRESS writeAddr){
    //1.consulting stage
    unsigned short blksize = writeRuquest.blksize == 0 ? blksize_default: min(writeRuquest.blksize, blksize_default);
    unsigned short timeout = writeRuquest.timeout == 0 ? timeout_default: min(writeRuquest.timeout, timeout_default);
    unsigned short max_retransmit = writeRuquest.maxRetransmit == 0 ? max_retransmit_default : min(writeRuquest.maxRetransmit, max_retransmit_default);
    //make OAck
    tftpOAck oAck;
    oAck.blksize = blksize;
    oAck.timeout = timeout;
    oAck.maxRetransmit = max_retransmit;
    char buf[512];
    int count = makeTftpOAck(oAck, buf);
    log_debug("handlePut count = %d", count);
    // send OAck
    if(sendto(protocalFileFd, buf, count, 0, (struct sockaddr*)&loaderAddr, sizeof(struct sockaddr_in)) < 0){
        log_error("SEND OACK ERROR");
        return -1;
    }
#ifdef OS_HAVE_FILE_SYSTEM
    int res = download(protocalFileFd, writeRuquest.fileName, loaderAddr, oAck.blksize, oAck.timeout, oAck.maxRetransmit, buf, count, NULL);
#elif defined(OS_NO_FILE_SYSTEM)
    int res=downloadflash(protocalFileFd, writeAddr, loaderAddr, oAck.blksize, oAck.timeout,oAck.maxRetransmit,buf,count, no);
#endif
//    struct timeval timeout_;
//    timeout_.tv_sec = 0;
//    timeout_.tv_usec = 0;
//    if(setsockopt(protocalFileFd, SOL_SOCKET, SO_RCVTIMEO, &timeout_, sizeof(timeout_)) < 0){
//        log_error("set timeout arg error");
//    }
    return res;
}


int makeTftpReadRequest(tftpRWRQ readRequest, char* buf){
    char* curp = buf;
    char digits[128];
    buf[0] = 0;
    buf[1] = 1;
    curp += 2;
    memcpy(curp, readRequest.fileName, strlen(readRequest.fileName) + 1);
    curp += strlen(readRequest.fileName) + 1;
    memcpy(curp, readRequest.mode, strlen(readRequest.mode) + 1);
    curp += strlen(readRequest.mode) + 1;
    memcpy(curp, "blksize", sizeof("blksize"));
    curp += sizeof("blksize");
    memcpy(curp, digits, sprintf(digits, "%hu", readRequest.blksize) + 1);
    curp += strlen(digits) + 1;
    memcpy(curp, "timeout", sizeof("timeout"));
    curp += sizeof("timeout");
    memcpy(curp, digits, sprintf(digits, "%hu", readRequest.timeout) + 1);
    curp += strlen(digits) + 1;
    memcpy(curp, "max-retransmit", sizeof("max-retransmit"));
    curp += sizeof("max-retransmit");
    memcpy(curp, digits, sprintf(digits, "%hu", readRequest.maxRetransmit) + 1);
    curp += strlen(digits) + 1;

    return curp -  buf;
}

int makeTftpWriteRequest(tftpRWRQ writeRequest, char* buf){
    char* curp = buf;
    char digits[128];
    buf[0] = 0;
    buf[1] = 2;
    curp += 2;
    memcpy(curp, writeRequest.fileName, strlen(writeRequest.fileName) + 1);
    curp += strlen(writeRequest.fileName) + 1;
    memcpy(curp, writeRequest.mode, strlen(writeRequest.mode) + 1);
    curp += strlen(writeRequest.mode) + 1;
    memcpy(curp, "blksize", sizeof("blksize"));
    curp += sizeof("blksize");
    memcpy(curp, digits, sprintf(digits, "%hu", writeRequest.blksize) + 1);
    curp += strlen(digits) + 1;
    memcpy(curp, "timeout", sizeof("timeout"));
    curp += sizeof("timeout");
    memcpy(curp, digits, sprintf(digits, "%hu", writeRequest.timeout) + 1);
    curp += strlen(digits) + 1;
    memcpy(curp, "max-retransmit", sizeof("max-retransmit"));
    curp += sizeof("max-retransmit");
    memcpy(curp, digits, sprintf(digits, "%hu", writeRequest.maxRetransmit) + 1);
    curp += strlen(digits) + 1;
    return curp - buf;
}

int makeTftpData(char* buf, unsigned blockNo){
    buf[0] = 0;
    buf[1] = 3;
    buf[2] = blockNo >> 8;
    buf[3] = blockNo;
    return 1;
}

int makeTftpAck(tftpAck ack, char* buf){
    //char* curp = buf;
    //char digits[128];
    buf[0] = 0;
    buf[1] = 4;
    //curp += 2;
    buf[2] = ack.ackNo >> 8;
    buf[3] = ack.ackNo;
    //memcpy(curp, digits, sprintf(digits, "%hu", ack.ackNo) + 1);
    //curp += strlen(digits) + 1;
    return 4;
}

flashInfo getFlashInfo(const char *buf, int len)
{
	int i = 0, j = 0;
	flashInfo tmpInfo;
	memset(&tmpInfo, 0, sizeof(flashInfo));
	for(i = 0; i < len; i++)
	{
		if('$' == buf[i])
		{
			strncpy(tmpInfo.fileName, buf, i);
			break;
		}
	}

	j = i + 1;
	for( ; j < len; j++)
	{
		if('$' == buf[j])
		{
			strncpy(tmpInfo.fileVersion, buf + i + 1, j - i - 1);
			break;
		}
	}

	//strncpy(tmpInfo.fileVersion, buf + i + 1, len - i - 1);

	return tmpInfo;
}


int makeTftpError(tftpError error, char* buf){
	return 0;
}

int makeTftpOAck(tftpOAck oAck, char* buf){
    char* curp = buf;
    char digits[128];
    buf[0] = 0;
    buf[1] = 6;
    curp += 2;
    memcpy(curp, "blksize", sizeof("blksize")); 
    curp += sizeof("blksize");
    //memcpy(curp, itoa())
    memcpy(curp, digits, sprintf(digits, "%hu", oAck.blksize) + 1);
    curp += strlen(digits) + 1;
    memcpy(curp, "timeout", sizeof("timeout"));
    curp += sizeof("timeout");
    memcpy(curp, digits, sprintf(digits, "%hu", oAck.timeout) + 1);
    curp += strlen(digits) + 1;
    memcpy(curp, "max-retransmit", sizeof("max-retransmit"));
    curp += sizeof("max-retransmit");
    memcpy(curp, digits, sprintf(digits, "%hu", oAck.maxRetransmit) + 1);
    curp += strlen(digits) + 1;
    return curp - buf;
}

#ifdef OS_HAVE_FILE_SYSTEM
int upload(int socketFd, char* fileName, struct sockaddr_in loaderAddr, unsigned short blksize, unsigned short timeout, unsigned short maxRetransmit){
    char filePathName[128] = {0};

    sprintf(filePathName, "C:/%s", fileName);

	//FILE* fp = fopen(fileName, "r");
	FILE* fp = fopen(filePathName, "r");
    if(fp == NULL){
        int errNum = errno;
        log_error("file %s open unsuccessful errorno = %d reason = %s", filePathName, errno, strerror(errNum));
        //log_error("file %s open unsuccessful errorno = %d reason = %s", fileName, errno, strerror(errNum));
        return -1;
    }
    struct timeval timeout_;
    //log_debug("timeout = %d", timeout);
    timeout_.tv_sec = timeout;
    timeout_.tv_usec = 0;
    if(setsockopt(socketFd, SOL_SOCKET, SO_RCVTIMEO, &timeout_, sizeof(timeout_)) < 0){
    	log_error("set timeout arg error errno = %d reason = %s", errno, strerror(errno));
        fclose(fp); 
        return -1;
    }
    unsigned short try;
    //log_debug("blksize = %d", blksize);  閺夆晜鐟ヨぐ鐐垫嫚濠靛棭鍤ら柤宄扮摠椤斿矂鏌ㄥ▎鎺濆殩
    log_debug("timeout = %d, blksize = %hu", timeout, blksize);
    char* buf = (char*)malloc(sizeof(char) * (blksize + 4));
    char ackPacket[128];
    //memcpy(buf, lastPacket, count);
    size_t count;
    unsigned expectedAck = 0;
    ssize_t packetLen = 0;
    struct sockaddr_in addr;
    size_t lastCount = 0;
    while(1){
        if((count = fread(buf + 4, 1, blksize, fp)) > 0){
            makeTftpData(buf, ++expectedAck);
            lastCount = count;
            count += 4;

            if(sendto(socketFd, buf, count, 0, (struct sockaddr*)&loaderAddr, sizeof(struct sockaddr_in)) < 0){
                log_error("send DATA PACKET %d to ip:%s error", expectedAck, inet_ntoa(loaderAddr.sin_addr));
                try++;
            }
        }else if(count == 0 && lastCount == blksize)
        {
        	//文件刚好是blocksize的整数倍时
        	memset(buf, 0, sizeof(char) * (blksize + 4));
        	log_debug("count = 0 and lastCount = blocksize");
        	makeTftpData(buf, ++expectedAck);
			//count += 4;
			lastCount = 0;
        	//给管理端发送一包空包 只包含4字节的包头
			if(sendto(socketFd, buf, 4, 0, (struct sockaddr*)&loaderAddr, sizeof(struct sockaddr_in)) < 0){
				log_error("send DATA PACKET %d to ip:%s error", expectedAck, inet_ntoa(loaderAddr.sin_addr));
				try++;
			}
        	//break;
        }
        else{
        	log_debug("lastcount = %d, count = %d", lastCount, count);
            break;
        }
        try = 1;
        socklen_t addrLen = sizeof(struct sockaddr_in);
        //int errorFlag = 0;
        unsigned short try2=1;
        while(1){

            if((packetLen = recvfrom(socketFd, ackPacket, sizeof(ackPacket), 0, (struct sockaddr*)&addr, &addrLen)) < 0){
                if(errno == EWOULDBLOCK || errno == EAGAIN){
                    log_debug("times up for ACK %d", expectedAck);
                    //errorFlag = 1;
                    //sendto(socketFd, buf, count, 0, (struct sockaddr*)&loaderAddr, sizeof(struct sockaddr_in));
                }
                else{
                    log_debug("recvfrom ack error");
                    //errorFlag = 1;
                    //sendto(socketFd, buf, count, 0, (struct sockaddr*)&loaderAddr, sizeof(struct sockaddr_in));
                }
            }
            else{
            	log_debug("packetLen = %d", packetLen);
                tftpAck ack = parseTftpAck(ackPacket, packetLen);
                if(ack.ackNo != expectedAck){
                    log_debug("expected to get ack %hu but get %hu", expectedAck, ack.ackNo);
                    //errorFlag = 1;
                    //sendto(socketFd, buf, count, 0, (struct sockaddr*)&loaderAddr, sizeof(struct sockaddr_in));
                    continue;
                }
                else{

                    break;
                }
            }
            if(try2 >= maxRetransmit + 1)
                break;
            else
            {
                if(sendto(socketFd, buf, count, 0, (struct sockaddr*)&loaderAddr, sizeof(struct sockaddr_in)) < 0){
                    log_error("send to error");
                }
                else
                {
                    ++try2;
                }
            }

            //log_debug("try %d maxRetransmit %d", try, maxRetransmit);

        }
        if(try >= maxRetransmit + 1){
            log_error("up to max-retransmit");
            free(buf);
            return -1;
            break;
        }
    }
    free(buf);
    fclose(fp);
    return 1;
}

#elif defined(OS_NO_FILE_SYSTEM)
int uploadflash(int socketFd, ADDRESS readAddr, struct sockaddr_in loaderAddr, unsigned short blksize, unsigned short timeout, unsigned short maxRetransmit, size_t len){

    struct timeval timeout_;
    //log_debug("timeout = %d", timeout);
    timeout_.tv_sec = timeout;
    timeout_.tv_usec = 0;
    if(setsockopt(socketFd, SOL_SOCKET, SO_RCVTIMEO, &timeout_, sizeof(timeout_)) < 0){
    	log_error("set timeout arg error errno = %d reason = %s", errno, strerror(errno));
        return -1;
    }
    unsigned short try;
    log_debug("timeout = %d, blksize = %hu", timeout, blksize);
    char* buf = (char*)malloc(sizeof(char) * (blksize + 4));
    char ackPacket[128];
    //memcpy(buf, lastPacket, count);
    size_t count;
    unsigned expectedAck = 0;
    ssize_t packetLen = 0;
    struct sockaddr_in addr;
    size_t lastCount = 0;
    log_debug("flash read begin , read addr = %p",readAddr);
    int flag =0;
    while(1){
    	if(flag == 1)
    	{
    		break;
    	}
//    	memcpy(mem, (void*) start, strlen(buf) + 1);
    	log_debug("len = %ld,blksize = %d",len,blksize);
    	if(len > blksize && blksize < len - count)
    	{
    		memset(buf,0,blksize+4);
//    		int ret=flashRead(readAddr,buf+4,blksize);
    		memcpy(buf+4,(void*)readAddr,blksize);
//    		if(ret != blksize)
//    		{
//    			log_error("flash read error");
//    			free(buf);
//    			return -1;
//    		}
    		readAddr += blksize;
    		count +=blksize;
    		makeTftpData(buf, ++expectedAck);
    		if(sendto(socketFd, buf, blksize+4, 0, (struct sockaddr*)&loaderAddr, sizeof(struct sockaddr_in)) < 0){
				log_error("send DATA PACKET %d to ip:%s error", expectedAck, inet_ntoa(loaderAddr.sin_addr));
				try++;
			}
    	}else if(len < blksize)
    	{
    		memset(buf,0,blksize+4);
    		log_debug("111");
//    		int ret=flashRead(readAddr,buf+4,len);
    		memcpy(buf+4,(void*)readAddr,len);
//    		log_debug("ret = %d",ret);
    		readAddr+=len;
    		makeTftpData(buf, ++expectedAck);
    		log_debug("send begin");
    		if(sendto(socketFd, buf, len+4, 0, (struct sockaddr*)&loaderAddr, sizeof(struct sockaddr_in)) < 0){
				log_error("send DATA PACKET %d to ip:%s error", expectedAck, inet_ntoa(loaderAddr.sin_addr));
				try++;
			}
    		flag = 1;
    	}
    	else
    	{
    		memset(buf,0,blksize+4);
//    		int ret=flashRead(readAddr,buf+4,len-count);
//			if(ret != blksize)
//			{
//				log_error("flash read error");
//				free(buf);
//				return -1;
//			}
    		memcpy(buf+4,readAddr,len-count);
    		readAddr=readAddr+len-count;
    		makeTftpData(buf, ++expectedAck);
    		if(sendto(socketFd, buf, len-count+4, 0, (struct sockaddr*)&loaderAddr, sizeof(struct sockaddr_in)) < 0){
				log_error("send DATA PACKET %d to ip:%s error", expectedAck, inet_ntoa(loaderAddr.sin_addr));
				try++;
			}
    		if( (len%blksize) == 0 )
    		{
    			memset(buf,0,blksize+4);
    			log_debug("this is a zero package");
    			makeTftpData(buf, ++expectedAck);
    			if(sendto(socketFd, buf, 4, 0, (struct sockaddr*)&loaderAddr, sizeof(struct sockaddr_in)) < 0){
					log_error("send DATA PACKET %d to ip:%s error", expectedAck, inet_ntoa(loaderAddr.sin_addr));
					try++;
				}
    		}

    		flag =1;
    	}
//        if((count = fread(buf + 4, 1, blksize, fp)) > 0){
//            makeTftpData(buf, ++expectedAck);
//            lastCount = count;
//            count += 4;
//
//            if(sendto(socketFd, buf, count, 0, (struct sockaddr*)&loaderAddr, sizeof(struct sockaddr_in)) < 0){
//                log_error("send DATA PACKET %d to ip:%s error", expectedAck, inet_ntoa(loaderAddr.sin_addr));
//                try++;
//            }
//        }else if(count == 0 && lastCount == blksize)
//        {
//        	//文件刚好是blocksize的整数倍时
//        	memset(buf, 0, sizeof(char) * (blksize + 4));
//        	log_debug("count = 0 and lastCount = blocksize");
//        	makeTftpData(buf, ++expectedAck);
//			//count += 4;
//			lastCount = 0;
//        	//给管理端发送一包空包 只包含4字节的包头
//			if(sendto(socketFd, buf, 4, 0, (struct sockaddr*)&loaderAddr, sizeof(struct sockaddr_in)) < 0){
//				log_error("send DATA PACKET %d to ip:%s error", expectedAck, inet_ntoa(loaderAddr.sin_addr));
//				try++;
//			}
//        	//break;
//        }
//        else{
//        	log_debug("lastcount = %d, count = %d", lastCount, count);
//            break;
//        }
        try = 1;
        socklen_t addrLen = sizeof(struct sockaddr_in);
        //int errorFlag = 0;
        unsigned short try2=1;
        while(1){
            
            if((packetLen = recvfrom(socketFd, ackPacket, sizeof(ackPacket), 0, (struct sockaddr*)&addr, &addrLen)) < 0){
                if(errno == EWOULDBLOCK || errno == EAGAIN){
                    log_debug("times up for ACK %d", expectedAck);
                    //errorFlag = 1;
                    //sendto(socketFd, buf, count, 0, (struct sockaddr*)&loaderAddr, sizeof(struct sockaddr_in));
                }
                else{
                    log_debug("recvfrom ack error");
                    //errorFlag = 1;
                    //sendto(socketFd, buf, count, 0, (struct sockaddr*)&loaderAddr, sizeof(struct sockaddr_in));
                }
            }
            else{
            	log_debug("packetLen = %d", packetLen);
                tftpAck ack = parseTftpAck(ackPacket, packetLen);
                if(ack.ackNo != expectedAck){
                    log_debug("expected to get ack %hu but get %hu", expectedAck, ack.ackNo);
                    //errorFlag = 1;
                    //sendto(socketFd, buf, count, 0, (struct sockaddr*)&loaderAddr, sizeof(struct sockaddr_in));
                    continue;
                }
                else{

                    break;
                }
            }
            if(try2 >= maxRetransmit + 1)
                break;
            else 
            {
                if(sendto(socketFd, buf, count, 0, (struct sockaddr*)&loaderAddr, sizeof(struct sockaddr_in)) < 0){
                    log_error("send to error");
                }
                else
                {
                    ++try2;
                }
            }
            
            //log_debug("try %d maxRetransmit %d", try, maxRetransmit);
            
        }
        if(try >= maxRetransmit + 1){
            log_error("up to max-retransmit");
            free(buf);
            return -1;
            break;
        }
    }
    free(buf);
    return 1;
}
#endif

#ifdef OS_HAVE_FILE_SYSTEM
int download(int socketFd, char* fileName, struct sockaddr_in loaderAddr, unsigned short blksize, unsigned short timeout, unsigned short maxRetransmit, char* lastPacket, unsigned short lastPacketLen, uint64_t* no){
	char filePathName[128] = {0};

    sprintf(filePathName, "C:/%s", fileName);

	//FILE* fp = fopen(fileName, "wb+");
	FILE* fp = fopen(filePathName, "wb+");
    if(fp == NULL){
    	log_error("file %s open unsuccessful", filePathName);
        //log_error("file %s open unsuccessful", fileName);
        return -1;
    }
    struct timeval timeout_;
    timeout_.tv_sec = timeout;
    timeout_.tv_usec = 0;
    if(setsockopt(socketFd, SOL_SOCKET, SO_RCVTIMEO, &timeout_, sizeof(timeout_)) < 0){
        log_error("set timeout arg error");
        fclose(fp); 
        return -1;
    }
    ssize_t count;
    char* buf = (char*)malloc(sizeof(char) * (blksize + 4));
    log_debug("blksize = %hu", blksize);
    log_debug("maxRetransmit = %hu", maxRetransmit);
    socklen_t addrLen = sizeof(struct sockaddr_in);
    unsigned short try = 1;
    unsigned short expectedBlockNo = 1;
    while(1){
        while((count = recvfrom(socketFd, buf, blksize + 4, 0, (struct sockaddr*)&loaderAddr, &addrLen)) < 0 ||
                parseTftpPacketType(buf, count) != DATA ||
                parseTftpAck(buf, count).ackNo != expectedBlockNo){
            log_debug("count = %hu, data block number = %d expectedBlockNo = %d", count, parseTftpAck(buf, count).ackNo, expectedBlockNo);
            if(try >= maxRetransmit + 1) break;
            else{
                sendto(socketFd, lastPacket, lastPacketLen, 0, (struct sockaddr*)&loaderAddr, sizeof(struct sockaddr_in));
            }
            ++try;
        }
        if(try >= maxRetransmit + 1){
            fclose(fp);
            free(buf);
            log_error("up to max-retransmit");
            return -1;
        }
        tftpAck ack;
        ack.ackNo = expectedBlockNo;
        lastPacketLen =  makeTftpAck(ack, lastPacket);
        sendto(socketFd, lastPacket, lastPacketLen, 0, (struct sockaddr*)&loaderAddr, sizeof(struct sockaddr_in));
        try = 1;
        log_debug("fwrite file %s.", filePathName);
        if(fwrite(buf + 4, 1, count - 4, fp) < count - 4){
            log_error("fwrite file error");
        }
        ++expectedBlockNo;
        if(no) *no = *no + 1;
        //log_debug("count - 4 = %d blksize = %d", count - 4, blksize);
        if(count - 4 != blksize){
            break;
        }
    }
    free(buf);
    fclose(fp);

    return 0;
}

#elif defined(OS_NO_FILE_SYSTEM)
int downloadflash(int socketFd, ADDRESS writeAddr, struct sockaddr_in loaderAddr, unsigned short blksize, unsigned short timeout, unsigned short maxRetransmit, char* lastPacket, unsigned short lastPacketLen, uint64_t* no){
    struct timeval timeout_;
    timeout_.tv_sec = timeout;
    timeout_.tv_usec = 0;
    if(setsockopt(socketFd, SOL_SOCKET, SO_RCVTIMEO, &timeout_, sizeof(timeout_)) < 0){
        log_error("set timeout arg error");
        return -1;
    }
    ssize_t count;
    char* buf = (char*)malloc(sizeof(char) * (blksize + 4));
    log_debug("blksize = %hu", blksize);
    log_debug("maxRetransmit = %hu", maxRetransmit);
    socklen_t addrLen = sizeof(struct sockaddr_in);
    unsigned short try = 1;
    unsigned short expectedBlockNo = 1;
    while(1){
        while((count = recvfrom(socketFd, buf, blksize + 4, 0, (struct sockaddr*)&loaderAddr, &addrLen)) < 0 ||
                parseTftpPacketType(buf, count) != DATA ||
                parseTftpAck(buf, count).ackNo != expectedBlockNo){
            log_debug("count = %hu, data block number = %d expectedBlockNo = %d", count, parseTftpAck(buf, count).ackNo, expectedBlockNo);
            if(try >= maxRetransmit + 1) break;
            else{
                sendto(socketFd, lastPacket, lastPacketLen, 0, (struct sockaddr*)&loaderAddr, sizeof(struct sockaddr_in));
            }
            ++try;
        }
        if(try >= maxRetransmit + 1){
//            fclose(fp);
            free(buf);
            log_error("up to max-retransmit");
            return -1;
        }
        tftpAck ack;
        ack.ackNo = expectedBlockNo;
        lastPacketLen =  makeTftpAck(ack, lastPacket);
        sendto(socketFd, lastPacket, lastPacketLen, 0, (struct sockaddr*)&loaderAddr, sizeof(struct sockaddr_in));
        try = 1;
//        if(fwrite(buf + 4, 1, count - 4, fp) < count - 4){
//            log_error("fwrite file error");
//        }
        ULONG ret = flashWrite(writeAddr, buf+4, count-4);
        writeAddr+=(count-4);
        log_debug("write addr = &p",writeAddr);
        if(ret <count - 4)
        {
        	log_error("fwrite file error");
        }
        ++expectedBlockNo;
        if(no) *no = *no + 1;
        //log_debug("count - 4 = %d blksize = %d", count - 4, blksize);
        if(count - 4 != blksize){
            break;
        }
    }
    free(buf);
//    fclose(fp);


    //如果没有文件系统，需要直接写入到FLASH中
#elif 0
    ssize_t count = 0;
	char* buf = (char*)malloc(sizeof(char) * (blksize + 4));
	log_debug("blksize = %hu", blksize);
	log_debug("maxRetransmit = %hu", maxRetransmit);
	socklen_t addrLen = sizeof(struct sockaddr_in);
	unsigned short try = 1;
	unsigned short expectedBlockNo = 1;
	unsigned long writeAddr = 0;
	if(NULL != strstr(fileName, "msl"))
	{
		writeAddr = FLASH_WRITE_MSL_ADDRESS;
	}else if(NULL != strstr(fileName, "LUR"))
	{
		writeAddr = FLASH_WRITE_LUR_ADDRESS;
	}else
	{

	}
	while(1){
		while((count = recvfrom(socketFd, buf, blksize + 4, 0, (struct sockaddr*)&loaderAddr, &addrLen)) < 0 ||
				parseTftpPacketType(buf, count) != DATA ||
				parseTftpAck(buf, count).ackNo != expectedBlockNo){
			log_debug("count = %hu, data block number = %d expectedBlockNo = %d", count, parseTftpAck(buf, count).ackNo, expectedBlockNo);
			if(try >= maxRetransmit + 1) break;
			else{
				sendto(socketFd, lastPacket, lastPacketLen, 0, (struct sockaddr*)&loaderAddr, sizeof(struct sockaddr_in));
			}
			++try;
		}
		if(try >= maxRetransmit + 1){
			fclose(fp);
			free(buf);
			log_error("up to max-retransmit");
			return -1;
		}
		tftpAck ack;
		ack.ackNo = expectedBlockNo;
		lastPacketLen =  makeTftpAck(ack, lastPacket);
		sendto(socketFd, lastPacket, lastPacketLen, 0, (struct sockaddr*)&loaderAddr, sizeof(struct sockaddr_in));
		try = 1;
		//if(fwrite(buf + 4, 1, count - 4, fp) < count - 4)
		if(flashWrite(writeAddr, buf + 4, count - 4) < count - 4)
		{
			log_error("fwrite flash error, addr = 0x%x", writeAddr);
		}else
		{
			//写完成后，对应的地址位向后移动对应字节的长度
			log_error("fwrite flash OK, write len = %d, addr = 0x%x.", count - 4, writeAddr);
			writeAddr += (count - 4);
		}
		++expectedBlockNo;
		if(no) *no = *no + 1;
		//log_debug("count - 4 = %d blksize = %d", count - 4, blksize);
		if(count - 4 != blksize){
			break;
		}
	}
	free(buf);

	//接收到文件后进行判断是否是压缩文件


	//如果有文件系统，从文件系统中读出文件内容，写入到FLASH中
	long filesize = 0;

	filesize = getFileSize(LUR.headerFiles[i].name);
	char *buf = (char *)malloc(sizeof(char) * filesize);



	ULONG ret = flashWrite(FLASH_WRITE_MSL_ADDRESS, buf, filesize);

	if(ret <filesize)
	{
		printf("flash write file %s Error!\n", LUR.headerFiles[i].name);
	}else
	{
		printf("flash write file %s finish!\n", LUR.headerFiles[i].name);
	}

    return 1;
}
#endif
