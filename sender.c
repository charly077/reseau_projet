#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdint.h>


//création d'un structure de donnée pour envoyer les messages:
typedef msgUDP struct msgUDP{
	uint8_t Type : 3;
	uint8_t Window : 5;
	uint8_t Seq_num;
	uint16_t Length;
	//je ne sais pas comment implémenter le payload de 512bytes dans la structur
	//uLong crc32;

} __attribute__((packed));


int main(int argc, char *argv[]){
	printf("Bonjour tu vas bien .. enfin voici un turk sympa ;) : %d " , sizeof(unsigned int));
}
