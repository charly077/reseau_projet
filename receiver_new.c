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

    if (argc != 3 && argc != 5) {
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
    s = getaddrinfo(hostname, port_to_string, &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    int sockett;
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sockett = socket(rp->ai_family, rp->ai_socktype,
                rp->ai_protocol);
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
    
    int length_receiv = PACKET_SIZE;
    char packet_buf[PACKET_SIZE]; 			// buffer de réception du packet
    char payload_buf[PACKET_SIZE]; // PAYLOAD_SIZE    		// buffer de réception du payload

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
    while(length_receiv > 5) // définir un cran d'arrêt
    {
    	// On reçoit un packet
        length_receiv = recvfrom(sockett, packet_buf, PACKET_SIZE, 0, rp->ai_addr, &(rp->ai_addrlen)); //(struct sockaddr *) &addr_sender, &addrlen); // !!!! ON A LES INFOS DU SENDER QUI SE METTE DANS addr_sender
        if (length_receiv == -1) {
            printf("Problème, le message reçu est trop court...\n" );               // Ignore failed request 
        }

        // On a reçu un buffer qui est en fait un packet. A la base, c'était une structure donc on la caste dans un structure.
        // !!!!! PAS OPTIMISE, declarer le struct a l'exterieur de la boucle
        //struct msgUDP *packet_struct = (struct msgUDP *) packet_buf;
        strcpy(payload_buf, packet_buf);
        // Ecriture dans le fichier	
        //fprintf(fichier, "%s\n", payload_buf);
        if(j >= minimum && j <= maximum)
        {
        	strcpy(buffer_tot[j], payload_buf);
        	if (j == minimum)
        	{
        		lastack++;
        		minimum++;
        		maximum++;
        	}
        	j++; 																	// A supprimer
        }
        else
        {
        	printf("packet discardé car numéro hors de la fenêtre\n");
        }
		
		printf("Nouveau message reçu de longueur: %d\n", length_receiv);

    }

    fprintf(fichier, "%s\n", buffer_tot[0]);
    fprintf(fichier, "%s\n", buffer_tot[1]);
    fprintf(fichier, "%s\n", buffer_tot[2]);


    fclose(fichier);
}