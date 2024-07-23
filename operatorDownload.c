#include "operatorDownload.h"
#include <string.h>
#include <dirent.h>
#include "global.h" 
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <sys/stat.h>
#include "log.h"


File_LNS lns;
File_LNA lna;

extern struct sockaddr_in targetTftpServerAddr;
extern ADDRESS infoaddr;
extern int tftpServerFd;
// uint64_t no1;
unsigned short stage = 1;
int pointerOfCurrentSendingDataFile = 0;
double transfer_speed = 0;//当前的传输速率
time_t startTime;//某个数据文件的开始传输时刻
off_t total_transfered_size = 0;//数据文件已经传输成功的总字节数
off_t total_dataFile_size = 0 ;
ADDRESS	dmiddleFileStart = 0x0;
File_LNO createFileLNO(File_LNO temp)
{
    File_LNO goal;
    memcpy(&goal,&temp,sizeof(temp));
    goal.file_len=htonl(temp.file_len);
    goal.pro_ver=htons(temp.pro_ver);
    goal.status_code=htons(temp.status_code);
    return goal;
}
File_LNL createFileLNL(File_LNL temp)
{
    File_LNL goal;
    memcpy(&goal,&temp,sizeof(temp));
    goal.file_len=htonl(temp.file_len);
    goal.numoffiles=htons(temp.numoffiles);
    goal.pro_ver=htons(temp.pro_ver);
    return goal;
}

File_LNS createFileLNS_OD(File_LNS temp)
{
	File_LNS goal;
	memcpy(&goal,&temp,sizeof(temp));
	goal.status_code=htons(temp.status_code);
	goal.count=htons(temp.count);
	goal.time_wait=htons(temp.time_wait);
	goal.time_estimate=htons(temp.time_estimate);
	goal.file_len=htonl(temp.file_len);
	goal.numOfDataFile=htons(temp.numOfDataFile);
	goal.pro_ver=htons(temp.pro_ver);
    
	for(int i=0;i<temp.numOfDataFile;i++)
	{
		goal.datafile[i].file_status=htons(temp.datafile[i].file_status);
	}
	return goal;
}

void* periodicallySendStatusFile(void* arg){
//    struct sigevent sigev;//设置定时器事件的通知方式和信号值
//    struct itimerspec its;//设置定时器的
//    timer_t timerid;//定时器标识符
//
//    memset(&sigev, 0, sizeof(sigev));
//    sigev.sigev_notify = SIGEV_SIGNAL;//定时器到期会发出一个信号
//    sigev.sigev_signo = SIGALRM;//定时器发送的信号名为SIGARLM
//    sigev.sigev_value.sival_ptr = &timerid;//将定时器标识符的地址存储在其中，以便在信号处理函数中使用。
//    signal(SIGALRM, putWriteRequest_LNS);
//    if(timer_create(CLOCK_REALTIME, &sigev, &timerid) == -1){
//        perror("TIMER CREATE");
//        exit(1);
//    }//创建一个定时器
//    its.it_interval.tv_sec = 3;//设置定时器的重复间隔,设置单位秒
//    its.it_interval.tv_nsec = 0;//设置定时器的重复间隔，设置单位纳秒
//    its.it_value.tv_sec = 3;//设置第一次到期的时间,设置单位秒
//    its.it_value.tv_nsec = 0;//设置第一次到期的时间,设置单位纳秒
//     // 启动定时器
//    if (timer_settime(timerid, 0, &its, NULL) == -1) {
//        perror("timer_settime");
//        exit(1);
//    }
    //设置信号处理函数
    // struct sigaction sa;
    // memset(&sa, 0, sizeof(sa));
    // sa.sa_handler = putWriteRequest_LNS;
    // sigaction(SIGALRM, &sa, NULL);//每发出一个SIGALRM信号就执行一次putWriteRequest_LNS("stage two")
    // log_debug("set handle funtion success");

    while(1){
    	putWriteRequest_LNS();
    	usleep(1000000);
        if(lns.status_code == 0x0003){
//            timer_delete(timerid);
//            stage++;
            putWriteRequest_LNS();
            break;
        }
    }

    log_debug("periodicallySendStatusFile Quit!");
}



