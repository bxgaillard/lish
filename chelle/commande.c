
#include <stdlib.h>
#include "commande.h"

/************************************************************
 *
 * FREE stuff
 *
 ************************************************************/
static void free_mots_liste(Mots * m)
{
    if ( m != NULL )
    {
        free(m->mot);
        free_mots_liste(m->suiv);
        free(m);
    }
}
static void free_redirection_list(Redirection * r)
{
    if ( r != NULL )
    {
        if ( r->type == FICHIER )
        {
            free(r->u.redirfichier->fichier);
            free(r->u.redirfichier);
        }
        else
            free(r->u.redirdesc);
        free_redirection_list(r->suiv);
        free(r);
    }
}
static void free_simple(Simple * s)
{
    if ( s->type == SIMPLE )
        free_mots_liste(s->u.mots);
    else
        free_commande(s->u.commande);
    free(s);
}
static void free_redirigee(Redirigee * r)
{
    free_simple(r->simple);
    free_redirection_list(r->redirection);
    free(r);
}
static void free_pipeline_list(Pipeline * p)
{
    if ( p != NULL )
    {
        free_pipeline_list(p->suiv);
        free_redirigee(p->redirigee);
        free(p);
    }
}
static void free_conditionnelle_list(Conditionnelle * c)
{
    if ( c != NULL )
    {
        free_conditionnelle_list(c->suiv);
        free_pipeline_list(c->pipeline);
        free(c);
    }
}
static void free_sequence_list(Sequence * s)
{
    if ( s != NULL )
    {
        free_sequence_list(s->suiv);
        free_conditionnelle_list(s->conditionnelle);
        free(s);
    }
}
void free_commande(Commande * c)
{
    free_sequence_list(c->sequence);
    free(c);
}


/************************************************************
 *
 * DUMP stuff
 *
 ************************************************************/

static void dump_commande_aux(Commande *, FILE *, int);

static void dump_prefix(FILE * f, int depth)
{
    int i;
    for ( i=0 ; i<depth ; i++ )
        fprintf(f,"|   ");
}
static void dump_mots(Mots * m, FILE * f, int n, int depth)
{
    if ( m != NULL )
    {
        if ( n == 0 )
            dump_prefix(f,depth);
        else
            fprintf(f," ");
        fprintf(f,"[%s]",m->mot);
        dump_mots(m->suiv,f,n+1,depth);
    }
}
static void dump_simple(Simple * s, FILE * f, int depth)
{
    dump_prefix(f,depth);
    fprintf(f,"SIMPLE (type=%s)\n",s->type==SIMPLE?"SIMPLE":"SOUS-SHELL");
    if ( s->type == SIMPLE )
    {
        dump_mots(s->u.mots,f,0,depth+1);
        fprintf(f,"\n");
    }
    else
    {
        dump_commande_aux(s->u.commande,f,depth+1);
    }
}
static void dump_redirection_fichier(RedirFichier * r, FILE * f)
{
    fprintf(f,"%d",r->desc);
    switch ( r->type )
    {
        case IN:  fprintf(f,"<"); break;
        case OUT: fprintf(f,">"); break;
        case APP: fprintf(f,">>"); break;
    }
    fprintf(f,"%s\n",r->fichier);
}
static void dump_redirection_desc(RedirDesc * r, FILE * f)
{
    fprintf(f,"%d",r->dst);
    switch ( r->mode )
    {
        case READ: fprintf(f,"<&"); break;
        case WRITE: fprintf(f,">&"); break;
    }
    if ( r->type == DUP || r->type == DUPCLOSE )
        fprintf(f,"%d",r->src);
    if ( r->type == CLOSE || r->type == DUPCLOSE )
        fprintf(f,"-");
    fprintf(f,"\n");
}
static void dump_redirection(Redirection * r, FILE * f, int depth)
{
    if ( r != NULL )
    {
        dump_prefix(f,depth);
        switch ( r->type )
        {
            case FICHIER:
                dump_redirection_fichier(r->u.redirfichier,f);
                break;
            case DESCRIPTEUR:
                dump_redirection_desc(r->u.redirdesc,f);
                break;
        }
        dump_redirection(r->suiv,f,depth);
    }
}
static void dump_redirigee(Redirigee * r, FILE * f, int depth)
{
    dump_prefix(f,depth);
    fprintf(f,"COMMANDE REDIRIGEE\n");
    dump_simple(r->simple,f,depth+1);
    if ( r->redirection != NULL )
    {
        dump_prefix(f,depth+1);
        fprintf(f,"REDIRECTIONS\n");
        dump_redirection(r->redirection,f,depth+2);
    }
}
static void dump_pipeline(Pipeline * p, FILE * f, int n, int depth)
{
    if ( p != NULL )
    {
        dump_prefix(f,depth);
        fprintf(f,"PIPELINE ELEMENT %d\n",n);
        dump_redirigee(p->redirigee,f,depth+1);
        dump_pipeline(p->suiv,f,n+1,depth);
    }
}
static char * dump_condop(Condop o)
{
    switch ( o )
    {
        case AND: return "AND";
        case OR:  return "OR";
        case NOP: return "NOP";
    }
    return NULL;
}
static void dump_conditionnelle(Conditionnelle * c, FILE * f, int n, int depth)
{
    if ( c != NULL )
    {
        dump_prefix(f,depth);
        fprintf(f,"CONDITIONNELLE %d (op=%s)\n",n,dump_condop(c->condop));
        dump_pipeline(c->pipeline,f,0,depth+1);
        dump_conditionnelle(c->suiv,f,n+1,depth);
    }
}
static char * dump_seqop(Seqop o)
{
    switch ( o )
    {
        case SEQ:  return "SEQ";
        case BACK: return "BACK";
    }
    return NULL;
}
static void dump_sequence(Sequence * s, FILE * f, int n, int depth)
{
    if ( s != NULL )
    {
        dump_prefix(f,depth);
        fprintf(f,"SEQUENCE %d (op=%s)\n",n,dump_seqop(s->seqop));
        dump_conditionnelle(s->conditionnelle,f,0,depth+1);
        dump_sequence(s->suiv,f,n+1,depth);
    }
}
static void dump_commande_aux(Commande * c, FILE * f, int depth)
{
    dump_prefix(f,depth);
    fprintf(f,"COMMANDE\n");
    dump_sequence(c->sequence,f,0,depth+1);
}

void dump_commande(Commande * c, FILE * f)
{
    dump_commande_aux(c,f,0);
}
