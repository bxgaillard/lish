
%{
#include <stdio.h>
#include <stdlib.h>
    extern int yylex(); /* from flex output */
    extern void yyerror(char * s); /* see below */
#include "commande.h"

    /* Une liste chaine des blocs alloues pendant l'analyse.
       Si l'analyse est ok, il faut appeler leaker_ok()
       Sinon, il faut appeler leaker_ko()
       Ne pas faire cela entrainerait des fuites de memoire
       La fonction parse_commande de chellelex.lex s'occupe de tout */
    struct genlist {
        void * a;
        struct genlist * n;
    };
    /*
    static int genlist_len(struct genlist *,int);
    static void genlist_arg(struct genlist * l, int * pargc, char *** pargv);
    */
    static void genlist_free(struct genlist * l, int freedata);

    /* Memory leaks avoided */
    static struct genlist * leaks = NULL;
    static void leaker_add(void *);
    extern void leader_ok();
    extern void leaker_ko();
#define LK(a) leaker_add(a)

    Commande * parse_result;

%}

%union {
    Commande       * comval;
    Sequence       * seqval;
    Conditionnelle * cndval;
    Pipeline       * pipval;
    Redirigee      * rcdval;
    Redirection    * redval;
    RedirDesc      * rddval;
    Simple         * smpval;
    Mots           * mtsval;
    char           * motval;
    Seqop            sopval;
    Condop           copval;
    int              intval;
}

%type <comval> commande
%type <seqval> sequence
%type <cndval> conditionnelle
%type <pipval> pipeline
%type <rcdval> redirigee
%type <redval> redirections
%type <redval> redirection
%type <smpval> simple
%type <mtsval> mots
%type <motval> mot

%left <copval> CONDOP
%left <sopval> SEQOP
%left PIPE
%token <intval> OUTTO INFROM APPTO
%token <rddval> REDIRDESC
%token PARO PARF
%token <motval> MOT

%%

commande
: sequence {
    LK( $$ = parse_result = malloc(sizeof(Commande)) );
    $$->sequence = $1;
}
;

sequence
: conditionnelle SEQOP sequence {
    LK( $$ = malloc(sizeof(Sequence)) );
    $$->conditionnelle = $1;
    $$->seqop = $2;
    $$->suiv = $3;
}
| conditionnelle SEQOP {
    LK( $$ = malloc(sizeof(Sequence)) );
    $$->conditionnelle = $1;
    $$->seqop = $2;
    $$->suiv = NULL;
}
| conditionnelle {
    LK( $$ = malloc(sizeof(Sequence)) );
    $$->conditionnelle = $1;
    $$->seqop = SEQ;
    $$->suiv = NULL;
}
;

conditionnelle
: pipeline CONDOP conditionnelle {
    LK( $$ = malloc(sizeof(Conditionnelle)) );
    $$->pipeline = $1;
    $$->suiv = $3;
    /* Hugh */
    $$->condop = NOP;
    $$->suiv->condop = $2;
}
| pipeline {
    LK( $$ = malloc(sizeof(Conditionnelle)) );
    $$->pipeline = $1;
    $$->condop = NOP;
    $$->suiv = NULL;
}
;

pipeline
: redirigee PIPE pipeline {
    LK( $$ = malloc(sizeof(Pipeline)) );
    $$->redirigee = $1;
    $$->suiv = $3;
}
| redirigee {
    LK( $$ = malloc(sizeof(Pipeline)) );
    $$->redirigee = $1;
    $$->suiv = NULL;
}
;

redirigee
: simple redirections {
    LK( $$ = malloc(sizeof(Redirigee)) );
    $$->simple = $1;
    $$->redirection = $2;
}
;

redirections
: redirection redirections {
    $1->suiv = $2;
    $$ = $1;
}
| /* rien */ {
    $$ = NULL;
}
;

redirection
: INFROM mot {
    LK( $$ = malloc(sizeof(Redirection)) );
    $$->type = FICHIER;
    LK( $$->u.redirfichier = malloc(sizeof(RedirFichier)) );
    $$->u.redirfichier->type = IN;
    $$->u.redirfichier->desc = $1;
    $$->u.redirfichier->fichier = $2;
    $$->suiv = NULL;
}
| OUTTO mot {
    LK( $$ = malloc(sizeof(Redirection)) );
    $$->type = FICHIER;
    LK( $$->u.redirfichier = malloc(sizeof(RedirFichier)) );
    $$->u.redirfichier->type = OUT;
    $$->u.redirfichier->desc = $1;
    $$->u.redirfichier->fichier = $2;
    $$->suiv = NULL;
}
| APPTO mot {
    LK( $$ = malloc(sizeof(Redirection)) );
    $$->type = FICHIER;
    LK( $$->u.redirfichier = malloc(sizeof(RedirFichier)) );
    $$->u.redirfichier->type = APP;
    $$->u.redirfichier->desc = $1;
    $$->u.redirfichier->fichier = $2;
    $$->suiv = NULL;
}
| REDIRDESC {
    LK( $$ = malloc(sizeof(Redirection)) );
    $$->type = DESCRIPTEUR;
    $$->u.redirdesc = $1;
    $$->suiv = NULL;
}
;

simple
: mots {
    LK( $$ = malloc(sizeof(Simple)) );
    $$->type = SIMPLE;
    $$->u.mots = $1;
}
| PARO commande PARF {
    LK( $$ = malloc(sizeof(Simple)) );
    $$->type = SUBSHELL;
    $$->u.commande = $2;
}
;

mots
: mot mots {
    LK( $$ = malloc(sizeof(Mots)) );
    $$->mot = $1;
    $$->suiv = $2;
}
| mot {
    LK( $$ = malloc(sizeof(Mots)) );
    $$->mot = $1;
    $$->suiv = NULL;
}
;

mot:
MOT { LK( $$ = $1 ); }
;

%%

void yyerror(char * s)
{
    s = s; /* shut up, stupid compiler */
    /*fprintf(stderr,"%s\n",s);*/
}

static void genlist_free(struct genlist * l, int freedata)
{
    struct genlist * curr = l;
    while ( curr != NULL )
    {
        struct genlist * thisone = curr;
        curr = curr->n;
        if ( freedata )
            free(thisone->a);
        free(thisone);
    }
    leaks = NULL;
}

/* Useless */
/*
static int genlist_len(struct genlist * l, int acc)
{
    if ( l == NULL )
        return acc;
    else
        return genlist_len(l->n,acc+1);
}
static void genlist_arg(struct genlist * l, int * pargc, char *** pargv)
{
    int n = genlist_len(l,0);
    struct genlist * currp;
    int curri;
    *pargc = n;
    *pargv = calloc(n,sizeof(char *));
    currp = l;
    curri = 0;
    while ( currp != NULL )
    {
        (*pargv)[curri] = currp->a;
        currp = currp->n;
        ++curri;
    }
    (*pargv)[curri] = NULL;
}
*/
/* End useless */

static void leaker_add(void * a)
{
    struct genlist * l = malloc(sizeof(struct genlist));
    l->a = a;
    l->n = leaks;
    leaks = l;
}
void leaker_ok()
{
    genlist_free(leaks,0);
}
void leaker_ko()
{
    genlist_free(leaks,1);
}
