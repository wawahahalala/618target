#include "find.h"
#include "log.h"
#include "global.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
const unsigned short find_listen_port = 1001;
const unsigned char find_packet_max_len = 128;
void* find(void * param){
	printf("in find\n");
	int fd;
	if((fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
		log_error("FIND SOCKET INIT ERROR\n");
		return -1;
	}
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	//addr.sin_addr.s_addr = hton
	addr.sin_port = htons(find_listen_port);
	int opt = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const void*)&opt, sizeof(opt));
	if(bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0){
		log_error("FIND SOCKET BIND ERROR");
		close(fd);
		return -1;
	}
	struct sockaddr_in loader_addr;
	socklen_t addr_len = sizeof(loader_addr); 
	char buf[find_packet_max_len];
	log_debug("FIND SERVER INIT SUCCESS");
	while(1){
		ssize_t len = recvfrom(fd, buf, find_packet_max_len, 0, (struct sockaddr*)&loader_addr, &addr_len);
		if(len < 0){
			//printf("recvfrom error\n");
			log_debug("FIND SOCKET RECVFROM RETURN -1");
			continue;
		}
		else{
			//check find packet illegal
			if(checkFindPacketIllegal(buf, len) < 0){
				log_debug("FIND PACKET ILLEGAL");
				continue;
			}
			len = makeFindResponsePacket(buf);
			if(sendto(fd, buf, len, 0, (struct sockaddr*)&loader_addr, addr_len) < 0){
				log_debug("SENDTO RETURN -1");
			}
			else{
				log_debug("FIND RESPONSE SUCCESS TO IP:%s PORT:%hu", inet_ntoa(loader_addr.sin_addr), ntohs(loader_addr.sin_port));
			}
		}
	}
}

int checkFindPacketIllegal(char* findPacket, ssize_t len){
	return 1;
}

ssize_t makeFindResponsePacket(char* responsePacket){
	char *begin = responsePacket;
	
	responsePacket[0]=0;
	responsePacket[1]=2;

	responsePacket = responsePacket + 2;
	memcpy(responsePacket, TARGET_HARDWARE_IDENTIFIER, sizeof(TARGET_HARDWARE_IDENTIFIER));
	responsePacket = responsePacket + sizeof(TARGET_HARDWARE_IDENTIFIER);

	memcpy(responsePacket, TARGET_TYPE_NAME, sizeof(TARGET_TYPE_NAME));
	responsePacket = responsePacket + sizeof(TARGET_TYPE_NAME);

	memcpy(responsePacket, TARGET_POSITION, sizeof(TARGET_POSITION));
	responsePacket = responsePacket + sizeof(TARGET_POSITION);

	memcpy(responsePacket, LITERAL_NAME, sizeof(LITERAL_NAME));
	responsePacket = responsePacket + sizeof(LITERAL_NAME);

	memcpy(responsePacket, MANUFACTURER_CODE, sizeof(MANUFACTURER_CODE));
	responsePacket = responsePacket + sizeof(MANUFACTURER_CODE);

	*responsePacket = 0x10;
	responsePacket = responsePacket + 1;

	return responsePacket - begin;
}



