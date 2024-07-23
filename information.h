#ifndef _INFORMATION_H_
#define _INFORMATION_H_
#include "tftp.h"
#include <arpa/inet.h>
#ifdef __cplusplus
extern "C"{
#endif

typedef struct File_LCI{
    uint32 file_len;//文件大小
    char Pro_ver[2];//协议版本号
    uint16 status_flag;//状态标志码
    unsigned char stat_des_len;//状态描述长度
    char stat_des[255];//状态描述
}File_LCI;

typedef struct File_LCL{
    uint32 file_len;//文件大小
    char Pro_ver[2];//协议版本号
    uint16 Hw_num;//硬件数
    struct Hardware *Hws;//硬件信息
}File_LCL;

typedef struct File_LCS{
    uint32 fileLen;
    char version[2];
    uint16 counter;
    uint16 statusCode;
    uint16 exceptionTimer;
    uint16 estimatedTime;
    unsigned char statusDescriptionLen;
    char statusDescription[256];
}File_LCS;


typedef struct Part{
    unsigned char parts_num_len;//部件号的字符个数
    char partsNo[256];//部件号
    unsigned char Amendment_len;//修正字符个数
    char Amendment[256];//修正值
    unsigned char Part_Designation_Length;//部件描述长度
    char Part_Designation_Text[256];//维护信息
}Part;

typedef struct Hardware{
    unsigned char name_len;//文字名称长度
    char name[256];//文字名称
    unsigned char se_num_len;//序列号长度
    char serial_num[256];//序列号
    uint16 parts_num;//部件个数
    struct Part *parts;//部件信息
}Hardware;

int information(tftpRWRQ readRequest, struct sockaddr_in loader_addr);
int makeTempFileLCI(char* fileName);
int makeTempFileLCL(char* fileName);
int makeTempFileLCS(char* fileName);
File_LCI createFileLCI(File_LCI temp);
File_LCL createFileLCL(File_LCL temp);
File_LCS createFileLCS(File_LCS temp);
#ifdef __cplusplus
}
#endif
#endif
