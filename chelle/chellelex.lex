
%{
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "commande.h"
#include "chelleparse.h"


    /* Managing input (because of history) */
    struct buffer_stack {
        YY_BUFFER_STATE yybs;
        struct buffer_stack * prev;
    };
    static struct buffer_stack * current_buffer = NULL;
    static void push_buffer(char * str, int freeable)
    {
        struct buffer_stack * b = malloc(sizeof(struct buffer_stack));
        b->yybs = yy_scan_string(str);
        if ( freeable )
            free(str);
        b->prev = current_buffer;
        current_buffer = b;
    }
    static void pop_buffer()
    {
        struct buffer_stack * n = current_buffer->prev;
        yy_delete_buffer(current_buffer->yybs);
        free(current_buffer);
        current_buffer = n;
        if ( current_buffer != NULL )
            yy_switch_to_buffer(current_buffer->yybs);
    }

    /* Scaning input/output redirection operator on file */
    static int scan_redirfile(const char *s)
    {
        const char * p = s;
        int n = 0;

        /* read a number (optional) */
        while ( isdigit(*p) )
            n = n*10 + (*p++ - '0');
        /* if no number was there, use defaults */
        if ( *p == '>' && n == 0 )
            n = 1;
        return n;
    }

    /* Scanning input/output descriptor redirection */
    static RedirDesc * scan_redirdesc(const char * s)
    {
        const char * p = s;
        int n = 0;
        int m = 0;
        int dupsome = 0;
        RedirDesc * r = malloc(sizeof(RedirDesc));

        /* read a number (optional) */
        while ( isdigit(*p) )
            n = n*10 + (*p++ - '0');
        /* if no number was there, use defaults */
        if ( *p == '>' && n == 0 )
            n = 1;
        r->dst = n;

        if ( *p == '<' )
            r->mode = READ;
        else
            r->mode = WRITE;

        /* Advance */
        ++p;
        /* Skip & */
        ++p;
        /* read a number (optional) */
        while ( isdigit(*p) )
        {
            m = m*10 + (*p++ - '0');
            dupsome = 1;
        }
        if ( dupsome == 1 )
            r->src = m;
        else
            r->src = -1;

        /* Do we have a '-' (meaning close n) */
        if ( *p == '-' )
        {
            if ( dupsome == 1 )
                r->type = DUPCLOSE;
            else
                r->type = CLOSE;
        }
        else
        {
            r->type = DUP;
        }
        return r;
    }

    /* Strip quotes */
    static char * strip_quotes(const char * s)
    {
        int l = strlen(s);
        char * t = calloc(l-1,sizeof(char));
        strncpy(t,s+1,l-2);
        t[l-2] = '\0';
        return t;
    }

    /* Sanitize \-quoted chars in string literals */
    static char * sanitize_string(const char * s)
    {
        char * t = calloc(strlen(s)+1,sizeof(char));
        const char * ps = s;
        char * pt = t;
        while ( *ps )
        {
            if ( *ps == '\\' )
            {
                switch ( *(++ps) )
                {
                    case 'a' : *pt++ = '\a'; break;
                    case 'b' : *pt++ = '\b'; break;
                    case 'f' : *pt++ = '\f'; break;
                    case 'n' : *pt++ = '\n'; break;
                    case 'r' : *pt++ = '\r'; break;
                    case 't' : *pt++ = '\t'; break;
                    case 'v' : *pt++ = '\v'; break;
                    case '\0': *pt++ = '\\'; break;
                    default: *pt++ = *ps;
                }
                if ( *ps )
                    ++ps;
            }
            else
                *pt++ = *ps++;
        }
        *pt = '\0';
        return t;
    }

    extern Commande * parse_result;
    extern void leaker_ok();
    extern void leaker_ko();
    extern int yyparse(void);

    /* Déclarée dans commande.h */
    Commande * parse_commande(char * src)
    {
        Commande * c;
        push_buffer(src,0);
        if ( yyparse() == 0 )
        {
            c = parse_result;
            leaker_ok();
        }
        else
        {
            c = NULL;
            leaker_ko();
            fprintf(stderr,"Problem on: %s\n",src);
        }
        pop_buffer();
        return c;
    }

    /* A fournir */
    extern char * historique_precedente();
    extern char * historique_numero(int);
    extern char * historique_chaine(const char *);

%}

%option nounput
%option nointeractive
%option noyywrap

%%

"!!" {
    push_buffer(historique_precedente(),1);
}
"!"[[:digit:]]+ {
    push_buffer(historique_numero(atoi(yytext+1)),1);
}
"!"[^[:space:]]+ {
    push_buffer(historique_chaine(yytext+1),1);
}

"&" { yylval.copval = BACK; return SEQOP; }
";" { yylval.sopval = SEQ;  return SEQOP; }

"(" return PARO;
")" return PARF;

[[:digit:]]*"<"  {
    yylval.intval = scan_redirfile(yytext);
    return INFROM;
}
[[:digit:]]*">"  {
    yylval.intval = scan_redirfile(yytext);
    return OUTTO;
}
[[:digit:]]*">>" {
    yylval.intval = scan_redirfile(yytext);
    return APPTO;
}

[[:digit:]]*[<>]&([[:digit:]]+|-|[[:digit:]]+-) {
    yylval.rddval = scan_redirdesc(yytext);
    return REDIRDESC;
}


"|" return PIPE;

"||" { yylval.sopval = OR;  return CONDOP; }
"&&" { yylval.sopval = AND; return CONDOP; }


\"[^\"]*\" {
    yylval.motval = strip_quotes(yytext);
    return MOT;
}

\'[^\']*\' {
    yylval.motval = strip_quotes(yytext);
    return MOT;
}

\`[^\`]*\` {
    yylval.motval = strip_quotes(yytext);
    return MOT;
}

([^|&><;()[:space:]]|\\[|&><;()[:space:]])+ {
    yylval.motval = sanitize_string(yytext);
    return MOT;
}

[[:space:]] ;

<<EOF>> {
    if ( current_buffer->prev == NULL )
        yyterminate();
    else
        pop_buffer();
}

%%
