/**
 * @file red_cliente.c
 * @brief crear sockets y enviar referido al cliente
 * @author Pablo Marcos  <pablo.marcos@estudiante.uam.es>
 * @author Dionisio Perez  <dionisio.perez@estudiante.uam.es>
 */

/**
 * @defgroup RedClienteIRC RedClienteIRC
 *
 * <hr>
 */
	

#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <syslog.h>
#include <stdlib.h>
#include <string.h>
#include <redes2/ircxchat.h>

#include "../includes/ssl.h"
#include "../includes/red_cliente.h"

/**
 * @addtogroup RedClienteIRC
 * Comprende funciones para creacion de sockets y envio referido al cliente
 *
 * <hr>
 */

/**
 * @ingroup RedClienteIRC
 *
 * @brief crea un socket TCP
 *
 * @synopsis
 * @code
 * 	status crearSocketTCP_cliente(int *sckfd, char *server, unsigned short port)
 * @endcode
 *
 * @param[in] sckfd el socket
 * @param[in] server nombre del servidor o direccion IPV4
 * @param[in] port el puerto
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
 */
status crearSocketTCP_cliente(int *sckfd, char *server, unsigned short port){

    struct sockaddr_in addr;
    struct hostent* host;

    /* Creamos socket TCP */
    if ((*sckfd=socket(AF_INET, SOCK_STREAM, 0)) == -1 ) {  
        return RED_ERROR;
    }
	/* Obtenemos IP del servidor */
    if((host = gethostbyname(server)) == NULL) {
        return RED_ERROR;
    }

    addr.sin_family = AF_INET;         
    addr.sin_port = htons(port); 
    memcpy(&addr.sin_addr.s_addr, (char *)host->h_addr , host->h_length); 
    memset(&addr.sin_zero,0,8); 
	/* Nos conectamos al servidor */
    if(connect(*sckfd, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
        return RED_ERROR;
    }
	/* Informacion de debugeo */
	syslog(LOG_DEBUG,"Socket tcp cliente conectado correctamente");
	return RED_OK;
}

/**
 * @ingroup RedClienteIRC
 *
 * @brief envia mensaje desde el socket especificado
 *
 * @synopsis
 * @code
 * 	status enviar_cliente(char *mensaje)
 * @endcode
 *
 * @param[in] mensaje el mensaje
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
 */
status enviar_cliente(char *mensaje){
	if(cliente.socket < 0 || mensaje==NULL){
		syslog(LOG_INFO,"Error en el envio de paquete");
		return RED_ERROR;
	}

	if(ssl_active) {
		enviar_datos_SSL(cliente.socket, mensaje, strlen(mensaje));
	} else {
		send(cliente.socket, mensaje, strlen(mensaje), 0);
	}

	IRCInterface_PlaneRegisterOutMessage(mensaje);

	return RED_OK;
}

/**
 * @ingroup RedClienteIRC
 *
 * @brief envia mensaje desde el socket especificado (funcion de hilos)
 *
 * @synopsis
 * @code
 * 	status enviar_clienteThread(char *mensaje)
 * @endcode
 *
 * @param[in] mensaje el mensaje
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
 */
status enviar_clienteThread(char *mensaje){
	if(cliente.socket < 0 || mensaje==NULL){
		syslog(LOG_INFO,"Error en el envio de paquete");
		return RED_ERROR;
	}

	pthread_mutex_lock(&cliente.envio_thread_mutex);

	if(ssl_active) {
		enviar_datos_SSL(cliente.socket, mensaje, strlen(mensaje));
	} else {
		send(cliente.socket, mensaje, strlen(mensaje), 0);
	}


	pthread_mutex_unlock(&cliente.envio_thread_mutex);

	IRCInterface_PlaneRegisterOutMessageThread(mensaje);

	return RED_OK;
}

/**
 * @ingroup RedClienteIRC
 *
 * @brief Crea un socket UDP
 *
 * @synopsis
 * @code
 * 	status socketUDP(int *sckfd, short puerto)
 * @endcode
 *
 * @param[in] sckfd el socket
 * @param[in] puerto el puerto
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
 */
status socketUDP(int *sckfd, short puerto){
   struct sockaddr_in direccion;

    socklen_t len = sizeof(direccion);

    if((*sckfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		return RED_ERROR;
	}

	/* Inicializamos estructura sockaddr_in */
	direccion.sin_family = AF_INET;  /* Socket de red */
	direccion.sin_port = htons(puerto); /* Puerto asociado */
	direccion.sin_addr.s_addr = INADDR_ANY; /* Cualquier direccion */
	memset((void *)&(direccion.sin_zero), 0, 8);

    if(bind(*sckfd, (struct sockaddr *)&direccion, len)<0){
		close(*sckfd);
		return RED_ERROR;
	}

	return RED_OK;
}

/**
 * @ingroup RedClienteIRC
 *
 * @brief Crea un socket TCP 
 *
 * @synopsis
 * @code
 * 	int crearSocketTCP_cliente_aux(int *sckfd, char *server, unsigned short port)
 * @endcode
 *
 * @param[in] sckfd el socket
 * @param[in] server servidor del que obtener direccin IP
 * @param[in] port el puerto
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
 */
int crearSocketTCP_cliente_aux(int *sckfd, char *server, unsigned short port){

    struct sockaddr_in addr;
    struct hostent* host;

    /* Creamos socket TCP */
    if ((*sckfd=socket(AF_INET, SOCK_STREAM, 0)) == -1 ) {  
        return -1;
    }
	/* Obtenemos IP del servidor */
    if((host = gethostbyname(server)) == NULL) {
        return -1;
    }

    addr.sin_family = AF_INET;         
    addr.sin_port = htons(port); 
    memcpy(&addr.sin_addr.s_addr, (char *)host->h_addr , host->h_length); 
    memset(&addr.sin_zero,0,8); 
	/* Nos conectamos al servidor */
    if(connect(*sckfd, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
        return -1;
    }
	return 0;
}










