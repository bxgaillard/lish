/* Modifié par Benjamin Gaillard pour éliminer les avertissements du
 * compilateur et faire un fichier plus concis.
 * Remarque : aucun nom n'a changé, seule les déclarations sont différentes*/

#ifndef _COMMANDE_H_
#define _COMMANDE_H_

#include <stdio.h>

typedef struct mots {
    char        *mot;
    struct mots *suiv;
} Mots;

typedef struct simple {
    enum { SIMPLE, SUBSHELL } type;
    union {
	struct mots     *mots;
	struct commande *commande;
    } u;
} Simple;

typedef struct redirection {
    enum { FICHIER, DESCRIPTEUR } type;
    union {
	struct redirfichier *redirfichier;
	struct redirdesc    *redirdesc;
    } u;
    struct redirection *suiv;
} Redirection;

typedef struct redirfichier {
    enum { IN, OUT, APP } type;
    int   desc;
    char *fichier;
} RedirFichier;

typedef struct redirdesc {
    enum { DUP, CLOSE, DUPCLOSE } type;
    int src;
    int dst;
    enum { READ, WRITE } mode;
} RedirDesc;

typedef struct redirigee {
    struct simple      *simple;
    struct redirection *redirection;
} Redirigee;

typedef struct pipeline {
    struct redirigee *redirigee;
    struct pipeline  *suiv;
} Pipeline;

typedef enum condop { NOP, AND, OR } Condop;
typedef struct conditionnelle {
    struct pipeline       *pipeline;
    enum condop            condop;
    struct conditionnelle *suiv;
} Conditionnelle;

typedef enum seqop { SEQ, BACK } Seqop;
typedef struct sequence {
    struct conditionnelle *conditionnelle;
    enum   seqop           seqop;
    struct sequence       *suiv;
} Sequence;

typedef struct commande {
    struct sequence *sequence;
} Commande;

extern Commande *parse_commande(char *);
extern void      dump_commande(Commande *, FILE *);
extern void      free_commande(Commande *);

#endif /* !_COMMANDE_H_ */
