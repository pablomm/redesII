
#include "../includes/red_servidor.h"
#include "../includes/config.h"


int main(int argc, char * argv[]){

	int sckt, desc, auxdesc;
	int i;
	fd_set active_fd_set, read_fd_set;
	ptread_t hiloAux;

	/* Recepcion de argumentos */




	/* deomonizamos / no si flag -h */
	abrirLog(LOG_DEBUG);
	/* daemonizar(LOG_DEBUG); */

	/* montamos manejadores */


	/* Creamos socket TCP */
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
          	break;
    	}

      	/* Bucle para procesar sockets pendientes */
      	for (i = 0; i < FD_SETSIZE; ++i){

			/* Solo procesamos sockets marcados */
		    if (FD_ISSET(i,&read_fd_set)){

				/* Caso conexion nueva */		        
				if (i == sckt){

					/* Aceptamos conexion */
		            if ((desc = aceptarConexion(sckt))  < 0){
		                syslog(LOG_WARNING, "No se ha podido procesar nueva conexion");
		            }

					/* Creamos estructuras nueva conexion */
					if(pthread_create(&hiloAux , NULL, nuevaConexion, (void *) &desc)  < 0 ){
						syslog(LOG_WARNING, "No se ha podido crear hilo con nuevaConexion()");

					}
					/* Incluimos nuevo descriptor */
		            FD_SET (desc, &active_fd_set);
				
				/* Caso ya aceptado */		       
				} else {
		            /* Crea un hilo para procesar el mensaje */
					/* hay que usar semaforo ?? */
		         	if (pthread_create(&hiloAux , NULL, procesarMensaje, (void *) &i)  < 0) {

		                if(close(i) < 0){
 							syslog(LOG_WARNING, "No se ha podido cerrar conexion");
						}
		                FD_CLR (i, &active_fd_set);
					}
				}
			}
		}	
	}

		/* liberar lo que sea */

 	closelog(void);
	/* liberarEstructuras(void); */

	exit(EXIT_SUCCESS);
}

