/*
 * 版权 2019-2020 中航工业西安航空计算技术研究所
 *
 * 对本文件的拷贝、发布、修改或任何其它用途必须得到
 * 中航工业西安航空计算技术研究所的书面协议许可.
 *
 * Copyrights（C）2019-2020 ACTRI
 * All Rights Reserved.
 */

/*
 * 变更历史
 * 2019-09-03  slei  创建该文件。
 */

/* 包含文件 */
#include <sysTypes.h>
#include <tasks.h>
#include <attribute.h>
#include <stdio.h>
#include <stdlib.h>
#include <printk.h>
#include <mosErrno.h>
#include <config_appcfg.h>

#if defined(ACOREMCOS_MCORE)
#include <acSmp.h>
#endif /* ACOREMCOS_MCORE */

//#ifdef CONFIG_APP_AUTO_TEST
//#include <acAutoTest.h>
//#endif /* CONFIG_APP_AUTO_TEST */

/* 全局变量 */
ACoreOs_id tstTaskid1, tstTaskid2;

/* 示例 */
/**
 * @brief
 *    示例任务1的入口。
 * @return
 *      无。
 * @implements
 */
void taskEntry1(void) {
#if defined(ACOREMCOS_MCORE)
	UINT32 cpuId;
#endif /* ACOREMCOS_MCORE */

	while (1) {
#if defined(ACOREMCOS_MCORE)
		cpuId = ACoreOs_smp_current_cpuid_get();
		printf("Task 1 running in core %d!\n", cpuId);
#else
		printf("Task 1 running !\n");
#endif /* ACOREMCOS_MCORE */
		ACoreOs_task_wake_after(1000);
	}
}

/**
 * @brief
 *    示例任务2的入口。
 * @return
 *      无。
 * @implements
 */
void taskEntry2(void) {
#if defined(ACOREMCOS_MCORE)
	UINT32 cpuId;
#endif /* ACOREMCOS_MCORE */

	while (1) {
#if defined(ACOREMCOS_MCORE)
		cpuId = ACoreOs_smp_current_cpuid_get();
		printf("Task 2 running in core %d!\n", cpuId);
#else
		printf("Task 2 running !\n");
#endif /* ACOREMCOS_MCORE */
		ACoreOs_task_wake_after(1000);
	}
}

/**
 * @brief
 *    应用入口，用户可在该接口中增加相应的程序。
 * @return
 *      无。
 * @implements
 */
#include "bsdiff.h"
#include "bspatch.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <socket.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#ifdef VXWORKS
#include <Vxworks.h>
#endif
#include "find.h"
#include "information.h"
#include "upload.h"
#include "operatorDownload.h"
#include "tftp.h"
#include "log.h"
#include "global.h"
#include <zlib.h>
int tftpServerFd;
int protocalFileFd;
int statusFileFd;
typedef void* (*start_routine)(void*);
extern const unsigned blksize_default = 512;
extern const unsigned timeout_default = 10;
extern const unsigned max_retransmit_default = 5;
extern const unsigned short dlp_retry = 2;
extern const char tftpTypeString[][20] = { "", "ReadRequest", "WriteRequest", "DATA", "ACK", "ERROR", "OACK" };
extern const unsigned short hardwaresCnt = 2;
Hardware hardwares[2];
unsigned short hardwaresPartsCount[2] = { 2, 1 };
int LUSflag = 0;
char hardwareName[2][MAX_HARDWARE_NAME_LEN + 1] = { "Hw1", "Hw2" };
char hardwareSerialNum[2][MAX_HARDWARE_SERIAL_NO_LEN + 1] = { "SELIFCBSJHB", "SELIFCBSJHB" };
char hardwarePartsPartNo[2][MAX_PART_COUNT][MAX_PART_NO_LEN + 1] = { { "part1", "part2" }, { "part3" } };
char hardwarePartsAmendment[2][MAX_PART_COUNT][MAX_PART_AMEMDMENT_LEN + 1] = { { "xiuz1", "xiuz2" }, { "xiuz3" } };

