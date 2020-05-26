#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <sys/mman.h>


void child_function(int child_num);


#define FILE_SIZE (1024*1024)
#define N 4
#define BUFFER_SIZE 1024

sem_t * proc_sem;
int fd;


int main() {
	int res;
	char * file_name = "output.txt";

	// open the file
	fd = open(file_name,
				  O_CREAT | O_TRUNC | O_WRONLY,
				  S_IRUSR | S_IWUSR
				 );
	if(fd == -1){
		perror("open()\n");
		exit(1);
	}

	// set the file size
	res = ftruncate(fd, 100);
	if(res == -1){
		perror("ftruncate()\n");
		exit(1);
	}

	proc_sem = mmap(NULL, // NULL: è il kernel a scegliere l'indirizzo
			sizeof(sem_t), // dimensione della memory map
			PROT_READ | PROT_WRITE, // memory map leggibile e scrivibile
			MAP_SHARED | MAP_ANONYMOUS, // memory map condivisibile con altri processi e senza file di appoggio
			-1,
			0); // offset nel file

	res = sem_init(proc_sem, // zona di memoria dove mettere il semaforo
		1, //1 => il semaforo è condiviso tra processi, in caso fosse zero è condiviso tra thread
		1  // valore iniziale del semaforo
		);
	if(res == -1){
		perror("sem_init()\n");
		exit(1);
	}

	pid_t sons[N];
	for(int i=0 ; i<N ; i++){
		sons[i] = fork();
		switch(sons[i]){
			case -1:
				perror("fork()\n");
				exit(1);
			case 0:
				child_function(i);
				exit(0);
		}
	}
	// here there is only the father



	// wait all the sons
	for(int i=0 ; i<N ; i++){
		res = wait(NULL);
		if(res == -1){
			perror("wait()\n");
			exit(1);
		}
	}

	// destroy the semaphore
	res = sem_destroy(proc_sem);
	if(res == -1){
		perror("sem_destroy()\n");
		exit(1);
	}

	close(fd);

	exit(0);
}

void child_function(int child_num){


	if (sem_wait(proc_sem) == -1) {
		perror("sem_wait");
		exit(EXIT_FAILURE);
	}
	printf("TEST0\n");

	char buffer[BUFFER_SIZE];
	int bytes_read;
	while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0) {
		printf("TEST1\n");
		for(int i=0 ; i<bytes_read ; i++){
			printf("TEST\n");
			if(buffer[i] == '\0'){
				printf("TEST2\n");
				char ch = 'A'+ child_num;
				int res = write(fd, &ch, 1);
				if(res == -1){
					perror("write()\n");
					exit(1);
				}


				break;
			}
		}
	}


	if (sem_post(proc_sem) == -1) {
		perror("sem_post");
		exit(EXIT_FAILURE);
	}



}
