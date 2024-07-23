#define __USE_POSIX199309
#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include "information.h"
#include "tftp.h"
#include "log.h"
#include "global.h"

#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
File_LCS LCS;
extern int tftpServerFd;
extern const unsigned blksize_default;
extern const unsigned timeout_default;
extern const unsigned max_retransmit_default;
extern const unsigned short dlp_retry;
extern struct sockaddr_in targetTftpServerAddr;
extern const unsigned short hardwaresCnt;
extern const Hardware hardwares[];


File_LCI createFileLCI(File_LCI temp)
{
    File_LCI goal;
    memcpy(&goal,&temp,sizeof(temp));
    goal.file_len=htonl(temp.file_len);
    goal.status_flag=htons(temp.status_flag);
    return goal;
}
File_LCL createFileLCL(File_LCL temp){
    File_LCL goal;
    memcpy(&goal,&temp,sizeof(temp));
    goal.file_len=htonl(temp.file_len);
    goal.Hw_num=htons(temp.Hw_num);
    for(int i=0;i<temp.Hw_num;i++)
    {
        goal.Hws[i].parts_num=temp.Hws[i].parts_num;
    }
    return goal;
    
}
File_LCS createFileLCS(File_LCS temp){
    File_LCS goal;
    memcpy(&goal,&temp,sizeof(temp));
    goal.counter=htons(temp.counter);
    goal.estimatedTime=htons(temp.estimatedTime);
    goal.exceptionTimer=htons(temp.exceptionTimer);
    goal.fileLen=htonl(temp.fileLen);
    goal.statusCode=htons(temp.statusCode);
    return goal;
}
void sendStatusFileLCS()
{
    if (LCS.statusCode == STATUS_CODE_COMPLETED)
        return;
    log_debug("start to send status file %s.LCS", TARGET_HARDWARE_IDENTIFIER);
    char fileName[128];
    sprintf(fileName, "c:/%s.LCS", TARGET_HARDWARE_IDENTIFIER);
    LCS.counter += 1;
    if (makeTempFileLCS(fileName) < 0)
    {
        log_error("MAKE STATUS FILE %s ERROR", fileName);
        return;
    }
    unsigned short try = 0;
    tftpRWRQ request;

    strcpy(request.fileName, fileName);
    request.blksize = blksize_default;
    request.maxRetransmit = max_retransmit_default;
    request.timeout = timeout_default;
    strcpy(request.mode, "octet");

    while(put(request, targetTftpServerAddr,0,NULL) < 0 && ++try < dlp_retry){}
    if(try >= dlp_retry){
        log_error("SEND STATUS FILE LUS ERROR");
        LCS.statusCode = 0x0003;
    }
    else
    {
        log_debug("SEND STATUS FILE LUS FINISHED");
    }
}
void *statusFileLCSThread(void *args)
{
    //sendStatusFileLCS();
    struct sigevent evp;
    struct itimerspec ts;
    timer_t timer;
    int ret;
    evp.sigev_value.sival_ptr = &timer;
    evp.sigev_notify = SIGEV_SIGNAL;
    evp.sigev_signo = SIGUSR1;
    signal(SIGUSR1, sendStatusFileLCS);
    ret = timer_create(0, &evp, &timer);
    if (ret)
        perror("timer_create");
    ts.it_interval.tv_sec = 1;  //间隔时间
    ts.it_interval.tv_nsec = 0;
    ts.it_value.tv_sec = 3;    //从定时器设定好到第一次生效的时间
    ts.it_value.tv_nsec = 0;
    ret = timer_settime(timer, 0, &ts, NULL);
    if (ret)
        perror("timer_settime");
    while (1)
    {
        if (LCS.statusCode == STATUS_CODE_COMPLETED)
        {
            timer_delete(timer);
            log_debug("SEND LCS THREAD END");
            break;
        }
        else if(LCS.statusCode == STATUS_CODE_ABORTED_BY_DLA || LCS.statusCode == STATUS_CODE_ABORTED_BY_OPERATOR || LCS.statusCode == STATUS_CODE_ABORTED_BY_THA){
            timer_delete(timer);
            log_debug("information operation aborted");
            break;
        }
    }
    return NULL;
}
int information(tftpRWRQ readRequest, struct sockaddr_in loader_addr)
{
    memset(&LCS, 0, sizeof(LCS));
    memcpy(LCS.version, PROTOCAL_VERSION, 2);
    targetTftpServerAddr.sin_addr = loader_addr.sin_addr;
    targetTftpServerAddr.sin_port = htons(TFTP_SERVER_PORT);
    targetTftpServerAddr.sin_family = AF_INET;
    if (makeTempFileLCI(readRequest.fileName) < 0)
    {
        log_error("LCI FILE MAKE ERROR");
        return -1;
    }
    handleGet(readRequest, loader_addr,0,NULL);
    LCS.statusCode = STATUS_CODE_ACCEPT;
    LCS.statusDescriptionLen = 0;

#ifdef __linux__
    pthread_t statusFileThreadTid;
    pthread_create(&statusFileThreadTid, NULL, statusFileLCSThread, NULL);
#elif VXWORKS
    taskSpawn("LCS_statusThread", TASK_PRI, VX_SPE_TASK, TASK_STACK_SIZE,
              (FUNCPTR)statusFileThread, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
#endif
    tftpRWRQ writeRequest;
    strcpy(writeRequest.mode, "octet");
    sprintf(writeRequest.fileName, "%s.LCL", "615A618test");
    writeRequest.blksize = blksize_default;
    writeRequest.timeout = timeout_default;
    writeRequest.maxRetransmit = max_retransmit_default;
    loader_addr.sin_port = htons(TFTP_SERVER_PORT);
    LCS.statusCode = STATUS_CODE_IN_PROGRESS;
    if (makeTempFileLCL(writeRequest.fileName) < 0)
    {
        const char *errorMessage = "LCL FILE MAKE ERROR";
        log_error("%s", errorMessage);
        LCS.statusCode = STATUS_CODE_ABORTED_BY_THA;
        strcpy(LCS.statusDescription, errorMessage);
        LCS.statusDescriptionLen = strlen(LCS.statusDescription);
#ifdef __linux__
        pthread_join(statusFileThreadTid, NULL);
#endif
        return -1;
    }
    LCS.statusCode = STATUS_CODE_IN_PROGRESS_AND_HAS_DESCRPTION;
    strcpy(LCS.statusDescription, "LCL File Make Success. About To Put LCL File");
    LCS.statusDescriptionLen = strlen(LCS.statusDescription) + 1;
    if(put(writeRequest, loader_addr,0,NULL) < 0){
        const char* errorMessage = "PUT LCL ERROR";
        log_error(errorMessage);
        LCS.statusCode = STATUS_CODE_ABORTED_BY_THA;
        strcpy(LCS.statusDescription, errorMessage);
        LCS.statusDescriptionLen = strlen(LCS.statusDescription);
#ifdef __linux__
        pthread_join(statusFileThreadTid, NULL);
#endif
        return -1;
    }
    LCS.statusCode = STATUS_CODE_COMPLETED;
    strcpy(LCS.statusDescription, "Information Completed");
    log_debug("Information Completed");
#ifdef __linux__
    pthread_join(statusFileThreadTid, NULL);
#endif
    return 1;
}

int makeTempFileLCI(char *fileName)
{
    struct File_LCI lci;
    int ret = 0;
    memcpy(lci.Pro_ver, PROTOCAL_VERSION, 2);
    lci.status_flag = 0x0001;

    FILE *fp9;
    memset(lci.stat_des, 0, sizeof(lci.stat_des));
    memcpy(lci.stat_des, "abc", strlen("abc"));
    lci.stat_des_len = strlen(lci.stat_des) + 1;
    lci.file_len = 9 + strlen(lci.stat_des);
    if ((fp9 = fopen(fileName, "wb")) == NULL)
    {
        log_error("cannot open %s\n", fileName);
        return -1;
    }

    fwrite(&lci, 9, 1, fp9);
    fwrite(&lci.stat_des, lci.stat_des_len, 1, fp9);

    log_debug("Write LCI %s completed.\n\n", fileName);
    fclose(fp9);
    fp9 = NULL;
    return 1;
}

int makeTempFileLCL(char *fileName)
{
    log_debug("make LCL begin");
    File_LCL LCL;
    memset(&LCL, 0, sizeof(File_LCL));
    FILE *fp;
    if ((fp = fopen(fileName, "wb")) == NULL)
    {
        log_error("create_LCL::cannot open file\n");
        return -1;
    }
    LCL.Hw_num = hardwaresCnt;
    LCL.Hws = hardwares;
    //fwrite(&LCL.file_len, sizeof(LCL.file_len), 1, fp2);
    fseek(fp, 4, SEEK_SET);
    fwrite(PROTOCAL_VERSION, 1, strlen(PROTOCAL_VERSION), fp);
    fwrite(&LCL.Hw_num, 1, sizeof(LCL.Hw_num), fp);
    for (int i = 0; i < LCL.Hw_num; i++)
    {
        fwrite(&LCL.Hws[i].name_len, 1, sizeof(LCL.Hws[i].name_len), fp);
        fwrite(LCL.Hws[i].name, 1, LCL.Hws[i].name_len + 1, fp);

        fwrite(&LCL.Hws[i].se_num_len, 1, sizeof(LCL.Hws[i].se_num_len), fp);
        fwrite(LCL.Hws[i].serial_num, 1, LCL.Hws[i].se_num_len + 1, fp);

        fwrite(&LCL.Hws[i].parts_num, 1, sizeof(LCL.Hws[i].parts_num), fp);
        for (int j = 0; j < LCL.Hws[i].parts_num; j++)
        {
            fwrite(&LCL.Hws[i].parts[j].parts_num_len, 1, sizeof(LCL.Hws[i].parts[j].parts_num_len), fp);
            fwrite(LCL.Hws[i].parts[j].partsNo, 1, LCL.Hws[i].parts[j].parts_num_len + 1, fp);

            fwrite(&LCL.Hws[i].parts[j].Amendment_len, 1, sizeof(LCL.Hws[i].parts[j].Amendment_len), fp);
            fwrite(LCL.Hws[i].parts[j].Amendment, 1, LCL.Hws[i].parts[j].Amendment_len + 1, fp);

            fwrite(&LCL.Hws[i].parts[j].Part_Designation_Length, 1, sizeof(LCL.Hws[i].parts[j].Part_Designation_Length), fp);
            fwrite(LCL.Hws[i].parts[j].Part_Designation_Text, 1, LCL.Hws[i].parts[j].Part_Designation_Length + 1, fp);
        }
    }
    LCL.file_len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    fwrite(&LCL.file_len, 1, sizeof(LCL.file_len), fp);
    fclose(fp);
    log_debug("make LCL finish");
    return 1;
}

int makeTempFileLCS(char *fileName)
{
    log_debug("make LCS begin");
    FILE *fp = fopen(fileName, "w");
    if (fp == NULL)
    {
        log_error("open file %s error errno = %d", fileName, errno);
        return -1;
    }
    fseek(fp, 4, SEEK_SET);
    fwrite(&LCS.version, 1, 2, fp);
    fwrite(&LCS.counter, 1, 2, fp);
    fwrite(&LCS.statusCode, 1, 2, fp);
    fwrite(&LCS.exceptionTimer, 1, 2, fp);
    fwrite(&LCS.estimatedTime, 1, 2, fp);
    fwrite(&LCS.statusDescriptionLen, 1, 1, fp);
    fwrite(&LCS.statusDescription, 1, LCS.statusDescriptionLen + 1, fp); //+1是为了加上结束符0
    LCS.fileLen = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    fwrite(&LCS.fileLen, 1, sizeof(LCS.fileLen), fp);
    fclose(fp);
}
