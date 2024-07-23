#include <signal.h>
#include <time.h>
#include "upload.h"
#include "log.h"
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef VXWORKS
#include <Vxworks.h>
#endif
#include "global.h"
#include "zlib.h"
#include "bspatch.h"
extern int tftpServerFd;
extern const unsigned blksize_default;
extern const unsigned timeout_default;
extern const unsigned max_retransmit_default;
extern const unsigned short dlp_retry;
extern int LUSflag;
//timer_t timer;
struct sockaddr_in targetTftpServerAddr;
File_LUS LUS;
int ii = 0;
uint64_t no;
//time_t timer_id;
//ADDRESS LUSstart = 0xa00000;
//ADDRESS middleFileStart = 0xa00100;
//ADDRESS DataFileStart = 0xa00300;
//extern ULONG flashRead(ADDRESS start, char * buf, ULONG nbytes);
//extern ULONG flashWrite(ADDRESS flashStart, char * buf, ULONG nbytes);
#ifdef FILE_WRITE_TO_FLASH
extern ULONG flashRead(ADDRESS start, char * buf, ULONG nbytes);
extern ULONG flashWrite(ADDRESS flashStart, char * buf, ULONG nbytes);
ADDRESS start = 0xa00000;
int writeaddr=0;
int readaddr=0;
#endif
uint64_t littleEndianToBigEndian64(uint64_t value) {
    // 閸掑棗鍩嗘径鍕倞妤傦拷2娴ｅ秴鎷版担锟�娴ｏ拷
	uint32_t low = value & 0xFFFFFFFF;
    uint32_t high = (value >> 32) & 0xFFFFFFFF;
    
    // 鏉烆剚宕叉稉鍝勩亣缁旑垰绨�
    uint32_t bigEndianLow = ((low >> 24) & 0xFF) |
                            ((low << 8) & 0xFF0000) |
                            ((low >> 8) & 0xFF00) |
                            ((low << 24) & 0xFF000000);
                            
    uint32_t bigEndianHigh = ((high >> 24) & 0xFF) |
                             ((high << 8) & 0xFF0000) |
                             ((high >> 8) & 0xFF00) |
                             ((high << 24) & 0xFF000000);
    
    // 鐏忓棝鐝�2娴ｅ秴鎷版担锟�娴ｅ秷绻涢幒銉ㄦ崳閺夛拷
    return ((uint64_t)bigEndianHigh << 32) | bigEndianLow;
}
// File_LUI createFileLCI(File_LUI temp)
// {
//     File_LUI goal;
//     memcpy(&goal,&temp,sizeof(temp));
//     goal.file_len=htonl(temp.file_len);
//     goal.status_flag=htons(temp.status_flag);
//     return goal;
// }
File_LUS createFileLUS(File_LUS temp)
{
    File_LUS goal;
	memcpy(&goal,&temp,sizeof(temp));
	goal.statusCode=htons(temp.statusCode);
	goal.counter=htons(temp.counter);
	goal.estimatedTime=htons(temp.estimatedTime);
	goal.exceptionTimer=htons(temp.exceptionTimer);
	goal.fileLength=htonl(temp.fileLength);
	goal.numOfHeaderFiles=htons(temp.numOfHeaderFiles);
	for(int i=0;i<temp.numOfHeaderFiles;i++)
	{
		goal.headerFiles[i].loadStatus=htons(temp.headerFiles[i].loadStatus);
        goal.headerFiles[i].headerFile.fileLen=littleEndianToBigEndian64(temp.headerFiles[i].headerFile.fileLen);
	}
	return goal;
}
void sendStatusFileLUS(int sigum){
    log_debug("start send status File LUS");
    if(LUS.numOfHeaderFiles > 0 && ii < LUS.numOfHeaderFiles){
        unsigned int rate = no * blksize_default * 100 / LUS.headerFiles[ii].headerFile.fileLen;
        log_debug("filename %s fileLen = %llu no = %llu rate = %d blksize_default = %d", LUS.headerFiles[ii].headerFile.name, LUS.headerFiles[ii].headerFile.fileLen, no, rate,blksize_default);
        for(int i = 0; i < 3; i++){
            LUS.headerFiles[ii].loadRatio[2 - i] = rate % 10 + '0';
            rate /= 10;
        }
    }
    log_debug("SEND STATUS FILE LUS, LUS.statusCode = %d", LUS.statusCode);
#ifdef OS_HAVE_FILE_SYSTEM
    char fileName[128] = {0};
    sprintf(fileName, "C:/%s.LUS", TARGET_HARDWARE_IDENTIFIER);
    makeTempFileLUS(fileName);
#elif OS_NO_HAVE_FILE_SYSTEM
    makeTempFileLUS(NULL);
#endif

    unsigned short try = 0;
    tftpRWRQ request;
    request.blksize = blksize_default;
    //strcpy(request.fileName, fileName);
    sprintf(request.fileName, "%s.LUS", TARGET_HARDWARE_IDENTIFIER);
    request.maxRetransmit = max_retransmit_default;
    request.timeout = timeout_default;
    strcpy(request.mode, "octet");
    while(put(request, targetTftpServerAddr,LUS.fileLength,LUSstart) < 0 && ++try < dlp_retry && LUS.statusCode != 0x0003){}
    if(try >= dlp_retry){
        log_error("SEND STATUS FILE LUS ERROR");
    }
    else{
        log_debug("SEND STATUS FILE LUS FINISHED");
    }
}

