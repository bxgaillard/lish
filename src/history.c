/*
 * ----------------------------------------------------------------------------
 *
 * Lish: Lightweight Interactive SHell
 * Copyright (C) 2005 Benjamin Gaillard
 *
 * ---------------------------------------------------------------------------
 *
 *        File: src/history.c
 *
 * Description: History Functions
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


#define _SVID_SOURCE /* For SHM/semaphore */
#define _BSD_SOURCE  /* For snprintf()    */

/* Standard C headers */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h> /* errno */

/* Standard UN*X headers */
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h> /* shmget(), shmctl(), shmat(), shmdt() */
#include <sys/sem.h> /* semget(), semctl(), semop()          */
#include <unistd.h>  /* getuid()                             */

/* Project headers */
#include <common.h>
#include "main.h"
#include "history.h"


/*****************************************************************************
 *
 * Constants and Variables
 *
 */

/* Reads can share access but write must have exclusive access */
#define READ_SEM  0 /* History reading semaphore */
#define WRITE_SEM 1 /* History writing semaphore */

/* If the command is executed from history (to not include it twice) */
int was_old_command = 0;

/* SHM and semaphore identifiers */
static int shm = -1, sem = -1;

/* History structure, stored in shared memory */
static struct {
    char commands[MAX_COMMANDS][MAX_COMMAND_LENGTH];
    int last, oldest, offset;
} *history = NULL;


/*****************************************************************************
 *
 * Data Types
 *
 */

/* Define a type that souldn't be defined, but usualy is, except in glibc */
#ifdef _SEM_SEMUN_UNDEFINED
/* Parameter used within semctl() */
union semun {
    int                 val;   /* Value for SETVAL             */
    struct semid_ds    *buf;   /* Buffer for IPC_STAT, IPC_SET */
    unsigned short int *array; /* Array for GETALL, SETALL     */
};
#endif /* _SEM_SEMUN_UNDEFINED */


/*****************************************************************************
 *
 * Shared Memory Semaphore Helpers
 *
 */

/*
 * Lock the history for reading
 */
static void history_read_lock(void)
{
    /* Wait for write to be zero, then increment read */
    struct sembuf ops[2];

    ops[0].sem_num = WRITE_SEM;
    ops[0].sem_op  = 0;
    ops[0].sem_flg = 0;

    ops[1].sem_num = READ_SEM;
    ops[1].sem_op  = 1;
    ops[1].sem_flg = SEM_UNDO;

    semop(sem, ops, 2);
}

/*
 * Unlock the history after reading
 */
static void history_read_unlock(void)
{
    /* Decrement read */
    struct sembuf op;

    op.sem_num = READ_SEM;
    op.sem_op  = -1;
    op.sem_flg = SEM_UNDO;

    semop(sem, &op, 1);
}

/*
 * Lock the history for writing
 */
static void history_write_lock(void)
{
    /* Wait for read and write to be zero, then increment write */
    struct sembuf ops[3];

    ops[0].sem_num = READ_SEM;
    ops[0].sem_op  = 0;
    ops[0].sem_flg = 0;

    ops[1].sem_num = WRITE_SEM;
    ops[1].sem_op  = 0;
    ops[1].sem_flg = 0;

    ops[2].sem_num = WRITE_SEM;
    ops[2].sem_op  = 1;
    ops[2].sem_flg = SEM_UNDO;

    semop(sem, ops, 3);
}

/*
 * Unkock the history after writing
 */
static void history_write_unlock(void)
{
    /* Decrement write */
    struct sembuf op;

    op.sem_num = WRITE_SEM;
    op.sem_op  = -1;
    op.sem_flg = SEM_UNDO;

    semop(sem, &op, 1);
}


/*****************************************************************************
 *
 * History Loading and Saving
 *
 */

/*
 * Get history filename
 */
static char *get_history_file(void)
{
    char *ret = NULL, *home; /* Result, home directory name */

    /* Construct history filename */
    if ((home = getenv("HOME")) != NULL)
	if ((ret = malloc(strlen(home) + strlen(HISTORY_FILE) + 2)) != NULL)
	    sprintf(ret, "%s/" HISTORY_FILE, home);

    return ret;
}

/*
 * Load history from a file
 */