int operatorDownload_615a(tftpRWRQ readRequest, struct sockaddr_in loader_addr)
{
	stage=1;
    memset(&lna, 0 ,sizeof(lna));
    memset(&lns, 0, sizeof(lns));
    // lns.datafile = (LNS_DataFile*)malloc(sizeof(LNS_DataFile));
    targetTftpServerAddr.sin_addr = loader_addr.sin_addr;
    targetTftpServerAddr.sin_port = htons(TFTP_SERVER_PORT);
    targetTftpServerAddr.sin_family = AF_INET;
    //设备端创建对应的LNO文件
    if(makeTempFileLNO(readRequest.fileName) < 0){
        log_error("MAKE LNO FILE ERROR");
        return -1;
    }
    //如果LNO创建成功就继续后面的LNO文件传输过程
    if(handleGet(readRequest, loader_addr,0,NULL) < 0){
        log_error("HANDLE LNO GET REQUEST ERROR");
        return -1;
    }
    log_debug("send lno file %s success", readRequest.fileName);

    // lns.file_len = sizeof(lns);
    // lns.pro_ver = 0xA3;
    // lns.status_code = 0x0001;
    // strcpy(lns.status_des, "The Target accepts the operation(not yet started)");
    // lns.status_des_len = strlen(lns.status_des);
    // lns.count += 1;

    //发送LNS写请求
    putWriteRequest_LNS();

    pthread_t periodicallySendStatusFileId;
    pthread_create(&periodicallySendStatusFileId, NULL, periodicallySendStatusFile, NULL);

    
    if(putWriteRequest_LNL() < 0 ){
        log_error("PUT LNL WRITE REQUEST ERROR");
        return -1;
    }

    //后边的不与设备进行交互
    //用户选择了要拉取的文件后，直接从数据库中获取该文件，不需要从设备中获取

    char tftpRequestBuf[max_tftpRequest_len];
    socklen_t addr_len = sizeof(struct sockaddr_in);
//    log_error("11");
    ssize_t len = recvfrom(tftpServerFd, tftpRequestBuf, max_tftpRequest_len, 0, (struct sockaddr*)&loader_addr, &addr_len);
    tftpRWRQ request = parseTftpReadWriteRquest(tftpRequestBuf, len);//收到的LNA写文件请求
    if (handlePut(request, loader_addr,dmiddleFileStart) < 0) {
        log_error("HANDLE LNA REQUEST ERROR");
        return -1;
    }
    log_debug("LNA FILE RECEIVED");
    //解析接收到的.LNA文件
//     File_LNA lna;
    if (parseLnaFile(request.fileName, &lna) < 0) {
        log_error("PARSE LNA FILE ERROR");
        return -1;
    }
    //获取LNA文件中数据文件的总长度
    for (int i = 0; i < lna.numoffiles; i++)
    {
        struct stat dataFile_stat;
        char filePathName[128] = {0};
        sprintf(filePathName, "C:/%s", lna.datafile[i].filename);
        //if(stat(lna.datafile[i].filename, &dataFile_stat) == 0)
        if(stat(filePathName, &dataFile_stat) == 0)
        {
            log_debug("file %s length available", lna.datafile[i].filename);
        }else{
            log_error("file %s length unavailable", lna.datafile[i].filename);
            return -1;
        }
        log_debug("%d", dataFile_stat.st_size);
        total_dataFile_size += dataFile_stat.st_size;
    }
    
    // startTime = time(NULL);//开始计时
    lns.datafile = (LNS_DataFile*)malloc(sizeof(LNS_DataFile) * lna.numoffiles);
    stage++;

    for(int i=0; i < lna.numoffiles; i++){
        FILE* fp;
        //检查该文件在本地存不存在
        char filePathName[128] = {0};
        sprintf(filePathName, "C:/%s", lna.datafile[i].filename);
        //Todo 如果是Flash则需要检查对应的地址处是否有文件
        if((fp = fopen(filePathName, "rb")) == NULL)
        //if((fp = fopen(lna.datafile[i].filename, "rb")) == NULL)
        {
            log_error("CAN NOT FIND FILE");
            return -1;
        }
        fclose(fp);
        //如果存在则继续
        tftpRWRQ writeRequest;
        //unsigned short try = 0 ;
        writeRequest.blksize = 512;
        writeRequest.maxRetransmit = 5;
        writeRequest.timeout = 2;
        strcpy(writeRequest.fileName, lna.datafile[i].filename);
        strcpy(writeRequest.mode, "octet");
        // pointerOfCurrentSendingDataFile++;
        // startTime = time(NULL);
        // loader_addr.sin_port = htons(59);
        // off_t last_total_transfered_size = total_transfered_size;//为了保证在第一次传输失败的前提下，第二次传输时能够回退
        // while(put(writeRequest, targetTftpServerAddr) < 0 && ++try < dlp_retry){
        //     // total_transfered_size = last_total_transfered_size;//如果第一次传输失败就重置
        // }
        // if(try >= dlp_retry){
        //     log_error("DATA FILE TRANSMIT ERROR");
        //     pointerOfCurrentSendingDataFile--;//传输失败就回滚指针
        //     return -1;
        // }else{
        //     log_debug("DATA FILE %s TRANSMIT SUCCESS", writeRequest.fileName);
        //     pointerOfCurrentSendingDataFile++;
        //     // pointerOfCurrentSendingDataFile++;
        // }
        if(put(writeRequest, targetTftpServerAddr,0,NULL) < 0){
            log_error("upload file %s error", writeRequest.fileName);
        }else{
            pointerOfCurrentSendingDataFile++;
        }
    }
    lns.status_code = 0x0003;
    sleep(2);

    return 1;
}

