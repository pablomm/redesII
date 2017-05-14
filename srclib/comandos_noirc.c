/**
 * @file comandos_noirc.c
 * @brief Funciones para el parseo de mensajes de envio de ficheros y audio
 * @author Pablo Marcos  <pablo.marcos@estudiante.uam.es>
 * @author Dionisio Perez  <dionisio.perez@estudiante.uam.es>
*/

/**
 * @defgroup ParseNoIRC ParseNoIRC
 *
 * <hr>
 */
#include "../includes/cliente.h"
#include "../includes/comandos_noirc.h"
#include "../includes/file_send.h"
#include "../includes/red_cliente.h"
#include "../includes/audiochat.h"

#include <redes2/ircxchat.h>
#include <redes2/irc.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <inttypes.h>

/**
 * @addtogroup ParseNoIRC
 * Comprende funciones para el parseo de mensajes de envio de ficheros y audio
 *
 * <hr>
 */

/**
 * @ingroup ParseNoIRC
 *
 * @brief Funcion de parseo
 *
 * @synopsis
 * @code
 * 	void parse_noIRC(char * comando, char *nick)
 * @endcode
 *
 * @param[in] comando comando a procesar
 * @param[in] nick nick del usuario
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
void parse_noIRC(char * comando, char *nick){

	char type[11] = {0};
	sscanf(comando,"%10s", type);


	if(!strcmp(type,"FSEND")){
		parse_fsend(comando,nick);
	}
	if(!strcmp(type,"FCANCEL")){
		parse_fcancel(comando,nick);
	}

	if(!strcmp(type,"AUDIOCHAT")){
		parse_audiochat(comando,nick);
	}

	if(!strcmp(type,"AUDIOEXIT")){
		parse_audioexit(comando,nick);
	}

}

/**
 * @ingroup ParseNoIRC
 *
 * @brief Funcion de parseo del fsend
 *
 * @synopsis
 * @code
 * 	void parse_fsend(char * comando, char *nick)
 * @endcode
 *
 * @param[in] comando comando a procesar
 * @param[in] nick nick del usuario
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
void parse_fsend(char * comando, char *nick){
	char type[11] = {0};
	char ip[31] = {0};
	char filename[101] = {0};
	char *mensaje = NULL;
	unsigned short puerto;
	unsigned long len;
	pthread_t taux;
	pRecepcion recepcion;

	 /*FSEND NOMBRE_FICHERO HOSTNAME_EMISOR PUERTO_EMISOR */	
	sscanf(comando,"%10s %100s %30s %hu %lu",type, filename, ip, &puerto, &len);


	if(cliente.recepcion == DISPONIBLE){
	
		if(!IRCInterface_ReceiveDialogThread(nick,filename)){
			IRCMsg_Privmsg(&mensaje, cliente.prefijo, nick, FCANCEL);
			enviar_clienteThread(mensaje);
			free(mensaje);

			return;
		}

		recepcion = (pRecepcion) calloc(1, sizeof(Recepcion));
		asprintf(&(recepcion->filename), "%s", filename);
		asprintf(&(recepcion->ip), "%s", ip);
		recepcion->puerto = puerto;
		recepcion->len = len;
		cliente.recepcion = OCUPADO;

		if(pthread_create(&taux, NULL, file_recv_func, recepcion) == -1){
			cliente.recepcion = DISPONIBLE;
			IRC_MFree(3, &(recepcion->filename), &(recepcion->ip), &recepcion);
		}
	}
}

/**
 * @ingroup ParseNoIRC
 *
 * @brief Funcion de parseo del fcancel
 *
 * @synopsis
 * @code
 * 	void parse_fcancel(char * comando, char *nick)
 * @endcode
 *
 * @param[in] comando comando a procesar
 * @param[in] nick nick del usuario
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
void parse_fcancel(char * comando, char *nick){

	(void) comando;

	/* Solo aceptamos fcancel que provengan del mismo nick que el envio */
	if(cliente.envio == ESPERANDO_ACCEPT || cliente.envio == ENVIANDO_ARCHIVO){
		/* Un poco excesivo pero quien sabe... */
		if(envio){
			if(envio->nick){
				if(!strcmp(envio->nick, nick)){
					if(cliente.file_thread_th != 0)
						pthread_kill(cliente.file_thread_th , SIGUSR2);
				}
			}
		}
	}
}

/**
 * @ingroup ParseNoIRC
 *
 * @brief Funcion de parseo del audioexit
 *
 * @synopsis
 * @code
 * 	void parse_audioexit(char * comando, char *nick)
 * @endcode
 *
 * @param[in] comando comando a procesar
 * @param[in] nick nick del usuario
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 * 
 *<hr>
*/
void parse_audioexit(char * comando, char *nick){

	(void) comando;
	(void) nick;
	if(cliente.audio_send_thread_th != 0){
		pthread_kill(cliente.audio_send_thread_th, SIGUSR1);
	}

}

/**
 * @ingroup ParseNoIRC
 *
 * @brief Funcion de parseo del audiochat
 *
 * @synopsis
 * @code
 * 	void parse_audiochat(char * comando, char *nick)
 * @endcode
 *
 * @param[in] comando comando a procesar
 * @param[in] nick nick del usuario
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 * 
 *<hr>
*/
void parse_audiochat(char * comando, char *nick){

	char *mensaje = NULL;
	char audiochat[10] = {0}, ip[100] = {0};
	unsigned short puerto;
	pAudioenvio audioenv;

	if(cliente.audiochatsend != DISPONIBLE){
		IRCMsg_Privmsg(&mensaje, cliente.prefijo, nick,"\001AUDIOCANCEL");
		enviar_cliente(mensaje);
		free(mensaje);
		return;
	}

	if(!IRCInterface_ReceiveDialogThread(nick,"Conversacion de audio")){
			IRCMsg_Privmsg(&mensaje, cliente.prefijo, nick, FCANCEL);
			enviar_clienteThread(mensaje);
			free(mensaje);
			return;
		}

	cliente.audiochatsend = OCUPADO;
	sscanf(comando,"%10s %100s %hu",audiochat, ip, &puerto);

	audioenv = (pAudioenvio) calloc(1, sizeof(Audioenvio));
	audioenv->ip = calloc(strlen(ip) + 1, sizeof(char));
	strcpy(audioenv->ip, ip);
	audioenv->puerto = puerto;
	audioenv->nick = calloc(strlen(ip) + 1, sizeof(char));
	strcpy(audioenv->nick, nick);

	if(pthread_create(&cliente.audio_send_thread_th, NULL, audiochatSend, audioenv) < 0){
		cliente.audiochatsend = DISPONIBLE;
		free(audioenv);
		return;
	}

}

