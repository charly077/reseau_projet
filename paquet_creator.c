#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <zlib.h>

#include "paquet_creator.h"
#include "struct.h"

#define TAILLE_PAYLOAD 512
/* TEST
int main(int argc, char *argv[]){
	int fd = file_desc("coucou1.txt");
	char *buf = NULL;
	printf("bonjour j'ai un element de %dbytes",tab_512b(fd, &buf));
	struct msgUDP *msg; 
	create_paquet(fd,1, &msg);

	return 0;
}
*/

int tab_512b(int desc, char **elem){
	char *buf = (char *) malloc(TAILLE_PAYLOAD); // allouer une place de 512 bytes pour le contenu de l'élément
	memset(buf, 0, TAILLE_PAYLOAD); // on est sur que si le fichier est plus petit, ou terminé, on a un padding de 0

	int size = read(desc, buf, TAILLE_PAYLOAD);
	if(size == -1){
		fprintf(stderr, "Il y a eu une erreur lors de la lecture du fichier :\n%s\n", strerror(errno)); 
		exit(EXIT_FAILURE);
	}
	*elem = buf;
	return size;
}

int file_desc(char *filename){
	int fd = open(filename, O_RDONLY);
	if(fd==-1){
		fprintf(stderr, "il y a eu une erreur lors de l'ouverture du fichier:\n%s",strerror(errno));
		exit(EXIT_FAILURE);
	}
	return fd; // ATTENTION ne pas oublier de fermer le file_desc
}

void create_paquet(int desc, int seq_num, struct msgUDP **paquet){
	char *payload;
	int size = tab_512b(desc, &payload);
	struct msgUDP *new_paquet = (struct msgUDP *) malloc(sizeof(struct msgUDP));
	new_paquet->type = 1;
	new_paquet->window=0;
	new_paquet->seq_num = seq_num;
	new_paquet->length = size; // taille de l'élément et donc si c'est le dernier paquet la taille est <à 512bytes
	strncpy(new_paquet->payload, payload, 512);

	//calcul du crc sur tout le contenu sauf lui mm
	new_paquet->crc32 = crc32( 0, (void *)new_paquet, sizeof(msgUDP) - sizeof(uLong));
}

