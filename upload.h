#ifndef _UPLOAD_H_
#define _UPLOAD_H_
#include "tftp.h"
#ifdef __cplusplus
extern "C"{
#endif
typedef struct File_LUI{//confirm
    uint32 file_len;//文件大小
    char Pro_ver[2];//协议版本号
    uint16 status_flag;//状态标志码
    unsigned char stat_des_len;//状态描述长度
    char stat_des[255];//状态描述
}File_LUI;

typedef struct HeaderFile{
    uint64_t fileLen;      //头文件大小，方便查看进度
    unsigned char name_len;//头文件名长度
    char name[256];//头文件名,最多包含255个字符，且以'\0'结尾，故需要数组长度为256
    unsigned char load_part_len_name;//加载部分长度
    char load_part_name[256];//加载部分文件名
}HeaderFile;

typedef struct File_LUR{
    uint32 fileLength;
    char protocalVersion[2];
    uint16 numOfHeaderFiles;
    HeaderFile* headerFiles;
}File_LUR;

typedef struct HeaderFilePlus{
    HeaderFile headerFile;
    char loadRatio[3];
    uint16 loadStatus;
    unsigned char loadStatusDescriptionLength;
    char loadStatusDescription[256];
}HeaderFilePlus;

typedef struct File_LUS{
    uint32 fileLength;
    char protocalVersion[2];
    uint16 statusCode;
    unsigned char descriptionLen;
    char description[256];
    uint16 counter;
    uint16 exceptionTimer;
    uint16 estimatedTime;
    char loadListRatio[3];
    uint16 numOfHeaderFiles;
    HeaderFilePlus* headerFiles;
}File_LUS;

typedef struct TargetId{
    uint16_t targetIdLength;  //目标机ID长度
    char targetId[65536];     //目标机ID
}TargetId;


typedef struct File{
    uint16_t nextDataFileOffset;  //下一个数据文件偏移量
    uint16_t dataFileNameLen;    //数据文件名字长度
    char dataFileName[65536];    //数据文件名
    uint16_t loadPartNameLen;    //数据文件的部件码长度
    char loadPartName[65536];    //数据文件的部件码
    uint32_t dataFileSize;       //数据文件大小
    uint16_t crc;                //数据文件的CRC校验值
    //扩展点3--------TODO
}File;


typedef struct File_LUH{
    uint32_t fileLen; //文件长度
    char version[2];  //加载模式版本
    uint16_t backup;  //备份
    uint32_t loadPartNoOffset;  //加载部件码偏移
    uint32_t targetIDListOffset;  //目标机ID列表偏移
    uint32_t dataFileListOffset;  //数据文件列表偏移
    uint32_t supportFileListOffset;  //支持文件列表偏移
    uint32_t userDefinedFieldOffset;  //用户自定义域偏移

    //扩展点1-------TODO
    uint16_t loadPartNoLen;  //加载部件码长度
    char loadPartNo[65536];  //部件码
    uint16_t targetIDNo;     //目标机ID数
    TargetId* targetIds;     //目标机ID

    uint16_t dataFileCount;  //数据文件个数
    File* dataFiles;     //数据文件

    //扩展点2-------TODO
    uint16_t supportFileCount;  //支持文件个数
    File* supportFiles;         //支持文件

    //扩展点4-------TODO
    char userDefine[256];       //用户自定义
    uint16_t crc16;             //头文件CRC校验值(LUH文件中除去域Header File CRC, Load CRC的CRC16计算值)
    uint32_t crc32;             //加载CRC校验值，CRC32(包括LUH文件除去域Load CRC的CRC32值、数据文件的CRC32值)



}File_LUH;

int upload_615a(tftpRWRQ readRequest, struct sockaddr_in loader_addr);
int makeTempFileLUI(char* fileName);
int makeTempFileLUS(char* fileName);
int parseTempFileLUR(char* fileName, File_LUR* LUR);
int decompAndBspatch(char *fileName,char *outfileName,char *currentFileName);
//char *getCurrentFilename(char *type,ADDRESS infoAddr);
//char *getnewFileName(char *type);
#ifdef __cplusplus
}
#endif
#endif
