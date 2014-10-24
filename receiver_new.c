#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h> // Pour la gestion des sockets
#include <stdio.h>	// for fprintf 
#include <string.h>	// for memcpy
#include <zlib.h>       // Pour le CRC
#include <stdint.h>     // Pour les uint8_t, ...
#include <netdb.h> 	   // à importer pour le getaddrinfo (surtout les structures avant)

#include "struct.h"

#define PACKET_SIZE 520
#define PAYLOAD_SIZE 512
#define WITHOUT_CRC_SIZE 516
#define PTYPE_DATA 1
#define PTYPE_ACK 2

// METTRE LA LONGUEUR DES MESSAGES ENVOYE A +1 !!! pour qu'il mette le 
int main(int argc, char *argv[])
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;

    char *filename;
	int port;
	char *port_to_string;
	char *hostname;

    if (argc != 3 && argc != 5) 
    {
        fprintf(stderr, "Usage: %s --file filename hostname port\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    if (argc == 5)
	{
		// copier le filename
		filename = argv[2];
		hostname = argv[3];
		port = atoi(argv[4]);
		port_to_string = argv[4];
	}
	else 
	{
		// prendre le filename sur le stdin
		printf("Veuillez entrez le nom de l'hote ou l'adresse IPv6\n");
		scanf("%s",filename);			// Comment on fait pour l'extension ???
		hostname = argv[1];
		port = atoi(argv[2]);
		port_to_string = argv[2];
	}

	printf("Nom du programme : %s , nom du fichier : %s , nom de l'hote attendu : %s et numéro de port : %d\n", argv[0], filename, hostname, port);

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET6;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
    hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
    hints.ai_protocol = 0;          /* Any protocol */


    int s;
    s = getaddrinfo(hostname, port_to_string, &hints, &result); //port_to_string
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    int sockett;
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sockett = socket(rp->ai_family, rp->ai_socktype,rp->ai_protocol);
        if (sockett == -1)
            continue;

        if (bind(sockett, rp->ai_addr, rp->ai_addrlen) == 0)
            break;                  // Success 

        close(sockett);
    }

    if (rp == NULL) {               // No address succeeded
        fprintf(stderr, "Could not bind\n");
        exit(EXIT_FAILURE);
    }

    printf("Connexion réussie !\n");

    freeaddrinfo(result);           // No longer needed 


    // ------------------------------------------------------------------
	// Programme proprement dit :::
    
    int longueur_recu = PAYLOAD_SIZE;
    char packet_buf[PACKET_SIZE]; 			// buffer de réception du packet
    char payload_buf[PAYLOAD_SIZE]; // PAYLOAD_SIZE    		// buffer de réception du payload
    char ack_buf[PACKET_SIZE];

    struct msgUDP *packet_struct= NULL;

    char buffer_tot[255][PACKET_SIZE];
    memset(buffer_tot, 0, sizeof(buffer_tot)); // Pour pouvoir le comparer au char c afin de déterminer là ou il y a des données et calculer le lastack
    int maximum = 15;						// Maximum là ou on peut écrire (-> définit aussi la taille de la fenêtre dans les ACC !!!, non, parce que pas int)
    int lastack	= -1;						// Dernier qu'on a reçu
    int attendu = 0;						// C'est celui attendu (= lastack + 1)
    int minimum = attendu;					// Minimum de la fenêtre d'acceptation des paquets

    char c[1];								// Pour trouver les endroits ou il y a rien sur le buffer_tot
    memset(c, 0, sizeof(c));

    // Ouvrir le fichier
	FILE *fichier = fopen(filename, "w");
	if (fichier == NULL)
	{
    	printf("Error opening file!\n");
    	exit(1);
	}

	// Préparation de l'accusé
	memset(ack_buf, 0, PACKET_SIZE); // Remet à 0 la zone d'accusé
	struct msgUDP *ack_struct = (struct msgUDP *) ack_buf;
	ack_struct->type = PTYPE_ACK;
	ack_struct->window = 15;
	ack_struct->seq_num = 0;
	ack_struct->length = PAYLOAD_SIZE;
	// Pas besoin pour le payload puisqu'il est déjà à 0
	ack_struct->crc32 = 0;

	int j = 0; 
	int h = 0;
    while(longueur_recu == PAYLOAD_SIZE)
    {
    	printf("On est dans la boucle !\n");
    	h++;
    	// On reçoit un packet
        longueur_recu = recvfrom(sockett, packet_buf, PACKET_SIZE, 0, rp->ai_addr, &(rp->ai_addrlen)); // !!!! ON A LES INFOS DU SENDER QUI SE METTE DANS addr_sender
        if (longueur_recu == -1) {
            printf("Problème, le message reçu n'est pas valide...\n" );            
        }

        // On a reçu un buffer qui est en fait un packet. A la base, c'était une structure donc on la caste dans un structure.
        packet_struct = (struct msgUDP *) packet_buf;

        // Checker Type et CRC
        uLong crc = crc32(0L, Z_NULL, 0);
   		crc = crc32(crc, packet_buf, strlen(packet_buf) - sizeof(uLong));

		if (crc != packet_struct->crc32)
		{
			
			// Le CRC n'est pas bon, le paquet doit être discardé
			printf("Les CRC ne correspondent pas, le packet va être discardé\n");
		}
		else 
		{
			// Si on est ici, c'est que les CRC correspondent
			
			// Copie de tous les éléments requis depuis la structure
	        strcpy(payload_buf, packet_struct->payload);
	        j = packet_struct->seq_num;
	        longueur_recu = packet_struct->length;

			// Vérification de si c'est la fin des packets
	        if (longueur_recu < PAYLOAD_SIZE)
	        {
	        	printf("C'est l'envoi qui marque la fin de la connexion car < 512\n");
	        	break;
	        }

	        // Gestion du stockage de l'élément en fonction de là ou est la fenêtre !!!
	        if(j >= minimum && j <= maximum && packet_struct->type == PTYPE_DATA)
	        {
	        	// Copie dans le buffer
	        	strcpy(buffer_tot[j], payload_buf);
	        }
	        else if (maximum < minimum && j >= minimum && packet_struct->type == PTYPE_DATA)
	        {
	        	// Ok aussi, ça veut dire que max = 13, min = 252 et j = 254
	        	strcpy(buffer_tot[j], payload_buf);
	        }
	        else if (maximum < minimum && j <= maximum && packet_struct->type == PTYPE_DATA)
	        {
	        	// Ok aussi, ça veut dire que max = 13, min = 252 et j = 8
	        	strcpy(buffer_tot[j], payload_buf);
	        }
	        else
	        {
	        	printf("packet discardé car numéro hors de la fenêtre\n");
	        }
			
			printf("Nouveau message reçu : type %d, window number : %d, sequence number : %d, length : %d \n", 
				packet_struct->type, packet_struct->window, packet_struct->seq_num, packet_struct->length);
			printf("Contenu : %s\n", payload_buf);
		}

		// Actualisation des variables							
		while(strncmp(buffer_tot[attendu] , c,  1) != 0) 					// Temps que le attendu est rempli
		{
			fprintf(fichier, "%s\n", buffer_tot[attendu]); 					// Copie dans le fichier
			memset(buffer_tot[attendu], 0, sizeof(buffer_tot[attendu])); 	// On remet à 0

			lastack = attendu;												// lastack = attendu
			attendu = (attendu + 1) % 256;									// attendu++
			minimum = attendu;												// minimum = attendu
			maximum = (maximum + 1) % 256;									// maximum++
		}

		// Envoyer un ACK
		ack_struct->seq_num = lastack;

		uLong crcAck = crc32(0L, Z_NULL, 0);
		crcAck = crc32(crcAck, ack_buf, strlen(ack_buf) - sizeof(uLong));
		ack_struct->crc32 = crcAck;

		if(sendto(sockett, ack_struct, sizeof(struct msgUDP), 0, rp->ai_addr, &(rp->ai_addrlen)) == sizeof(struct msgUDP))
		{
			printf("Accusé envoyé avec numéro : %d\n", lastack);
		}

    }


    fclose(fichier);
}