/**
*
* header contenant toutes les structures utiles pour le sender
*
**/

#ifndef STRUCT_H
#define STRUCT_H

#include <pthread.h>
#include <zlib.h>


//création d'un structure de donnée pour envoyer les messages:
typedef struct msgUDP{
	uint8_t type : 3;
	uint8_t window : 5;
	uint8_t seq_num;
	uint16_t length;
	char payload[512];//je ne sais pas comment implémenter le payload de 512bytes dans la structure
	uLong crc32; 
}__attribute__((packed)) msgUDP;



/*
* structure pour gérer la fenetre
*/
struct window {
	char **buffer; // tableau de pointeurs vers les structures contenant les paquets envoyé ainsi que le moment auquel ils ont été envoyé
	int nb_elem_vide; // pour savoir le nombre d'élément libre dans la fenetre
	pthread_mutex_t *mutex; // mutex pour proteger l'utilisation des informations contenue dans la fenetre
};


/*
* structure pour gérer les paquets dans la fenetre
*/
struct paquet {
	char *paq; //element à envoyer
	int time; // à modifier lors de la création du timer
	int ack; // booléan pour dire si l'élément à été ack (1), 0 sinon
};


struct paquet_to_send{
	struct paquet *paq; //liste de paquets
	int nb_elem; //nombre d'elem dans la liste
	pthread_mutex_t mutex; // protège l'élément
};

#endif // fin du header STRUCT_H
