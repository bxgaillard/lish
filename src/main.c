/*
 * ----------------------------------------------------------------------------
 *
 * Lish: Lightweight Interactive SHell
 * Copyright (C) 2005 Benjamin Gaillard
 *
 * ---------------------------------------------------------------------------
 *
 *        File: src/main.c
 *
 * Description: Main Functions
 *
 * ---------------------------------------------------------------------------
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * ---------------------------------------------------------------------------
 */


#define _POSIX_SOURCE          /* For kill()                      */
#define _BSD_SOURCE            /* For gethostname() and setenv()  */
#define _POSIX_C_SOURCE 200112 /* For gethostname() under FreeBSD */

/* Standard C headers */
#include <limits.h> /* PATH_MAX, HOST_NAME_MAX                        */
#include <stdio.h>  /* printf(), *puts(), fgets(), putchar() perror() */
#include <stdlib.h> /* NULL, malloc(), free()                         */
#include <string.h> /* strlen(), strcmp(), strncmp(), strrchr()       */
#include <errno.h>  /* errno                                          */

/* Standard Unix headers */
#include <sys/types.h>
#include <sys/wait.h> /* waitpid()                                    */
#include <unistd.h>   /* getuid(), geteuid(), gethostname(), getcwd() */
#include <signal.h>   /* sighandler_t, signal(), kill()               */
#include <pwd.h>      /* struct passwd, getpwuid()                    */
#include <libgen.h>   /* basename()                                   */

#ifdef HAS_MALLOC_MTRACE
# include <mcheck.h>  /* mtrace() */
#endif

/* Project headers */
#include <command.h>
#include <common.h>
#include "version.h"
#include "execcmd.h"
#include "history.h"
#include "main.h"

#ifndef PATH_MAX
# define PATH_MAX 4096
#endif /* !PATH_MAX */
#ifndef HOST_NAME_MAX
# define HOST_NAME_MAX 255
#endif /* !HOST_NAME_MAX */


/*****************************************************************************
 *
 * Global Variables
 *
 */

/* Program name, from executable filename */
const char *exe_name;

/* Current working directory */
char *cwd = NULL;
static int cwd_len = PATH_MAX;

/* Number of killed children by signal handler */
int killed = 0;


/*****************************************************************************
 *
 * Local Function Prototypes
 *
 */

static void display_prompt(void);
static void sig_int_quit_tstp(int sig);


/*****************************************************************************
 *
 * Initialization and Finalization Functions
 *
 */

/*
 * Main (initialization) function
 */
