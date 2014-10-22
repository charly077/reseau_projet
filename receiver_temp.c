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
//#define EXIT_FAILURE 1

//création d'un structure de donnée pour envoyer les messages:
typedef struct msgUDP{
	uint8_t Type : 3;
	uint8_t Window : 5;
	uint8_t Seq_num;
	uint16_t Length;
	char payload[512]; //je ne sais pas comment implémenter le payload de 512bytes dans la structure
	int crc32;         //j'ai pris un int parce qu'il a la bonne dimension, c'est a dire 4 bits ... mais normalement uLong crc32 -> ouai, bizarre...
}__attribute__((packed)) msgUDP;



// MAIN --------------------------------------------------------
int main(int argc, char *argv[])
{
	char *filename;
	int port;
	char *port_to_string;
	char *hostname;

	// On vérifie le nombre d'argument
	if (argc != 3 && argc != 5)
	{
		printf("Problème, nombre d'argument incorrect %d\n", argc);
		return 1;
	}
	// argv : 0 = nom de l'app, 1 = "--file", 2 = filename, 3 = hostname, 4 = port
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

	// On crée les variables d'adresses
	//struct sockaddr_in addr_perso;      /* notre address */
    //struct sockaddr_in addr_sender;     /* address du client */
    //socklen_t addrlen = sizeof(addr_sender); // longueur de l'adresse du client

    struct addrinfo *liste_obtenue;		/* tableau des infos concernant le sender */
	struct addrinfo *adresse_sender;   /* pour pouvoir parcourir les éléments de la liste obtenue */
	struct addrinfo adresse_pour_struct;			/* permet de définir le type de connexion voulue */

 	// Obtention des infos sur le receveur
 	memset(&adresse_pour_struct, 0, sizeof(struct addrinfo)); // Met des 0 sur tout le buffer pour être sûr qu'il n'envoie pas de 1
	adresse_pour_struct.ai_family = AF_INET6; // on veut de l'IPv6
	adresse_pour_struct.ai_socktype =  SOCK_DGRAM; //SOCK_RAW;
	adresse_pour_struct.ai_flags = AI_PASSIVE;
	adresse_pour_struct.ai_protocol = 0; // IPPROTO_UDP; // peut importe le protocol

	int s; // Variable d'erreur
	if ((s = getaddrinfo(NULL, port_to_string, &adresse_pour_struct, &liste_obtenue)) != 0) // Nous permet d'avoir une liste de structure addrinfo qui concernent le client
	{
		fprintf(stderr, "getaddrinfo: %s\n",gai_strerror(s));
		exit(EXIT_FAILURE);
	}

	// Vérifier que notre hôte existe bien, mais on a connecté le packet à une adresse !!! On n'est pas entré dans le protocole TCP pour autant !
	int j = 1;
	int sockett;
	for (adresse_sender = liste_obtenue; adresse_sender != NULL; adresse_sender = adresse_sender->ai_next) {
		printf("%d\n", j);
		j++;
        sockett = socket(adresse_sender->ai_family, adresse_sender->ai_socktype,adresse_sender->ai_protocol);
        if (sockett == -1)
        	printf("Le socket vaut -1, on continue\n");
            continue;

        if (bind(sockett, adresse_sender->ai_addr, adresse_sender->ai_addrlen) != 0)
        {
            printf("Problème lors du bind\n");                  /* Success */
        }

        close(sockett);
    }

    if (adresse_sender == NULL) {               /* No address succeeded */
        fprintf(stderr, "Aucune adresse IP valide dans la liste retournée\n");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(liste_obtenue);           /* On en a plus besoin, on a trouvé une adresse IP fixe et qui fonctionne, adresse_sender */



	// ------------------------------------------------------------------
	// Programme proprement dit :::
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

}


