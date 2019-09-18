#include <stdio.h>          /* printf()                 			   */
#include <stdlib.h>         /* exit(),								   */
#include <sys/types.h>      /* key_t, sem_t, pid_t      			   */
#include <sys/shm.h>        /* shmget, shmat, shmctl     			   */
#include <sys/sem.h>        /* semget, semop, semctl				   */
#include <stdbool.h>		/* bool vars				               */
#include <time.h>			/* recursos de tiempo sleep(s)             */
#include <sys/wait.h>		/* wait(NULL)      						   */
#include <unistd.h>			/* fork()								   */
#include <semaphore.h>

void mensajeError(char* errorInfo) {
    fprintf(stderr,"%s",errorInfo);
    exit(1);
};

struct Jugador{
	char equipo; //caracter del Equipo
  	int idP; //numero del proceso
	int idJ; //numero del jugador
	int numPartido; //numero del proceso Padre
};

struct Bola{
	int balon;			//Proceso que tiene la bola
  	struct Jugador jug; //jugador del proceso que la tiene
};

struct Cancha{
    int anotaciones; //cant goles al momento
	char equipo;	 //caracter del Equipo
    struct Jugador jug;	//jugador que posee la cancha
};

struct Partido{ // Segmento Compartido Directo
  	struct Cancha A; // Contiene la Cancha A
  	struct Cancha B; // Contiene la Cancha B
 	struct Bola b;   // Bola del juego
 	bool running;	   // se esta jugando o no 
};

void inicializarPartido(struct Partido *partido){
	partido->running = false; 	 // No ha comenzado el partido
	partido->A.anotaciones = 0;	 // 0 anotaciones del equipo A
	partido->A.equipo = 'A';	 // Settea el caracter 'A'
	partido->B.anotaciones = 0;  // 0 anotaciones del equipo B
	partido->A.jug.idP = 0;
	partido->B.jug.idP = 0;
	partido->B.equipo = 'B';	 // Settea el caracter 'B'
	partido->b.jug.idP = 0;		 // Proceso del jugador en 0, nadie la tiene
	partido->b.jug.idJ = 0;		 // Proceso del jugador en 0, nadie la tiene
	partido->b.jug.equipo = ' '; // Equipo del jugador en '', nadie la tiene
	partido->b.balon = 0;		 // Ningun proceso tiene la bola 	
}

int randomNumero(int min,int max){
	srand((unsigned int)getpid()); //settea el # proceso a la función para que funcione con fork
	int randomm = min + rand () / (RAND_MAX / (max-min+1)+1); // # random
	return randomm;	// retorna el entero
}
/*
bool inicializarContadorTiempo(int min){
	Struct tm *local;
	time_t t;
	t = time(NULL);
	local = localtime(&t);
}
*/

union semaforo {
  	int val;
  	struct semid_ds *buf;
	unsigned short int *array;
	struct seminfo *__buf;
};

int semaforo_allocation(key_t key, int sem_flags){
 	return semget (key, 1, sem_flags);
}

int semaforo_deallocate(int semid)
{
  union semaforo ignored_argument;
  return semctl (semid, 1, IPC_RMID, ignored_argument);
}

int semaforo_inicializacion(int semid){
  	union semaforo argument;
  	unsigned short values[1];
  	values[0] = 1; //Inicializa con un 1
  	argument.array = values;
  	return semctl (semid, 0, SETALL, argument);
}

/*Block until the semaphore value is positive, then decrement it by one.  */
int semaforo_wait (int semid){
  	struct sembuf operations[1];
  	/* Use the first (and only) semaphore.  */
  	operations[0].sem_num = 0;
  	/* Decrement by 1.  */
  	operations[0].sem_op = -1;
  	/* Permit undo'ing.  */
  	operations[0].sem_flg = SEM_UNDO;
  
  	return semop (semid, operations, 1);
}

/* Retorna el valor inmediatamente  */
int semaforo_post (int semid){
  	struct sembuf operations[1];
  	/* Use the first (and only) semaphore.  */
  	operations[0].sem_num = 0;
  	/* Increment by 1.  */
  	operations[0].sem_op = 1;
  	/* Permit undo'ing.  */
  	operations[0].sem_flg = SEM_UNDO;
  	return semop (semid, operations, 1);
}