int makeTempFileLNO(char *fileName)
{
    struct File_LNO lno;
    lno.pro_ver = 0xA3;
    lno.status_code = 0x0001;
    char *str1 = "abc";

    strcpy(lno.status_des, str1);
    lno.status_des_len = strlen(lno.status_des);
    lno.file_len = 9 + strlen(lno.status_des);
#ifdef OS_HAVE_FILE_SYSTEM

    char filePathName[128] = {0};
    sprintf(filePathName, "C:/%s", fileName);

    FILE *fp;
    //if((fp = fopen(fileName,"wb")) == NULL){
    if((fp = fopen(filePathName,"wb")) == NULL){
        log_error("cannot open file_LNO"); 
        return -1;
    }
    //log_debug("make lno file %s success", fileName);
    log_debug("make lno file %s success", filePathName);
    fwrite(&lno, lno.file_len, 1, fp);
    fclose(fp);
    fp = NULL;
#elif defined(OS_NO_FILE_SYSTEM)
    ULONG flashRet = 0;
    log_debug("*NO FILE_WRITE_TO_FLASH Begining!");
    flashRet = flashWrite(FLASH_WRITE_LNO_ADDRESS, &lno, lno.file_len);
    if(flashRet == lno.file_len)
    {
    	log_debug("*NO FILE_WRITE_TO_FLASH finished, len = %ld, lno.file_len = %ld", flashRet, lno.file_len);
    }else
    {
    	log_error("*NO FILE_WRITE_TO_FLASH Error, len = %ld, lno.file_len = %ld", flashRet, lno.file_len);
    }

#endif

    return 1;
}

