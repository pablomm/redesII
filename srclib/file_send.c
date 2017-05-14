/**
 * @file file_send.c
 * @brief Funciones para el envio de archivos
 * @author Pablo Marcos  <pablo.marcos@estudiante.uam.es>
 * @author Dionisio Perez  <dionisio.perez@estudiante.uam.es>
*/

/**
 * @defgroup EnviarIRC EnviarIRC
 *
 * <hr>
 */
#include <redes2/irc.h>
#include <redes2/ircxchat.h>

#include "../includes/cliente.h"
#include "../includes/red_servidor.h"
#include "../includes/red_cliente.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <inttypes.h>

int socket_envio_archivo;
int desc_envio_archivo;	
int socket_recv_archivo;
FILE *fp_recv;
pthread_t t_aux;

/**
 * @addtogroup EnviarIRC
 * Comprende funciones para el envio de archivos 
 *
 * <hr>
 */

/**
 * @ingroup EnviarIRC
 *
 * @brief Funcion para cerrar conexion de envio
 *
 * @synopsis
 * @code
 *	#include <file_send.h>
 *
 * 	void cerrar_envio(void)
 * @endcode
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
void cerrar_envio(void){
	char mensaje[TAM_LINE] = {0};

	if(cliente.envio == ENVIO_COMPLETADO){
			snprintf(mensaje, TAM_LINE -1, SEND_COMPLETED_MSG, envio->filename);
			IRCInterface_ErrorDialogThread(mensaje);
	} else if(cliente.envio == ESPERANDO_ACCEPT){
		IRCInterface_ErrorDialogThread(N_ACCEPT_FILE_MSG);
	} else {
		IRCInterface_ErrorDialogThread(CANCEL_SEND_MSG);
	}

	if(desc_envio_archivo)
		close(desc_envio_archivo);
	if(socket_envio_archivo)
		close(socket_envio_archivo);

	if(t_aux != 0)
		pthread_kill(t_aux, SIGUSR1);

	IRC_MFree(4, &envio->filename,&envio->data, &envio->nick, &envio);
	cliente.file_thread_th = 0;
	cliente.envio = DISPONIBLE;
}

/**
 * @ingroup EnviarIRC
 *
 * @brief Manejador de señal SIGALRM
 *
 * @synopsis
 * @code
 *	#include <file_send.h>
 *
 * 	void manejador_aceptar(int sig)
 * @endcode
 *
 * @param[in] sig señal a manejar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
void manejador_aceptar(int sig){
	(void) sig;
	cerrar_envio();
	pthread_exit(NULL);
}

/**
 * @ingroup EnviarIRC
 *
 * @brief Manejador de señal SIGUSR1
 *
 * @synopsis
 * @code
 *	#include <file_send.h>
 *
 * 	void manejador_timeout(int sig)
 * @endcode
 *
 * @param[in] sig señal a manejar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
void manejador_timeout(int sig){
	(void) sig;
	t_aux = 0;
	pthread_exit(NULL);
}

/**
 * @ingroup EnviarIRC
 *
 * @brief Manejador de señal SIGUSR2
 *
 * @synopsis
 * @code
 *	#include <file_send.h>
 *
 * 	void manejador_fcancel(int sig)
 * @endcode
 *
 * @param[in] sig señal a manejar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
void manejador_fcancel(int sig){
	(void) sig;

	cerrar_envio();
	pthread_exit(NULL);
}

/**
 * @ingroup EnviarIRC
 *
 * @brief Funcion que controla el timeout hasta la acepcion del envio de ficheros
 *
 * @synopsis
 * @code
 *	#include <file_send.h>
 *
 * 	void timeout_aceptar(void *id_thread)
 * @endcode
 *
 * @param[in] id_thread identificador del hilo de control
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
void * timeout_aceptar(void *id_thread){

	pthread_detach(pthread_self());
	signal(SIGUSR1, manejador_timeout);
	sleep(TIMEOUT_ACCEPT);
	pthread_kill(*((pthread_t *)id_thread), SIGALRM);
	t_aux = 0;
	pthread_exit(NULL);
}

/**
 * @ingroup EnviarIRC
 *
 * @brief Funcion que controla el envio de ficheros
 *
 * @synopsis
 * @code
 *	#include <file_send.h>
 *
 * 	void file_send_func(void * envioVoid)
 * @endcode
 *
 * @param[in] envioVoid estructura con la informacion del envio
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
void * file_send_func(void * envioVoid){

	unsigned long i;
	size_t tam_pack;
	char *mensaje;
	char comando[COMANDO_LEN] = {0};
	char *ip_local;
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);
	struct ifreq ifr;
	pthread_t self_id;

	envio = (pEnvio) envioVoid;

	socket_envio_archivo = 0;
	desc_envio_archivo = 0;
	t_aux = 0;

	self_id = pthread_self();

	pthread_detach(self_id);

	/* Manejador recepcion de FCANCEL */
	if(signal(SIGUSR2, manejador_fcancel) == SIG_ERR){
		cerrar_envio();
		pthread_exit(NULL);
	}

	/* Ponemos a 0 el numero de puerto para elegirlo aleatoriamente */
	if(crearSocketTCP(&socket_envio_archivo, 0) == RED_ERROR){
		cerrar_envio();
		pthread_exit(NULL);

	}

	/* Sacamos el puerto que se ha elegido */
	if(getsockname(socket_envio_archivo,(struct sockaddr*) &addr, &addrlen) == -1){
		cerrar_envio();
		pthread_exit(NULL);
	}

	/* Obtenemos IP local */
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, interfaz, IFNAMSIZ-1);
	ioctl(socket_envio_archivo, SIOCGIFADDR, &ifr);
	ip_local = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);

	/* Enviamos FSEND */
	snprintf(comando, COMANDO_LEN-1, "\001FSEND %s %s %" PRIu16 " %lu", envio->filename, ip_local, ntohs(addr.sin_port), envio->length);
	IRCMsg_Privmsg(&mensaje, cliente.prefijo, envio->nick,comando);
	enviar_clienteThread(mensaje);
	free(mensaje);

	/* Manejador aceptar */
	if(signal(SIGALRM, manejador_aceptar) == SIG_ERR){
		cerrar_envio();
		pthread_exit(NULL);
	}

	/* Lanzamos hilo para el timeout del accept */
	pthread_create(&t_aux, NULL, timeout_aceptar, &self_id);

	cliente.envio = ESPERANDO_ACCEPT;

	/* Aceptar conexion entrante */
	if(aceptarConexion(socket_envio_archivo, &desc_envio_archivo, &addr) == RED_ERROR){
		cerrar_envio();
		pthread_exit(NULL);
	}

	/* Ignoramos señal de timeout de aceptar */
	if(signal(SIGALRM, SIG_IGN) == SIG_ERR){
		cerrar_envio();
		pthread_exit(NULL);
	}

	/* Matamos timeout de aceptar */
	pthread_kill(t_aux, SIGUSR1);

	cliente.envio = ENVIANDO_ARCHIVO;
	
	/* Como diria Ignatius: los paquetitos!! */ 
	for(i=0; i< envio->length; i += TAM_PACKAGE){
		/* El ultimo paquete tendra tamaño menor que TAM_PACKAGE */
		tam_pack = (envio->length - i < TAM_PACKAGE)?(envio->length - i):TAM_PACKAGE;

		if(send(desc_envio_archivo, &(envio->data[i]), tam_pack, 0) == - 1){
			cerrar_envio();
			pthread_exit(NULL);
		}
	}

	cliente.envio = ENVIO_COMPLETADO;
	cerrar_envio();

	pthread_exit(NULL);
}

