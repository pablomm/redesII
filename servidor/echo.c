
#include "redes2.h"
#include <signal.h>
 #include <ctype.h>

#define BUFFER_ECHO 100
#define PUERTO_ECHO 1235
#define MAX_QUEUE 10


void manejadorGestor_SIGINT(int sig){
	int i;

	/* Cerramos descriptores abiertos */
	for(i=0; i < getdtablesize(); i++)
		close(i);

	exit(EXIT_SUCCESS);
}



void upper(char* toUpper){
    int i;

    /* Control de errores */
    if(toUpper[0]=='\0'){
        return;
    }
    for(i=0; toUpper[i] != '\0';i++){
        toUpper[i] = toupper(toUpper[i]);
    }
}



int procesarCliente(int descr){
	char buffer[BUFFER_ECHO] = {0};

	if (recv(descr, buffer, BUFFER_ECHO,0) <= 0)
		return -1;

	/* Data read. */
	fprintf (stderr, "Descriptor %d: %s", descr, buffer);

	upper(buffer);
	send(descr, buffer, strlen(buffer), 0);

	return 0;
}


int main(void){


	int sckt, desc;
	int i;
	fd_set active_fd_set, read_fd_set;


	abrirLog(LOG_DEBUG);
	/* daemonizar(LOG_DEBUG); */

	if(signal(SIGINT,manejadorGestor_SIGINT) == SIG_ERR){
		syslog(LOG_ERR, "Error en llamada a select");
		exit(EXIT_FAILURE);
    }



	/* Creamos socket */
	sckt = crearSocketTCP(PUERTO_ECHO, MAX_QUEUE);
	if(sckt < 0){
		exit(EXIT_FAILURE);
	}

	/* Inicializamos conjunto de sockets */
	FD_ZERO(&active_fd_set);
  	FD_SET(sckt, &active_fd_set);

	while(1){

		/* Igualamos sockets leidos a los activos */
		read_fd_set = active_fd_set;

		/* LLamada select bloqueante */
		if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0){
          syslog(LOG_ERR, "Error en llamada a select");
          exit (EXIT_FAILURE);
    	}

      	/* Bucle para procesar sockets pendientes */
      	for (i = 0; i < FD_SETSIZE; ++i)

			/* Solo procesamos sockets marcados como tal */
		    if (FD_ISSET(i,&read_fd_set)){

				/* Caso conexion nueva */		        
				if (i == sckt){
		            
					/* Aceptamos nueva conexion */
					desc = aceptarConexion(sckt);
		            if (desc < 0){
		                syslog(LOG_ERR, "No se ha podido procesar nueva conexion");
		            }

					/* Incluimos nuevo descriptor en el conjunto de descriptores */
		            FD_SET (desc, &active_fd_set);
				
				/* Caso ya aceptado */		       
				} else {
		            /* Data arriving on an already-connected socket. */
		         	if (procesarCliente(i) < 0) {
		                close (i);
		                FD_CLR (i, &active_fd_set);
					}
		          }
		      }
			}

	exit(EXIT_SUCCESS);
}
