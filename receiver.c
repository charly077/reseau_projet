#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h> // Pour la gestion des sockets
#include <stdio.h>	// for fprintf 
#include <string.h>	// for memcpy
#include <zlib.h> // Pour le CRC

#define PACKET_SIZE 520
#define BUFFER_SIZE 512

//création d'un structure de donnée pour envoyer les messages:
typedef struct msgUDP{
	uint8_t Type : 3;
	uint8_t Window : 5;
	uint8_t Seq_num;
	uint16_t Length;
	char payload[512]; //je ne sais pas comment implémenter le payload de 512bytes dans la structure
	int crc32;         //j'ai pris un int parce qu'il a la bonne dimension, c'est a dire 4 bits ... mais normalement uLong crc32
}__attribute__((packed)) msgUDP;

int main(int argc, char *argv[])
{
	char *filename;
	int port;
	char *hostname

	// On vérifie le nombre d'argument
	if (argc != 3 || argc != 5)
	{
		printf("Problème, nombre d'argument incorrect\n");
		return 1;
	}
	// argv : 0 = nom de l'app, 1 = "--file", 2 = filename, 3 = hostname, 4 = port
	if (argc == 5)
	{
		// copier le filename
		filename = argv[2];
		hostname = argv[3];
		port = atoi(argv[4]);
	}
	else 
	{
		// prendre le filename sur le stdin
		printf("Veuillez entrez le nom de l'hote ou l'adresse IPv6\n");
		scanf("%s", &filename);			// Comment on fait pour l'extension ???
		hostname = argv[1];
		port = atoi(argv[2]);
	}

	printf("Nom du programme : %s , nom du fichier : %s , nom de l'hote attendu : %s et numéro de port : %d\n", argv[0], filename, hostname, port);

	// On crée les variables d'adresses
	struct sockaddr_in addr_perso;      /* notre address */
    struct sockaddr_in addr_sender;     /* address du client */
    socklen_t addrlen = sizeof(addr_sender); // longueur de l'adresse du client

    struct addrinfo *liste_obtenue;		/* tableau des infos concernant le sender */
	struct addrinfo *adresse_sender;   /* pour pouvoir parcourir les éléments de la liste obtenue */
	struct addrinfo template;			/* permet de définir le type de connexion voulue */

    int socket;

 	// Obtention des infos sur le receveur
 	memset(&template, 0, sizeof(struct addrinfo)); // Met des 0 sur tout le buffer pour être sûr qu'il n'envoie pas de 1
	template.ai_family = AF_INET6; // on veut de l'IPv6
	template.ai_socktype =  SOCK_RAW; //SOCK_DGRAM;
	template.ai_flags=0;
	template.ai_protocol = IPPROTO_UDP; // 0; // peut importe le protocol

	int s; // Variable d'erreur
	if ((s = getaddrinfo(hostname, port, &template, &liste_obtenue)) != 0) // Nous permet d'avoir une liste de structure addrinfo qui concernent le client
	{
		fprintf(stderr, "getaddrinfo: %s\n",gai_strerror(s));
		exit(EXIT_FAILURE);
	}

	// Vérifier que notre hôte existe bien, mais on a connecté le packet à une adresse !!! On n'est pas entré dans le protocole TCP pour autant !
	int sockett;
	for (adresse_sender = liste_obtenue; adresse_sender != NULL; adresse_sender = adresse_sender->ai_next) {
        sockett = socket(adresse_sender->ai_family, adresse_sender->ai_socktype,adresse_sender->ai_protocol);
        if (sockett == -1)
            continue;

        if (bind(sockett, adresse_sender->ai_addr, adresse_sender->ai_addrlen) == 0)
            break;                  /* Success */

        close(sockett);
    }

    if (adresse_sender == NULL) {               /* No address succeeded */
        fprintf(stderr, "Aucune adresse IP valide dans la liste retournée\n");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(liste_obtenue);           /* On en a plus besoin, on a trouvé une adresse IP fixe et qui fonctionne, adresse_sender */



	// ------------------------------------------------------------------
	/* Programme proprement dit ::: */
    int length_receiv;
    unsigned char temp_buf[BUFFER_SIZE];     /* buffer de réception */

    // Ouvrir le fichier
	FILE *fichier = fopen(filename, "w");
	if (fichier == NULL)
	{
    	printf("Error opening file!\n");
    	exit(1);
	}

    while(length_receiv == PACKET_SIZE) 
    {
        length_receiv = recvfrom(sockett, temp_buf, PACKET_SIZE, 0, (struct sockaddr *) &addr_sender, &addrlen); // !!!! ON A LES INFOS DU SENDER QUI SE METTE DANS addr_sender
        if (length_receiv == -1) {
            printf("Problème, le message reçu est trop court...\n", );               /* Ignore failed request */
        }

        // Vérification du CRC


        // Vérification de si le type est bien 1


        // Mettre dans un buffer temporaire qui est dans une fenêtre


        // Mettre à jour le lastack


        // Ecriture dans le fichier		
		fprintf(fichier, "Nouveau message reçu : %s\n", temp_buf);


		// Envoyer ACK


       
    }    

    fclose(fichier);
}
