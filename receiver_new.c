#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h> // Pour la gestion des sockets
#include <stdio.h>	// for fprintf 
#include <string.h>	// for memcpy
#include <zlib.h>       // Pour le CRC
#include <stdint.h>     // Pour les uint8_t, ...
#include <netdb.h> 	   // à importer pour le getaddrinfo (surtout les structures avant)
#include <sys/time.h>	// Pour les timers de select (qu'on n'a pas ici mais bon...)
#include <unistd.h>	// Pour select()

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

    	char filename[20];
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
		//filename = argv[2];
		strcpy(filename, argv[2]);
		hostname = argv[3];
		port = atoi(argv[4]);
		port_to_string = argv[4];
	}
	else 
	{
		// prendre le filename sur le stdin
		printf("Veuillez entrez le nom du fichier (max 19 cara.)\n");
		scanf("%19s",filename);					// Comment on fait pour l'extension ???
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

    int sockett;   						// ATTENTION -> socket = descripteur de fichier (comme stdout,FILE, ...) 
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
    uint8_t maximum = 15;						// Maximum là ou on peut écrire (-> définit aussi la taille de la fenêtre dans les ACC !!!, non, parce que pas int)
    uint8_t lastack = -1;						// Dernier qu'on a reçu
    uint8_t attendu = 0;						// C'est celui attendu (= lastack + 1)
    uint8_t minimum = attendu;						// Minimum de la fenêtre d'acceptation des paquets

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

	//int j = 0; 
	uint8_t j = 0;
	int permission_lecture = 0;
	fd_set donne_dispo;
	int permission_envoi = 0;
	fd_set dispo_envoi;
	
	//-------------------------------------
	// BOUCLE PRINCIPALE !!!
	//------------------------------------
    while(longueur_recu >= PAYLOAD_SIZE) 			// Petit erreur -> 520 ou 512 ???
    {
    	printf("On est dans la boucle !\n");
	
	FD_ZERO(&donne_dispo);	  				// Remise à 0 de la struct donne dispo
	FD_SET(sockett, &donne_dispo);				// Pas compris ???
	
    	// On reçoit un packet 
	if((permission_lecture = select(sockett+1, &donne_dispo, NULL, NULL, NULL)) < 0)
	{
		printf("Erreur lors du select()");
		return EXIT_FAILURE;
	}
	
	if(permission_lecture == 0)
	{
		// timer a expiré -> on n'aura pas ce cas là puisqu'il n'y a pas de timer
		printf("Le timer a expiré de lecture a expiré...");
	}
	if(FD_ISSET(sockett, &donne_dispo))
   	{
      		/* des données sont disponibles sur le socket */
      		/* traitement des données */
		printf("On va écouter\n");
		longueur_recu = recvfrom(sockett, packet_buf, PACKET_SIZE, 0, rp->ai_addr, &(rp->ai_addrlen)); // !!!! ON A LES INFOS DU SENDER QUI SE METTE DANS addr_sender
		if (longueur_recu == -1) {
		    printf("Problème, le message reçu n'est pas valide...\n" );            
		}

		// On a reçu un buffer qui est en fait un packet. A la base, c'était une structure donc on la caste dans un structure.
		packet_struct = (struct msgUDP *) packet_buf;

		// Checker Type et CRC
		//uLong crc = crc32(0L, Z_NULL, 0);   		
		int crc = (int) crc32(0, (void *) packet_struct, sizeof(msgUDP)-sizeof(int)); //- sizeof(uLong)); // J'ai modifié ici (de gcc à clang)

		if (crc != packet_struct->crc32 || packet_struct->type != PTYPE_DATA)
		{	
			// Le CRC n'est pas bon ou le type ne vaut pas 1, le paquet doit être discardé
			printf("Les CRC %d et %d ne correspondent pas, le packet va être discardé\n", crc, packet_struct->crc32);
			longueur_recu = -1; // Pour quand même continuer dans la boucle
		}
		//----------------------------------------------
		// Les CRC correspondent et le type = 1, le  packet est donc potentiellement correct (faut encore vérifier le numéro de séquence
		else 
		{
			// Si on est ici, c'est que les CRC correspondent
			printf("Nouveau message reçu : type %d, window number : %d, sequence number : %d, length : %d \n", 
				packet_struct->type, packet_struct->window, packet_struct->seq_num, packet_struct->length);
			// Copie de tous les éléments requis depuis la structure
			j = packet_struct->seq_num;			
			strcpy(payload_buf, packet_struct->payload);
			longueur_recu = packet_struct->length;
			
			// Gestion du stockage de l'élément en fonction de là ou est la fenêtre !!!
			printf("min : %d < %d < %d : max\n", minimum, j,  maximum);
			if(j >= minimum && j <= maximum)
			{
				// Copie dans le buffer
				strncpy(buffer_tot[j], payload_buf, longueur_recu);
			}
			else if (maximum < minimum && j >= minimum)
			{
				// Ok aussi, ça veut dire que max = 13, min = 252 et j = 254
				strncpy(buffer_tot[j], payload_buf, longueur_recu);
			}
			else if (maximum < minimum && j <= maximum)
			{
				// Ok aussi, ça veut dire que max = 13, min = 252 et j = 8
				strncpy(buffer_tot[j], payload_buf, longueur_recu);
			}
			else
			{
				printf("packet discardé car numéro hors de la fenêtre\n");
			}
			
			// Vérification de si c'est la fin des packets
			/*
			if (longueur_recu < PAYLOAD_SIZE)
			{
				printf("C'est l'envoi qui marque la fin de la connexion car < 512\n");
				break;
			}
			*/
		} // On sort de la partie : si les CRC correspondent bien, parce que même si les CRC correspondent pas, il faut envoyer un ACK
		
		// -----------------------------------------
		// Actualisation des variables							
		while(strncmp(buffer_tot[attendu] , c,  1) != 0) 					// Temps que le attendu est rempli
		{
			fprintf(fichier, "%s\n", buffer_tot[attendu]); 					// Copie dans le fichier
			//printf("attendu dans la boucle : %d\n", attendu);
			//printf("%s \n", buffer_tot[attendu]);
			memset(buffer_tot[attendu], 0, sizeof(buffer_tot[attendu])); 			// On remet à 0
			lastack = attendu;												// lastack = attendu
			attendu = (attendu + 1) % 256;									// attendu++
			minimum = attendu;												// minimum = attendu
			maximum = (maximum + 1) % 256;									// maximum++
			
		}
	//} // Fin du select -> si les données sont dispo en lecture -> A remettre
	
	//FD_ZERO(&dispo_envoi);	 -> a remettre 				// Remise à 0 de la struct donne dispo
	//FD_SET(sockett, &dispo_envoi); -> a remettre		

	// ----------------------------------------
	// Envoyer un ACK
	// Si les données sont dispo en envoi (pour envoyer l'ACK)
		
	// PARTIE SELECT, il me la faut ?  -> a remettre

	/*
	printf("On attend d'envoyer...\n");
	if ((permission_envoi = select(sockett, NULL, &dispo_envoi, NULL, NULL)) < 0) //-> il me faut un autre socket nn ? 
	{
		printf("Erreur lors du select pour l'envoi\n");
		return EXIT_FAILURE;
	}
	if (permission_envoi == 0)
	{
		// Le timer a été déclenché, mais il y en pas
		printf("Le timer pour l'envoi à été écoulé, mais il en a pas donc on ne devrait jamais voir ce message donc j'écris pour le plaisir #happy\n");
	}
	if (FD_ISSET(sockett, &dispo_envoi))
	{	
	*/
		if (longueur_recu >= PAYLOAD_SIZE || longueur_recu == -1) // Permet de vérifier si le packet qu'on a reçu est le dernier ou pas		
		{		
			//printf("Envoi du socket dispo\n");
			ack_struct->seq_num = lastack+1;
			int crcAck = crc32(0L, Z_NULL, 0);
			//crcAck = crc32(crcAck, ack_buf, strlen(ack_buf) - sizeof(uLong));       // j'ai modifié (compilait avec gcc ici)
			crcAck = (int) crc32(0, (void *) ack_struct, sizeof(ack_struct) -sizeof(int)); //- sizeof(uLong));
			ack_struct->crc32 = crcAck;
		
			if(sendto(sockett, ack_struct, sizeof(struct msgUDP), 0, rp->ai_addr, rp->ai_addrlen) == sizeof(struct msgUDP))
			{
				printf("Accusé envoyé avec numéro : %d\n", lastack+1);
			}
			longueur_recu = PAYLOAD_SIZE;
		}
	} //-> a enlever
	//} -> a remettre
	
    }	// fin du while


    fclose(fichier);
    printf("Fin du programme\n");
} // fin du main
