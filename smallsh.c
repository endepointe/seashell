#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

// set errorcodes for debugging
void test(void*,void*) {
#ifdef DBG
#endif
}

int errno;
pid_t getpgid(pid_t);
int setenv(const char*,const char*,int);
int create_process(char**);
static int t = 0;

static int 
initialize() {
	char* ps1 = "$";
	char* ifs = "IFS";
	if (setenv(ps1,"$ \0",1) < 0) return -1;
	if (setenv(ifs," \t\n",1) < 0) return -1;
	return getpid();	
}

void
check_unwaited_background_processes(pid_t *pgid, int *status) {
	pid_t gpid, waited_pid;
	//int status;
	gpid = getpgrp();
	//if (gpid == *pgid) {
		do {
			waited_pid = waitpid(-1, status, WNOHANG);
			// exited status
			if (WIFEXITED(*status)) {
				fprintf(stderr, "Child process %d done. Exit status %d.\n", getpid(), WEXITSTATUS(*status));
			}
			// signaled status
			if (WIFSIGNALED(*status)) {
				fprintf(stderr, "Child process %d done. Signaled %d.\n", getpid(), *status);
				exit(*status);
			}
			// stopped	
			if (WIFSTOPPED(*status)) {
				fprintf(stderr, "Child process %d stopped. Continuing.\n", getpid());
			}
		} while (waited_pid > 0);
	//}
}

int
get_line_length(char* line) {
	char* ch = line;
	ssize_t count = 0;
	while (*ch != '\0') {ch++;count++;}
	return count;
}

char*
n_to_c(size_t num) {
	char* str = (char*)malloc(10*sizeof(char));
	sprintf(str, "%d\n", num);
	return str;
}

char*
match_expansions(char* str) {
	if (strstr(str,"~/") != NULL) {
		char *s = malloc(sizeof(char) * (strlen(getenv("HOME")) + strlen(str) + 1));  
		str++;
		strcpy(s,getenv("HOME"));
		strcat(s,str);
		return s;
	}
	if (strcmp(str,"$$") == 0) {return n_to_c(getpid());}
	if (strcmp(str,"$?") == 0) {/*exit status of the waited for command. see 6. Waiting*/}
	if (strcmp(str,"$!") == 0) {/*the updated pid of the child process*/}
	return str;
}

char**
split_line(char* line) {
	char** tokens = NULL;
	size_t token_count = 0;
	char* saveptr = NULL;
	saveptr = strtok(line, " \n\t");
	while (saveptr) {
		tokens = (char**)realloc(tokens, (token_count + 1) * sizeof(char*));
		char* str = match_expansions(saveptr);
		tokens[token_count] = (char*)malloc(strlen(str) + 1);
		strcpy(tokens[token_count], str);
		token_count++;
		saveptr = strtok(NULL, " \n\t");
	}	
	tokens = (char**)realloc(tokens, (token_count + 1) * sizeof(char*));
	tokens[token_count] = NULL;
	return tokens;
}

int
create_process(char **args) {
	if (strcmp(args[0],"exit") == 0) {
		return -1;
	}
	pid_t cpid, w;
	int wstatus;
	int test = -7;
	int *ptest = &test;
	int tstatus;
	cpid = fork();

	switch (cpid) {
		case -1:
			fprintf(stderr,"fork() failed on %d\n"),getpid();
			wstatus = -1;
			break;
		case 0:
			*ptest = 77;
			printf("The cpid %d test value is %d\n", cpid, test);
			fflush(stdout);
			if (strstr("/",args[0]) == NULL) {
				execvp(args[0],args);	
			}
			execv(args[0],args);
			break;
		default:
			printf("The ppid %d test value is %d\n", cpid, test);
			do {
				w = waitpid(cpid,&wstatus,WUNTRACED);
				//w = waitpid(cpid,&wstatus,WUNTRACED);
				// wait for any process
				//w = waitpid(-1, &wstatus, 0);
				//w = waitpid(-1, &wstatus, WNOHANG);
				//w = waitpid(cpid, &wstatus, 0);
				if (w == -1) {
					break;
				}
				if (WIFEXITED(wstatus)) {
					printf("exited, status=%d\n", WEXITSTATUS(wstatus));
				} else if (WIFSIGNALED(wstatus)) {
					printf("killed by signal %d\n", WTERMSIG(wstatus));
				} else if (WIFSTOPPED(wstatus)) {
					printf("stopped by signal %d\n", WSTOPSIG(wstatus));
				} else if (WIFCONTINUED(wstatus)) {
					printf("continued\n");
				}
			} while (w > 0);
			break;
	}
	return wstatus;
}



int
main(int argc, char *argv[])
{
	pid_t smallsh_pgid = getpgid(initialize());
	pid_t *pgid_ptr = &smallsh_pgid;
	fprintf(stdout,"smallsh pgid: %d\n",*pgid_ptr);
	fflush(stdout);
	if (argc == 1) {
		char *line = NULL;
		size_t len = 0;
		do {
			fputs(getenv("$"), stdout);
			ssize_t nread = getline(&line, &len, stdin);
			if (nread < 0) return errno;
			if (nread == 1 && strcmp(line,"\n") == 0) continue;
			char **tokens = split_line(line);
			int status = create_process(tokens);
			if (status < 0) {
				free(tokens);
				break;
			}
			free(tokens);
			check_unwaited_background_processes(pgid_ptr,&status);
		} while (1);
		free(line);
		return 0;
	}
	const char *msg = "Too many arguments...\0";
	fputs(msg,stdout);
	fflush(stdout);
	return 1;

}
