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
}

struct memoriaCompartidaCanchas {
	int canchaA;
	int canchaB;
};

struct memoriaCompartidaBola {
	bool bola;
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
void initSem(int semid, int numSem, int valor) { //iniciar un semaforo
  
    if (semctl(semid, numSem, SETVAL, valor) < 0) {        
    	perror(NULL);
        mensajeError("Error iniciando semaforo");
    }
}

int main(int argc, char * argv[]) {

	void *shared_memory1 = (void *)0;
	void *shared_memory2 = (void *)0;
	struct memoriaCompartidaCanchas *shared_canchas;
	struct memoriaCompartidaBola *shared_bola;
	char buffer[BUFSIZ];
	srand((unsigned int)getpid());

	//Creamos un semaforo y damos permisos para compartirlo
	int shBola, shCancha;
	shBola = shmget((key_t)1234, sizeof(struct memoriaCompartidaCanchas), 0666 | IPC_CREAT);
	shCancha = shmget((key_t)1234, sizeof(struct memoriaCompartidaBola), 0666 | IPC_CREAT);

	if (shBola == -1 && shCancha == -1) {
		mensajeError("Error : semget\n");
		exit(EXIT_FAILURE);
	}
	shared_memory1 = shmat(shBola, (void *)0, 0);
	shared_memory2 = shmat(shCancha, (void *)0, 0);
	if (shared_memory1 == (void *)-1 && shared_memory2 == (void *)-1) {
		mensajeError("Error : shmat\n");
		exit(EXIT_FAILURE);
	}
	//printf("Memory attached at %X\n", (int)shared_memory1);
	//printf("Memory attached at %X\n", (int)shared_memory2);

	int pid;
	int jugadores = 1 ;
	int semaforoColaPartido, semaforoMutex;
    semaforoColaPartido=semget(IPC_PRIVATE,1,IPC_CREAT | 0700);
    semaforoMutex=semget(IPC_PRIVATE,1,IPC_CREAT | 0700);

    //Creamos un semaforo y damos permisos para compartirlo
    if(semaforoColaPartido<0 && semaforoMutex<0) {
        perror(NULL);
        mensajeError("Semaforo: semget");
    }

	initSem(semaforoColaPartido,0,1);
	initSem(semaforoMutex,0,1);
  
  
	printf("Número del Partido # %d \n",getpid());
	printf("   Lista de Jugadores\n");
	printf("Jugador	IDJUgador #Paritdo	\n");
  
  
  	//P(mutex)
  	//P(P)
	pid=fork();
	//Creamos los jugadores
    if (pid < 0) {
        	fprintf(stderr,"Jugador no se ha creado, se cancela el partido...");
        	return 1;
    	}else if (pid == 0) {
      		//P()
        	printf("   %d 	%d     %d\n",jugadores,getpid(),getppid());
      		//
      		exit(0);
    	}else{
        	while(jugadores++ < 12){
				pid=fork();
               	if (pid < 0) {
             		fprintf(stderr,"Jugador no se ha creado, se cancela el partido...");
                    return 1;
                }else if (pid == 0) {
                    //
              	    //P()
                    printf("   %d 	%d     %d\n",jugadores,getpid(),getppid());
                    //
                   	exit(0);
               }else{
                  
					wait(NULL);
               }
			}
      
		//V(P)
        wait(NULL);
	}
/*
 * 
 * Lector()	{
		//	bloquearse	si	hay	algún	escritor	trabajando
		//	bloquearse	si:	nescritores	>	0
		P	(	mutex	);
		while	(	p	)	{	
				contadorLectores++;
				V	(	mutex	);
				P	(	colaLectores	);
				P	(	mutex	);
		}
		nlectores++;
		V	(	mutex	);
        
        
		P	(	mutex	);
		nlectores--;
		if	(	nlectores	==	0	)	{
				if	(	contadorEscritores	>	0	)	{
						contadorEscritores--;
						V	(	colaEscritores	);
				}
		}
		V	(	mutex	);
}

*/  

  	if (shmdt(shared_memory1) == -1 && shmdt(shared_memory2) == -1) {
		fprintf(stderr, "shmdt failed\n");
		exit(EXIT_FAILURE);
	}
}