//static void stop_timer(int signum)
//{
//	log_debug("signum receive");
//	timer_delete(timer_id);
//}
void* statusFileLUSThread(void* arg){

	//这个线程需要别的停止信号，如果在交互过程中管理端出错
	//状态码03发不过来，该线程会一直执行
//	log_debug("statusFileLUSThread Running!");
//
//    struct sigevent evp;
//    struct itimerspec ts;
//    timer_t timer;
//    int ret;
//    evp.sigev_value.sival_ptr = &timer;
//    evp.sigev_notify = SIGEV_SIGNAL;
//    evp.sigev_signo = SIGUSR1;
//    signal(SIGUSR1, sendStatusFileLUS);
//    ret = timer_create(CLOCK_REALTIME, &evp, &timer);
//    if(ret) perror("timer_create");
//    ts.it_interval.tv_sec = 1;
//    ts.it_interval.tv_nsec = 0;
//    ts.it_value.tv_sec = 1;
//    ts.it_value.tv_nsec = 0;
//    ret = timer_settime(timer, 0, &ts, NULL);
//    if(ret) perror("timer_settime");

//    while(1){
//        if(LUS.statusCode == 0x0003 || LUSflag == 1){
//        	log_debug("timer_delete before");
//        	log_debug("timer = %d", timer);
//            timer_delete(timer);
//            log_debug("timer_delete SEND END STATUS, LUS.statusCode = %d", LUS.statusCode);
            //if(LUSflag == 1)
            //{
//            	log_debug("timer_delete before");
//            	timer_delete(timer);
//            	log_error("error output");
            //}
            //else
//            {
//            	sendStatusFileLUS();
//            }
//            break;
//        }else
//        {
        	//每Sleep一个时间发送一个LUS
        	//log_debug("statusFileLUSThread SEND Running STATUS");
        	//sendStatusFileLUS();
        	
//        }
        
//    }
	log_debug("statusFileLUSThread Running!");
//	struct sigaction sa;
//	struct itimerspec timer_spec;
//	
//	struct sigevent sev;
//	sa.sa_sigaction = sendStatusFileLUS;
//	sigemptyset(&sa.sa_mask);
//	sa.sa_flags = 0;
//	sigaction(SIGALRM,&sa,NULL);
//	alarm(4);
//	sa.sa_handler = stop_timer;
//	sigaction(SIGINT,&sa,NULL);
	
//	sev.sigev_notify = SIGEV_SIGNAL;
//	sev.sigev_signo = SIGALRM;
//	sev.sigev_value.sival_ptr = &timer_id;
//	timer_create(CLOCK_REALTIME,&sev,&timer_id);
//	
//	timer_spec.it_value.tv_sec = 0;
//	timer_spec.it_value.tv_nsec = 500000000;
//	timer_spec.it_interval.tv_sec = 0;
//	timer_spec.it_interval.tv_nsec = 500000000;
//	timer_settime(timer_id,0,&timer_spec,NULL);
	while(1)
	{
		sendStatusFileLUS(NULL);
		usleep(500000);
		if(LUS.statusCode == 0X0003 )
		{
			sendStatusFileLUS(NULL);
//			raise(SIGINT);
//			timer_delete(timer_id);
			break;
		}	
	}
    log_debug("statusFileLUSThread Quit!");
}

