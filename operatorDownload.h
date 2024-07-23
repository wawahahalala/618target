#ifndef _OPERATORDOWNLOAD_H_
#define _OPERATORDOWNLOAD_H_
#include "tftp.h"
#ifdef __cplusplus
extern "C"{
#endif
typedef unsigned int uint32;
typedef unsigned short uint16;
typedef struct File_LNO
{
    uint32 file_len;
    uint16 pro_ver;
    uint16 status_code;
    unsigned char status_des_len;
    char status_des[255];
}File_LNO;


typedef struct LNS_DataFile 
{
    unsigned char filename_len;
    char filename[255];
    uint16 file_status;
    unsigned char file_status_des_len;
    char file_status_des[255];

}LNS_DataFile;

typedef struct File_LNS
{
    uint32 file_len;
    uint16 pro_ver;
    uint16 status_code;
    unsigned char status_des_len;
    char status_des[255];
    uint16 count;
    uint16 time_wait;
    uint16 time_estimate;
    char load_list_completion_ratio[3];
    uint16 numOfDataFile;
    LNS_DataFile* datafile;//数据文件
}File_LNS;

typedef struct LNL_DataFile 
{
    unsigned char filename_len;
    char filename[255];
    unsigned char file_des_len;
    char file_des[255];
}LNL_DataFile;

typedef struct File_LNL
{
    uint32 file_len;
    uint16 pro_ver;
    uint16 numoffiles;
    LNL_DataFile* datafile;
}File_LNL;

typedef struct LNA_DataFile 
{
    unsigned char filename_len;
    char filename[255];
}LNA_DataFile;

typedef struct File_LNA
{
    uint32 file_len;
    uint16 pro_ver;
    uint16 numoffiles;
    LNA_DataFile* datafile;
}File_LNA;



int putWriteRequest_LNL();
void putWriteRequest_LNS();
void* periodicallySendStatusFile(void* arg);
int operatorDownload_615a(tftpRWRQ readRequest, struct sockaddr_in loader_addr);
int makeTempFileLNO(char* fileName);
int makeTempFileLNL(char* fileName);
int makeTempFileLNS(char* fileName);
int parseLnaFile(char* fileName, File_LNA* lna);

File_LNS createFileLNS_OD(File_LNS temp);
File_LNL createFileLNLI(File_LNL temp);
File_LNO createFileLNO(File_LNO temp);
#ifdef __cplusplus
}
#endif
#endif
