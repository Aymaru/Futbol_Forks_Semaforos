#include "shareInfo.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <time.h>


int randomNumero(){
	srand(time(NULL));
	int randomm = 5 + rand () / (RAND_MAX / (20-5+1)+1);
	printf("%d\n",randomm);
	return randomm;
}

bool contadorTiempo(int min){
	int segundos, minutos;
    segundos = minutos = 0;
    while(true){
		printf("%d'  %d \n", minutos, segundos);
        if(segundos == 60) {
			minutos++;
			segundos = 0;
		}
        if(minutos == min) {
			return true;
		}
        segundos++;
        sleep(1);
    }
    return 0;
}

int main(int argc, char * argv[]) {
	//SEgmento de Memoria Compatida
	int running = 1;
	void *shared_memory = (void *)0;
	struct memoriaCompartida * shared_stuff;
	int shmid; 
	srand((unsigned int)getpid());

	//shmid = shmget((key_t)1234, sizeof(struct memoriaCompartida), 0666 | IPC_CREAT);
	shmid = 163841;
	printf("Shmid : %d",shmid);
	//ipcs -m Ver el status de los segmentos compartidos existenes
	//ipcs -s Ver el status de los semaforos
	//ipcs -q Ver el status de la cola de mensages 
	if (shmid == -1) {
		fprintf(stderr, "Error la Cancha y la bola no fué compartida\n");
		exit(EXIT_FAILURE);
	}
	shared_memory = shmat(shmid, (void *)0, 0);
	if (shared_memory == (void *)-1) {
		fprintf(stderr, "shmat failed\n");
		exit(EXIT_FAILURE);
	}
	//printf("Memory attached at %X\n", (int)shared_memory);
	//Segmento de semaforos

	//Segmento de cola de mensajes

	
	int pid;
	int jugadores = 0 ;
	int listaDeJugadores[10];
	
	printf("Número del Partido # %d \n",getpid());
	printf("   Lista de Jugadores\n");
	printf("Jugador	IDJUgador #Paritdo	\n");
	pid=fork();
	//Creamos los jugadores
	while(jugadores++ < 10){
		if (pid < 0) {
			fprintf(stderr,"Jugador no se ha creado, se cancela el partido...");
			return 1;
		}else if (pid == 0) {
			printf("   %d 	%d      %d\n",jugadores,getpid(),getppid());
			listaDeJugadores[jugadores] = getppid();
			exit (0);
		}else{
			pid=fork();
			wait(NULL);
		}
	}
	wait(NULL);
}
