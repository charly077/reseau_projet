/**
*
*header du fichier paquet_creator.c, qui permettra de créer les paquets en partant de l'entrée standart, ou d'un fichier.
*
**/

#ifndef PAQUET_CREATOR_H
#define PAQUET_CREATOR_H


/*
*@pre : prend en argument un descripteur de fichier et un pointeur vers un tableau de char. 
*@post : renvoie 0 si tout vas bien, 1 dans le cas contraire
* 	 Cette fonction remplace *elem par un tableau de char de 512 bytes étant l'élément suivant à envoyer. Dans le cas où un élément à une taille inférieur, il fait 
*	 un padding avec des 0.
*/
int tab_512b(int desc, char **elem);

/*
*@pre : prend le nom d'un fichier
*@post : renvoie le descripteur du fichier
*/
int file_desc(char *filename);



/*
*@pre : le descripteur de l'entrée standart (0) ou le descripteur du fichier utilisé.
*@post : fait pointer le tableau de char *paquet vers un tableau de 520bytes correspondant au paquet suivant, null
*	 si tous les paquets on été envoyé
*/
void create_paquet_from_stdin(int desc, char **paquet);

#endif // fin du header PAQUET_CREATOR_H
