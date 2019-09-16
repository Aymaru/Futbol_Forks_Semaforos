#include <stdio.h>          /* printf()                 */
#include <stdlib.h>         /* exit(), malloc(), free() */
#include <sys/types.h>      /* key_t, sem_t, pid_t      */
#include <sys/shm.h>        /* shmat(), IPC_RMID        */
#include <errno.h>          /* errno, ECHILD            */
#include <semaphore.h>      /* sem_open(), sem_destroy(), sem_wait().. */
#include <fcntl.h>          /* O_CREAT, O_EXEC          */
#include <stdbool.h>
#include <time.h>
#include <sys/wait.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/sem.h>

void mensajeError(char* errorInfo) {
    fprintf(stderr,"%s",errorInfo);
    exit(1);
};

struct Jugador{
	char equipo;
  	int id;
};

struct Bola{
  	int balon;
  	struct Jugador jug;
};

struct Cancha{
    int anotaciones;
    struct Jugador jug;
};

struct Partido{
  struct Jugador A;
  struct Jugador B;
  struct Bola b;
  int running;
};


int randomNumero(){
	srand(time(NULL));
	int randomm = 5 + rand () / (RAND_MAX / (20-5+1)+1);
	printf("%d\n",randomm);
	return randomm;
}
/*
bool inicializarContadorTiempo(int min){
	Struct tm *local;
	time_t t;
	t = time(NULL);
	local = localtime(&t);
}
*/

void V(int semid) {
    struct sembuf sops; //Signal
    sops.sem_op = 1;
    sops.sem_flg = 0;

    if (semop(semid, &sops, 1) == -1) {
        perror(NULL);
        mensajeError("Error al hacer Signal");
    }
}

void P(int semid) {
    struct sembuf sops;
    sops.sem_op = -1; /* ... un wait (resto 1) */
    sops.sem_flg = 0;

    if (semop(semid, &sops, 1) == -1) {
        perror(NULL);
        mensajeError("Error al hacer el Wait");
    }
}
void initSem(int semid, int valor) { //iniciar un semaforo
  
    if (semctl(semid, 0, SETVAL, valor) < 0) {        
    	perror(NULL);
        mensajeError("Error iniciando semaforo");
    }
}

int main(int argc, char * argv[]) {
	
	return contadorTiempo(5);

	int running = 1;
	void *shared_memory = (void *)0;
	struct memoriaCompartida * shared_stuff;
	int shmid; 
	srand((unsigned int)getpid());

	shmid = shmget((key_t)1234, sizeof(struct memoriaCompartida), 0666 | IPC_CREAT);
	if (shmid == -1) {
		fprintf(stderr, "Error la Cancha y la bola no fué compartida\n");
		//exit(EXIT_FAILURE);
	}
	shared_memory = shmat(shmid, (void *)0, 0);
	if (shared_memory == (void *)-1) {
		fprintf(stderr, "shmat failed\n");
		//exit(EXIT_FAILURE);
	}
	//printf("Memory attached at %X\n", (int)shared_memory);

	//Segmento de cola de mensajes

	
	int pid;
	int jugadores = 1 ;
	int semaforoColaPartido, semaforoMutex;
  
  	//Creamos un semaforo y damos permisos para compartirlo
    semaforoColaPartido=semget(IPC_PRIVATE,1,IPC_CREAT | 0700);
    semaforoMutex=semget(IPC_PRIVATE,1,IPC_CREAT | 0700);

    //Creamos un semaforo y damos permisos para compartirlo
    if(semaforoColaPartido<0 && semaforoMutex<0) {
        perror(NULL);
        mensajeError("Semaforo: semget");
    }
  
	initSem(semaforoColaPartido,1);
	initSem(semaforoMutex,1);
  
  	//P(mutex)
  	//P(P)
	pid=fork();
	//Creamos los jugadores
	while(jugadores++ < 10){
		if (pid < 0) {
			fprintf(stderr,"Jugador no se ha creado, se cancela el partido...");
			return 1;
		}else if (pid == 0) {
			printf("   %d 	%d     %d\n",jugadores,getpid(),getppid());
			listaDeJugadores[jugadores] = getppid();
			exit (0);
		}else{
			pid=fork();
			wait(NULL);
		}
	}

  	if (shmdt(shared_memory) == -1) {
		fprintf(stderr, "shmdt failed\n");
		exit(EXIT_FAILURE);
	}
 	//Liberacion del semaforo
    if ((semctl(semaforoColaPartido, 0, IPC_RMID)) == -1 && (semctl(semaforoMutex, 0, IPC_RMID)) == -1) {
        perror(NULL);
        mensajeError("Semaforo borrando");
    }
}