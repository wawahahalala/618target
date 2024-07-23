#ifndef _FIND_H_
#define _FIND_H_
#include<sys/types.h>
#ifdef __cplusplus
extern "C"{
#endif
int checkFindPacketIllegal(char* findPacket, ssize_t len);
void* find(void * param);
ssize_t makeFindResponsePacket(char* responsePacket);
#ifdef __cplusplus
}
#endif
#endif
