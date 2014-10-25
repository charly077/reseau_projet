#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>


#include "struct.h"
#include "paquet_creator.h"
#include "selective_repeat.h"



int can_send(struct window *win){
	return win->nb_elem_vide > 0;
}

void ack_recu(int n_seq, struct window *win){
	//parcourir toute la window, pour voir si le paquet est dedans
	int i;
	struct paquet *paq;
	for(i=0; i< ((win->nb_elem)-(win->nb_elem_vide)); i++){
		paq = *((win->buffer)+i);
		if ((paq->msg)->seq_num <= n_seq){
			paq->ack =1;
			printf("le paquet avec le numéro de séquence %d à été ack\n", (paq->msg)->seq_num);
		}
		else{
			break; //si le n° de seq est plus grand alors ça sert à rien de continuer
		}
	}
	//déplacer la fenetre, si le premier élément est ack, on déplace la fenetre
	while((*(win->buffer))->ack == 1){
		win->nb_elem_vide ++;
		free((*(win->buffer))->msg); // libere l'espace du msgUDP
		struct paquet *paq_tofree = *(win->buffer);
		free(paq_tofree);
		(*(win->buffer))->ack=0;//pour s'assurer de ne pas boucler car l'element n'est pas suprimer de la mémoire si il n'y a rien à copier dessus
		int j;
		for(j=0; j< ((win->nb_elem)-(win->nb_elem_vide)) && j< ((win->nb_elem)-1);j++){
			*(win->buffer + j) = *(win->buffer+j+1);
		}
		//struct paquet ** buff2 = (struct paquet **) malloc((win->nb_elem -1 )*sizeof(struct paquet *));
		//strncpy((void *)buff2, (void *)((win->buffer)+1),((win->nb_elem)-1) * sizeof(struct paquet *));
		//strncpy((void *)(win->buffer),(void *)(buff2), ((win->nb_elem)-1) * sizeof(struct paquet *));
		//free(buff2);
		printf("la fenetre à été déplacée\n");
		
	}
}


void send_window(struct window *win, int fd, int *next_seq_num,int *fini_send,int sock, struct addrinfo *addr,int sber, int splr){
	if(can_send(win) == 0 && *fini_send == 1){
		return;
	}
	else{
		int i = 0;
		 while(win->nb_elem_vide >0 && *fini_send !=1){
		 	struct msgUDP *msg;
			create_paquet(fd,*next_seq_num ,&msg,fini_send);
			struct paquet *paq = (struct paquet *) malloc(sizeof(struct paquet));
			paq->ack = 0;
			paq->msg = msg;
			*((win->buffer)+(win->nb_elem)-(win->nb_elem_vide)) = paq;
			int err_bit = (random()%1000 < sber);
			if(err_bit){
				(paq->msg)->payload[0] ^=0xff; // si on doit créer une erreur on inverse le premier bit
			}
			// envoie du paquet
			if(random()%100 > splr)
			{
				int nb = sendto(sock, msg, sizeof(struct msgUDP), 0, addr->ai_addr,addr->ai_addrlen);
				if(nb != sizeof(struct msgUDP)){
					fprintf(stderr, "il y a eu une erreur lors de l'envoie d'un messageUDP \n%s\ntaille msgUDP = %lu, taille envoyé = %d\n", strerror(errno), sizeof(struct msgUDP), nb);
				}
				printf("\nle crc du paquet avec le n° %d est de %d, de plus le paquet est de type %d et de taille %d\n",msg->seq_num, msg->crc32,msg->type,msg->length);
			}
			if(err_bit){
				(paq->msg)->payload[0] ^=0xff; // si une erreur a été crée on reinverse le premier bit
			}
			printf("le paquet avec le numéro de séquence %d à été envoyé\n", *next_seq_num);
			win->nb_elem_vide --;
			(*next_seq_num) = (*next_seq_num)+1 % 255;
		 }
	}
}


void create_window(struct window **win, int buffer_size){
	*win = (struct window *) malloc(sizeof(struct window));
	(*win)->buffer = (struct paquet **) malloc(sizeof(struct paquet *) * buffer_size);
	(*win)->nb_elem_vide = buffer_size;
	(*win)->nb_elem = buffer_size;
	printf("La window à été crée avec une taille initiale de %d\n",buffer_size);
}

void window_resize(struct window *win, int buffer_size){
	if(((win->nb_elem)-(win->nb_elem_vide)) <= buffer_size){ // si ces condition il y a déjà plus d element envoyé que la nouvelle taille, on attend ...
		struct paquet **new_buff = (struct paquet **) malloc(sizeof(struct paquet *));
		strncpy((void *)new_buff, (void *)win->buffer, sizeof(struct paquet *) * ((win->nb_elem)-(win->nb_elem_vide)));
		free(win->buffer);
		(win->buffer) = new_buff;
		win->nb_elem_vide = buffer_size - ((win->nb_elem)-(win->nb_elem_vide));
		win->nb_elem = buffer_size;
	}
	printf("La taille de la window à été changée, elle est maintenant de %d\n", buffer_size);
}

void free_window(struct window *win){
	printf("Suppression des structures paquets contenu dans la fenêtre\n");
	int i;
	int nb_elem_sup = (win->nb_elem) - (win->nb_elem_vide);
	for(i=0; i<nb_elem_sup;i++){
		free((*(win->buffer))->msg); //suppression de la mémoire allouée pour la struct msgUDP
		free(*(win)->buffer); // suppression de la mémoire allouée pour le paquet
	}
	//free(win->buffer);//suppression de la mémoire allouée pour le buffer
	free(win); // suppression de la mémoire allouée pour la windows
	printf("la mémoire alloué à la fenêtre et à ses sous-éléments à été libérée\n");
}