int makeTempFileLNL(char * fileName)
{
#ifdef OS_HAVE_FILE_SYSTEM
    DIR* currentDir;
    struct dirent* currentFile;
    struct LNL_DataFile listOfDataFiles[255];
    int dataFileCount = 0;
    int dataFileLenSum = 0;

    if ((currentDir = opendir("C:/")) == NULL) {
        log_error("OPEN CURRENT DIR ERROR");
        return -1;
    }
    log_debug("OPEN DIR");
    while ((currentFile = readdir(currentDir)) != NULL) {
        // log_debug("fileName = %s", currentFile->d_name);
        if (strstr(currentFile->d_name, ".bin") != NULL) {//当文件是.bin
            log_debug("%s", currentFile->d_name);
            strcpy(listOfDataFiles[dataFileCount].filename, currentFile->d_name);
            log_debug("%d", strlen(currentFile->d_name));
            listOfDataFiles[dataFileCount].filename_len = strlen(currentFile->d_name);
            strcpy(listOfDataFiles[dataFileCount].file_des, "TEST");
            listOfDataFiles[dataFileCount].file_des_len = strlen("TEST");
            dataFileCount++;
            dataFileLenSum += strlen("TEST") + 2 + strlen(currentFile->d_name);
            // log_debug("dataFileLenSum = %d", dataFileLenSum);
            if (dataFileCount >= 255) {
                // log_error("MAX FILE REACHED, LNL FILE GENERATION FAILED");
                //return -1;
            	log_error("MAX FILE REACHED!");
            	break;
            }
        }
    }
    //Todo 怎样获取文件列表？




    struct File_LNL lnl;
    lnl.file_len = 8 + dataFileLenSum;
    lnl.pro_ver = 0xA3;
    lnl.numoffiles = dataFileCount;
    lnl.datafile = listOfDataFiles;

    //char filePathName[128] = {0};
    //sprintf(filePathName, "C:/%s", fileName);


    FILE* fp;
    if ((fp = fopen(fileName, "wb")) == NULL) {
    //if ((fp = fopen(filePathName, "wb")) == NULL) {
        log_error("cannot open file LNL");
        return -1;
    }
    
    //将除了数据文件的部分写入.LNL中
    fwrite(&lnl.file_len, sizeof(lnl.file_len), 1, fp);
    fwrite(&lnl.pro_ver, sizeof(lnl.pro_ver), 1, fp);
    fwrite(&lnl.numoffiles, sizeof(lnl.numoffiles), 1, fp); 
    //将数据文件写入.LNL中
    for(int i=0; i<dataFileCount; i++){
        log_debug("%d", lnl.datafile[i].filename_len);
        log_debug("%s", lnl.datafile[i].filename);
        fwrite(&lnl.datafile[i].filename_len, sizeof(lnl.datafile[i].filename_len), 1, fp);
        fwrite(&lnl.datafile[i].filename, sizeof(lnl.datafile[i].filename), 1, fp);
        fwrite(&lnl.datafile[i].file_des_len, sizeof(lnl.datafile[i].file_des_len), 1, fp);
        fwrite(&lnl.datafile[i].file_des, sizeof(lnl.datafile[i].file_des), 1, fp);
    }
    // fwrite(&lnl, lnl.file_len, 1, fp);
    fclose(fp);
    fp = NULL;
#elif defined(OS_NO_FILE_SYSTEM)
   char fileNameLen;
   int num = 0;
   size_t FileLen = 0;
   total_size = 0;
   for(int i = 0;i<3;i++)
   {

	   memcpy(&fileNameLen,(void*)infoaddr,1);
	   infoaddr += 2;
	   log_debug("fileNameLen = %d",fileNameLen);
	   num = fileNameLen - '0';
	   char *fileName = (char*)malloc(num);
	   memcpy(fileName,(void*)infoaddr,num);
	   infoaddr += (num+1);
	   memcpy((size_t)&FileLen,(void*)infoaddr,sizeof(size_t));
	   log_debug("fileName = %s",fileName);
	   strcpy(listOfDataFiles[dataFileCount].filename, fileName);
	   log_debug("fileNameLen = %d",num);
	   listOfDataFiles[i].filename_len = num;
	   strcpy(listOfDataFiles[i].file_des, "TEST");
	   listOfDataFiles[i].file_des_len = strlen("TEST");
	   dataFileLenSum += strlen("TEST") + 2 + strlen(fileName);
   }
   struct File_LNL lnl;
   lnl.file_len = 8 + dataFileLenSum;
   lnl.pro_ver = 0xA3;
   lnl.numoffiles = 3;
   lnl.datafile = listOfDataFiles;
   char *buf = (char*)malloc(lnl.file_len);
   memcpy(buf,&lui,8);
   int pos=8;
   for(int j = 0;j<3;j++)
   {
	   memcpy(buf+pos,&lnl.datafile[i].filename_len,sizeof(lnl.datafile[i].filename_len));
	   pos += sizeof(lnl.datafile[i].filename_len);
	   memcpy(buf+pos,&lnl.datafile[i].filename, sizeof(lnl.datafile[i].filename));
	   pos += sizeof(lnl.datafile[i].filename);
	   memcpy(buf+pos,&lnl.datafile[i].file_des_len, sizeof(lnl.datafile[i].file_des_len));
	   pos += sizeof(lnl.datafile[i].file_des_len);
	   memcpy(buf+pos,&lnl.datafile[i].file_des, sizeof(lnl.datafile[i].file_des));
	   pos += sizeof(lnl.datafile[i].file_des);
   }
   free(buf);
   free(filename);
#endif
    return 1;

}