int main(int argc, char * argv[]) { 
	//Segmento Compartido   
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
	int semaforoBola, semaforoCanchaA, semaforoCanchaB;
  
  	//Creamos un semaforo y damos permisos para compartirlo
  	semaforo_allocation ((key_t)1841, IPC_CREAT | IPC_EXCL | 0666);
  	semaforo_inicializacion (semaforoBola);
	semaforo_allocation ((key_t)1877, IPC_CREAT | IPC_EXCL | 0666);
  	semaforo_inicializacion (semaforoCanchaA);
	semaforo_allocation ((key_t)1871, IPC_CREAT | IPC_EXCL | 0666);
  	semaforo_inicializacion (semaforoCanchaB);
  	
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
				player.idP = getpid();
				player.numPartido = getppid();
				player.idJ = jugadores;

				semaforo_wait(semaforoBola);
				if(jugadores == 12 || jugadores ==6){
					printf("   %c      %d	 %d	   %d	(Portero)\n",player.equipo,player.idJ,player.idP,player.numPartido);
				}else{
					printf("   %c      %d	 %d	   %d	(Jugador)\n",player.equipo,player.idJ,player.idP,player.numPartido);
				}
				semaforo_post(semaforoBola);

				sleep(randomNumero(5,20));
				while(true) //Ciclo que ejecutan los jugadores durante su vida.
				{
					if(partido->running == true) //Avisa a los jugadores cuando se inicio el partido
					{
						semaforo_wait(semaforoBola); //Pide el recurso de la bola
						if(partido->b.jug.idP == player.idP) //Verifica si el jugador tiene control de la bola
						{
							//Esta seccion la realizan los jugadores del equipo A
							if((char)partido->b.jug.equipo == 'A')
							{
								semaforo_post(semaforoBola); //Libera el recurso de la bola
								//Tiene control de la bola, Entonces intenta obtener la cancha B
								int i;
								for(i=1;i<=3;i++) //El jugador tiene 3 oportunidades de obtener la cancha contraria
								{
									semaforo_wait(semaforoCanchaB); //Pide el recurso de la cancha
									if(partido->B.jug.idP == player.idP) //Verifica si el jugador tiene control de la cancha contraria
									{
										//En caso de tener control de la cancha B y ya tenia control del balon
										//Anota y libera la cancha
										partido->B.jug.idJ = 0;
										partido->B.jug.idP = 0;
										partido->B.jug.equipo = 0;
										semaforo_post(semaforoCanchaB); //Devuelve el recurso de la cancha B

										//Anota un gol para el equipo A
										semaforo_wait(semaforoCanchaA);
										partido->A.anotaciones++; //anotación para el Equipo A
										semaforo_post(semaforoCanchaA);						

                                  		printf("Anotación del equipo %c por el jugador %d\n",player.equipo,player.idJ);
                                  		printf("Marcador: A(%d) - B(%d)\n",partido->A.anotaciones,partido->B.anotaciones);
										sleep(randomNumero(5,20));
										break;
									} 									
									else if(partido->B.jug.idP == 0) //Verifica si nadie tiene la cancha
									{
										//En este caso el jugador toma control del recurso
										partido->B.jug.idJ = player.idJ;
										partido->B.jug.idP = player.idP;
										partido->B.jug.equipo = player.equipo;
										semaforo_post(semaforoCanchaB); //Libera el recurso de la cancha B
										printf("El jugador %d del equipo %c obtuvo la cancha B (Intento %d)\n",player.idJ,player.equipo,i);	
										i--;									
										continue;
									} else //En caso de que la cancha se encuentre ocupada, espera un segundo y realiza otro intento
									{
										semaforo_post(semaforoCanchaB);
										printf("El jugador %d del equipo %c no obtuvo la cancha B. (Intento %d \n", player.idJ, player.equipo, i);
										sleep(1);
									}
								}
								//Devuelve la bola
								semaforo_wait(semaforoBola); //Libera el recurso de la bola
								partido->b.jug.idJ = 0;
								partido->b.jug.idP = 0;
								partido->b.jug.equipo = 0;
								semaforo_post(semaforoBola);
								sleep(randomNumero(5,20));

							//Esta seccion la realizan los jugadores del equipo B

							}else if((char)partido->b.jug.equipo == 'B')
							{ 	
								semaforo_post(semaforoBola); //Libera el recurso de la bola
								//Tiene control de la bola, Entonces intenta obtener la cancha A
								int i;
								for(i=1;i<=3;i++) //El jugador tiene 3 oportunidades de obtener la cancha contraria
								{
									semaforo_wait(semaforoCanchaA);
									if(partido->A.jug.idP == player.idP) //Verifica si el jugador tiene control de la cancha contraria
									{
										//En caso de tener control de la cancha A y ya tenia control del balon
										//Anota y libera la cancha
										partido->A.jug.idJ = 0;
										partido->A.jug.idP = 0;
										partido->A.jug.equipo = 0;
										semaforo_post(semaforoCanchaA); 

										//Anota un gol para el equipo B
										semaforo_wait(semaforoCanchaB);
										partido->B.anotaciones++; //anotación para el Equipo B
										semaforo_post(semaforoCanchaB);						

                                  		printf("Anotación del equipo %c por el jugador %d\n",player.equipo,player.idJ);
                                  		printf("Marcador: A(%d) - B(%d)\n",partido->A.anotaciones,partido->B.anotaciones);
										sleep(randomNumero(5,20));
										break;
									} 
									else 
									{
										if(partido->A.jug.idP == 0) //Verifica si nadie tiene la cancha
										{
											//En este caso el jugador toma control del recurso
											partido->A.jug.idJ = player.idJ;
											partido->A.jug.idP = player.idP;
											partido->A.jug.equipo = player.equipo;
											semaforo_post(semaforoCanchaA); //Libera el recurso de la cancha B
											printf("El jugador %d del equipo %c obtuvo la cancha A (Intento %d)\n",player.idJ,player.equipo,i);	
											i--;									
											continue;
										}
										else //En caso de que la cancha se encuentre ocupada, espera un segundo y realiza otro intento
										{
											semaforo_post(semaforoCanchaA);
											printf("El jugador %d del equipo %c no obtuvo la cancha A. (Intento %d)\n",player.idJ,player.equipo,i);
											sleep(1);
										}
									}
								}
								//Luego de anotar o perder los 3 intentos de anotar, el jugador libera la bola
								partido->b.jug.idJ = 0;
								partido->b.jug.idP = 0;
								partido->b.jug.equipo = 0;
								semaforo_post(semaforoBola);
								sleep(randomNumero(5,20));
							}

						}else if(player.idJ == 6) //Esta seccion es la realiza el portero del equipo A
						{
							semaforo_post(semaforoBola); //Libera la bola							
							semaforo_wait(semaforoCanchaA); //Pide la cancha A
							if( partido->A.jug.idJ == 0 ) //Verifica si nadie tiene el recurso
							{
								//En este caso el portero toma el recurso
								partido->A.jug.idJ = player.idJ;
								partido->A.jug.idP = player.idP;
								partido->A.jug.equipo = player.equipo;
								semaforo_post(semaforoCanchaA); //Libera la cancha A
								printf("EL portero %d obtuvo la cancha A\n",player.idJ);
								sleep(3); //Espera 3 segundos manteniendo la cancha
								semaforo_wait(semaforoCanchaA);	
								//Libera la cancha
								partido->A.jug.idJ = 0;
								partido->A.jug.idP = 0;
								partido->A.jug.equipo = 0; 
								printf("EL portero %d soltó la cancha A\n",player.idJ);
							}
							semaforo_post(semaforoCanchaA);
							sleep(randomNumero(5,20)); //Vuelve a esperar antes de intentar tomar la cancha
							
						} else if(player.idJ == 12) //Esta seccion es la realiza el portero del equipo B

						{	semaforo_post(semaforoBola); //Libera la bola
							semaforo_wait(semaforoCanchaB); //Pide la cancha B
							if( partido->B.jug.idJ == 0 ) //Verifica si nadie tiene el recurso
							{	
								//En este caso el portero toma el recurso
								partido->B.jug.idJ = player.idJ; 
								partido->B.jug.idP = player.idP;
								partido->B.jug.equipo = player.equipo;
								semaforo_post(semaforoCanchaB); //Libera la cancha B
								printf("EL portero %d obtuvo la cancha B\n",player.idJ);
								sleep(3); //Espera 3 segundos manteniendo la cancha
								semaforo_wait(semaforoCanchaB);
								//Libera la cancha
								partido->B.jug.idJ = 0;
								partido->B.jug.idP = 0;
								partido->B.jug.equipo = 0;
								printf("EL portero %d soltó la cancha B\n",player.idJ);
							}
							semaforo_post(semaforoCanchaB); //Libera la cancha B
							sleep(randomNumero(5,20));
						}
						else if(partido->b.jug.idP == 0)
						{	
							semaforo_post(semaforoBola);

							semaforo_wait(semaforoBola);
							//Toma el recurso
                        	partido->b.jug.idP = player.idP; 
                        	partido->b.jug.equipo = player.equipo; 
                        	partido->b.jug.idJ = player.idJ; 	
                        	semaforo_post(semaforoBola);
							printf("EL jugador %d obtuvo la bola\n",player.idJ);
							sleep(1);
							
						}
						else
						{
							semaforo_post(semaforoBola);
							sleep(randomNumero(5,20));
						}						
					}					
				}
			}
		}
		sleep(1);
		printf("...Inicia el partido...");
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
	semaforo_deallocate(semaforoBola);
	semaforo_deallocate(semaforoCanchaA);
	semaforo_deallocate(semaforoCanchaB);
}