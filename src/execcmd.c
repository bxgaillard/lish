/*
 * ----------------------------------------------------------------------------
 *
 * Lish: Lightweight Interactive SHell
 * Copyright (C) 2005 Benjamin Gaillard
 *
 * ---------------------------------------------------------------------------
 *
 *        File: src/execcmd.c
 *
 * Description: Command Processing and Execution
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

/* Standard C headers */
#include <stdio.h>  /* std*, *printf(), *puts(), perror() */
#include <stdlib.h> /* NULL, malloc(), free()             */
#include <string.h> /* str*cpy(), str*cmp()               */

/* Standard UN*X headers */
#include <sys/types.h>
#include <sys/wait.h> /* wait(), waitpid()                              */
#include <unistd.h>   /* close(), dup(), dup2(), pipe(), fork(), exec() */
#include <fcntl.h>    /* open(), creat(), fcntl()                       */
#include <signal.h>   /* signal(), kill()                               */

/* Project headers */
#include <command.h>
#include <common.h>
#include "main.h"
#include "internal.h"
#include "execcmd.h"


/*****************************************************************************
 *
 * Global Variables
 *
 */

/* Currently processed command */
command_t *current_command = NULL;

/* Running processes */
int    proc_count = 0;   /* Number of running pipeline processes */
pid_t *processes = NULL; /* Array of pipeline processes          */

/* Backup file descriptors (3), plus one used for the pipeline */
static int backup_fd[4] = { -1, -1, -1, -1 };

/* Execution mode: lets a single command, not pipelined, to be exec*()'ed
   without forking in a background "subshell" */
static enum { EXEC_SEQ, EXEC_BACK, EXEC_SINGLE1, EXEC_SINGLE2 } exec_mode;

/* Return code */
static int   ret_code = 0; /* The returned code                       */
static pid_t ret_pid = -1; /* PID of the processus returning the code */


/*****************************************************************************
 *
 * File Descriptors Manipulation Functions
 *
 */

/*
 * Backup standard file descriptors (0, 1, 2)
 */
static void backup_fds(void)
{
    int i;

    for (i = 0; i < 3; i++)
	fcntl((backup_fd[i] = dup(i)), F_SETFD, FD_CLOEXEC);
}

/*
 * Restore standard file descriptors (0, 1, 2)
 */
static void restore_fds(void)
{
    int i;

    for (i = 0; i < 3; i++)
	if (backup_fd[i] != -1) {
	    dup2(backup_fd[i], i);
	    close(backup_fd[i]);
	    backup_fd[i] = -1;
	}
}

/*
 * Close backup'ed standard file descriptors (0, 1, 2)
 */
void close_fds(void)
{
    int i;

    for (i = 0; i < 4; i++)
	if (backup_fd[i] != -1)
	    close(backup_fd[i]);
}

/*
 * Ensure a than file descriptor is free (no backup use it)
 */
static int free_fd(int fd)
{
    int i;

    if (fd != -1)
	for (i = 0; i < 4; i++)
	    if (fd == backup_fd[i]) {
		if ((fd = dup(fd)) == -1)
		    return -1;
		close(backup_fd[i]);
		backup_fd[i] = fd;
		break;
	    }

    return fd;
}

/*
 * Close descriptors created for a command (excluding backups)
 */
static void clean_fds(int max)
{
    int i, fd;

    for (fd = 3; fd <= max; fd++)
	for (i = 0; i < 4; i++) {
	    if (backup_fd[i] == fd)
		break;
	    close(fd);
	}
}


/*****************************************************************************
 *
 * Command Processing Functions
 *
 */

/* Prototypes */
static pid_t exec_simple(simple_t *simple);
static pid_t exec_redirected(redirected_t *redirected);
static int   exec_pipeline(pipeline_t *pipeline);
static int   exec_conditional(conditional_t *conditional);
static int   exec_sequence(sequence_t *sequence);