int makeTempFileLNS(char *fileName)
{
#ifdef OS_HAVE_FILE_SYSTEM
    FILE* fp;
    //char filePathName[128] = {0};
    //sprintf(filePathName, "C:/%s", fileName);

    if((fp = fopen(fileName, "wb")) == NULL){
        log_error("CAN NOT OPEN LNS FILE %s ", fileName);
        return -1;
    }
    if(fwrite(&lns, 1, 9, fp) < 9){
        log_error("MAKE LNS FILE %s ERROR", fileName);
        return -1;
    }
    if(fwrite(lns.status_des, 1, lns.status_des_len, fp) < lns.status_des_len){
        log_error("MAKE LNS FILE %s ERROR",fileName);
        return -1;
    }
    if(fwrite(&lns.count, 1, 6, fp) < 6){
        log_error("MAKE LNS FILE %s ERROR", fileName);
        return -1;
    }
    if(fwrite(lns.load_list_completion_ratio, 1, strlen(lns.load_list_completion_ratio), fp) < strlen(lns.load_list_completion_ratio)){
        log_error("MAKE LNS FILE %s ERROR", fileName);
        return -1;
    }
    if(fwrite(&lns.numOfDataFile, 1, 2, fp) < 2){
        log_error("MAKE LNS FILE %s ERROR", fileName);
        return -1;
    }
    
    for(int i=0; i<lns.numOfDataFile; i++){
        fwrite(&lns.datafile[i].filename_len, 1, 1, fp);
        fwrite(lns.datafile[i].filename, 1, lns.datafile[i].filename_len, fp);
        fwrite(&lns.datafile[i].file_status, 1, 2, fp);
        fwrite(&lns.datafile[i].file_status_des_len, 1, 1, fp);
        fwrite(lns.datafile[i].file_status_des, 1, lns.datafile[i].file_status_des_len, fp);
    }
    fclose(fp);
    log_debug("make lns file %s success", fileName);

#elif defined(OS_NO_FILE_SYSTEM)
    ULONG fwsize = 0;
    unsigned long flashAddr = FLASH_WRITE_LNS_ADDRESS;
    fwsize = flashWrite(flashAddr, (char*)&lns, 9);
    flashAddr += 9;
    //fwrite(lns.status_des, 1, lns.status_des_len, fp)
    fwsize = flashWrite(flashAddr, lns.status_des, lns.status_des_len);
    flashAddr += lns.status_des_len;
    //fwrite(&lns.count, 1, 6, fp)
    fwsize = flashWrite(flashAddr, &lns.count, 6);
    flashAddr += 6;
    //fwrite(lns.load_list_completion_ratio, 1, strlen(lns.load_list_completion_ratio), fp)
    fwsize = flashWrite(flashAddr, lns.load_list_completion_ratio, strlen(lns.load_list_completion_ratio));
    flashAddr += strlen(lns.load_list_completion_ratio);
    //fwrite(&lns.numOfDataFile, 1, 2, fp);
    fwsize = flashWrite(flashAddr, &lns.numOfDataFile, 2);
    flashAddr += 2;
    //
    for(int i=0; i<lns.numOfDataFile; i++){
    	//fwrite(&lns.datafile[i].filename_len, 1, 1, fp);
    	fwsize = flashWrite(flashAddr, &lns.datafile[i].filename_len, 1);
    	flashAddr += 1;
		//fwrite(lns.datafile[i].filename, 1, lns.datafile[i].filename_len, fp);
    	fwsize = flashWrite(flashAddr, lns.datafile[i].filename, lns.datafile[i].filename_len);
    	flashAddr += lns.datafile[i].filename_len;
		//fwrite(&lns.datafile[i].file_status, 1, 2, fp);
    	fwsize = flashWrite(flashAddr, &lns.datafile[i].file_status, 2);
    	flashAddr += 2;
		//fwrite(&lns.datafile[i].file_status_des_len, 1, 1, fp);
    	fwsize = flashWrite(flashAddr, &lns.datafile[i].file_status_des_len, 1);
    	flashAddr += 1;
		//fwrite(lns.datafile[i].file_status_des, 1, lns.datafile[i].file_status_des_len, fp);
    	fwsize = flashWrite(flashAddr, lns.datafile[i].file_status_des, lns.datafile[i].file_status_des_len);
    	flashAddr += lns.datafile[i].file_status_des_len;
    }
#endif

    return 1;
}

