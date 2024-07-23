#ifndef _INFORMATION_H_
#define _INFORMATION_H_
#include "tftp.h"
#include <arpa/inet.h>
#ifdef __cplusplus
extern "C"{
#endif

typedef struct File_LCI{
    uint32 file_len;//�ļ���С
    char Pro_ver[2];//Э��汾��
    uint16 status_flag;//״̬��־��
    unsigned char stat_des_len;//״̬��������
    char stat_des[255];//״̬����
}File_LCI;

typedef struct File_LCL{
    uint32 file_len;//�ļ���С
    char Pro_ver[2];//Э��汾��
    uint16 Hw_num;//Ӳ����
    struct Hardware *Hws;//Ӳ����Ϣ
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
    unsigned char parts_num_len;//�����ŵ��ַ�����
    char partsNo[256];//������
    unsigned char Amendment_len;//�����ַ�����
    char Amendment[256];//����ֵ
    unsigned char Part_Designation_Length;//������������
    char Part_Designation_Text[256];//ά����Ϣ
}Part;

typedef struct Hardware{
    unsigned char name_len;//�������Ƴ���
    char name[256];//��������
    unsigned char se_num_len;//���кų���
    char serial_num[256];//���к�
    uint16 parts_num;//��������
    struct Part *parts;//������Ϣ
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
