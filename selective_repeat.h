/**
*
* header du fichier qui gère les fonctions consernant le selective repeat
*
**/

#ifndef SELECTIVE_REPEAT_H
#define SELECTIVE_REPEAT_H


/*
*@pre prend la structure windows
*@post renvoie 1 si il y a un element "libre" pour pouvoir décaller la fenetre et faire un nouvel envoie. 0 sinon.
*/
int can_send(struct window *win);


/*
*@pre: reçoit un numéro de sequence, la window
*@post: si le numéro de sequence est dans la liste window, alors on met le ack à 1 pour dire qu'il a été ack
*/
void ack_recu(int n_seq, struct window *win);

/*
*@pre un pointeur vers la structure du paquet à envoyer, et un file descriptor, numéro
* 	de séquence du dernier paquet envoyé, enfin un pointeur vers l'element à mettre
*	à jour si on le fichier est terminé, un int représentant le socket_fd, 
*@post si on ne peut pas envoyer d'élem, *paq est mis à null sinon, contient un pointeur vers une structure d'un paquet
*/
void send_window(struct window *win, int fd, int last_seq_num, int *fini_send,int sock, struct addrinfo *addr );

/*
* @pre : un pointeur vers un pointeur sur la fenetre, et la taille du buffer
* @post : alloue la mémoire pour la fenetre, pour le buffer, et pour le tableau d'int et crée la fenetre avec les bon éléments ainsi qu'initialise le mutex
*/
void create_window(struct window **win,int buffer_size);

/*
*@pre pointeur vers la fenetre, supprime la mémoire allouée pour tout le contenu, paquet, buffer, et fenetre
*/
void free_window(struct window *win);

/*
*@pre, un pointeur vers la window et la taille de la nouvelle window
*@post: change la taille de la window
*/
void window_resize(struct window *win, int buffer_size);

#endif // fin du header SELECTIVE_REPEAT_H
