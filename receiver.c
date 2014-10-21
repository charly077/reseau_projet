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

//   1) Problème lors de la compile des crc32 ...
//   2) Mettre les packets d'IPv6 ?
//   3) Pq pas mettre SOCK_RAW ?
//   4) Comment faire pour les numéros de fenêtres / séquence ? 

//
//			!!! METTRE LES STRUCTURES EN IPv6 !!!
//

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
	struct sockaddr_in addr_perso;      /* notre address */
    struct sockaddr_in addr_sender;     /* address du client */
    socklen_t addrlen = sizeof(addr_sender); // longueur de l'adresse du client

    struct addrinfo *liste_obtenue;		/* tableau des infos concernant le sender */
	struct addrinfo *adresse_sender;   /* pour pouvoir parcourir les éléments de la liste obtenue */
	struct addrinfo template;			/* permet de définir le type de connexion voulue */

    //int socket;

 	// Obtention des infos sur le receveur
 	memset(&template, 0, sizeof(struct addrinfo)); // Met des 0 sur tout le buffer pour être sûr qu'il n'envoie pas de 1
	template.ai_family = AF_INET6; // on veut de l'IPv6
	template.ai_socktype =  SOCK_RAW; //SOCK_DGRAM;
	template.ai_flags=0;
	template.ai_protocol = 0; // IPPROTO_UDP; // 0; // peut importe le protocol

	int s; // Variable d'erreur
	if ((s = getaddrinfo(hostname, port_to_string, &template, &liste_obtenue)) != 0) // Nous permet d'avoir une liste de structure addrinfo qui concernent le client
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
    char packet_buf[PACKET_SIZE]; 			// buffer de réception du packet
    char payload_buf[PAYLOAD_SIZE];    		// buffer de réception du payload
    char calculerCRC_buf[WITHOUT_CRC_SIZE]; // Stocker la partie sur laquelle doit être calculée le CRC

    char a[31][PACKET_SIZE]; 				// Largeur de fenêtre totale.
    int minimum = 0;    					// Minimum là ou on peut écrire
    int maximum = 15;						// Maximum là ou on peut écrire

    // Ouvrir le fichier
	FILE *fichier = fopen(filename, "w");
	if (fichier == NULL)
	{
    	printf("Error opening file!\n");
    	exit(1);
	}

    while(length_receiv == PACKET_SIZE) 
    {
        // On reçoit un packet
        length_receiv = recvfrom(sockett, packet_buf, PACKET_SIZE, 0, (struct sockaddr *) &addr_sender, &addrlen); // !!!! ON A LES INFOS DU SENDER QUI SE METTE DANS addr_sender
        if (length_receiv == -1) {
            printf("Problème, le message reçu est trop court...\n" );               /* Ignore failed request */
        }
		
		// On a reçu un buffer qui est en fait un packet. A la base, c'était une structure donc on la caste dans un structure.
        struct msgUDP *packet_struct = (struct msgUDP *) packet_buf;

        // Vérification du CRC et de si le type est bien 1

        uLong crc = crc32(0L, Z_NULL, 0); 											// INT !!!!!!!!!
        memcpy(calculerCRC_buf, packet_buf, strlen(packet_buf) - 4); 				// Je crée un buffer surlequel je vais pouvoir calculer le CRC
   		crc = crc32(crc, calculerCRC_buf, strlen(calculerCRC_buf));
		
		if (crc != packet_struct->crc32 || packet_struct->Type != PTYPE_DATA)
		{
			// discard packet
			printf("Les 2 CRC ne correspondent pas, le packet doit être discardé \n");
			printf("Ou alors, le message reçu n'est pas un message de type DATA (le Type n'est pas 1) \n");
		}
		else
		{        
	        // Vérifie qu'on est dans les bons numéros de séquences....
	        if(packet_struct->Seq_num > minimum && packet_struct->Seq_num < maximum) // Hein ? Je fais comment ? C'est lié normalement la taille de la fenêtre et le numéro de séquence....
	        {
	        	// Le numéro de packet est ok, je peux le sauver dans une des fenêtres !
	        	// Mettre dans un buffer temporaire qui est dans une fenêtre
			strcpy(payload_buf, packet_struct->payload);        
			//payload_buf = packet_struct->payload;


		    // Mettre à jour le lastack



		    // Ecriture dans le fichier	
		    fwrite(payload_buf, 1, sizeof(payload_buf), fichier);	
			printf("Nouveau message reçu et sauvé : %s\n", payload_buf);


			// Envoyer ACK

	        }
	        
		}
       
    }    

    fclose(fichier);
}



/*
// Doit etre remplacé par le sockaddr_in6 !!!!!!!!!
struct in_addr {
   in_addr_t	s_addr;
};

// Doit etre remplacé par le sockaddr_in6 !!!!!!!!!
struct sockaddr_in {
   uint8_t         sin_len;       // longueur totale     
   sa_family_t     sin_family;    // famille : AF_INET    
   in_port_t       sin_port;      // le numéro de port    
   struct in_addr  sin_addr;      // l'adresse internet   
   unsigned char   sin_zero[8];   // un champ de 8 zéros  
};



struct addrinfo {
    int              ai_flags;
    int              ai_family;
    int              ai_socktype;
    int              ai_protocol;
    size_t           ai_addrlen;
    struct sockaddr *ai_addr;
    char            *ai_canonname;
    struct addrinfo *ai_next;
};


struct in6_addr {
    unsigned char   s6_addr[16];   // load with inet_pton()
};


struct sockaddr_in6 {
    u_int16_t       sin6_family;   // address family, AF_INET6
    u_int16_t       sin6_port;     // port number, Network Byte Order
    u_int32_t       sin6_flowinfo; // IPv6 flow information
    struct in6_addr sin6_addr;     // IPv6 address
    u_int32_t       sin6_scope_id; // Scope ID
};

*/
/*
struct sockaddr {
   unsigned char   sa_len;         // longueur totale 		
   sa_family_t     sa_family;      // famille d'adresse 	
   char            sa_data[14];    // valeur de l'adresse	
};
*/