int parseLnaFile(char* fileName, File_LNA* lna)
{
    if (lna == NULL) {
        log_error("NULL POINTER LNA");
        return -1;
    }

#ifdef OS_HAVE_FILE_SYSTEM
    char filePathName[128] = {0};
    sprintf(filePathName, "C:/%s", fileName);

    FILE* fp;
    if ((fp = fopen(filePathName, "rb")) == NULL) {
        log_error("OPEN LNA FILE ERROR");
        return -1;
    }

    fread(&lna->file_len, sizeof(lna->file_len), 1, fp);
    fread(&lna->pro_ver, sizeof(lna->pro_ver), 1, fp);
    fread(&lna->numoffiles, sizeof(lna->numoffiles), 1, fp);
    lna->datafile = (LNA_DataFile*)malloc(sizeof(LNA_DataFile) * lna->numoffiles);
    for(int i = 0; i < lna->numoffiles; i++){
        fread(&lna->datafile[i].filename_len,sizeof(lna->datafile[i].filename_len), 1, fp);
        fread(&lna->datafile[i].filename,sizeof(lna->datafile[i].filename), 1, fp);
    }
    fclose(fp);
    //Todo
//    fread(&lna->file_len, sizeof(lna->file_len), 1, fp);
//	fread(&lna->pro_ver, sizeof(lna->pro_ver), 1, fp);
//	fread(&lna->numoffiles, sizeof(lna->numoffiles), 1, fp);
//	lna->datafile = (LNA_DataFile*)malloc(sizeof(LNA_DataFile) * lna->numoffiles);
//	for(int i = 0; i < lna->numoffiles; i++){
//		fread(&lna->datafile[i].filename_len,sizeof(lna->datafile[i].filename_len), 1, fp);
//		fread(&lna->datafile[i].filename,sizeof(lna->datafile[i].filename), 1, fp);
//	}
#endif
    return 1;
}

int putWriteRequest_LNL(){

    log_debug("进入putWriteRequest_LNL");
    //构造.LNL文件的写请求包
    char fileName[128] = {0};
    unsigned short try = 0;
    sprintf(fileName, "C:/%s.LNL", TARGET_HARDWARE_IDENTIFIER);
    log_debug("%s", fileName);
    makeTempFileLNL(fileName);//构造.LNL文件，包含本路径下的所有.bin文件
    tftpRWRQ writeRequest;
    //strcpy(writeRequest.fileName, fileName);
    sprintf(writeRequest.fileName, "%s.LNL", TARGET_HARDWARE_IDENTIFIER);
    writeRequest.blksize = 512;
    writeRequest.maxRetransmit = 1;
    writeRequest.timeout  = 2;
    strcpy(writeRequest.mode, "octet");
    //while( put(writeRequest, targetTftpServerAddr) < 0 && ++try < dlp_retry){}
    while( put(writeRequest, targetTftpServerAddr,0,NULL) < 0 && ++try < 10){}
    if(try >= dlp_retry){
        log_error("LNL WRITE REQUEST SEND ERROR");
        return -1;
    }else{
        log_debug("LNL WRITE REQUEST SEND SUCCESS");
        return 1;
    }

}

