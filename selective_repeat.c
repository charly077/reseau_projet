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
	for(i=0; i< win->nb_elem; i++){
		paq = win->buffer +i;
		if ((paq->msg)->seq_num == n_seq){
			paq->ack =1;
			break; 
		}
	}
	//déplacer la fenetre, si le premier élément est ack, on déplace la fenetre
	while((win->buffer)->ack == 1){
		win->nb_elem_vide ++;
		free((win->buffer)->msg); // libere l'espace du msgUDP
		free(*(win->buffer)); // le but est de libéré la mémoire, win->buffer me renvoie un tableau de pointeur, je prends le premier element pour le free (pas sur)
		strncpy((win->buffer),(win->buffer)+1, ((win->nb_elem)-1) * sizeof(struct paquet *));
		
	}
}


void send_window(struct window *win, int fd, int last_seq_num,int *fini_send,int sock, struct addrinfo *addr){
	if(can_send(win) == 0){
		return;
	}
	else{
		int i = 1;
		 while(win->nb_elem_vide >0){
		 	struct msgUDP *msg;
			create_paquet(fd,last_seq_num + i ,&msg,fini_send);
			struct paquet *paq = (struct paquet *) malloc(sizeof(struct paquet));
			paq->ack = 0;
			paq->msg = msg;
			((win->buffer)+(win->nb_elem)-(win->nb_elem_vide)) = paq;
			// envoie du paquet
			int nb = sendto(sock, msg, sizeof(struct msgUDP), 0, addr->ai_addr,addr->ai_addrlen);
			if(nb != sizeof(msgUDP)){
				fprintf(stderr, "il y a eu une erreur lors de l'envoie d'un messageUDP \n%s\n", strerror(errno));
			}
			win->nb_elem_vide --;
			i++;
		 }
	}
}


void create_window(struct window **win, int buffer_size){
	*win = (struct window *) malloc(sizeof(struct window));
	(*win)->buffer = (struct paquet *) malloc(sizeof(struct paquet *) * buffer_size);
	(*win)->nb_elem_vide = buffer_size;
	(*win)->nb_elem = buffer_size;
}

void window_resize(struct window *win, int buffer_size){
	if(((win->nb_elem)-(win->nb_elem_vide)) <= buffer_size){ // si ces condition il y a déjà plus d element envoyé que la nouvelle taille, on attend ...
		struct paquet *new_buff = (struct paquet *) malloc(sizeof(struct paquet *));
		strncpy((void *)new_buff, (void *)win->buffer, sizeof(struct paquet *) * ((win->nb_elem)-(win->nb_elem_vide)));
		free(win->buffer);
		(win->buffer) = new_buff;
		win->nb_elem_vide = buffer_size - ((win->nb_elem)-(win->nb_elem_vide));
		win->nb_elem = buffer_size;
	}
}

void free_window(struct window *win){
	free(win->buffer);
	free(win);
}
