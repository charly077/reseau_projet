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
    
    uint16_t length_receiv = PACKET_SIZE;
    int nbre_bits_recu;
    char packet_buf[PACKET_SIZE]; 			// buffer de réception du packet
    char payload_buf[PAYLOAD_SIZE]; // PAYLOAD_SIZE    		// buffer de réception du payload

    struct msgUDP *packet_struct= NULL;

    char buffer_tot[255][PACKET_SIZE];
    int minimum = 0;    					// Minimum là ou on peut écrire
    int maximum = 15;						// Maximum là ou on peut écrire
    int lastack	= 0;						// Suivant attendu 

    // Ouvrir le fichier
	FILE *fichier = fopen(filename, "w");
	if (fichier == NULL)
	{
    	printf("Error opening file!\n");
    	exit(1);
	}

	int j = 0; 
	int h = 0;
    while(length_receiv == PACKET_SIZE && h < 3) // définir un cran d'arrêt
    {
    	printf("On est dans la boucle !\n");
    	h++;
    	// On reçoit un packet
        nbre_bits_recu = recvfrom(sockett, packet_buf, PACKET_SIZE, 0, rp->ai_addr, &(rp->ai_addrlen)); // !!!! ON A LES INFOS DU SENDER QUI SE METTE DANS addr_sender
        if (nbre_bits_recu == -1) {
            printf("Problème, le message reçu n'est pas valide...\n" );               // Ignore failed request 
        }

        // On a reçu un buffer qui est en fait un packet. A la base, c'était une structure donc on la caste dans un structure.
        packet_struct = (struct msgUDP *) packet_buf;

        // Checker Type et CRC
        /*
        uLong crc = crc32(0L, Z_NULL, 0); 											// INT !!!!!!!!!
        memcpy(calculerCRC_buf, packet_buf, strlen(packet_buf) - sizeof(uLong)); 				// Je crée un buffer surlequel je vais pouvoir calculer le CRC
   		crc = crc32(crc, calculerCRC_buf, strlen(calculerCRC_buf));
		*/

		// Vérification de si c'est la fin des packets
        if (packet_struct->length < PAYLOAD_SIZE)
        {
        	printf("C'est l'envoi qui marque la fin de la connexion car < 512\n");
        	break;
        }
		
        // Copie de tous les éléments requis depuis la structure
        strcpy(payload_buf, packet_struct->payload);
        j = packet_struct->seq_num;
        
        if(j >= minimum && j <= maximum && packet_struct->type == PTYPE_DATA)
        {
        	// Copie dans le buffer
        	strcpy(buffer_tot[j], payload_buf);

        	// Si c'est le plus petit élément, alors on déplace la fenêtre et on l'écrit dans le fichier.
        	if (j == minimum)
        	{
        		// Ecriture dans le fichier	
        		fprintf(fichier, "%s\n", buffer_tot[minimum]);
        		lastack++;
        		minimum++;
        		maximum++;
        	}
        	j++;																// A supprimer
        }
        else
        {
        	printf("packet discardé car numéro hors de la fenêtre\n");
        }
		

        // ENVOYER ACK

		printf("Nouveau message reçu : type %d, window number : %d, sequence number : %d, length : %d et le CRC est %s \n", 
			packet_struct->type, packet_struct->window, packet_struct->seq_num, packet_struct->length, packet_struct->crc32);
		printf("Contenu : %s\n", packet_struct->payload_buf);



    }


    fclose(fichier);
}