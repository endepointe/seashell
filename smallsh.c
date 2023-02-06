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

//const char *const sys_errlist[];
//int sys_nerr;

int errno;
int setenv(const char*,const char*,int);
int create_process(char**,size_t);


static int 
initialize() {
	char* ps1 = "$";
	char* ifs = "IFS";
	if (setenv(ps1,"$ \0",1) < 0) return -1;
	if (setenv(ifs," \t\n",1) < 0) return -1;
	return getpid();	
}

void
check_unwaited_background_processes(pid_t *pgid) {
	pid_t child_pid, waited_pid;
	int status;
	child_pid = getpgrp();
	if (child_pid == *pgid) {
		do {
			waited_pid = waitpid(child_pid, &status, WNOHANG);
			// exited status
			if (WIFEXITED(status) != 0) {
				fprintf(stderr, "Child process %d done. Exit status %d.\n", child_pid, status);
			}
			// signaled status
			if (WIFSIGNALED(status) != 0) {
				fprintf(stderr, "Child process %d done. Signaled %d.\n", child_pid, status);
			}
			// stopped	
			if (WIFSTOPPED(status) != 0) {
				fprintf(stderr, "Child process %d stopped. Continuing.\n", child_pid);
			}
		} while (waited_pid > 0);
	}
	/*
	// exited status
	if (WIFEXITED(status) != 0) {
		fprintf(stderr, "Child process %d done. Exit status %d.\n", child_pid, status);
	}
	// signaled status
	if (WIFSIGNALED(status) != 0) {
		fprintf(stderr, "Child process %d done. Signaled %d.\n", child_pid, status);
	}
	// stopped	
	if (WIFSTOPPED(status) != 0) {
		fprintf(stderr, "Child process %d stopped. Continuing.\n", child_pid);
	}
	*/
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
	sprintf(str, "%d", num);
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

int
split_line(char* line) {
	char** tokens = NULL;
	size_t token_count = 0;
	char* saveptr = NULL;
	saveptr = strtok(line, " ");
	while (saveptr) {
		tokens = (char**)realloc(tokens, (token_count + 1) * sizeof(char*));
		char* str = match_expansions(saveptr);
		tokens[token_count] = (char*)malloc(strlen(str) + 1);
		strcpy(tokens[token_count], str);
		token_count++;
		saveptr = strtok(NULL, " ");
	}	
	create_process(tokens,token_count);
	return 0;
}

int 
create_process(char **args,size_t size) {
	for (int i = 0; i < size; i++) {
		fputs(args[i],stdout);
	}
	//char *cmd = malloc(sizeof(char)*strlen("/bin/")+1);
	//strcpy(cmd,"/bin/");
	//strcat(cmd, args[0]);
	//char *newargv[] = {cmd, "-al", NULL};
	/*
	int child_status;
	pid_t spawnpid = fork();
	int ret = -1;
	switch (spawnpid) {
		case -1:
			perror("fork() failed");
			break;
		case 0:
			fputs("\t\nchild executing process\t\n",stdout);
			fflush(stdout);
			execlp(args[0], "-al",NULL);
			//execv(newargv[0], newargv);
			perror("execv");
			break;
		default:
			fputs("\t\nspawnpid is the child so parent executing\t\n",stdout);
			fflush(stdout);
			//spawnpid = waitpid(spawnpid,&child_status, 0);
			break;
	}
	fputs("\t\nexecuting by both parent and child pids\t\n",stdout);
	return spawnpid;
	*/

	return 0;
}

int
main(int argc, char *argv[])
{
	pid_t smallsh_pgid = getpgid(initialize());
	pid_t *pgid_ptr = &smallsh_pgid;
	fprintf(stdout,"smallsh pid: %d\n",*pgid_ptr);
	fflush(stdout);
	if (argc == 1) {
		char *line = NULL;
		size_t len = 0;
		do {
			fputs(getenv("$"), stdout);
			ssize_t nread = getline(&line, &len, stdin);
			//printf("%d | %d\n",nread, get_line_length(line));
			if (nread < 0) return errno;
			split_line(line);
			if (strstr(line, "exit") != NULL || strcmp(line, "exit\n") == 0) {
				printf("%d\n",4);
				break;
			}
			fflush(stdout);
			check_unwaited_background_processes(pgid_ptr);
		} while (1);
		free(line);
		return 0;
	}
	const char *msg = "Too many arguments...\0";
	fputs(msg,stdout);
	fflush(stdout);
	return 1;

}
