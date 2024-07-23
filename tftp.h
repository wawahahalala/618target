#ifndef _TFTP_H_
#define _TFTP_H_
#include <sys/socket.h>
#include <arpa/inet.h>
typedef unsigned short uint16;
typedef unsigned uint32;
typedef unsigned long long uint64;
extern const unsigned short dlp_retry;
#define max(a,b) ((a) > (b) ? (a) : (b))
#define min(a,b) ((a) < (b) ? (a) : (b))
extern const unsigned short max_tftpRequest_len;
//const unsigned short max_tftp_blksize = 16384;
#ifdef __cplusplus
extern "C"{
#endif
typedef enum tftpType{
    RRQ = 1,
    WRQ,
    DATA,
    ACK,
    ERR,
    OACK,
    UNDEFINED
}tftpType;
//标识是否为文件操作系统
//#define OS_NO_FILE_SYSTEM
#define OS_HAVE_FILE_SYSTEM
//标识是否为FLASH读写
//#define FLASH_READ_WRITE




typedef struct tftpRWRQ{
    char fileName[128];
    char mode[10];
    unsigned short timeout;
    unsigned short maxRetransmit;
    unsigned short blksize;
}tftpRWRQ;

//存储在Flash上的文件信息 add2024-06-05
typedef struct flashFileInfo{
	char fileName[64];
	char fileVersion[32];

}flashInfo;

//从字符串中解析文件信息
flashInfo getFlashInfo(const char *buf, int len);

typedef struct tftpData{
    unsigned short blockNo;
    unsigned short len;
    char data[16384];
}tftpData;

typedef struct tftpAck{
    unsigned short ackNo;
}tftpAck;

typedef struct tftpError{


}tftpError;

typedef struct tftpOAck{
    unsigned short timeout;
    unsigned short maxRetransmit;
    unsigned short blksize;
}tftpOAck;

tftpRWRQ parseTftpReadWriteRquest(char* buf, unsigned short len);

tftpType parseTftpPacketType(char* buf, unsigned short len);

tftpAck parseTftpAck(char* buf, unsigned len);

tftpOAck parseTftpOAck(char* buf, unsigned short len);


//size_t flashWriteFile(char *filename, size_t startAddr, size_t infoAddr, size_t maxFileAddr);


size_t getFileSize(char *filename);

int get(tftpRWRQ readRequest, struct sockaddr_in loaderAddr, uint64_t* no,ADDRESS writeAddr);

int put(tftpRWRQ writeRequest, struct sockaddr_in loaderAddr, size_t len, ADDRESS readAddr);

int handleGet(tftpRWRQ readRuquest, struct sockaddr_in loaderAddr, size_t len, ADDRESS readAddr);

int handlePut(tftpRWRQ writeRuquest, struct sockaddr_in loaderAddr,ADDRESS writeAddr);

int makeTftpReadRequest(tftpRWRQ readRequest, char* buf);

int makeTftpWriteRequest(tftpRWRQ writeRequest, char* buf);

int makeTftpData(char* buf, unsigned blockNo);

int makeTftpAck(tftpAck ack, char* buf);

int makeTftpError(tftpError error, char* buf);

int makeTftpOAck(tftpOAck oAck, char* buf);
#ifdef OS_HAVE_FILE_SYSTEM
int download(int socketFd, char* fileName, struct sockaddr_in loaderAddr, unsigned short blksize, unsigned short timeout, unsigned short maxRetransmit, char* lastPacket, unsigned short lastPacketLen, uint64_t* no);
int upload(int socketFd, char* fileName, struct sockaddr_in loaderAddr, unsigned short blksize, unsigned short timeout, unsigned short maxRetransmit);
#elif defined(OS_NO_FILE_SYSTEM)
int uploadflash(int socketFd, ADDRESS readAddr, struct sockaddr_in loaderAddr, unsigned short blksize, unsigned short timeout, unsigned short maxRetransmit, size_t len);
int downloadflash(int socketFd, ADDRESS writeAddr, struct sockaddr_in loaderAddr, unsigned short blksize, unsigned short timeout, unsigned short maxRetransmit, char* lastPacket, unsigned short lastPacketLen, uint64_t* no);
#endif
#ifdef __cplusplus
}
#endif
#endif
