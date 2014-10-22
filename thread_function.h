/**
*
* Header pour tout ce qui est la gestions des threads
*
**/

#ifndef THREAD_FUNCTION_H
#define THREAD_FUNCTION_H


/*
* fonction pour gérer l'envoie des paquets, tant qu'il y a des elem libre dans paquet_to_send, il l'envoie
* et supprime le pointeur
*/
void *thread_envoie(void *arg);

/*
* attend les ack, et réagit lance la fonction ack_recu de window.h
*/
void *thread_recoit_ack(void *arg);

/*
* fonction utilisée sur un thread qui va surveiller l'état de la fenetre, c'est à dire
*	-> parcourir la fenetre toutes les N ms
*	-> si le premier elem est ack -> le supprime et appelle les fonctions pour décaller la fenetre
*	-> si le timer d'un élement est dépassé, le remet à 0 et le reenvoie. (cfr plus tard mais peut-etre faire une liste pour elem à envoyer)
*/
void *check_window(void *arg); // thread vérifiant l'état de la fenetre

#endif // fin du header THREAD_FUNCTION_H