/*
 * Execute a simple command and return the created process PID (if applicable)
 */
static pid_t exec_simple(simple_t *simple)
{
    words_t *word;     /* Current command word   */
    int      count;    /* Argument count         */
    char   **argv;     /* Argument table         */
    pid_t    pid = -1; /* PID of created process */

    /* Upon entry to this function, some file descriptors are open beside
       those open by the command: the backup descriptors for standard input
       and output and error output, and the pipe descriptor that will be used
       by the next command in pipeline queue.  These descriptors should be
       closed before calling exec*().  Thanks to the close-on-exec flag that
       has been set before on these descriptors, no close() is required.

       But, unfortunately, they are required for subshell, since no exec() is
       done there.  So close_fds() is called. */

    switch (simple->type) {
    case SIMPLE:
	/* Count the word number */
	count = 0;
	for (word = simple->u.words; word; word = word->next)
	    count++;
	if (count == 0) {
	    fputs("Error: empty command.\n", stderr);
	    lish_exit(RET_ERROR);
	}

	/* Allocate memory for the `argv' array */
	if ((argv = malloc((count + 1) * sizeof (char *))) == NULL) {
	    fputs("Error: no more memory.\n", stderr);
	    lish_exit(RET_ERROR);
	}

	/* Fill the `argv' array */
	count = 0;
	for (word = simple->u.words; word; word = word->next) {
	    if (word->word[0] == '$') {
		if ((argv[count] = getenv(word->word + 1)) == NULL)
		    argv[count] = "";
	    } else
		argv[count] = word->word;
	    count++;
	}
	argv[count] = NULL;

	/* Execute internal command or do the fork and execute program */
	if ((ret_code = exec_internal(count, argv)) == -1 &&
	    (exec_mode == EXEC_SINGLE2 || (pid = fork()) == 0)) {
	    /* Execute program */
	    execvp(argv[0], argv);
	    lish_perror(argv[0]);
	    free(argv);
	    lish_exit(RET_ERROR);
	}

	free(argv);
	break;

    case SUBSHELL:
	/* Do the fork and execute subshell */
	if (exec_mode == EXEC_SINGLE2 || (pid = fork()) == 0) {
	    /* Close descriptors that are useless to the child */
	    close_fds();

	    /* Execute subshell sequence */
	    lish_exit(exec_sequence(simple->u.command->sequence));
	}
    }

    return pid;
}

/*
 * Execute a command after setting up its file descriptor redirections
 */
static pid_t exec_redirected(redirected_t *redirected)
{
    redirection_t *redir;             /* Current redirection              */
    redir_file_t  *redir_file;        /* File redirection                 */
    redir_desc_t  *redir_desc;        /* Descriptor redirection           */
    int            fd = 0, maxfd = 0; /* Created file descriptor, maximum */
    int            mode;              /* Descriptor access mode           */
    pid_t          pid;               /* Created process PID              */

    for (redir = redirected->redirection; redir; redir = redir->next)
	switch (redir->type) {
	case RFILE:
	    /* File redirection */
	    redir_file = redir->u.redir_file;

	    /* Open file */
	    switch (redir_file->type) {
	    case IN:
		fd = open(redir_file->file, O_RDONLY);
		break;
	    case OUT:
		fd = creat(redir_file->file, 0666);
		break;
	    case APP:
		fd = open(redir_file->file, O_CREAT | O_WRONLY | O_APPEND,
			  0666);
	    }

	    /* Replace destination descriptor by this file descriptor */
	    if (free_fd(redir_file->desc) == -1 ||
		dup2(fd, redir_file->desc) == -1) {
		lish_perror(redir_file->file);
		ret_code = RET_ERROR;
		return 0;
	    }
	    close(fd);
	    if (redir_file->desc > maxfd)
		maxfd = redir_file->desc;
	    break;

	case DESCRIPTOR:
	    /* Descriptor redirection */
	    redir_desc = redir->u.redir_desc;

	    /* Retrieve access mode */
	    if ((mode = fcntl(redir_desc->src, F_GETFL)) == -1) {
		fprintf(stderr, "%s: %d: ", exe_name, redir_desc->src);
		perror(NULL);
		ret_code = RET_ERROR;
		return 0;
	    }

	    /* Check access mode */
	    mode &= O_ACCMODE;
	    if (mode == (redir_desc->mode == READ ? O_WRONLY : O_RDONLY)) {
		fprintf(stderr, "%s: %d: wrong access mode\n", exe_name,
			redir_desc->src);
		ret_code = RET_ERROR;
		return 0;
	    }

	    /* Duplicate descriptor */
	    if ((redir_desc->type == DUP || redir_desc->type == DUPCLOSE) &&
		(free_fd(redir_desc->dst) == -1 ||
		dup2(redir_desc->src, redir_desc->dst) == -1)) {
		fprintf(stderr, "%s: %d: ", exe_name, redir_desc->dst);
		perror(NULL);
		ret_code = RET_ERROR;
		return 0;
	    }

	    /* Close descriptor */
	    if (redir_desc->type == CLOSE || redir_desc->type == DUPCLOSE)
		close(redir_desc->src);
	    break;
	}

    /* Execute simple command */
    pid = exec_simple(redirected->simple);

    /* Clean descriptors */
    clean_fds(maxfd);

    return pid;
}

