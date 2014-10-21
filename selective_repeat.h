/**
*
* header du fichier qui gère les fonctions consernant le selective repeat
*
**/

#ifndef SELECTIVE_REPEAT_H
#define SELECTIVE_REPEAT_H

/*
* structure pour gérer la fenetre
*/
struct window {
	char **buffer; // tableau de pointeurs vers les structures contenant les paquets envoyé ainsi que le moment auquel ils ont été envoyé
	int nb_elem_vide; // pour savoir le nombre d'élément libre dans la fenetre
	pthread_mutex_t *mutex; // mutex pour proteger l'utilisation des informations contenue dans la fenetre
};

/*
* structure pour gérer les paquets dans la fenetre
*/
struct paquet {
	char *paq; //element à envoyer
	int time; // à modifier lors de la création du timer
	int ack; // booléan pour dire si l'élément à été ack (1), 0 sinon
};


/*
*@pre prend la structure windows
*@post renvoie 1 si il y a un element "libre" pour pouvoir décaller la fenetre et faire un nouvel envoie. 0 sinon.
*/
int can_send(struct window *win);


/*
*@pre: reçoit un numéro de sequence
*@post: si le numéro de sequence est dans la liste window, alors on met le ack à 1 pour dire qu'il a été ack
*/
void ack_recu(int n_seq);

/*
*@pre un pointeur vers la structure du paquet à envoyer
*@post si on ne peut pas envoyer d'élem, *paq est mis à null sinon, contient un pointeur vers une structure d'un paquet
*/
void envoyer_paquet(struct paquet **paq);

/*
* @pre : un pointeur vers un pointeur sur la fenetre
* @post : alloue la mémoire pour la fenetre, pour le buffer, et pour le tableau d'int et crée la fenetre avec les bon éléments ainsi qu'initialise le mutex
*/
void create_window(struct window **win);

/*
*@pre pointeur vers la fenetre, supprime la mémoire allouée pour tout le contenu, paquet, buffer, et fenetre
*/
void free_window();

/*
* fonction utilisée sur un thread qui va surveiller l'état de la fenetre, c'est à dire
*	-> parcourir la fenetre toutes les N ms
*	-> si le premier elem est ack -> le supprime et appelle les fonctions pour décaller la fenetre
*	-> si le timer d'un élement est dépassé, le remet à 0 et le reenvoie. (cfr plus tard mais peut-etre faire une liste pour elem à envoyer)
*/
void check_window(); // thread vérifiant l'état de la fenetre

#endif // fin du header SELECTIVE_REPEAT_H