static void history_load(void)
{
    int   next;  /* Next command in history */
    char *fname; /* Filename                */
    FILE *fd;    /* File descriptor         */

    /* Read commands from file */
    if ((fname = get_history_file()) != NULL) {
	if ((fd = fopen(fname, "r")) != NULL) {
	    while (fgets(history->commands[(next = (history->last + 1) %
					    MAX_COMMANDS)], MAX_COMMANDS, fd)
		   != NULL) {
		history->last = next;

		/* Update indexes */
		if (history->last == history->oldest) {
		    history->oldest = (history->oldest + 1) % MAX_COMMANDS;
		    history->offset++;
		} else if (history->oldest == -1)
		    history->oldest = 0;
	    }
	    fclose(fd);
	}

	free(fname);
    }
}

/*
 * Store history from to a file
 */
static void history_save(void)
{
    int i, limit; /* Counter, index limit */
    char *fname;  /* Filename             */
    FILE *fd;     /* File descriptor      */

    /* Write commands to file */
    if ((fname = get_history_file()) != NULL) {
	if ((fd = fopen(fname, "w")) != NULL) {
	    limit = (history->last + 1) % MAX_COMMANDS;
	    i = history->oldest;

	    /* Write command or stop if empty */
	    if (i != -1 && history->commands[i][0] != '\0')
		do {
		    fputs(history->commands[i], fd);
		    i = (i + 1) % MAX_COMMANDS;
		} while (i != limit);
	    fclose(fd);
	}

	free(fname);
    }
}


/*****************************************************************************
 *
 * History Initialization and Finalization
 *
 */

/*
 * Initialize history upon program starting
 */
void history_init(void)
{
    int             i, uid = getuid(); /* Counter, user ID          */
    struct shmid_ds shmds;             /* SHM description structure */
    union semun     val;               /* Value for semctl()        */

    /* Open or create shared memory */
    if ((shm = shmget(SHM_KEY + uid, sizeof *history, IPC_CREAT | 0600))
	== -1 || (history = shmat(shm, NULL, 0)) == NULL ||
	shmctl(shm, IPC_STAT, &shmds) == -1) {
	lish_perror("SHM fatal error");
	lish_exit(RET_ERROR);
    }

    /* Use or create semaphores */
    val.val = 0;
    if ((sem = semget(SEM_KEY + uid, 2, IPC_CREAT | IPC_EXCL | 0600)) != -1) {
	/* First process */
	if (semctl(sem, READ_SEM,  SETVAL, val) == -1 ||
	    semctl(sem, WRITE_SEM, SETVAL, val) == -1) {
	    lish_perror("semaphore fatal error");
	    lish_exit(RET_ERROR);
	}
    } else if ((sem = semget(SEM_KEY + uid, 2, IPC_CREAT | 0600)) == -1) {
	/* Other processe(s) already running */
	lish_perror("semaphore fatal error");
	lish_exit(RET_ERROR);
    }

    if (shmds.shm_nattch == 1) {
	/* First process to use this SHM */
	history_write_lock();

	/* Load history */
	for (i = 0; i < MAX_COMMANDS; i++)
	    history->commands[i][0] = 0;
	history->last = -1 % MAX_COMMANDS;
	history->oldest = -1;
	history->offset = 0;
	history_load();

	history_write_unlock();
    }
}

/*
 * Free history upon program termination
 */
void history_exit(void)
{
    struct shmid_ds shmds; /* SHM description structure */

    if (history != NULL && shm != -1) {
	/* Get SHM infos */
	if (shmctl(shm, IPC_STAT, &shmds) == -1) {
	    lish_perror("SHM fatal error");
	    history = NULL;
	    lish_abort();
	}

	history_read_lock();
	if (shmds.shm_nattch == 1) {
	    /* Last process to use this SHM */

	    /* Destroy shared memory after program termination */
	    shmctl(shm, IPC_RMID, NULL);

	    /* Save history */
	    history_save();

	    /* Destroy semaphore */
	    if (sem != -1 && semctl(sem, 0, IPC_RMID) == -1) {
		lish_perror("semaphore fatal error");
		sem = -1;
		lish_abort();
	    }
	} else
	    history_read_unlock();

	/* Detach shared memory */
	if (shmdt(history) == -1) {
	    lish_perror("SHM fatal error");
	    history = NULL;
	    lish_abort();
	}
    }
}


/*****************************************************************************
 *
 * History operations
 *
 */

/* Convenient macros */
#define TO_STR(str)   #str
#define INT_TO_STR(i) TO_STR(i)
#define ECHO_CMD(msg) "(echo " msg "; exit " INT_TO_STR(RET_ERROR) ")"