char hardwarePartsDesignation[2][MAX_PART_COUNT][MAX_PART_DESIGNATION_LEN + 1] = {
		{ "this is a part", "this is a part" }, { "this is a part" } };
pthread_mutex_t gm;
void log_lock(bool lock, void *udata) {
	pthread_mutex_t* lm = (pthread_mutex_t*) udata;
	if (lock == true) {
		pthread_mutex_lock(lm);
	} else {
		pthread_mutex_unlock(lm);
	}
	return;
}

int initHardWares() {
	for (int i = 0; i < hardwaresCnt; i++) {
		strcpy(hardwares[i].name, hardwareName[i]);
		hardwares[i].name_len = strlen(hardwares[i].name);
		strcpy(hardwares[i].serial_num, hardwareSerialNum[i]);
		hardwares[i].se_num_len = strlen(hardwareSerialNum[i]);

		hardwares[i].parts_num = hardwaresPartsCount[i];
		hardwares[i].parts = (Part*) malloc(sizeof(Part) * hardwaresPartsCount[i]);
		for (int j = 0; j < hardwaresPartsCount[i]; j++) {
			strcpy(hardwares[i].parts[j].partsNo, hardwarePartsPartNo[i][j]);
			hardwares[i].parts[j].parts_num_len = strlen(hardwares[i].parts[j].partsNo);
			strcpy(hardwares[i].parts[j].Amendment, hardwarePartsAmendment[i][j]);
			hardwares[i].parts[j].Amendment_len = strlen(hardwares[i].parts[j].Amendment);
			strcpy(hardwares[i].parts[j].Part_Designation_Text, hardwarePartsDesignation[i][j]);
			hardwares[i].parts[j].Part_Designation_Length = strlen(hardwares[i].parts[j].Part_Designation_Text);
		}
	}

	return 0;
}

int deInitHardwares() {
	for (int i = 0; i < hardwaresCnt; i++) {
		free(hardwares[i].parts);
	}

	return 0;
}

/*
 * type = 1测试压缩
 * type = 其他，测试解压缩
 *
 */
//int testZip(int type)
//{
//	int retvaldef = 0;
//
//	char oldFileName[] = "C:/fa.bin";
//	char zipNewFileName[] = "C:/zipFa.bin";
//	char newFileName[] = "C:/fa_zip_new.bin";
//
//	if(1 == type)
//	{
//		testZipFile(oldFileName, zipNewFileName, newFileName);
//	}else
//	{
//
//	}
//
//	return retvaldef;
//
//}

/*
 * type = 1测试差分
 * type = 其他，测试逆查分
 *
 */
int testBSDiff(int type) {
	int retvaldef = 0;

	char oldFileName[] = "C:/fa.bin";
	char diffFileName[] = "C:/newfilename.bin";

	if (1 == type) {
		char newFileName[] = "C:/fan.bin";
		retvaldef = bsdiffImp(oldFileName, diffFileName, newFileName);
	} else {
		char patchFileName[] = "C:/patchfile.bin";
		retvaldef = bspatchImp(oldFileName, diffFileName, patchFileName);
	}

	return retvaldef;
}

extern ULONG flashRead(ADDRESS start, char * buf, ULONG nbytes);
extern ULONG flashWrite(ADDRESS flashStart, char * buf, ULONG nbytes);//d2000 flash大小0x1000000


void testFlashWrite()
{
	char buf[16]="hello flash";
	ULONG ret = flashWrite(0xa00000, buf, strlen(buf)+1);
	printf("1111  %ld\n", ret);
	char rdbuf[16]={0};
//	ret = flashRead(0xa00000,rdbuf,strlen(buf)+1);
	void *addr=(void*)0xa00000;
	memcpy(rdbuf,addr,strlen(buf)+1);
	printf("2222  %ld, %s \n", ret, rdbuf);
}

