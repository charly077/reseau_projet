#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdint.h>
#include <zlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>

#include "struct.h"
#include "selective_repeat.h"
#include "paquet_creator.h"


#define TIMER 800 // définition du timer pour réenvoyer le premier parquet de la window 


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
	int errr; // compteur d'erreur
	int last_ack_test = 200; //nb impossible 
	struct window *win; // la window

	int select_result; // pour stocker le résultat du select()
	fd_set read_ack; // pour le select()
	struct timeval timeout; // timeout pour le select(), dès qu'il sera expiré, on saura qu'il faut réenvoyer le premier element de la window

	int fini = 0; // variable qui dit que le programme est envoyé, soit lorsque la length d'un paquet est inférieur à 512
	int fini_send = 0; // géré lorsqu'on crée des paquets, si la taille <512bytes alors on sait qu'on ne doit plus créer de paquets

	int fd; // file descriptor du fichier ou de stdin
	int window_size=1; // permet de gérer les changement de taille de window
	int next_seq_num=0;

	//premièrement, il faut un minimum d'arguments
	if(argc<3){
		fprintf(stderr, "Le nombre d'arguments n'est pas suffisant, les arguments obligatoires sont \"./sender addresse/nom port\" \nex: \"./sender ::1 30000\" qui sera le localhost sur le port 30000 et utilisant l'entrée standart\n ");
		exit(EXIT_FAILURE);
	}
	while(i<argc){
		if(strcmp(argv[i], "--file")==0){
			filename = argv[i+1];
			filenamegive = 1;
		}
		else if(strcmp(argv[i], "--sber")==0){
			sber = atoi(argv[i+1]); // si il y a une erreur sber est mis à 0 ....
		}
		else if(strcmp(argv[i], "--splr")==0){
			splr = atoi(argv[i+1]); // mis à 0 en cas d'erreur
		}
		else if(strcmp(argv[i], "--delay")==0){
			delay = atoi(argv[i+1]); // mis à 0 en cas d'erreur
		}
		else if(argv[i][1] == '-'){
			fprintf(stderr,"l'argument %s n'est pas correcte\n les arguments accepté sont [--file nomfichier] [--sber x] [--splr y] [--delay d], x en pourmille, y en pourcent, d en milliseconde",argv[i]);
		}

		i++;
	}


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
		freeaddrinfo(res);
		exit(EXIT_FAILURE);
	}
	
	
	//if filenamegive == 0 then fdread = 0 (soit le stdin)
	if(filenamegive == 1) fd = file_desc(filename);
	else {
		fd = STDIN_FILENO;
	}

	//création de la window:
	create_window(&win, window_size);

	//si il n'y a pas de fichier il faut dire à l'utilisateur qu'on lit sur le stdin
	printf("Vu qu'aucun fichier n'a été spécifier, nous envoyons ce que le fichier reçoit sur l'entrée standard. \nVeuillez entrer ce qu'il faut envoyer:\n");

	//création du timeout pour select:
	timeout.tv_sec = 0;
	timeout.tv_usec = TIMER * 1000; // timer défini en define en ms


	while(fini == 0){
		timeout.tv_sec = 0;
		timeout.tv_usec = TIMER * 1000; // recréation du timer
		//tant qu'il y a de la place dans la window, on envoie ...
		if(fini_send == 0 && win->nb_elem == window_size){ 
			send_window(win,fd, &next_seq_num, &fini_send, sock,addr,sber, splr,delay); // à implémenter ; envoyer un élément si il y en a encore à envoyer		
		}
		else if (fini_send == 0 && (win->nb_elem)!=window_size){
			if(window_size<1) window_resize(win,1);
			else window_resize(win, window_size);
		}
		do{
			FD_ZERO(&read_ack);
			FD_SET(sock,&read_ack);
			select_result = select(sock+1,&read_ack,NULL,NULL,&timeout);
		}while(select_result == -1 || errno == EINTR); // permet d'éviter des erreurs lors de l'exécution de select
	
		if(select_result<=-1){
			fprintf(stderr, "Il y a eu une erreur lors du select: %s\n", strerror(errno));
			free_window(win);
			freeaddrinfo(res);
			close(sock);
			exit(EXIT_FAILURE);
		}	
		else if (select_result == 0){
			//ça signifie que le timer a expiré, il faut donc réenvoyer le premier élément de la liste
			if(win->nb_elem_vide != win->nb_elem){
			int size_sendto = sendto(sock, (*(win->buffer))->msg,sizeof(struct msgUDP),0,addr->ai_addr,addr->ai_addrlen);
			if(size_sendto != sizeof(struct msgUDP)){
				fprintf(stderr, "il y a une erreur lors de l'envoie d'un message après timer select:\n%s\n",strerror(errno));
				free_window(win);
				freeaddrinfo(res);
				close(sock);
				exit(EXIT_FAILURE);
			}
			printf("Suite à l'expiration du timer, le premier paquet de la fenêtre à été réenvoyé\n"); 
			}
			if(errr > 5){
				fprintf(stderr, "Expiration du timer répetitive sans rien dans la window à réenvoyer");
				exit(EXIT_FAILURE);
			}
		}
		else if(select_result>0 && FD_ISSET(sock,&read_ack)) {
			errr =0; // remise à 0 du compteur à erreur ...
			//ça veut dire que j'ai recu un ack
			struct msgUDP *msg = (struct msgUDP *) malloc(sizeof(struct msgUDP));
			int size_recv =  recvfrom(sock, (void *) msg, sizeof(struct msgUDP),0,  addr->ai_addr, &(addr->ai_addrlen));
			if(size_recv != sizeof(struct msgUDP)){
				fprintf(stderr, "erreur lors de la réception d'un ack\n%s\n%lu!=%d\nEn cas d'un connection refused, peut être qu'il n'y a pas de receiver connecté\n",strerror(errno),sizeof(struct msgUDP), size_recv);
				free_window(win);
				freeaddrinfo(res);
				close(sock);				
				exit(EXIT_FAILURE);
			}
			else if(msg->type == PTYPE_ACK){ // uniquement si c'est bien un ack 
				if((int) (msg->window)>32) window_size = 31;
				else if((int) (msg->window)<1){
					window_size = 1;
				}
				else{
					window_size = (int)(msg->window); // attention vérif conversion
				}	
				if(msg->seq_num != last_ack_test){
					ack_recu(msg->seq_num - 1, win);// il faut faire moins 1
				}
				else{
					int ij;
					int ok=0;
					for(ij=0; ij< (win->nb_elem - win->nb_elem_vide); i++){
						if(((*(win->buffer+ij))->msg)->seq_num == last_ack_test){
							int size_sendto = sendto(sock, (*(win->buffer+ij))->msg,sizeof(struct msgUDP),0,addr->ai_addr,addr->ai_addrlen);
							if(size_sendto != sizeof(struct msgUDP)){
								fprintf(stderr, "il y a une erreur lors de l'envoie d'un message après 2 ack similaires \n%s\n",strerror(errno));
								free_window(win);
								freeaddrinfo(res);
								close(sock);
								exit(EXIT_FAILURE);
							}
							ok=1;
							printf("l'élement %d a été réenvoyé après 2 ack similaires\n", ((*(win->buffer+ij))->msg)->seq_num);
							last_ack_test--;
							usleep(10000);//s'arrêter un peu pour éviter de toujours recevoir le mm ack
							break;
						}
					}
					if(ok == 0){
						fprintf(stderr,"il y a eu une erreur lors du réenvoi après un ack, elem 1 win %d elem demandé %d\n",((*(win->buffer+ij))->msg)->seq_num, msg->seq_num);
						exit(EXIT_FAILURE);
					}
				
				}

				last_ack_test = msg->seq_num;
				printf("Un ack a été reçu avec %d comme prochain numéro de séquence attendu \n", msg->seq_num);
				if((win->nb_elem_vide)==(win->nb_elem) && fini_send == 1){
					fini =1; // fin de l'envoie
				}
			}
			free(msg);

		}		
	} // fin boucle pour d'envoi


	freeaddrinfo(res); // libération de addrinfo car  on en a plus besoin après
	//FERMETURE DES DESCRIPTEURS :
	if(filenamegive == 1)
		close(fd); //fermeture du descripteur du fichier
	close(sock);
	free_window(win);
	printf("l'envoie du fichier c'est correctement terminé\n");

}