/*
 * Get last typed command
 */
char *history_last(void)
{
    char *buffer; /* String buffer */

    /* Allocate memory for command */
    if ((buffer = malloc(MAX_COMMAND_LENGTH)) == NULL) {
	lish_perror("fatal error");
	lish_exit(RET_ERROR);
    }

    was_old_command = 1;
    history_read_lock();

    /* Copy command from shared memory or print error message */
    if (history->commands[history->last][0] != '\0')
	strcpy(buffer, history->commands[history->last]);
    else
	snprintf(buffer, MAX_COMMAND_LENGTH,
		 ECHO_CMD("%s: no command entered yet"), exe_name);

    history_read_unlock();
    return buffer;
}

/*
 * Get the last ith typed command
 */
char *history_number(int i)
{
    int   pos;    /* Position in history */
    char *buffer; /* String buffer       */

    /* Allocate memory for command */
    if ((buffer = malloc(MAX_COMMAND_LENGTH)) == NULL) {
	lish_perror("fatal error");
	lish_exit(RET_ERROR);
    }

    was_old_command = 1;
    history_read_lock();

    /* Copy command from shared memory or print error message */
    pos = (history->oldest - history->offset + i) % MAX_COMMANDS;
    if (i >= history->offset && i < history->offset + MAX_COMMANDS &&
	history->commands[pos][0] != '\0')
	strcpy(buffer, history->commands[pos]);
    else
	snprintf(buffer, MAX_COMMAND_LENGTH,
		 ECHO_CMD("%s: %d: no such history index"), exe_name, i);

    history_read_unlock();
    return buffer;
}

/*
 * Get the last typed command beginning by `str'
 */
char *history_string(const char *str)
{
    int i, len, oldest; /* Counter, `str' length, oldest command */
    char *buffer;       /* String buffer                         */

    /* Allocate memory */
    if ((buffer = malloc(MAX_COMMAND_LENGTH)) == NULL) {
	lish_perror("fatal error");
	lish_exit(RET_ERROR);
    }

    was_old_command = 1;
    history_read_lock();

    /* Initialize variables */
    len = strlen(str);
    buffer[0] = '\0';
    oldest = (history->last + 1) % MAX_COMMANDS;

    /* Search the command throughout history */
    i = history->last;
    do {
	if (history->commands[i][0] != '\0') {
	    if (!strncmp(history->commands[i], str, len)) {
		strcpy(buffer, history->commands[i]);
		break;
	    }
	} else
	    break;
	i = (i - 1) % MAX_COMMANDS;
    } while (i != history->last);

    /* Print error message if not found */
    if (buffer[0] == '\0')
	sprintf(buffer, ECHO_CMD("%s: %s: no command beginning with that in "
				 "history"), exe_name, str);

    history_read_unlock();
    return buffer;
}

/*
 * Add a command to history
 */
void history_add(const char *cmd)
{
    history_write_lock();

    /* Add command */
    history->last = (history->last + 1) % MAX_COMMANDS;
    strncpy(history->commands[history->last], cmd, MAX_COMMAND_LENGTH - 1);
    history->commands[history->last][MAX_COMMAND_LENGTH - 1] = '\0';

    /* Update indexes */
    if (history->last == history->oldest) {
	history->oldest = (history->oldest + 1) % MAX_COMMANDS;
	history->offset++;
    } else if (history->oldest == -1)
	history->oldest = 0;

    history_write_unlock();
}

/*
 * Print history contents
 */
void history_list(void)
{
    int i, num, limit; /* Counter, user command number, index limit */

    was_old_command = 1;
    history_read_lock();

    /* Find oldest command */
    limit = (history->last + 1) % MAX_COMMANDS;
    i = history->oldest;

    /* Print each command, stopping upon history end */
    num = history->offset;
    if (i != -1 && history->commands[i][0] != '\0')
	do {
	    printf("[%2d] %s", num++, history->commands[i]);
	    i = (i + 1) % MAX_COMMANDS;
	} while (i != limit);

    history_read_unlock();
}

/*
 * Clear history contents
 */
void history_clear(void)
{
    int i; /* Counter */

    was_old_command = 1;
    history_write_lock();

    /* Reset fields */
    for (i = 0; i < MAX_COMMANDS; i++)
	history->commands[i][0] = 0;
    history->last = -1 % MAX_COMMANDS;
    history->oldest = -1;
    history->offset = 0;

    history_write_unlock();
}

/* End of file */