/**
 * @ingroup EnviarIRC
 *
 * @brief Funcion para cerrar conexion de recepcion de ficheros
 *
 * @synopsis
 * @code
 *	#include <file_send.h>
 *
 * 	void cerrar_recepcion(void)
 * @endcode
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
void cerrar_recepcion(void){
	char mensaje[TAM_LINE];
	
	switch(cliente.recepcion){
		case ENVIO_COMPLETADO:
			snprintf(mensaje, TAM_LINE -1, SEND_COMPLETED_MSG, recepcion?recepcion->filename:"");
			IRCInterface_ErrorDialogThread(mensaje);
			break;
		case ESPERANDO_ACCEPT:
			IRCInterface_ErrorDialogThread(ERR_CON_RECV_MSG);
			break;
		default:
			IRCInterface_ErrorDialogThread(ERR_RECV_MSG);
	}

	if(socket_recv_archivo) {
		close(socket_recv_archivo);
		socket_recv_archivo = 0;
	}

	if(fp_recv){
		fclose(fp_recv);
		fp_recv = NULL;
	}

	IRC_MFree(3, &(recepcion->filename), &(recepcion->ip), &(recepcion));
	cliente.recepcion = DISPONIBLE;
}

/**
 * @ingroup EnviarIRC
 *
 * @brief Funcion que controla la recepcion de ficheros
 *
 * @synopsis
 * @code
 *	#include <file_send.h>
 *
 * 	void file_recv_func(void * recvVoid)
 * @endcode
 *
 * @param[in] recvVoid estructura con la informacion para recibir envio
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
void * file_recv_func(void * recvVoid){

	char buffer[TAM_PACKAGE] = {0};
	char destino[TAM_DEST] = {0};
	unsigned long recibido = 0;
	ssize_t recv_ret = 0;
	recepcion = (pRecepcion) recvVoid;
	socket_recv_archivo = 0;
	fp_recv = NULL;

	pthread_detach(pthread_self());

	/* Creamos archivo donde recibir */
	snprintf(destino, TAM_DEST,"%s/%s", RECV_FILE, recepcion->filename);

	if(!(fp_recv = fopen(destino,"w"))){
		cerrar_recepcion();
		pthread_exit(NULL);	
	}
	
	/* Establecemos conexion con otro cliente */
	cliente.recepcion = ESPERANDO_ACCEPT;
	if(crearSocketTCP_cliente(&socket_recv_archivo,recepcion->ip, recepcion->puerto) == RED_ERROR){
		cerrar_recepcion();
		pthread_exit(NULL);
	}

	cliente.envio = ENVIANDO_ARCHIVO;

	/* Recibimos los paquetitos */
	for(recibido = 0; recibido < recepcion->len; recibido += recv_ret){
		recv_ret = recv(socket_recv_archivo, buffer, TAM_PACKAGE, 0);
		if(recv_ret <= 0){
			cerrar_recepcion();
			pthread_exit(NULL);	
		}
        fwrite(buffer, sizeof(char), recv_ret, fp_recv);
	}

	
	cliente.recepcion = ENVIO_COMPLETADO;

	cerrar_recepcion();
	pthread_exit(NULL);
}

/**
 * @ingroup EnviarIRC
 *
 * @brief Funcion para sustituir en el nombre de archivo espacios en blanco por barras bajas
 *
 * @synopsis
 * @code
 *	#include <file_send.h>
 *
 * 	void normalize_file(char* f)
 * @endcode
 *
 * @param[in] f archivo a modificar su nombre
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
void normalize_file(char* f){

	while(*f){
		if(*f == ' ') *f = '_';
		f++;
	}
}