/*
 * Execute a pileline of commands
 */
static int exec_pipeline(pipeline_t *pipeline)
{
    int         i;                 /* Counter                       */
    int         status;            /* Returned error code           */
    pid_t       pid;               /* Created process PID           */
    int         count = 0;         /* Children and command count    */
    int         fd, pipe_fd[2][2]; /* Pipeline file descriptors     */
    pipeline_t *pipe_count;        /* Used for command counting     */

    if (exec_mode == EXEC_SINGLE1 && !pipeline->next)
	exec_mode = EXEC_SINGLE2;

    /* Count commands in pipeline and allocate enough memory */
    for (pipe_count = pipeline; pipe_count; pipe_count = pipe_count->next)
	count++;
    if ((processes = malloc(count * sizeof (pid_t))) == NULL) {
	lish_perror("error");
	return RET_ERROR;
    }
    proc_count = 0;
    ret_code = RET_ERROR;

    /* Save standard input and output descriptors */
    backup_fds();

    /* Launch each simple command after setting pipelines */
    count = 0;
    killed = 0;
    signal(SIGCHLD, SIG_DFL);
    while (pipeline) {
	/* Replace standard input by pipeline input */
	if (count > 0) {
	    if (dup2((fd = pipe_fd[(count - 1) % 2][0]), STDIN_FILENO)
		== -1) {
		close(fd);
		lish_perror("cannot duplicate file descriptor");
		lish_exit(RET_ERROR);
	    }
	    close(fd);
	}

	if (pipeline->next) {
	    /* Create pipeline for the current and the next simple commands */
	    if (pipe(pipe_fd[count % 2]) == -1) {
		lish_perror("cannot create pipe");
		lish_exit(RET_ERROR);
	    }

	    /* Replace standard output by pipeline output */
	    if (dup2((fd = pipe_fd[count % 2][1]), STDOUT_FILENO) == -1) {
		close(fd);
		lish_perror("cannot duplicate file descriptor");
		lish_exit(RET_ERROR);
	    }
	    close(fd);

	    /* Close the descriptor used as the input of the next command */
	    fcntl((backup_fd[3] = pipe_fd[count % 2][0]),
		  F_SETFD, FD_CLOEXEC);
	} else
	    /* Restore standard input */
	    dup2(backup_fd[STDOUT_FILENO], STDOUT_FILENO);

	/* Execute simple command with redirections */
	if ((ret_pid = exec_redirected(pipeline->redirected)) > 0)
	    processes[proc_count++] = ret_pid;
	else if (ret_pid == 0)
	    lish_perror(NULL);

	fcntl(backup_fd[3], F_SETFD, 0);
	count++;
	pipeline = pipeline->next;
    }

    /* Restore standard input and output descriptors */
    restore_fds();
    backup_fd[3] = -1;

    /* Wait for children to terminate and get return code of the last one */
    while (proc_count != 0) {
	if ((pid = waitpid(-1, &status, WUNTRACED)) == -1)
	    break;
	if (pid == ret_pid)
	    ret_code = WIFEXITED(status) ? WEXITSTATUS(status) : RET_ERROR;
	if (WIFSTOPPED(status)) {
	    /* Put in background stopped process */
	    kill(pid, SIGCONT);
	    printf("[%d] in background", pid);
	}

	/* Check if it is a command of the current pipeline */
	for (i = 0; i < proc_count; i++)
	    if (processes[i] == pid)
		break;
	if (i < proc_count) {
	    while (++i < proc_count)
		processes[i - 1] = processes[i];
	    proc_count--;
	} else if (!WIFSTOPPED(status))
	    /* It isn't, so display PID and return code */
	    printf("[%d] %d\n", pid, ret_code);
    }
    sig_chld(SIGCHLD);

    /* Imitate the behaviour of bash */
    if (killed != 0) {
	putchar('\n');
	killed = 0;
    }

    free(processes);
    return ret_code;
}

