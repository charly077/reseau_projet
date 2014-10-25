#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdint.h>
#include <zlib.h>
#include <string.h>

#include "struct.h"
#include "paquet_creator.h"
int main(int argc, char *argv[]){
	//VARIABLES ET GESTION DES PARAMETRE:
	int i = 1;
	char *filename; // Attention à bien vérifier le bool pour si on prend sur le stdin ou pas ...
	int filenamegive=0; // le nom n'a pas été donné --> on prend sur le stdin
	int sber=0,splr=0, delay=0; // pour les param
	struct addrinfo *res, *addr; // pointeur vers le res qui sera l'info sur l'addresse du reveiver
	struct addrinfo hints; // va nous permettre de dire qu'on veut du IPv6 et du UDP
	int s; // pour gérer les erreur de getaddrinfo
	int sock; // pour définir le socket


	while(i<argc){
		if(strcmp(argv[i], "--file")==0){
			filename = argv[i+1];
			filenamegive = 1;
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
	//printf("filename %s,\nsber %d,\nsplr %d,\ndelay %d.\n", filename, sber, splr, delay); // test pour vérification des arguments

	//RECUPERATION DES INFO SUR L'ADDRESSE DU RECEIVER :

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET6; // je veux juste de l'IPv6
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags=0;
	hints.ai_protocol = 0 ; // peut importe le protocol ...


	s = getaddrinfo(argv[argc-2], argv[argc-1], &hints, &res); // obtention de la liste d'addrinfo
	if (s != 0){
		fprintf(stderr, "getaddrinfo: %s\n",gai_strerror(s));
		exit(EXIT_FAILURE);
	}
	//res est une liste de strucutures d'addresse à laquelle on essaye de se connecter 
	for(addr = res; addr != NULL; addr = addr->ai_next){
		sock =  socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
		if(sock == -1) continue; // si on arrive pas à créer un socket on continue
		if(connect(sock, addr->ai_addr, addr->ai_addrlen) != 1) break; // si la connection à fonctionnée, on sort de la boucle

		close(sock); // si on est pas sorti de la boucle il faut fermer le socket
	}

	if (addr == NULL){
		fprintf(stderr, "Impossible de se connecter\n");
		exit(EXIT_FAILURE);
	}
	
	printf("Arrivé au point");
	

	//test d'envoi d'un paquet :
	int j = 0;
	int fin = 10;
	
	//struct msgUDP *msg = NULL;

	while (j <= fin)
	{	
		char total[520];
		char payload[512];		
		sprintf(payload, "Message %d : Coucou tout le monde, comment ça va, j'aimerais bien etre à l'eglise pour l'instant", j);
		struct msgUDP *msg = (struct msgUDP *) total;
		msg->type = 1;
		msg->window = 0;
		msg->seq_num = j;
		msg->length = 512;
		if (j == fin)
		{
			msg->length = 0;			
		}
		strcpy(msg->payload, payload);
		msg->crc32 = crc32(0, (void *)msg, sizeof(msgUDP) - sizeof(uLong));
	
		if(sendto(sock, msg, sizeof(struct msgUDP),0,addr->ai_addr, addr->ai_addrlen)==sizeof(struct msgUDP))
		{
			printf("message numéro %d envoyé \n", j);
		}
		j++;
	}

	freeaddrinfo(res); 


	close(sock);

}
