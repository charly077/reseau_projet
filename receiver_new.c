#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h> // Pour la gestion des sockets
#include <stdio.h>	// for fprintf 
#include <string.h>	// for memcpy
#include <zlib.h>       // Pour le CRC
#include <stdint.h>     // Pour les uint8_t, ...
#include <netdb.h> 	   // à importer pour le getaddrinfo (surtout les structures avant)

#define PACKET_SIZE 520
#define PAYLOAD_SIZE 512
#define WITHOUT_CRC_SIZE 516
#define PTYPE_DATA 1
#define PTYPE_ACK 2

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
    /*
    int length_receiv = PACKET_SIZE;
    char packet_buf[PACKET_SIZE]; 			// buffer de réception du packet
    char payload_buf[PAYLOAD_SIZE];    		// buffer de réception du payload
    char calculerCRC_buf[WITHOUT_CRC_SIZE]; // Stocker la partie sur laquelle doit être calculée le CRC

    while(length_receiv == PACKET_SIZE) 
    {
    	// On reçoit un packet
        length_receiv = recvfrom(sockett, packet_buf, PACKET_SIZE, 0, adresse_sender->ai_addr, &(adresse_sender->ai_addrlen)); //(struct sockaddr *) &addr_sender, &addrlen); // !!!! ON A LES INFOS DU SENDER QUI SE METTE DANS addr_sender
        if (length_receiv == -1) {
            printf("Problème, le message reçu est trop court...\n" );               // Ignore failed request 
        }

        // On a reçu un buffer qui est en fait un packet. A la base, c'était une structure donc on la caste dans un structure.
        // !!!!! PAS OPTIMISE, declarer le struct a l'exterieur de la boucle
        struct msgUDP *packet_struct = (struct msgUDP *) packet_buf;

        strcpy(payload_buf, packet_struct->payload);
        printf("message : %s\n", payload_buf);

    }
    */
    /* Read datagrams and echo them back to sender */
    /*
    for (;;) {
        peer_addr_len = sizeof(struct sockaddr_storage);
        nread = recvfrom(sockett, buf, BUF_SIZE, 0,
                (struct sockaddr *) &peer_addr, &peer_addr_len);
        if (nread == -1)
            continue;               // Ignore failed request 

        char host[NI_MAXHOST], service[NI_MAXSERV];

        s = getnameinfo((struct sockaddr *) &peer_addr,
                        peer_addr_len, host, NI_MAXHOST,
                        service, NI_MAXSERV, NI_NUMERICSERV);
       if (s == 0)
            printf("Received %ld bytes from %s:%s\n",
                    (long) nread, host, service);
        else
            fprintf(stderr, "getnameinfo: %s\n", gai_strerror(s));

        if (sendto(sockett, buf, nread, 0,
                    (struct sockaddr *) &peer_addr,
                    peer_addr_len) != nread)
            fprintf(stderr, "Error sending response\n");
    }
    */
}