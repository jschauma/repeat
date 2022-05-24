/*
 * Copyright (c) 2022, Jan Schaumann <jschauma@netmeister.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/wait.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

extern char *__progname;

char *replstr;

int JFlag = 0;
int count = -1;
int sleepSeconds = 0;
int waitForJobs = 1;
int verbose = 0;

void catchExecFailure(int);
int execCommand(char **, int, int);
int repeat(char **, int);
char *replaceNum(char *, int);
char **replaceNumInArgs(char **, int, int);
void usage(FILE *);

void
catchExecFailure(int s) {
	(void)s;
	exit(EXIT_FAILURE);
	/* NOTREACHED */
}

int
main(int argc, char **argv) {
	int ch;

	/* GNU getopt(3) needs '+' to enable POSIXLY_CORRECT behavior. */
	while ((ch = getopt(argc, argv, "+J:dhn:s:v")) != -1) {
		switch(ch) {
		case 'J':
			JFlag = 1;
			replstr = optarg;
			if (replstr != NULL && *replstr == '\0') {
				errx(EXIT_FAILURE, "replstr may not be empty");
			}
			break;
		case 'd':
			waitForJobs = 0;
			sleepSeconds = 0;
			break;
		case 'h':
			usage(stdout);
			exit(EXIT_SUCCESS);
			/* NOTREACHED */
			break;
		case 'n':
			count = atoi(optarg);
			if (count <= 0) {
				errx(EXIT_FAILURE, "count must be > 0");
			}
			break;
		case 's':
			waitForJobs = 1;
			sleepSeconds = atoi(optarg);
			if (sleepSeconds <= 0) {
				errx(EXIT_FAILURE, "sleep must be > 0");
			}
			break;
		case 'v':
			verbose = 1;
			break;
		default:
			usage(stderr);
			exit(EXIT_FAILURE);
			/* NOTREACHED */
		}
	}

	argc -= optind;
	argv += optind;
	if (!argc) {
		errx(EXIT_FAILURE, "you need to specify a command to invoke");
		/* NOTREACHED */
	}

	if (signal(SIGUSR1, catchExecFailure) == SIG_ERR) {
		err(EXIT_FAILURE, "unable to install signal handler");
		/* NOTREACHED */
	}

	return repeat(argv, argc);
}

int
execCommand(char **argv, int argc, int num) {
	int pid, status;
	int e = 0;
	int freeLargv = 1;

	char **largv;

	if ((largv = replaceNumInArgs(argv, argc, num)) == NULL) {
		largv = argv;
		freeLargv = 0;
	}

	if ((pid = fork()) < 0) {
		errx(EXIT_FAILURE, "unable to fork");
		/* NOTREACHED */
	}

	if (pid == 0) {
		int ppid = getppid();
		if (verbose) {
			int padding = floor(log10(count)) + 1;
			(void)printf("%0*d: ", padding, num);
		}
		(void)fflush(stdout);
		execvp(largv[0], largv);
		if (ppid > 1 && kill(ppid, SIGUSR1) < 0) {
			err(EXIT_FAILURE, "unable to raise SIGUSR1");
			/* NOTREACHED */
		}
		err(errno, "unable to execute '%s'", largv[0]);
		/* NOTREACHED */
	}

	if (waitForJobs) {
		if (waitpid(pid, &status, 0) < 0) {
			err(EXIT_FAILURE, "unable to wait for pid %d", pid);
			/* NOTREACHED */
		}
		e = WEXITSTATUS(status);
	}

	if (freeLargv) {
		int i;
		for (i = 0; i < argc; i++) {
			free(largv[i]);
		}
		free(largv);
	}
	return e;
}

void
usage(FILE *out) {
	(void)fprintf(out,
"Usage: %s [-dhv] [-J replstr] [-n num] [-s num]\n"
"             [command [argument ...]]\n"
"           -J replstr  replace 'replstr' with invocation number in the command\n"
"           -d          don't wait\n"
"           -h          print this help and exit\n"
"           -n num      only repeat command num times\n"
"           -s num      sleep num seconds in between invocations\n"
"           -v          prefix output with invocation number\n", __progname);
}

int
repeat(char **argv, int argc) {
	int num = 1;
	int rval = 0;

	while (count < 0 || num <= count) {
		rval = execCommand(argv, argc, num);
		if ((sleepSeconds > 0) && (num != count)) {
			sleep(sleepSeconds);
		}
		num++;
	}

	if (!waitForJobs) {
		while (wait(NULL) >0);
	}

	return rval;
}

char **
replaceNumInArgs(char **argv, int argc, int num) {
	int i;
	char **largv;

	if (!JFlag) {
		return NULL;
	}

	if ((largv = calloc(argc + 1, sizeof(char *))) == NULL) {
		err(EXIT_FAILURE, "unable to calloc");
		/* NOTREACHED */
	}

	for (i = 0; argv[i] != NULL; i++) {
		largv[i] = replaceNum(argv[i], num);
	}
	return largv;
}

/* based on xargs/strnsubst.c */
char *
replaceNum(char *input, int num) {
	assert(input != NULL);
	assert(replstr != NULL);

	char *newstr, *numstr, *ptr;

	int ndigits = 1;
	if (num > 9) {
		ndigits = floor(log10(num)) + 1;
	}

	if ((numstr = calloc(ndigits, sizeof(char))) == NULL) {
		err(EXIT_FAILURE, "unable to calloc");
		/* NOTREACHED */
	}
	(void)snprintf(numstr, ndigits + 1, "%d", num);

	/* Not always exact, but always enough. */
	int newlen = strlen(input) + ndigits + 1;
	if ((newstr = malloc(newlen)) == NULL) {
		err(EXIT_FAILURE, "unable to malloc");
		/* NOTREACHED */
	}
	bzero(newstr, newlen);

	for (;;) {
		if ((ptr = strstr(input, replstr)) == NULL) {
			(void)strncat(newstr, input, strlen(input));
			break;
		}
		(void)strncat(newstr, input, ptr - input);
		(void)strncat(newstr, numstr, ndigits);
		input = ptr + strlen(replstr);
	}
	return newstr;
}