int main(int argc, char *argv[])
{
    int i, ret = 0, debug = 0;       /* Counter, return code, debugging? */
    char chr;                        /* Current string character         */
    char buffer[MAX_COMMAND_LENGTH]; /* Input buffer                     */
    command_t *cmd;                  /* Current command                  */

    /* Default name if it cannot be retrieved from argv[0] */
    static const char default_exe_name[] = "lish";

    /* Sexy prompt ;) */
    static const char sexy_prompt[] = "\\e[33;1m[\\e[32;1m\\u\\e[33;1m@"
	"\\e[35;1m\\h\\e[33;1m:\\e[34;1m\\w\\e[33;1m]\\e[31;1m\\$\\e[0m ";

    /* Memory allocation tracing */
#ifdef HAS_MALLOC_MTRACE
    setenv("MALLOC_TRACE", "lish.mtrace", 0);
    mtrace();
#endif

    /* Examine arguments */
    for (i = 1; i < argc; i++) {
	/* Use a very sexy prompt :) */
	if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "--sexy")) {
	    if (setenv("PS1", sexy_prompt, 1) == -1)
		lish_perror("prompt");
	    continue;
	}

	/* Enable command debugging */
	if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--debug")) {
	    debug = 1;
	    continue;
	}

	/* Display version and exit */
	if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--version")) {
	    printf("%s %s\n"
		   "Copyright (C) 2005 Benjamin Gaillard\n"
		   "\n"
		   "This is a basic bash-like shell.\n"
		   "Integrated commands: cd, echo, exec, exit, export, "
		   "history, kill.\n"
		   "\n"
		   "Have fun with %s!\n", lish_name, lish_version, lish_name);
	    return 0;
	}

	/* Display help and exit */
	if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
	    printf("Usage: %s [-s | --sexy] [-d | --debug] [-v | --version] "
		   "[-h | --help]\n"
		   "    -s: use an improved predefined prompt\n"
		   "    -d: display command parsing debug informations\n"
		   "    -v: display version information\n"
		   "    -h: display this help\n"
		   "\n", argv[0]);
	    printf("The prompt is based on $PS1, some bash $PS1 escape codes "
		   "are supported:\n"
		   "    \\$: `$' if normal user, `#' if root\n"
		   "    \\\\: `\\'\n"
		   "    \\[ and \\]: ignored\n"
		   "    \\e: escape code (\\033)\n"
		   "    \\h: hostname up to the first `.'\n"
		   "    \\H: full hostname\n"
		   "    \\n: line break\n"
		   "    \\s: shell name\n"
		   "    \\u: user name\n"
		   "    \\v and \\V: current shell version\n"
		   "    \\w: current working directory (full path)\n"
		   "    \\W: current working directory (name only)\n"
		   "The prompt used with -s/--sexy is:\n"
		   "    %s\n", sexy_prompt);
	    return 0;
	}
    }

    /* Find executable name */
    if (argc == 0 || (exe_name = strrchr(argv[0], '/')) == NULL ||
	(++exe_name)[0] == '\0')
	exe_name = default_exe_name;

    /* Install signal handlers */
    signal(SIGTERM, SIG_IGN); /* Ignore SIGTERM, as bash does */
    signal(SIGCHLD, sig_chld);
    signal(SIGINT,  sig_int_quit_tstp);
    signal(SIGQUIT, sig_int_quit_tstp);
    signal(SIGTSTP, sig_int_quit_tstp);

    /* Update current directory and display first prompt */
    change_cwd();
    display_prompt();

    /* Initialize history */
    history_init();

    /* Input and process command lines */
    while (fgets(buffer, sizeof buffer, stdin) != NULL) {
	/* Check for empty line */
	for (i = 0; (chr = buffer[i]) != '\0'; i++)
	    if (chr != ' ' && chr != '\t' && chr != '\n')
		break;

	/* Parse and execute command */
	was_old_command = 0;
        if (chr != '\0' && (cmd = parse_command(buffer)) != NULL) {
	    /* Execute command */
	    if (debug)
		dump_command(cmd, stderr);
            ret = exec_command(cmd);
            free_command(cmd);

	    /* Add command to history (only if it's valid) */
	    if (!was_old_command)
		history_add(buffer);
        }

	/* Re-display prompt */
	display_prompt();
    }

    history_exit();

    /* Like bash */
    puts("exit");

#ifdef HAS_MALLOC_MTRACE
    muntrace();
#endif

    return ret;
}

/*
 * Free memory and exit Lish
 */
void NORETURN lish_exit(int error_code)
{
    close_fds();
    if (current_command)
	free_command(current_command);
    history_exit();

    exit(error_code);
}

/*
 * Free memory and cause abnormal program termination
 */
void NORETURN lish_abort(void)
{
    close_fds();
    if (current_command)
	free_command(current_command);
    history_exit();

    abort();
}


/*****************************************************************************
 *
 * Utility Functions
 *
 */

/*
 * Display the prompt
 */
