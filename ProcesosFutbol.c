#include <stdio.h>          /* printf()                 			   */
#include <stdlib.h>         /* exit(),								   */
#include <sys/types.h>      /* key_t, sem_t, pid_t      			   */
#include <sys/shm.h>        /* shmget, shmat, shmctl     			   */
#include <sys/sem.h>        /* semget, semop, semctl				   */
#include <stdbool.h>		/* bool vars				               */
#include <time.h>			/* recursos de tiempo sleep(s)             */
#include <sys/wait.h>		/* wait(NULL)      						   */
#include <unistd.h>			/* fork()								   */


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
	partido->B.equipo = 'B';	 // Settea el caracter 'B'
	partido->b.jug.idP = 0;		 // Proceso del jugador en 0, nadie la tiene
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
	inicializarPartido(partido);
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
				while(true)
				{
					if(partido->running == true)
					{
						//semaforoBola, semaforoCanchaA, semaforoCanchaB
                      	//semaforo_wait();
                      	//semaforo_post();
						semaforo_wait(semaforoBola);
						if(partido->b.jug.idP == player.idP)
						{
							if((char)partido->b.jug.equipo =='A')
							{
								semaforo_post(semaforoBola);
								int i;
								for(i=0;i<3;i++)
								{
									semaforo_wait(semaforoCanchaB);
									if(partido->B.jug.idP == player.idP)
									{
										partido->B.jug.idJ = 0;
										partido->B.jug.idP = 0;
										partido->B.jug.equipo = 0;
										semaforo_post(semaforoCanchaB);

										semaforo_wait(semaforoCanchaA);
										partido->A.anotaciones++; //anotación para el Equipo A
										semaforo_post(semaforoCanchaA);						

                                  		printf("Anotación del equipo %c por el jugador %d\n",player.equipo,player.idJ);
                                  		printf("Marcador: A(%d) - B(%d)\n",partido->A.anotaciones,partido->B.anotaciones);
										break;
									} 
									else
									{

										if(partido->B.jug.idP == 0)//Si la cancha esta vacia
										{
											partido->B.jug.idJ = player.idJ;
											partido->B.jug.idP = player.idP;
											partido->B.jug.equipo = player.equipo;
											semaforo_post(semaforoCanchaB);
											printf("El jugador %d del equipo %c obtuvo la cancha B (Intento %d)\n",player.idJ,player.equipo,i);
											break;
										}
										else 
										{
											semaforo_post(semaforoCanchaB);
											printf("El jugador %d del equipo %c no obtuvo la cancha B. (Intento %d)\n",player.idJ,player.equipo,i);
											sleep(1);
										}
									}
								}
								semaforo_wait(semaforoBola);
								//libera la bola
								partido->b.jug.idJ = 0;
								partido->b.jug.idP = 0;
								partido->b.jug.equipo = 0;
								semaforo_post(semaforoBola);

							} else 
							{ //jugador de equipo b
								semaforo_post(semaforoBola);
								int i;
								for(i=0;i<3;i++)
								{
									semaforo_wait(semaforoCanchaA);
									if(partido->A.jug.idP == player.idP)
									{
										partido->A.jug.idJ = 0;
										partido->A.jug.idP = 0;
										partido->A.jug.equipo = 0;
										semaforo_post(semaforoCanchaA);

										semaforo_wait(semaforoCanchaB);
										partido->B.anotaciones++; //anotación para el Equipo A
										semaforo_post(semaforoCanchaB);						

                                  		printf("Anotación del equipo %c por el jugador %d\n",player.equipo,player.idJ);
                                  		printf("Marcador: A(%d) - B(%d)\n",partido->A.anotaciones,partido->B.anotaciones);
										break;
									} 
									else
									{

										if(partido->A.jug.idP == 0)//Si la cancha esta vacia
										{
											partido->A.jug.idJ = player.idJ;
											partido->A.jug.idP = player.idP;
											partido->A.jug.equipo = player.equipo;
											semaforo_post(semaforoCanchaA);
											printf("El jugador %d del equipo %c obtuvo la cancha A (Intento %d)\n",player.idJ,player.equipo,i);
											break;
										}
										else 
										{
											semaforo_post(semaforoCanchaA);
											printf("El jugador %d del equipo %c no obtuvo la cancha A. (Intento %d)\n",player.idJ,player.equipo,i);
											sleep(1);
										}
									}
								}
								semaforo_wait(semaforoBola);
								//libera la bola
								partido->b.jug.idJ = 0;
								partido->b.jug.idP = 0;
								partido->b.jug.equipo = 0;
								semaforo_post(semaforoBola);
							}

						} /*else if(player.idJ == 6)
						{
							semaforo_post(semaforoBola);
							sleep(randomNumero(5,10));
							
							semaforo_wait(semaforoCanchaA);
							if( partido->A.jug.idJ == 0 )
							{
								partido->A.jug.idJ = player.idJ;
								partido->A.jug.idP = player.idP;
								partido->A.jug.equipo = player.equipo;
								semaforo_post(semaforoCanchaA);

								sleep(3);

								semaforo_wait(semaforoCanchaA);	
								partido->A.jug.idJ = 0;
								partido->A.jug.idP = 0;
								partido->A.jug.equipo = 0;
							}
							semaforo_post(semaforoCanchaA);
							
						} else if(player.idJ == 12)

						{	semaforo_post(semaforoBola);
							sleep(randomNumero(5,10));

							semaforo_wait(semaforoCanchaB);
							if( partido->B.jug.idJ == 0 )
							{
								partido->B.jug.idJ = player.idJ;
								partido->B.jug.idP = player.idP;
								partido->B.jug.equipo = player.equipo;
								semaforo_post(semaforoCanchaB);

								sleep(3);

								semaforo_wait(semaforoCanchaB);
								partido->B.jug.idJ = 0;
								partido->B.jug.idP = 0;
								partido->B.jug.equipo = 0;
							}
							semaforo_post(semaforoCanchaB);
							
						}*/
						else if(partido->b.jug.idP == 0)
						{	
							semaforo_post(semaforoBola);
							//sleep(randomNumero(5,20));

							//semaforo_wait(semaforoBola);
                        	partido->b.jug.idP = player.idP; // Setea el jugador(proceso) a la bola
                        	partido->b.jug.equipo = player.equipo; // Setea el jugador(numero) a la hola
                        	partido->b.jug.idJ = player.idJ; // Setea el jugador(numero) a la bola	
                        	semaforo_post(semaforoBola);
							printf("EL jugador %d obtuvo la bola\n",player.idJ);
							
						}

					}					

				}
				/*
               	while(true){
                	if(partido->running == true){
						sleep(randomNumero(2,3)); //Tiempo de Espera
						//semaforoBola, semaforoCanchaA, semaforoCanchaB
                      	//semaforo_wait();
                      	//semaforo_post();
						if(partido->b.jug.idP == player.idP){ //Si el proceso tenía la bola
                        	int aux;
                          	if((char)partido->b.jug.equipo =='A'){ //Si el jugador es del Equipo A
								//semaforo_wait(semaforoCanchaB);
                              	if(partido->B.jug.idP == player.idP){ //Si el jugador tiene la cancha B
                                  	partido->A.anotaciones++; //anotación para el Equipo A
                                  	printf("Anotación del equipo %c por el jugador %d\n",player.equipo,player.idJ);
                                  	printf("Marcador: A(%d) - B(%d)\n",partido->A.anotaciones,partido->B.anotaciones);
                                  	
                             	}else{ //Si el jugador NO tiene la cancha B
                                  	partido->B.jug.idP = player.idP; //Setea en cancha (proceso) al jugador que la NO la tenía
                                  	aux = partido->B.jug.idJ; //Guarda el que perdío la cancha(numero)		
                                  	partido->B.jug.idJ =player.idJ; //Setea en cancha (numero) al jugador que la NO la tenía
                                  	printf("EL jugador %d le quito la Cancha A a %d\n",player.idJ,aux);	
                             	}
								//semaforo_post(semaforoCanchaB);
                          	}else if((char)partido->b.jug.equipo =='B'){ //Si el jugador es del Equipo B
								//semaforo_wait(semaforoCanchaA);
                              	if(partido->A.jug.idP == player.idP){ //Si el jugador tiene la cancha A
                                  	partido->B.anotaciones++; //anotación para el Equipo B
                                  	printf("Anotación del equipo %c por el jugador %d\n",player.equipo,player.idJ);
                                  	printf("Marcador: A(%d) - B(%d)\n",partido->A.anotaciones,partido->B.anotaciones);
                                  	
                              	}else{  //Si el jugador tiene la cancha B
                                  	partido->A.jug.idP = player.idP;  //Setea la cancha  al jugador(proceso) que la NO la tenía
                                  	aux = partido->A.jug.idJ;	//Guarda el que perdío la cancha (numero)	
                                  	partido->A.jug.idJ =player.idJ; //Setea en cancha al jugador(numero) que la NO la tenía
                                  	printf("EL jugador %d le quito la Cancha B a %d\n",player.idJ,aux);
                                  	
                              	}
								//semaforo_post(semaforoCanchaA);
                          	}
                    	}else{ //Si el proceso NO tenía la bola
							//semaforo_wait(semaforoBola);
                        	partido->b.jug.idP = player.idP; // Setea el jugador(proceso) a la bola
                        	partido->b.jug.equipo = player.equipo; // Setea el jugador(numero) a la hola
                        	int viejo = partido->b.jug.idJ; //Guarda el que perdío la bola (numero)	
                        	partido->b.jug.idJ = player.idJ; // Setea el jugador(numero) a la bola	
                        	printf("EL jugador %d perdió la bola y la obtuvo el jugador %d\n",viejo,player.idJ);
							//semaforo_post(semaforoBola);
                    	}							              		
                   	}
           		}	*/
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
	semaforo_deallocate(semaforoBola);
	semaforo_deallocate(semaforoCanchaA);
	semaforo_deallocate(semaforoCanchaB);
}
