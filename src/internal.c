/*
 * ----------------------------------------------------------------------------
 *
 * Lish: Lightweight Interactive SHell
 * Copyright (C) 2005 Benjamin Gaillard
 *
 * ---------------------------------------------------------------------------
 *
 *        File: src/internal.c
 *
 * Description: Internal Commands Execution
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


#define _POSIX_SOURCE /* For kill() */
#define _BSD_SOURCE   /* For setenv() */

/* Standard C headers */
#include <stdlib.h>  /* NULL, getenv(), setenv(), strtol() */
#include <stdio.h>   /* stderr, fprintf(), perror() */
#include <string.h>  /* strcmp(), strchr() */
#include <strings.h> /* strcasecmp() */

/* Standard Unix headers */
#include <sys/types.h>
#include <unistd.h> /* chdir(), execvp() */
#include <signal.h> /* SIG*, kill() */

/* Project headers */
#include "main.h"
#include "history.h"
#include "internal.h"


/*****************************************************************************
 *
 * Signal Utility Functions
 *
 */

/* Convenient macro */
#define MAKE_SIGNAL(signal) { signal, #signal }

/* Table of known signals (standard ones) */
static const struct {
    int         number;
    const char *name;
} signals[] = {
    MAKE_SIGNAL(SIGHUP),
    MAKE_SIGNAL(SIGINT),
    MAKE_SIGNAL(SIGQUIT),
    MAKE_SIGNAL(SIGILL),
    MAKE_SIGNAL(SIGABRT),
    MAKE_SIGNAL(SIGFPE),
    MAKE_SIGNAL(SIGKILL),
    MAKE_SIGNAL(SIGSEGV),
    MAKE_SIGNAL(SIGPIPE),
    MAKE_SIGNAL(SIGALRM),
    MAKE_SIGNAL(SIGTERM),
    MAKE_SIGNAL(SIGUSR1),
    MAKE_SIGNAL(SIGUSR2),
    MAKE_SIGNAL(SIGCHLD),
    MAKE_SIGNAL(SIGCONT),
    MAKE_SIGNAL(SIGSTOP),
    MAKE_SIGNAL(SIGTSTP),
    MAKE_SIGNAL(SIGTTIN),
    MAKE_SIGNAL(SIGTTOU)
};

/* Number of known signals */
#define SIGNAL_COUNT ((int) (sizeof signals / sizeof *signals))

/*
 * Find a signal by its number
 */
static int find_signal_by_number(char *key)
{
    int i, num;
    char *endptr;

    num = strtol(key, &endptr, 10);
    if (*endptr == '\0') {
	for (i = 0; i < SIGNAL_COUNT; i++)
	    if (signals[i].number == num)
		break;

	if (i < SIGNAL_COUNT)
	    return i;
    }

    return -1;
}

/*
 * Find a signal by its name (string, with or without 'SIG' at the beginning)
 */
static int find_signal_by_name(char *key)
{
    int i;

    for (i = 0; i < SIGNAL_COUNT; i++)
	if (!strcasecmp(signals[i].name, key) ||
	    !strcasecmp(signals[i].name + 3, key))
	    break;

    return i < SIGNAL_COUNT ? i : -1;
}

static int find_signal(char *key)
{
    int num;

    if ((num = find_signal_by_number(key)) == -1)
	num = find_signal_by_name(key);
    return num;
}


/*****************************************************************************
 *
 * Internal Commands Implementation
 *
 */

/*
 * Internal command: `cd' (change directory)
 */
static int internal_cd(int argc, char *argv[])
{
    char *dir; /* Directory name */

    if (argc > 2) {
	fprintf(stderr, "%s: cd: syntax error: cd [directory]\n", exe_name);
	return 1;
    }

    /* If no argument is given, cd to home */
    if (argc == 2)
	dir = argv[1];
    else if ((dir = getenv("HOME")) == NULL) {
	fprintf(stderr, "%s: cd: $HOME not set\n", exe_name);
	return 2;
    }

    /* Change directory */
    if (chdir(dir) == -1) {
	fprintf(stderr, "%s: cd: ", exe_name);
	perror(argv[1]);
	return 3;
    }

    /* Update $PWD and prompt */
    change_cwd();
    return 0;
}

/*
 * Internal command: `echo' (print text)
 */
static int internal_echo(int argc, char *argv[])
{
    int i; /* Counter */

    /* Print arguments */
    if (argc > 1) {
	fputs(argv[1], stdout);
	for (i = 2; i < argc; i++) {
	    putchar(' ');
	    fputs(argv[i], stdout);
	}
    }
    putchar('\n');

    return 0;
}

/*
 * Internal command: `exec' (replace shell with command)
 */
static int internal_exec(int argc, char *argv[])
{
    int code; /* Return code for internal command */

    if (argc < 2) {
	fprintf(stderr, "%s: exec: syntax error: exec command [args...]\n",
		exe_name);
	return 1;
    }

    /* Execute internal command if possible */
    if ((code = exec_internal(argc - 1, argv + 1)) != -1)
	lish_exit(code);

    /* Execute program */
    execvp(argv[1], argv + 1);
    lish_perror(argv[1]);
    return RET_ERROR;
}

/*
 * Internal command: `exit' (exist shell)
 */