/*
 * Execute a conditional command set
 */
static int exec_conditional(conditional_t *conditional)
{
    int ret = 0; /* Return code */

    if (exec_mode == EXEC_BACK && !conditional->next)
	exec_mode = EXEC_SINGLE1;

    /* Execute each command if appropriate */
    while (conditional) {
	if ((conditional->cond_op == AND && ret != 0) ||
	    (conditional->cond_op == OR  && ret == 0))
	    break;

	ret = exec_pipeline(conditional->pipeline);
	conditional = conditional->next;
    }

    return ret;
}

/*
 * Execute a sequence command
 */
static int exec_sequence(sequence_t *sequence)
{
    int   ret = 0;    /* Return code               */
    pid_t pid;        /* Created process PID       */
    int   pipe_fd[2]; /* Pipeline file descriptors */

    while (sequence) {
	switch (sequence->seq_op) {
	case SEQ:
	    exec_mode = EXEC_SEQ;

	    /* Execute conditional command set */
	    ret = exec_conditional(sequence->conditional);
	    break;

	case BACK:
	    exec_mode = EXEC_BACK;

	    /* Create a pipe for the commands to have en empty input */
	    if (pipe(pipe_fd) == -1) {
		lish_perror("cannot create pipe");
		lish_exit(RET_ERROR);
	    }

	    if ((pid = fork()) == 0) {
		/* Create a new session (useful for SIGINT/SIGQUIT) */
		setsid();
		close(pipe_fd[1]);

		/* Replace standard input by pipe input */
		if (dup2(pipe_fd[0], STDIN_FILENO) == -1) {
		    lish_perror("cannot duplicate file descriptor");
		    lish_exit(RET_ERROR);
		}
		close(pipe_fd[0]);

		/* Execute conditional command set */
		lish_exit(exec_conditional(sequence->conditional));
	    }

	    /* Close pipe */
	    close(pipe_fd[0]);
	    close(pipe_fd[1]);

	    /* Print created process PID */
	    if (pid != -1) {
		printf("[%d]\n", pid);
		ret = 0;
	    } else {
		perror("Could not fork");
		ret = RET_ERROR;
	    }
	}

	/* Next conditional command set */
	sequence = sequence->next;
    }

    return ret;
}


/*****************************************************************************
 *
 * Main Public Function
 *
 */

/*
 * Execute a full command (a sequence)
 */
int exec_command(command_t *command)
{
    int ret; /* Return code */

    current_command = command;
    ret = exec_sequence(command->sequence);
    current_command = NULL;
    return ret;
}

/* End of file */
