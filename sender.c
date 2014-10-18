#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdint.h>
#include <zlib.h>
#include <string.h>


//création d'un structure de donnée pour envoyer les messages:
typedef struct msgUDP{
	uint8_t Type : 3;
	uint8_t Window : 5;
	uint8_t Seq_num;
	uint16_t Length;
	char payload[512];//je ne sais pas comment implémenter le payload de 512bytes dans la structure
	int crc32; //j'ai pris un int parce qu'il a la bonne dimension, c'est a dire 4 bits ... mais normalement uLong crc32
}__attribute__((packed)) msgUDP;

typedef struct param{
	char *filename;
	char *hostname;
	int port;
	int sber;
	int splr;
	int delay;
}param;


int main(int argc, char *argv[]){
	//gestion des paramètres:
	int i = 1;
	char *filename;
	int sber=0,splr=0, delay=0;
	while(i<argc){
		if(strcmp(argv[i], "--file")==0){
			filename = argv[i+1];
		}
		if(strcmp(argv[i], "--sber")==0){
			sber = atoi(argv[i+1]); // si il y a une erreur sber est mis à 0 ....
		}
		if(strcmp(argv[i], "--splr")==0){
			splr = atoi(argv[i+1]); // mis à 0 en cas d'erreur
		}
		if(strcmp(argv[i], "--delay")==0){
			delay = atoi(argv[i+1]); // mis à 0 en cas d'erreur
		}

		i++;
	}
	printf("filename %s,\nsber %d,\nsplr %d,\ndelay %d.\n", filename, sber, splr, delay);
}