void putWriteRequest_LNS()
{
    // if(lna.numoffiles > 0 && pointerOfCurrentSendingDataFile < lna.numoffiles){
    //     struct stat file_stat;
    //     if(stat(lna.datafile[pointerOfCurrentSendingDataFile].filename, &file_stat) == 0){
    //         log_debug("file %s length available", lna.datafile[pointerOfCurrentSendingDataFile].filename);
    //     }else{
    //         log_error("file %s length unavailable", lna.datafile[pointerOfCurrentSendingDataFile].filename);
    //         return -1;
    //     }
    // unsigned int rate = 0;
    // if( total_dataFile_size != 0){
    //     rate = total_transfered_size / total_dataFile_size;
    //     for(int i = 0; i < 3; i++){
    //         lns.load_list_completion_ratio[2-i] = rate % 10 + '0';
    //         rate /= 10;
    //     }  
    // }
    

    // time_t currentTime = time(NULL);
    // lns.datafile = (LNS_DataFile*)malloc(sizeof(LNS_DataFile));

    if( stage == 1){
        lns.file_len = sizeof(lns);
        lns.pro_ver = 0xA3;
        lns.status_code = 0x0001;
        strcpy(lns.status_des, "The Target accepts the operation(not yet started)");
        lns.status_des_len = strlen(lns.status_des);
        lns.count += 1;
        strcpy(lns.load_list_completion_ratio, "000");
        //此后的字段stage 1状态都为0，已经通过前面的memset赋值
    }else if( stage == 2){

        lns.file_len = sizeof(lns);
        lns.pro_ver = 0xA3;
        lns.status_code = 0x0002;
        strcpy(lns.status_des, "The operation is in progress");
        lns.status_des_len = strlen(lns.status_des);
        lns.count += 1;
        lns.time_wait = 0x0000;
        lns.time_estimate = 0xFFFF;
        strcpy(lns.load_list_completion_ratio, "050");
        // time_t currentTime = time(NULL);
        // double transfer_time = difftime(currentTime, startTime);
        // log_debug("transfer time: %.2f second", transfer_time);
        // transfer_speed = total_transfered_size / transfer_time;
        // log_debug("transfer speed: %.2f", transfer_speed);
        // lns.time_estimate = (total_dataFile_size = total_transfered_size) / transfer_speed;
        // lns.time_estimate = 0xFFFF;
        lns.numOfDataFile = lna.numoffiles;
       

        for(int i = 0; i < lna.numoffiles; i++){
            strcpy(lns.datafile[i].filename, lna.datafile[i].filename);
            lns.datafile[i].filename_len = lna.datafile[i].filename_len;
            if(i > pointerOfCurrentSendingDataFile){
                lns.datafile[i].file_status = 0x0001;//该文件还没有开始下载
                strcpy(lns.datafile[i].file_status_des, "The download of this file has not started yet");
                lns.datafile[i].file_status_des_len = strlen(lns.datafile[i].file_status_des);
            }else if(i == pointerOfCurrentSendingDataFile){
                lns.datafile[i].file_status = 0x0002;
                strcpy(lns.datafile[i].file_status_des, "The operation is in progress");
                lns.datafile[i].file_status_des_len = strlen(lns.datafile[i].file_status_des);
            }else{
                lns.datafile[i].file_status = 0x0003;
                strcpy(lns.datafile[i].file_status_des,"The download of this file has been complete without error");
                lns.datafile[i].file_status_des_len = strlen(lns.datafile[i].file_status_des);
            }
        }
    }else if( stage == 3 ){
        lns.file_len = sizeof(lns);
        lns.pro_ver = 0xA3;
        lns.status_code = 0x0003;
        lns.status_des_len = strlen(lns.status_des);
        strcpy(lns.status_des, "OperatorDownload completed");
        lns.count += 1;
        lns.time_wait = 0x0000;
        lns.time_estimate = 0xFFFF;
        strcpy(lns.load_list_completion_ratio, "100");
        lns.numOfDataFile = lna.numoffiles;

    }else{
        log_error("NO SUCH LNS TRANSMIT STAGE");
        return;
    }

    char fileName[128];
    sprintf(fileName, "C:/%s.LNS", TARGET_HARDWARE_IDENTIFIER);
    makeTempFileLNS(fileName);
    tftpRWRQ writeRequest;
    unsigned short try = 0;
    //strcpy(writeRequest.fileName, fileName);
    sprintf(writeRequest.fileName, "%s.LNS", TARGET_HARDWARE_IDENTIFIER);
    writeRequest.blksize = 512;
    writeRequest.maxRetransmit = 1;
    writeRequest.timeout = 2;
    strcpy(writeRequest.mode, "octet");
    while (put(writeRequest, targetTftpServerAddr,0,NULL) < 0 && ++try<dlp_retry){};
    if(try >= dlp_retry){
        log_error("LNS WRITE REQUEST SEND ERROR");
        return;
    }else{
        log_debug("lns write request send success");
        return;
    }
}
