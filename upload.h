#ifndef _UPLOAD_H_
#define _UPLOAD_H_
#include "tftp.h"
#ifdef __cplusplus
extern "C"{
#endif
typedef struct File_LUI{//confirm
    uint32 file_len;//�ļ���С
    char Pro_ver[2];//Э��汾��
    uint16 status_flag;//״̬��־��
    unsigned char stat_des_len;//״̬��������
    char stat_des[255];//״̬����
}File_LUI;

typedef struct HeaderFile{
    uint64_t fileLen;      //ͷ�ļ���С������鿴����
    unsigned char name_len;//ͷ�ļ�������
    char name[256];//ͷ�ļ���,������255���ַ�������'\0'��β������Ҫ���鳤��Ϊ256
    unsigned char load_part_len_name;//���ز��ֳ���
    char load_part_name[256];//���ز����ļ���
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
    uint16_t targetIdLength;  //Ŀ���ID����
    char targetId[65536];     //Ŀ���ID
}TargetId;


typedef struct File{
    uint16_t nextDataFileOffset;  //��һ�������ļ�ƫ����
    uint16_t dataFileNameLen;    //�����ļ����ֳ���
    char dataFileName[65536];    //�����ļ���
    uint16_t loadPartNameLen;    //�����ļ��Ĳ����볤��
    char loadPartName[65536];    //�����ļ��Ĳ�����
    uint32_t dataFileSize;       //�����ļ���С
    uint16_t crc;                //�����ļ���CRCУ��ֵ
    //��չ��3--------TODO
}File;


typedef struct File_LUH{
    uint32_t fileLen; //�ļ�����
    char version[2];  //����ģʽ�汾
    uint16_t backup;  //����
    uint32_t loadPartNoOffset;  //���ز�����ƫ��
    uint32_t targetIDListOffset;  //Ŀ���ID�б�ƫ��
    uint32_t dataFileListOffset;  //�����ļ��б�ƫ��
    uint32_t supportFileListOffset;  //֧���ļ��б�ƫ��
    uint32_t userDefinedFieldOffset;  //�û��Զ�����ƫ��

    //��չ��1-------TODO
    uint16_t loadPartNoLen;  //���ز����볤��
    char loadPartNo[65536];  //������
    uint16_t targetIDNo;     //Ŀ���ID��
    TargetId* targetIds;     //Ŀ���ID

    uint16_t dataFileCount;  //�����ļ�����
    File* dataFiles;     //�����ļ�

    //��չ��2-------TODO
    uint16_t supportFileCount;  //֧���ļ�����
    File* supportFiles;         //֧���ļ�

    //��չ��4-------TODO
    char userDefine[256];       //�û��Զ���
    uint16_t crc16;             //ͷ�ļ�CRCУ��ֵ(LUH�ļ��г�ȥ��Header File CRC, Load CRC��CRC16����ֵ)
    uint32_t crc32;             //����CRCУ��ֵ��CRC32(����LUH�ļ���ȥ��Load CRC��CRC32ֵ�������ļ���CRC32ֵ)



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
