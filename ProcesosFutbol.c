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
#include<string.h>

void mensajeError(char* errorInfo) {
    fprintf(stderr,"%s",errorInfo);
    exit(1);
};

struct Jugador{
	char equipo;
  	int idP; //numero del proceso
	int idJ; //numero del jugador
	int numPartido;
};

struct Bola{
  	int balon;
  	struct Jugador jug;
};

struct Cancha{
    int anotaciones;
	char equipo;
    struct Jugador jug;
};

struct Partido{
  struct Cancha A;
  struct Cancha B;
  struct Bola b;
  bool running;
};

void acceder(struct Partido *partido, struct Jugador player){
	if(partido->b.jug.idP == player.idP){
		int aux;
		if((char)partido->b.jug.equipo =='A'){
			if(partido->B.jug.idP == player.idP){
				partido->A.anotaciones++;
				printf("Anotación del equipo %c por el jugador %d\n",player.equipo,player.idJ);
				printf("Marcador: A(%d) - B(%d)\n",partido->A.anotaciones,partido->B.anotaciones);
			}else{
				partido->B.jug.idP = player.idP;
				aux = partido->B.jug.idJ;		
				partido->B.jug.idJ =player.idJ;
				printf("EL jugador %d le quito la Cancha A a %d\n",player.idJ,aux);
			}
		}else if((char)partido->b.jug.equipo =='B'){ //Equipo B
			if(partido->A.jug.idP == player.idP){
				partido->B.anotaciones++;
				printf("Anotación del equipo %c por el jugador %d\n",player.equipo,player.idJ);
				printf("Marcador: A(%d) - B(%d)\n",partido->A.anotaciones,partido->B.anotaciones);
			}else{
				partido->A.jug.idP = player.idP;
				aux = partido->A.jug.idJ;		
				partido->A.jug.idJ =player.idJ;
				printf("EL jugador %d le quito la Cancha B a %d\n",player.idJ,aux);
			}
		}
	}else{
		partido->b.jug.idP = player.idP;
		partido->b.jug.equipo = player.equipo;
		int viejo = partido->b.jug.idJ;
		partido->b.jug.idJ = player.idJ;
		printf("EL jugador %d perdió la bola y la obtuvo el jugador %d\n",viejo,player.idJ);
	}
}

void inicializarPartido(struct Partido *partido){
	partido->running = false;
	partido->A.anotaciones = 0;
	partido->A.equipo = 'A';
	partido->B.anotaciones = 0;
	partido->B.equipo = 'B';
	partido->b.balon = 0;
	partido->b.jug.idP = 0;
	partido->b.jug.equipo = ' ';
}

int randomNumero(int min,int max){
	srand(time(NULL));
	int randomm = min + rand () / (RAND_MAX / (max-min+1)+1);
	//printf("%d\n",randomm);
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
	char buffer[BUFSIZ];
	int shmid;
	void *shared_memory = (void *)0;
  	struct Partido *partido; 
	srand((unsigned int)getpid()); 
    shmid = shmget((key_t)1947, sizeof(struct Partido), 0666 | IPC_CREAT);
    if (shmid == -1) {
          mensajeError("Error : semget\n");
          exit(EXIT_FAILURE);
    }
  	
	shared_memory = shmat(shmid, (void *)0, 0);
  
    if (shared_memory == (void *)-1 ) {
		mensajeError("Error : shmat\n");
		exit(EXIT_FAILURE);
	}
	partido =(struct Partido*)shared_memory;
	
	int pid;
	int jugadores = 0;
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
  
  	printf("Número del Partido # %d \n",getpid());
	printf("   Lista de Jugadores\n");
	printf("Equipo Jugador	IDJUgador #Partido	\n");	
	inicializarPartido(partido);
	struct Jugador player;
	pid=fork();
	//Creamos los jugadores
    if (pid < 0) {
        fprintf(stderr,"Jugador no se ha creado, se cancela el partido...");
        return 1;
    }else if (pid == 0) {
      	exit(0);
    }else{
        while(jugadores++ < 12){
			pid=fork();
            if (pid < 0) {
            	fprintf(stderr,"Jugador no se ha creado, se cancela el partido...");
                return 1;
            }else if (pid == 0) { //Hijos nuevos
				if(jugadores < 7){
					player.equipo = 'A';
				}else{
					player.equipo = 'B';
				}
				// Porteros
				if(jugadores == 6){
					partido->A.jug.idP = getpid();
					partido->A.jug.idJ = 6;
				}else if(jugadores == 12){
					partido->B.jug.idP = getpid();
					partido->B.jug.idJ = 12;
				}
				player.idP = getpid();
				player.numPartido = getppid();
				player.idJ = jugadores;
				partido->b.jug.idP =  jugadores;
				printf("   %c      %d	 %d	   %d\n",player.equipo,player.idJ,player.idP,player.numPartido);
               	while(true){
                   	if(partido->running == true){
                       	//P()
						sleep(1);
						acceder(partido,player);
						acceder(partido,player);
						acceder(partido,player);
						sleep(1);
						exit(0);
	          			//                   		
                   	}
           		}	
			}
		}
		sleep(1);
		partido->running = true;
		wait(NULL);
	}
	wait(NULL);
	//Liberacion del segmento compartido
  	if (shmdt(shared_memory) == -1) {
		fprintf(stderr, "shmdt failed\n");
		exit(EXIT_FAILURE);
	}
 	//Liberacion del semaforo
    if ((semctl(semaforoColaPartido, 0, IPC_RMID)) == -1 && (semctl(semaforoMutex, 0, IPC_RMID)) == -1) {
        perror(NULL);
        mensajeError("Semaforo borrando\n");
    }
}