static void display_prompt(void)
{
    int         i, j, home_len; /* Counters, home directory length */
    char        chr;            /* Current character               */
    char       *home;           /* Home directory name             */
    const char *prompt;         /* Prompt string                   */

    /* Information used to create the prompt */
    static int uid = -1, euid = -1;
    static struct passwd *username = NULL;
    static char hostname[HOST_NAME_MAX + 1];
    static const char default_prompt[] = "\\s\\$ ";

    /* Use default prompt if not present in the $PS1 environment variable */
    if ((prompt = getenv("PS1")) == NULL)
	prompt = default_prompt;

    /* Find informations if not already done */
    if (uid == -1)
	uid = getuid();
    if (euid == -1)
	euid = geteuid();
    if (username == NULL)
	username = getpwuid((uid = getuid()));

    /* Informations that may change throuth execution, (re)get them */
    gethostname(hostname, sizeof hostname);
    home = getenv("HOME");

    /* Replace escape sequences */
    for (i = 0; (chr = prompt[i]) != '\0'; i++) {
	if (chr != '\\')
	    putchar(chr);
	else
	    switch ((chr = prompt[++i])) {
	    case '$':
		putchar(euid != 0 ? '$' : '#');
		break;

	    case 'H':
		fputs(hostname, stdout);
		break;

	    case 'W':
		if (!strcmp(cwd, home))
		    putchar('~');
		else
		    fputs(basename(cwd), stdout);
		break;

	    case '\\':
		putchar('\\');
		break;

	    case '[':
	    case ']':
		break;

	    case 'e':
		putchar('\033');
		break;

	    case 'h':
		for (j = 0; hostname[j] != '\0'; j++)
		    if (hostname[j] == '.') {
			hostname[j] = '\0';
			break;
		    }
		fputs(hostname, stdout);
		break;

	    case 'n':
		putchar('\n');
		break;

	    case 's':
		fputs(exe_name, stdout);
		break;

	    case 'u':
		if (username)
		    fputs(username->pw_name, stdout);
		else
		    printf("%d", uid);
		break;

	    case 'v':
	    case 'V':
		fputs(lish_version, stdout);
		break;

	    case 'w':
		if (home) {
		    if ((home_len = strlen(home)) > 1) {
			if (home[home_len - 1] == '/')
			    home_len--;

			if (home_len > 1 && !strncmp(cwd, home, home_len)) {
			    putchar('~');
			    fputs(cwd + home_len, stdout);
			} else
			    fputs(cwd, stdout);
		    } else
			fputs(cwd, stdout);
		} else
		    fputs(cwd, stdout);
		break;

	    default:
		putchar('\\');
		if (chr != '\0')
		    putchar(chr);
		else
		    i--;
	    }
    }

    fflush(stdout);
}

/*
 * Update $PWD and directory used in prompt
 */
void change_cwd(void)
{
    /* Allocate memory if not already done */
    if (cwd == NULL && (cwd = malloc(PATH_MAX)) == NULL) {
	lish_perror("fatal error");
	lish_exit(RET_ERROR);
    }

    /* Get working directory and allocate more memory if necessary */
    while (getcwd(cwd, cwd_len) == NULL) {
	if (errno == ERANGE) {
	    free(cwd);
	    if ((cwd = malloc((cwd_len += PATH_MAX))) == NULL) {
		lish_perror("fatal error");
		lish_exit(RET_ERROR);
	    }
	} else {
	    lish_perror("fatal error");
	    lish_exit(RET_ERROR);
	}
    }

    /* Set environment variable $PWD */
    setenv("PWD", cwd, 1);
}

/*
 * Print an error message, preceded by shell name and followed by the standard
 * error string
 */
void lish_perror(const char *str)
{
    fprintf(stderr, "%s: ", exe_name);
    perror(str);
}


/*****************************************************************************
 *
 * Signal Handlers
 *
 */

/*
 * Called upon SIGCHLD (a child terminated)
 */
void sig_chld(int sig UNUSED)
{
    int   status; /* Return code            */
    pid_t pid;    /* Terminated process PID */

    /* Re-install handler */
    signal(SIGCHLD, sig_chld);

    /* Wait for all pending children (non-blocking) */
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
	if (WIFSTOPPED(status)) {
	    kill(pid, SIGCONT);
	    printf("\n[%d] in background\n", pid);
	} else
	    printf("\n[%d] %d\n", pid,
		   WIFEXITED(status) ? WEXITSTATUS(status) : RET_ERROR);
	/* Handler only active when no command is executing */
	display_prompt();
    }
}

/*
 * Called upon SIGINT (interrupted: Ctrl-C), SIGQUIT (Ctrl-\) or
 * SIGTSTP (Ctrl-Z)
 */
static void sig_int_quit_tstp(int sig)
{
    int i; /* Counter */

    /* Re-install handler */
    signal(sig, sig_int_quit_tstp);

    /* Transmit signal to currently executing processes */
    for (i = 0; i < proc_count; i++) {
	/* Signal is normally automatically sent to children, but ensure it is
	   by doing so ourselves */
	kill(processes[i], sig);
	killed++;
    }

    /* Imitate the behaviour of bash */
    if (sig == SIGINT && !current_command) {
	putchar('\n');
	display_prompt();
    }
}


/*****************************************************************************
 *
 * Interface with History Functions
 *
 */

/*
 * historique_precedente -> history_last
 */
char *historique_precedente(void)
{
    return history_last();
}

/*
 * historique_numero -> history_number
 */
char *historique_numero(int i)
{
    return history_number(i);
}

/*
 * historique_chaine -> history_string
 */
char *historique_chaine(const char *c)
{
    return history_string(c);
}

/* End of file */