static int internal_exit(int argc, char *argv[])
{
    int code;     /* Return code                */
    char *endptr; /* End of buffer for strtol() */

    if (argc > 2) {
	fprintf(stderr, "%s: exit: syntax error: exit [code]\n", exe_name);
	return 1;
    }

    /* Get exit code */
    if (argc == 1)
	code = 0;
    else {
	code = strtol(argv[1], &endptr, 0);
	if (*endptr != '\0') {
	    fprintf(stderr, "%s: exit: %s: invalid code\n", exe_name,
		    argv[1]);
	    return 2;
	}
    }

    lish_exit(code);
}

/*
 * Internal command: `export' (set an environment variable)
 */
static int internal_export(int argc, char *argv[])
{
    int i, ret = 0; /* Counter, return code   */
    char *equal;    /* Location of equal sign */

    if (argc == 1) {
	fprintf(stderr, "%s: export: syntax error: export key1=value1 "
		"[key2=value2...]\n", exe_name);
	return RET_ERROR;
    }

    /* Set each argument */
    for (i = 1; i < argc; i++) {
	if ((equal = strchr(argv[i], '=')) == NULL) {
	    fprintf(stderr, "%s: export: %s: syntax must be key=value",
		    exe_name, argv[i]);
	    ret = i;
	    continue;
	}

	/* Export the environment variable */
	*equal = '\0';
	if (setenv(argv[i], equal + 1, 1) == -1) {
	    fprintf(stderr, "%s: export: ", exe_name);
	    perror(argv[i]);
	    ret = i;
	}
	*equal = '=';
    }

    return ret;
}

/*
 * Internal command: `history' (print command history)
 */
static int internal_history(int argc, char *argv[] UNUSED)
{
    if (argc != 1 && (argc != 2 || strcmp(argv[1], "-c"))) {
	fprintf(stderr, "%s: history: syntax error: history [-c]\n",
		exe_name);
	return 1;
    }

    if (argc == 1)
	history_list();
    else
	history_clear();
    return 0;
}

/*
 * Internal command: `kill' (send signal to a process)
 */
static int internal_kill(int argc, char *argv[])
{
    int   i, num; /* Signal number         */
    pid_t pid;    /* Pid to send signal to */
    char *endptr; /* Pointer for strtol()  */
    int   ret;    /* Return value          */

    /* Error messages */
#define USAGE      { fprintf(stderr, usage, exe_name);      return 2; }
#define SPEC(name) { fprintf(stderr, spec, exe_name, name); return 3; }
    static const char usage[] = "%s: kill: usage: kill "
	"[{-[SIG]NAME | -NUMBER}] pid [pid...]\n";
    static const char spec[] = "%s: kill: %s: invalid signal specification\n";

    /* Check syntax */
    if (argc == 1)
	USAGE

    /* List signals */
    if (!strcmp(argv[1], "-l")) {
	if (argc == 2)
	    for (i = 0; i < SIGNAL_COUNT; i++)
		printf("[%2d] %s\n", signals[i].number, signals[i].name);
	else if (argc == 3) {
	    if ((num = find_signal_by_number(argv[2])) != -1)
		puts(signals[num].name);
	    else if ((num = find_signal_by_name(argv[2])) != -1)
		printf("%d\n", signals[num].number);
	    else
		SPEC(argv[2])
	} else
	    USAGE
	return 0;
    }

    /* Send SIGTERM by default */
    num = SIGTERM;

    /* Choose the signal */
    if (!strcmp(argv[1], "-s")) {
	if (argc <= 3)
	    USAGE;

	if ((num = find_signal_by_name(argv[2])) != -1) {
	    num = signals[num].number;
	    i = 3;
	} else
	    SPEC(argv[2])
    } else if (!strcmp(argv[1], "-n")) {
	if (argc <= 3)
	    USAGE

	if ((num = find_signal_by_number(argv[2])) != -1) {
	    num = signals[num].number;
	    i = 3;
	} else
	    SPEC(argv[2])
    } else if (argv[1][0] == '-') {
	if (argc == 2)
	    USAGE

	if ((num = find_signal(argv[1] + 1)) != -1) {
	    num = signals[num].number;
	    i = 2;
	} else
	    SPEC(argv[1] + 1)
    } else
	i = 1;

    /* Send signals */
    ret = 0;
    while (i < argc) {
	pid = strtol(argv[i], &endptr, 10);
	if (*endptr == '\0') {
	    if (kill(pid, num) == -1) {
		fprintf(stderr, "%s: kill: %d: ", exe_name, pid);
		perror(NULL);
	    }
	} else {
	    fprintf(stderr, "%s: kill: %s: arguments must be process IDs\n",
		    exe_name, argv[i]);
	    ret = 1;
	}
	i++;
    }

    return ret;
}

/* Internal command table */
static const struct {
    const char *name;
    int       (*function)(int argc, char *argv[]);
} internals[] = {
    { "cd",      internal_cd      },
    { "echo",    internal_echo    },
    { "exec",    internal_exec    },
    { "exit",    internal_exit    },
    { "export",  internal_export  },
    { "history", internal_history },
    { "kill",    internal_kill    }
};


/*****************************************************************************
 *
 * Main Internal Command Caller
 *
 */

/*
 * Execute an internal command and return error code or -1 if not found
 */
int exec_internal(char argc, char *argv[])
{
    int i; /* Counter */

    if (argc > 0)
	for (i = 0; i < (int) (sizeof internals / sizeof *internals); i++)
	    if (!strcmp(argv[0], internals[i].name))
		return internals[i].function(argc, argv);

    return -1;
}

/* End of file */