int upload_615a(tftpRWRQ readRequest, struct sockaddr_in loader_addr){
//	LUSstart = 0xa00000;
//	middleFileStart = 0xa00100;
//	DataFileStart = 0xa00300;
	ii = 0;
    memset(&LUS, 0, sizeof(LUS));
    targetTftpServerAddr.sin_addr = loader_addr.sin_addr;
    targetTftpServerAddr.sin_port = htons(TFTP_SERVER_PORT);
    targetTftpServerAddr.sin_family = AF_INET;
    char *currentFileName = "C:/MSL.bin";
    char *newFileName="C:/newout.bin";
    if(makeTempFileLUI(readRequest.fileName) < 0){
        log_error("MAKE LUI FILE ERROR");
        return -1;
    }
    unsigned short try = 0;
#ifdef OS_HAVE_FILE_SYSTEM
    while(handleGet(readRequest, loader_addr,NULL,NULL) < 0 && ++try < dlp_retry + 1){
        log_debug("handle Get LUI error");
    }
#elif defined(OS_NO_FILE_SYSTEM)
    while(handleGet(readRequest, loader_addr,13,middleFileStart-13) < 0 && ++try < dlp_retry + 1){
            log_debug("handle Get LUI error");
       }
#endif
    // if(handleGet(readRequest, loader_addr) < 0){
    //     log_error("handle LUI GET REQUEST ERROR");
    //     return -1;
    // }
    LUS.statusCode = STATUS_CODE_ACCEPT;
    memcpy(LUS.protocalVersion, "A4", 2);
    //LUS.protocalVersion = 0xA3;
    memcpy(LUS.loadListRatio, "000", 3);
    LUS.descriptionLen = 0;
    //閹恒儱褰堥幙宥勭稊娑斿鎮楅棁锟筋渽妞诡兛绗傞崣鎴︼拷閻樿埖锟介弬鍥︽LUS
    sendStatusFileLUS(NULL);
    //閸掓稑缂撻悩鑸碉拷閺傚洣娆㈢痪璺ㄢ柤閿涘苯鎳嗛張鐔革拷閸欐垿锟介悩鑸碉拷閺傚洣娆�
#ifdef __linux__
    pthread_t statusFileThreadTid;
    pthread_create(&statusFileThreadTid, NULL, statusFileLUSThread, NULL);
#elif VXWORKS
	taskSpawn("LUS_statusThread", TASK_PRI, VX_SPE_TASK, TASK_STACK_SIZE, 
			(FUNCPTR)statusFileThread, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
#endif
//    sleep(2);
	LUS.statusCode = STATUS_CODE_IN_PROGRESS;
	//发一个状态码为2的LUS
//	sendStatusFileLUS();
//    sendStatusFileLUS();
    char tftpRequestBuf[max_tftpRequest_len];
    //struct sockaddr_in targetTftpClientAddr;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    ssize_t len = recvfrom(tftpServerFd, tftpRequestBuf, max_tftpRequest_len, 0, (struct sockaddr*)&loader_addr, &addr_len);
    tftpRWRQ request =  parseTftpReadWriteRquest(tftpRequestBuf, len);
    if(handlePut(request, loader_addr,middleFileStart) < 0){
        log_error("handle LUR PUT REQUEST ERROR");
        return -1;
    }
    File_LUR LUR;
    if(parseTempFileLUR(request.fileName, &LUR) < 0){
        log_error("parse LUR FILE ERROR");
        return -1;
    }
    LUS.numOfHeaderFiles = LUR.numOfHeaderFiles;
    LUS.headerFiles = (HeaderFilePlus*)malloc(sizeof(HeaderFilePlus) * LUS.numOfHeaderFiles);
    for(int i = 0; i < LUS.numOfHeaderFiles; ++i){
        memcpy(&LUS.headerFiles[i].headerFile, &LUR.headerFiles[i], sizeof(HeaderFile));
        memcpy(LUS.headerFiles[i].loadRatio, "000", 3);
        LUS.headerFiles[i].loadStatus = 0x0001;
        LUS.headerFiles[i].loadStatusDescriptionLength = 0;
    }
    log_debug("parse LUR FILE SUCCESS, file number = %d", LUR.numOfHeaderFiles);

    //发一个状态码为2的LUS
    sendStatusFileLUS(NULL);

    for(int i = 0; i < LUR.numOfHeaderFiles; ++i){

        no = 0;
        LUS.headerFiles[i].loadStatus = STATUS_CODE_IN_PROGRESS;
        tftpRWRQ request;
        request.blksize = blksize_default;
        request.maxRetransmit = max_retransmit_default;
        request.timeout = timeout_default;
        strcpy(request.mode, "octet");
        strcpy(request.fileName, LUR.headerFiles[i].name);

        //Todo
//        log_debug("1111");
        if(get(request, targetTftpServerAddr, &no,DataFileStart) < 0){
            log_error("donwload file c:/%s error", request.fileName);
            LUS.headerFiles[i].loadStatus = 0x1007;
            memcpy(LUS.headerFiles[i].loadRatio, "000", 3);
            strcpy(LUS.headerFiles[i].loadStatusDescription, "download failed");

        }
        else{
            LUS.headerFiles[i].loadStatus = STATUS_CODE_COMPLETED;
            memcpy(LUS.headerFiles[i].loadRatio, "100", 3);
            //strcpy(LUS.headerFiles[i].loadStatusDescription, "The operation is completed without error");
        }
        LUS.headerFiles[i].loadStatusDescriptionLength = strlen(LUS.headerFiles[i].loadStatusDescription);
        //targetTftpServerAddr.sin_port = htons(TFTP_SERVER_PORT);
        log_debug("download file %1 SUCCESS.", ii);
//        char *newFileName = getnewFileName(LUR.headerFiles[i].load_part_name);
//        int ret = decompAndBspatch(request.fileName,newFileName,currentFileName);
//        if(ret != 0)
//        {
//        	log_error("%s file parse error");
//        }
        ++ii;
        //sendStatusFileLUS();
    }
    
    
    //log_debug("file number = %d", LUR.numOfHeaderFiles);

    //先进行解压缩，将文件统一命名为patchfile.bin
    //然后进入flash,查询当前文件的名字，并进行逆差分
    
    //将文件写入flash
//    for(int i = 0; i < LUR.numOfHeaderFiles; ++i)
//    {
//    	char filename[64] = {0};
//    	size_t addr = 0;			//固件写入的起始地址
//    	size_t infoAddr = 0;		//记录固件信息的起始地址
//    	size_t maxAddr = 0;			//支持固件保存的最大地址
//    	size_t flashWriteRet = 0;	//文件写入的返回值
//    	sprintf(filename, "C:/%s", LUR.headerFiles[i].name);
//
//    	log_debug("flash begin write file %s.", filename);
//
//    	//对文件进行分类，识别文件类型
//    	//如果是差分压缩文件,先进行逆差分和解压缩
//
//    	//判断文件是哪种文件：操作系统、驱动程序、应用，不同的文件需要写入到flash的不同地方
//    	if(NULL != strstr(LUR.headerFiles[i].load_part_name, "OS"))
//		{
//    		//操作系统
//    		addr = 0xB00000;
//    		infoAddr = 0xa00100;
//    		maxAddr = 0xC00000;
//    		
//		}else if(NULL != strstr(LUR.headerFiles[i].load_part_name, "MSL"))
//		{
//			//驱动程序
//			log_debug("MSI FILE WRITE BEGIN");
//			addr = 0xa01000;
//			infoAddr = 0xa00000;
//			maxAddr = 0xB00000;
//		}else if(NULL != strstr(LUR.headerFiles[i].load_part_name, "APP"))
//		{
//			//应用
//			addr = 0xC00000;
//			infoAddr = 0xa00200;
//			maxAddr = 0xD00000;
//		}else if(NULL != strstr(LUR.headerFiles[i].load_part_name, "driver")){
//			addr = 0xC00000;
//			infoAddr = 0xa00000;
//			maxAddr = 0xd00000;
//		}
//    	else{
//    				addr = 0xd00000;
//    				infoAddr = 0xa00300;
//    				maxAddr = 0xe00000;
//    				log_debug("flash begin write file type %s.", LUR.headerFiles[i].load_part_name);
//    			}
//    	flashWriteRet = flashWriteFile(filename, addr, infoAddr, maxAddr);
//    	log_debug("flash write file %d, return = %d", filename, flashWriteRet);
//    }
//    测试写入是否成功
//    在板子重启后，输入dcache; go 0xa00180\r
//    LUS.statusCode = STATUS_CODE_COMPLETED;
//    strcpy(LUS.description, "Uploading Completed.");
//    LUS.descriptionLen = strlen(LUS.description);
//    memcpy(LUS.loadListRatio, "100", 3);
//    LUSflag = 1;
//    sleep(2);
//    timer_delete(timer);
    LUS.statusCode = STATUS_CODE_COMPLETED;
	strcpy(LUS.description, "Uploading Completed.");
	LUS.descriptionLen = strlen(LUS.description);
	memcpy(LUS.loadListRatio, "100", 3);
//	raise(SIGINT);
//    sendStatusFileLUS();
	sleep(2);
    return 1;
}


int makeTempFileLUI(char* fileName){
    struct File_LUI lui;
    memcpy(lui.Pro_ver, PROTOCAL_VERSION, 2);
    lui.status_flag = 0x0001;
    char *str1 = "abc";
    strcpy(lui.stat_des,str1);
    lui.stat_des_len = strlen(lui.stat_des);
    lui.file_len = 10 + strlen(lui.stat_des);

#ifdef OS_HAVE_FILE_SYSTEM
    char filePathName[128] = {0};

    //天脉写文件加的盘符
    sprintf(filePathName, "C:/%s", fileName);

    FILE *fp;
    if((fp = fopen(filePathName,"wb")) == NULL){
        log_error("cannot open file_LUI"); 
        return -1;
    }
    log_debug("Write *UI completed");
    fwrite(&lui , lui.file_len, 1, fp);
    fclose(fp);
    fp = NULL;
//    free(buf);
#elif defined(OS_NO_FILE_SYSTEM)
//    ULONG flashRet = 0;
//
//    log_debug("*UI FILE_WRITE_TO_FLASH Begining!");
//	flashRet = flashWrite(FLASH_WRITE_LUI_ADDRESS, &lui, lui.file_len);
//	log_error("*UI FILE_WRITE_TO_FLASH Error, len = %ld, lui.file_len = %ld", flashRet, lui.file_len);
    log_debug("middleFileStart addr %p",middleFileStart);
    char *buf=(char*)malloc(lui.file_len);
    memcpy(buf,&lui,lui.file_len);
    log_debug("file_len = %d",lui.file_len);
    ULONG ret = flashWrite(middleFileStart, &lui, lui.file_len);
    middleFileStart+=lui.file_len;
    log_debug("flash write %ld, middleFileStartaddr=%p\n", ret, middleFileStart);
//    char *mem=(char*)malloc(lui.file_len);
//    memset(mem, 0, lui.file_len);
//    memcpy(mem, (void*) start, lui.file_len);
//    printf("flash read %s\n", mem);
    log_debug("flash read");
//    for(int i=0; i<lui.file_len;i++)
//    {
//    	log_debug("buf [%d] = %c",i,buf[i]);
//    	log_debug("mem [%d] = %c",i,mem[i]);
//    }
    free(buf);
//    free(mem);
	//if(flashRet == lui.file_len)
	//{
	//	log_debug("*UI FILE_WRITE_TO_FLASH finished, len = %ld, lui.file_len = %ld", flashRet, lui.file_len);
	//}else
	//{
	//	log_error("*UI FILE_WRITE_TO_FLASH Error, len = %ld, lui.file_len = %ld", flashRet, lui.file_len);
	//}
#endif
    return 1;
}


int parseTempFileLUR(char* fileName, File_LUR* LUR){
    if(LUR == NULL){
        log_error("LUR POINTER IS NULL");
        return -1;
    }
#ifdef OS_HAVE_FILE_SYSTEM
    FILE *fp;
    char filePathName[128] = {0};

    sprintf(filePathName, "C:/%s", fileName);

    //if((fp = fopen(fileName, "rb")) == NULL){
    if((fp = fopen(filePathName, "rb")) == NULL){
        log_error("cannot open file LUR");
        return -1;
    }
    fread(&LUR->fileLength, sizeof(LUR->fileLength), 1, fp);
    fread(&LUR->protocalVersion, sizeof(LUR->protocalVersion), 1, fp);
    fread(&LUR->numOfHeaderFiles, sizeof(LUR->numOfHeaderFiles), 1, fp);
//    log_debug("filename = %s, fileLength = %u, protocalVersion = %x, numberOfHeaderFiles = %hu",
//    		fileName, LUR->fileLength, LUR->protocalVersion, LUR->numOfHeaderFiles);
    log_debug("filename = %s, fileLength = %u, protocalVersion = %04x, numberOfHeaderFiles = %hu",
    		filePathName, LUR->fileLength, LUR->protocalVersion, LUR->numOfHeaderFiles);
    LUR->headerFiles = (HeaderFile*)malloc(LUR->numOfHeaderFiles * sizeof(HeaderFile));
    for(int i = 0; i < LUR->numOfHeaderFiles; ++i){
        fread(&LUR->headerFiles[i].fileLen, sizeof(LUR->headerFiles[i].fileLen), 1, fp);
        fread(&LUR->headerFiles[i].name_len, sizeof(LUR->headerFiles[i].name_len), 1, fp);
        fread(LUR->headerFiles[i].name, LUR->headerFiles[i].name_len + 1, 1, fp);
        fread(&LUR->headerFiles[i].load_part_len_name, sizeof(LUR->headerFiles[i].load_part_len_name), 1, fp);
        fread(LUR->headerFiles[i].load_part_name, LUR->headerFiles[i].load_part_len_name + 1, 1, fp);
        log_debug("headFile name = %s", LUR->headerFiles[i].name);
        log_debug("load part name = %s", LUR->headerFiles[i].load_part_name);
    }
    fclose(fp);
#elif 0
    unsigned long address = middleFileStart;
    log_debug("*UR FILE_READ_TO_FLASH Begining!");
    memcpy(&LUR->fileLength, (void*)address, sizeof(LUR->fileLength));
    address += sizeof(LUR->fileLength);
    memcpy(&LUR->protocalVersion, (void*)address, sizeof(LUR->protocalVersion));
    address += sizeof(LUR->protocalVersion);
    memcpy(&LUR->numOfHeaderFiles, (void*)address, sizeof(LUR->numOfHeaderFiles));
    address += sizeof(LUR->numOfHeaderFiles);
    log_debug("flash read filename = %s, fileLength = %u, protocalVersion = %x, numberOfHeaderFiles = %hu",
        		filePathName, LUR->fileLength, LUR->protocalVersion, LUR->numOfHeaderFiles);
    LUR->headerFiles = (HeaderFile*)malloc(LUR->numOfHeaderFiles * sizeof(HeaderFile));
    for(int i = 0; i < LUR->numOfHeaderFiles; ++i){
    	memcpy(&LUR->headerFiles[i].fileLen, (void*)address, sizeof(LUR->headerFiles[i].fileLen));
    	address += sizeof(LUR->headerFiles[i].fileLen);
    	memcpy(&LUR->headerFiles[i].name_len, (void*)address, sizeof(LUR->headerFiles[i].name_len));
    	address += sizeof(LUR->headerFiles[i].name_len);
    	memcpy(LUR->headerFiles[i].name, (void*)address, LUR->headerFiles[i].name_len + 1);
    	address += LUR->headerFiles[i].name_len + 1;
    	memcpy(&LUR->headerFiles[i].load_part_len_name, (void*)address, sizeof(LUR->headerFiles[i].load_part_len_name));
    	address += sizeof(LUR->headerFiles[i].load_part_len_name);
    	memcpy(LUR->headerFiles[i].load_part_name, (void*)address, LUR->headerFiles[i].load_part_len_name + 1);
    	address += LUR->headerFiles[i].load_part_len_name + 1;
    	log_debug("headFile name = %s", LUR->headerFiles[i].name);
    	log_debug("load part name = %s", LUR->headerFiles[i].load_part_name);
    }
    log_debug("*UR FILE_READ_TO_FLASH finished!");
#endif
    return 0;
}
int makeTempFileLUS(char* fileName){
#ifdef OS_HAVE_FILE_SYSTEM
    FILE* fp;
    if((fp = fopen(fileName, "wb")) == NULL){
        log_error("can not open file %s", fileName);
        return -1;
    }
    fseek(fp, 4, SEEK_SET);
    if(fwrite((char*)&LUS + 4, 1, 5, fp) < 5){
        log_error("fwrite file %s error", fileName);
        fclose(fp);
        return -1;
    }
    if(fwrite(LUS.description, 1, LUS.descriptionLen, fp) < LUS.descriptionLen){
        log_error("fwrite file %s error", fileName);
        fclose(fp);
        return -1;
    }
    if(fwrite(&LUS.counter, 1, 9, fp) < 9){
        log_error("fwrite file %s error", fileName);
        fclose(fp);
        return -1;
    }
    log_debug("numOfHeaderFiles = %d", LUS.numOfHeaderFiles);
    fwrite(&LUS.numOfHeaderFiles, 1, 2, fp);
    for(int i = 0; i < LUS.numOfHeaderFiles; ++i){
        fwrite(&LUS.headerFiles[i].headerFile.fileLen, 1, 8, fp);
        fwrite(&LUS.headerFiles[i].headerFile.name_len, 1, 1, fp);
        fwrite(LUS.headerFiles[i].headerFile.name, 1, LUS.headerFiles[i].headerFile.name_len + 1, fp);
        fwrite(&LUS.headerFiles[i].headerFile.load_part_len_name, 1, 1, fp);
        fwrite(LUS.headerFiles[i].headerFile.load_part_name, 1, LUS.headerFiles[i].headerFile.load_part_len_name + 1, fp);
        fwrite(LUS.headerFiles[i].loadRatio, 1, 3, fp);
        fwrite(&LUS.headerFiles[i].loadStatus, 1, 2, fp);
        fwrite(&LUS.headerFiles[i].loadStatusDescriptionLength, 1, 1, fp);
        fwrite(LUS.headerFiles[i].loadStatusDescription, 1, LUS.headerFiles[i].loadStatusDescriptionLength + 1, fp);
    }
    LUS.fileLength = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    fwrite(&LUS.fileLength, 1, sizeof(LUS.fileLength), fp);
    fclose(fp);
#elif defined(OS_NO_FILE_SYSTEM)
    ULONG fwsize = 0;
    unsigned long flashAddr = LUSstart;
    //跳过4字节
    flashAddr += 4;
    fwsize = flashWrite(flashAddr, (char*)&LUS + 4, 5);
    //if(fwsize < 5)
    //{
	//	log_error("flash write file %s error", fileName);
		//fclose(fp);
	//	return -1;
	//}
    flashAddr += 5;
    fwsize = flashWrite(flashAddr, LUS.description, LUS.descriptionLen);
	//if(fwsize < LUS.descriptionLen){
	//	log_error("flash write file %s error", fileName);
		//fclose(fp);
	//	return -1;
	//}
	flashAddr += LUS.descriptionLen;
	fwsize = flashWrite(flashAddr, &LUS.counter, 9);
	//if(fwsize < 9){
	//	log_error("flash write file %s error", fileName);
		//fclose(fp);
	//	return -1;
	//}
	log_debug("numOfHeaderFiles = %d", LUS.numOfHeaderFiles);
	//fwrite(&LUS.numOfHeaderFiles, 1, 2, fp);
	flashAddr += 9;
	fwsize = flashWrite(flashAddr, &LUS.numOfHeaderFiles, 2);
	flashAddr += 2;
	for(int i = 0; i < LUS.numOfHeaderFiles; ++i){
		flashWrite(flashAddr, &LUS.headerFiles[i].headerFile.fileLen, 8);
		flashAddr += 8;
		//fwrite(&LUS.headerFiles[i].headerFile.fileLen, 1, 8, fp);
		flashWrite(flashAddr, &LUS.headerFiles[i].headerFile.name_len, 1);
		flashAddr += 1;
		//fwrite(&LUS.headerFiles[i].headerFile.name_len, 1, 1, fp);
		flashWrite(flashAddr, LUS.headerFiles[i].headerFile.name, LUS.headerFiles[i].headerFile.name_len + 1);
		flashAddr += (LUS.headerFiles[i].headerFile.name_len + 1);
		//fwrite(LUS.headerFiles[i].headerFile.name, 1, LUS.headerFiles[i].headerFile.name_len + 1, fp);
		flashWrite(flashAddr, &LUS.headerFiles[i].headerFile.load_part_len_name, 1);
		flashAddr += 1;
		//fwrite(&LUS.headerFiles[i].headerFile.load_part_len_name, 1, 1, fp);
		flashWrite(flashAddr, LUS.headerFiles[i].headerFile.load_part_name, LUS.headerFiles[i].headerFile.load_part_len_name + 1);
		flashAddr += (LUS.headerFiles[i].headerFile.load_part_len_name + 1);
		//fwrite(LUS.headerFiles[i].headerFile.load_part_name, 1, LUS.headerFiles[i].headerFile.load_part_len_name + 1, fp);
		flashWrite(flashAddr, LUS.headerFiles[i].loadRatio, 3);
		flashAddr += 3;
		//fwrite(LUS.headerFiles[i].loadRatio, 1, 3, fp);
		flashWrite(flashAddr, &LUS.headerFiles[i].loadStatus, 2);
		flashAddr += 2;
		//fwrite(&LUS.headerFiles[i].loadStatus, 1, 2, fp);
		flashWrite(flashAddr, &LUS.headerFiles[i].loadStatusDescriptionLength, 1);
		flashAddr += 1;
		//fwrite(&LUS.headerFiles[i].loadStatusDescriptionLength, 1, 1, fp);
		flashWrite(flashAddr, LUS.headerFiles[i].loadStatusDescription, LUS.headerFiles[i].loadStatusDescriptionLength + 1);
		flashAddr += (LUS.headerFiles[i].loadStatusDescriptionLength + 1);
		//fwrite(LUS.headerFiles[i].loadStatusDescription, 1, LUS.headerFiles[i].loadStatusDescriptionLength + 1, fp);
	}
	LUS.fileLength = flashAddr -LUSstart ;
	//LUS.fileLength = ftell(fp);
	flashAddr = LUSstart;
	//fseek(fp, 0, SEEK_SET);
	flashWrite(flashAddr, &LUS.fileLength, sizeof(LUS.fileLength));
	//fwrite(&LUS.fileLength, 1, sizeof(LUS.fileLength), fp);
#endif
    return 1;
}

//int decompAndBspatch(char *fileName,char *outfileName,char *currentFileName)
//{
//	gzFile fp;
//	FILE *op;
//	int ret = 0;
//	
//	char *tempFileName = "C:/tempFile.bin";
//	op=fopen(tempFileName,"wb");
//	if(op == NULL)
//	{
//		log_error("file open error");
//		return -1;
//	}
//	fp = gzopen(fileName,"rb");
//	if(fp == NULL)
//	{
//		fclose(op);
//		log_error("gzfile open error");
//		return -1;
//	}
//	int bufsize=4096;
//	gzseek(fp,0,SEEK_END);
//	size_t length = gztell(fp);
//	gzseek(fp,0,SEEK_SET);
//	log_debug("file length = %ld ",length);
//	char *buf=(char*)malloc(length);
//	int buf_read=0;
//	int total_size=0;
//	buf_read = gzread(fp,buf,bufsize);
//	log_debug(" buf _read = %d",buf_read);
//	while(bufsize == buf_read)
//	{
//		fwrite(buf,1,buf_read,op);
//		total_size += buf_read;
//		buf_read = gzread(fp,buf,bufsize);
//	}
//	if(buf_read <0)
//	{
//	
//		log_error("buf_read = %d",buf_read);
//		free(buf);
//		fclose(op);
//		gzclose(fp);
//	}
//	if(buf_read != 0)
//	{
//		fwrite(buf,1,buf_read,op);
//	}
//	free(buf);
//	gzclose(fp);
//	fclose(op);
//	ret = bspatchImp(currentFileName, tempFileName,  outfileName);
//	if(ret != 0)
//	{
//		log_error("bspatch error");
//		return -1;
//	}
//	return 0;
//}
//char *getCurrentFilename(char *type,ADDRESS infoAddr){
//	char buf[128]={0};
//	char *fileName;
//	char *position;
//	int fileNameLen;
//	if(NULL != strstr(type, "OS"))
//		{
//			//操作系统
//		
//		memcpy(buf,(void*)infoAddr,128);
//		position=strrchr(buf,'$');
//		fileNameLen=(int)buf[0];
//		fileName=(char*)malloc(fileNameLen);
//		if(position != NULL)
//		{
//			position=position +1;
//			strncpy(fileName,position,fileNameLen);
//		}
//		log_debug("current file name = %s",fileName);
//		return fileName;
//		
//	}else if(NULL != strstr(type, "MSL"))
//	{
//		//驱动程序
//		memcpy(buf,(void*)infoAddr,128);
//		position=strrchr(buf,'$');
//		fileNameLen=(int)buf[0];
//		fileName=(char*)malloc(fileNameLen);
//		if(position != NULL)
//		{
//			position=position +1;
//			strncpy(fileName,position,fileNameLen);
//		}
//		log_debug("current file name = %s",fileName);
//		return fileName;
//		
//	}else if(NULL != strstr(type, "APP"))
//	{
//		//应用
//		memcpy(buf,(void*)infoAddr,128);
//		position=strrchr(buf,'$');
//		fileNameLen=(int)buf[0];
//		fileName=(char*)malloc(fileNameLen);
//		if(position != NULL)
//		{
//			position=position +1;
//			strncpy(fileName,position,fileNameLen);
//		}
//		log_debug("current file name = %s",fileName);
//		return fileName;
//		
//	}else if(NULL != strstr(type, "driver")){
//		memcpy(buf,(void*)infoAddr,128);
//		position=strrchr(buf,'$');
//		fileNameLen=(int)buf[0];
//		fileName=(char*)malloc(fileNameLen);
//		if(position != NULL)
//		{
//			position=position +1;
//			strncpy(fileName,position,fileNameLen);
//		}
//		log_debug("current file name = %s",fileName);
//		return fileName;
//		
//	}
//}