int fileUpdateMain() {
	pthread_mutex_init(&gm, NULL);
//	log_debug("11");
	log_set_lock(log_lock, (void*) &gm);
	initHardWares();
	FILE* logFile = fopen("C:/logfile.txt", "w+");
	if (logFile == NULL) {

		log_error("FIND SOCKET INIT ERROR");

	}
//	log_error("it's error");
	log_add_fp(logFile, LOG_TRACE);

//
#ifdef __linux__
    pthread_t tid;
	pthread_create(&tid, NULL, (start_routine)find, NULL);
#elif VXWORKS
	taskSpawn("findBegin", TASK_PRI, VX_SPE_TASK, TASK_STACK_SIZE,
			(FUNCPTR)find, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
#endif
	tftpServerFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	protocalFileFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	statusFileFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (tftpServerFd < 0) {

		log_error("FIND SOCKET INIT ERROR");
		return 1;
	}
	struct sockaddr_in localTftpServerAddr;
	localTftpServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	localTftpServerAddr.sin_family = AF_INET;
	localTftpServerAddr.sin_port = htons(TFTP_SERVER_PORT);

	int opt = 1;
	setsockopt(tftpServerFd, SOL_SOCKET, SO_REUSEADDR, (const void*) &opt, sizeof(opt));
	if (bind(tftpServerFd, (struct sockaddr*) &localTftpServerAddr, sizeof(localTftpServerAddr)) < 0) {
		log_error("LOCAL TFTP SERVER SOCKET BIND ERROR. IP:%s PORT:%hu", inet_ntoa(localTftpServerAddr.sin_addr),
				ntohs(localTftpServerAddr.sin_port));
		close(tftpServerFd);
		return 2;
	}
//	log_debug("tftp fd = %d",tftpServerFd);
	localTftpServerAddr.sin_port = htons(PROTOCAL_FILE_PORT);
	if (bind(protocalFileFd, (struct sockaddr*) &localTftpServerAddr, sizeof(localTftpServerAddr)) < 0) {
		log_error("PROTOCAL FILE SOCKET BIND ERROR. errno = %d", errno);
		close(protocalFileFd);
		return 2;
	}
	localTftpServerAddr.sin_port = htons(STATUS_FILE_PORT);
	if (bind(statusFileFd, (struct sockaddr*) &localTftpServerAddr, sizeof(localTftpServerAddr)) < 0) {
		log_error("STATUS FILE SOCKET BIND ERROR.");
		close(statusFileFd);
		return 2;
	}

	char buf[max_tftpRequest_len];
	struct sockaddr_in targetTftpClientAddr;
	socklen_t addr_len = sizeof(struct sockaddr_in);
	log_debug("LOCAL TFTP SERVER INIT SUCCESS");
	while (1) {
		ssize_t len = recvfrom(tftpServerFd, buf, max_tftpRequest_len, 0, (struct sockaddr*) &targetTftpClientAddr,
				&addr_len);
//		log_debug("tftp fd = %d",tftpServerFd);
//		return -1;
		if (len < 0) {
			log_debug("RECVFROM ERROR");
			continue;
		}
		log_debug("recvfrom len = %d", len);
		switch (parseTftpPacketType(buf, len)) {
		case RRQ: {
			LUSflag =0;
			log_error("error");
			tftpRWRQ rrq = parseTftpReadWriteRquest(buf, len);

			log_debug("RECEIVE RRQ OF FILE:%s FROM IP:%s PORT:%hu", rrq.fileName,
					inet_ntoa(targetTftpClientAddr.sin_addr), ntohs(targetTftpClientAddr.sin_port));
			if (strstr(rrq.fileName, ".LCI")) {
				if (information(rrq, targetTftpClientAddr) > 0) {
					log_debug("INFORMATION SUCCESS");
				} else {
					log_error("INFORMATION UNSUCCESS");
				}
			} else if (strstr(rrq.fileName, ".LUI")) {
				if (upload_615a(rrq, targetTftpClientAddr) > 0) {
					log_debug("UPLOAD SUCCESS");
				} else {
					LUSflag = 1;
					log_error("UPLOAD UNSUCCESS");
				}
			} else if (strstr(rrq.fileName, ".LND")) {
				//                if(mediaDownloadProcess(rrq, targetTftpClientAddr) > 0){
				//                    log_debug("ARINC615A MEDIADOWNLOAD SUCCESS");
				//                }
				//                else{
				//                    log_error("ARINC615A MEDIADOWNLOAD UNSUCCESS");
				//                }
			} else if (strstr(rrq.fileName, ".LNO")) {
				if (operatorDownload_615a(rrq, targetTftpClientAddr) > 0) {
					log_debug("DOWNLOAD SUCCESS");
				} else {
					log_error("DOWNLOAD UNSUCCESS");
				}
			}
			break;
		}
		default:
			log_debug("EXPECTED TO RECEIVE ReadRequest BUT RECEIVE %s", tftpTypeString[buf[1]]);
			break;
		}
//		struct timeval timeout_;
//		timeout_.tv_sec = 0;
//		timeout_.tv_usec = 0;
//		if (setsockopt(tftpServerFd, SOL_SOCKET, SO_RCVTIMEO, &timeout_, sizeof(timeout_)) < 0) {
//			log_error("set timeout arg error");
//		}
//		if (setsockopt(protocalFileFd, SOL_SOCKET, SO_RCVTIMEO, &timeout_, sizeof(timeout_)) < 0) {
//			log_error("set timeout arg error");
//		}
//		if (setsockopt(statusFileFd, SOL_SOCKET, SO_RCVTIMEO, &timeout_, sizeof(timeout_)) < 0) {
//			log_error("set timeout arg error");
//		}
	}
	close(tftpServerFd);
	//    fclose(logFile);
	deInitHardwares();
#ifdef __linux__
	void* ret;
	pthread_join(tid, &ret);
#endif

	return 0;
}

#include "zlib.h"
#include <stdlib.h>
#include "log.h"
#include "upload.h"
#define OS_NO_FILE_SYSTEM

void testZip(char *infileName,char *outfileName)
{
	FILE *ip;
	gzFile op;
	op=gzopen(outfileName,"wb");
	ip=fopen(infileName,"rb");
	fseek(ip,0,SEEK_END);
	size_t length = ftell(ip);
	fseek(ip,0,SEEK_SET);
	log_debug("length = %ld KB",length/1024);
	char *buf=(char*)malloc(length);
	int ret=fread(buf,1,length,ip);
	log_debug("ret = %ld KB",ret/1024);
	if(ret != length)
	{
		log_error("ret = %ld",ret);
		free(buf);
		fclose(ip);
		gzclose(op);
	}
	ret = gzwrite(op,buf,length);
	log_debug("ret = %ld ", ret);
	if (ret != length)
	{
		log_error("ret = %d",ret);
		free(buf);
		fclose(ip);
		gzclose(op);
	}
	free(buf);
	fclose(ip);
    gzclose(op);
}

void testUnzip(char *infileName,char *outfileName)
{
	FILE *op;
	gzFile ip;
	ip=gzopen(infileName,"rb");
	op=fopen(outfileName,"wb");
	int bufsize=4096;
	char buf[4096];
	int buf_read=0;
	int total_size=0;
	buf_read = gzread(ip,buf,bufsize);
	log_debug(" buf _read = %d",buf_read);
	while(bufsize == buf_read)
	{
		fwrite(buf,1,buf_read,op);
		total_size += buf_read;
		buf_read = gzread(ip,buf,bufsize);
	}
	if(buf_read <0)
		{
			log_error("buf_read = %d",buf_read);
			free(buf);
			fclose(op);
			gzclose(ip);
		}
	if(buf_read != 0)
	{
		fwrite(buf,1,buf_read,op);
	}
	free(buf);
	fclose(op);
	gzclose(ip);
}

int bsp_decom(char *oldName,char *newName,char *patchName,char *outName)
{
	int ret = bsdiffImp(oldName, patchName, newName);
	log_debug("bsdiff output ret = %d",ret);
	if(ret != 0)
	{
		log_error("create bsdiff error");
		return -1;
	}
//	ret = bspatchImp(oldName,patchName, outName);
//	if(ret != 0)
//	{
//		log_error("bspatch error ret = %d",ret);
//		return -1;
//	}
	return 0;
}
#define memsize 2048
const ADDRESS flashStartAddr = 0xa00000;
size_t totalsize = 0;
void flashwritefile() {
	ADDRESS flashWriteAddr = flashStartAddr;
	FILE* fp;
	fp = fopen("C:/MSL000.bin", "r");
	if (fp == NULL) {
		printf("fp open fail\n");
		return;
	}
	unsigned char mem[memsize] = { 0 };
	size_t rdsize = 0;
	ULONG fwsize = 0;
	size_t flashtotalsize = 0;
	size_t readtotalsize = 0;
	while (!feof(fp)) {
		printf("flashWriteAddr=%p\n", flashWriteAddr);
		//		memset(mem,0,memsize);
		rdsize = fread(mem, 1, memsize, fp);
		fwsize = flashWrite(flashWriteAddr, (char*) mem, rdsize);
		//		fwsize = rdsize;
		flashtotalsize += fwsize;
		readtotalsize += rdsize;
		flashWriteAddr = flashWriteAddr + rdsize;
//		flashWriteAddr = flashWriteAddr + fwsize;
		printf("fileread size=%d\t\tfile read total size=%ld\t\tflashwrite size=%ld\t\tflash total size=%ld\n", rdsize,
				readtotalsize, fwsize, flashtotalsize);
	}
	fclose(fp);
	totalsize = readtotalsize;
}

#if 0
size_t flashWriteFile(char *filename, size_t startAddr, size_t infoAddr)
{
	ADDRESS flashWriteAddr = startAddr;
	FILE* fp;
	//每次读取和写入的大小为2048字节
	int readWriteSize = 2048;
	fp = fopen(filename, "rb");
	if (fp == NULL) {
		log_error("flashWriteFile fp open fail\n");
		return 0;
	}
	unsigned char mem[readWriteSize] = {0};
	unsigned char infoBuf[128] = {0};
	size_t rdsize = 0;
	ULONG fwsize = 0;
	size_t flashtotalsize = 0;
	size_t readtotalsize = 0;
	while (!feof(fp)) {
		log_debug("flashWriteAddr=%p\n", flashWriteAddr);
		rdsize = fread(mem, 1, readWriteSize, fp);
		fwsize = flashWrite(flashWriteAddr, (char*) mem, rdsize);
		flashtotalsize += fwsize;
		readtotalsize += rdsize;
		flashWriteAddr = flashWriteAddr + rdsize;
//		flashWriteAddr = flashWriteAddr + fwsize;
		log_debug("fileread size=%d\t\tfile read total size=%ld\t\tflashwrite size=%ld\t\tflash total size=%ld\n", rdsize,
				readtotalsize, fwsize, flashtotalsize);
	}
	fclose(fp);

	//再记录一下文件的相关内容
	//               文件名$文件大小
	sprintf(infoBuf, "%s$%d", filename, readtotalsize);

	//写入到flash中
	fwsize = flashWrite(infoAddr, (char*) infoBuf, 128);

	log_debug("write info : %s OK , ret = %d", infoBuf, fwsize);

	return readtotalsize;
}
#endif
void flashreadfile() {
	printf("\n\nfile totalsize=%ld\n", totalsize);

	ADDRESS flashReadAddr = flashStartAddr;
	FILE* fp;
	fp = fopen("C:/outMSL.bin", "wb");
	if (fp == NULL) {
		printf("fp open fail\n");
		return;
	}
	unsigned char mem[memsize] = { 0 };
	size_t cpysize = 0;
	size_t wtsize = 0;
	size_t writetotalsize = 0;
	size_t remainsize = totalsize;
	while (remainsize) {
		printf("flashReadAddr=%p\n", flashReadAddr);
		memset(mem, 0, memsize);
		if (remainsize > memsize) {
			cpysize = memsize;
		} else {
			cpysize = remainsize;
		}
		memcpy(mem, (void*) flashReadAddr, cpysize);
		wtsize = fwrite(mem, 1, cpysize, fp);
		writetotalsize += wtsize;
		remainsize -= wtsize;
		flashReadAddr += cpysize;
		printf("cpysize=%ld\t\tfilewrite size=%ld\t\tfile write total size=%ld\t\tremainsize=%ld\n", cpysize, wtsize,
				writetotalsize, remainsize);
	}
	fclose(fp);
}
void Init(void) {
//	flashwritefile();
//	flashreadfile();
//	__asm__("bkpt #0");
	//test();
//	char *oldName="C:/fao.bin";
//	char *newName="C:/fan.bin";
//	char *patchName="C:/patch.gz";
//	char *outName="C:/newout.bin";
//	int ret = bsp_decom(oldName,newName,patchName,outName);
//	if(ret != 0)
//	{
//		log_error("bspdecom output error");
//	}
	
	//testFlashWrite();

	//testZip(1);
//	ADDRESS startAddr = 0xa01000;
//	ADDRESS infoAddr = 0xa00000;
//	ADDRESS maxFileAddr = 0xf00000;
//	char *fileName = "C:/MSLflsh.bin";
	//testBSDiff(1);
//	testZip("C:/glacier237++11d.bin","C:/out1.gz");
//	testUnzip("C:/out1.gz","C:/out.bin");
//	makeTempFileLUI("test.LUI");
	fileUpdateMain();
//	flashWriteFile(fileName,startAddr, infoAddr,maxFileAddr);

	//demo();
//#ifndef CONFIG_APP_AUTO_TEST
//	ACoreOs_Task_Param taskCreateParam;
//	ACoreOs_status_code retCode = ACOREOS_SUCCESSFUL;
//#endif /* CONFIG_APP_AUTO_TEST */
//
//#ifndef CONFIG_APP_AUTO_TEST
//
//    /* 在此加入应用代码 */
//    /* 在此加入应用代码，任务无亲和属性设置ACOREOS_TASK_NO_AFFINITY，否则给定处理器核号 */
//    taskCreateParam.affinity = ACOREOS_TASK_NO_AFFINITY;
//    taskCreateParam.attribute_set =  ACOREOS_PREEMPT | ACOREOS_TIMESLICE;
//    taskCreateParam.domain = (void *)ACOREOS_KERNEL_ID;
//    taskCreateParam.initial_priority = 11;
//    taskCreateParam.stack_size = 4096;
//
//    retCode = ACoreOs_task_create((ACoreOs_name)"Task1", &taskCreateParam, &tstTaskid1);
//    if ( retCode != ACOREOS_SUCCESSFUL )
//    {
//    	printf("Failed to create Task1, return code:%#x!\n",retCode);
//    }
//
//    retCode = ACoreOs_task_start(tstTaskid1, (ACoreOs_task_entry)taskEntry1, 0);
//    if ( retCode != ACOREOS_SUCCESSFUL )
//    {
//      	printf("Failed to start Task1, return code:%d!\n", retCode);
//    }
//
//    taskCreateParam.affinity = ACOREOS_TASK_NO_AFFINITY;
//    taskCreateParam.attribute_set =  ACOREOS_PREEMPT | ACOREOS_TIMESLICE;
//    taskCreateParam.domain = (void *)ACOREOS_KERNEL_ID;
//    taskCreateParam.initial_priority = 12;
//    taskCreateParam.stack_size = 4096;
//
//    retCode = ACoreOs_task_create((ACoreOs_name)"Task2", &taskCreateParam, &tstTaskid2);
//    if ( retCode != ACOREOS_SUCCESSFUL )
//    {
//    	printf("Failed to create Task2, return code:%#x!\n",retCode);
//    }
//
//    retCode = ACoreOs_task_start(tstTaskid2, (ACoreOs_task_entry)taskEntry2, 0);
//    if ( retCode != ACOREOS_SUCCESSFUL )
//    {
//        printf("Failed to start Task2, return code:%d!\n", retCode);
//    }
//#else /* 以下为测试的自动入口调用 */
//    /*
//     * 调用自动测试入库程序，完成用例的自动运行和日志记录。
//     */
//    auto_test_procedure();
//#endif /* CONFIG_APP_AUTO_TEST */
}
