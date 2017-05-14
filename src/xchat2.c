/**
 * @file xchat2.c
 * @brief funciones referidas al uso de la interfaz xchat2
 * @author Pablo Marcos  <pablo.marcos@estudiante.uam.es>
 * @author Dionisio Perez  <dionisio.perez@estudiante.uam.es>
*/

/** 
 * @defgroup IRCInterface Interface
 *
 */

/** 
 * @defgroup IRCInterfaceCallbacks Callbaks
 * @ingroup IRCInterface
 *
 */

/** 
 * @addtogroup IRCInterfaceCallbacks
 * Funciones que van a ser llamadas desde el interface y que deben ser implementadas por el usuario.
 * Todas estas funciones pertenecen al hilo del interfaz.
 *
 * El programador puede, por supuesto, separar todas estas funciones en múltiples ficheros a
 * efectos de desarrollo y modularización.
 *
 * <hr>
 */

#include <redes2/ircxchat.h>
#include <redes2/irc.h>

#include "../includes/red_cliente.h"
#include "../includes/cliente.h"
#include "../includes/irc_cliente.h"
#include "../includes/file_send.h"
#include "../includes/comandos_cliente.h"
#include "../includes/audiochat.h"
#include "../includes/ssl.h"

#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <inttypes.h>
#include <getopt.h>

#define PATH_CLIENT_CERT "certs/cliente.pem"
#define PATH_CLIENT_PKEY "certs/clientkeypri.pem"

/**
 * @ingroup IRCInterface
 *
 * @brief Imprime por stderr la ayuda de usuario
 *
 * @synopsis
 * @code
 *  void usage(void)
 * @endcode
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
void usage(void){
	fprintf(stderr, "usage: [-h -i <interface>]\n");
	fprintf(stderr, " -i <interface> Fuerza una interfaz utilizada para el envio de audio y ficheros\n");
	fprintf(stderr, " -h Muestra la ayuda y finaliza la ejecucion del programa\n");
}

/**
 * \ingroup IRCInterface
 *
 * @brief Funcion para crear un socket TCP para el cliente echo
 *
 * @synopsis
 * @code
 * 	status inicializar_cliente(char *nick, char*realname, char *user, char *server, int port)
 * @endcode
 *
 * @param[in] nick nick del usuario
 * @param[in] realname realname del usuario
 * @param[in] user campo user del usuario
 * @param[in] server servidor desde el que se conecta
 * @param[in] port puerto desde el que se conecta
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/

status inicializar_cliente(char *nick, char*realname, char *user, char *server, int port){

	/* Guardamos datos del usuario */
	cliente.nick = (char *) calloc(strlen(nick), sizeof(char));
	strcpy(cliente.nick, nick);
	cliente.realname = (char *) calloc(strlen(realname), sizeof(char));
	strcpy(cliente.realname, realname);
	cliente.user = (char *) calloc(strlen(user), sizeof(char));
	strcpy(cliente.user, user);
	cliente.server = (char *) calloc(strlen(server), sizeof(char));
	strcpy(cliente.server, server);
	cliente.puerto = port;
	cliente.envio = DISPONIBLE;
	cliente.file_thread_th = 0;
	cliente.recepcion = DISPONIBLE;
	cliente.file_recv_thread_th = 0;
	pthread_mutex_init(&cliente.envio_thread_mutex , NULL);
	envio = NULL;
	recepcion = NULL;
	cliente.audiochatsend = DISPONIBLE;
	cliente.audio_send_thread_th = 0;
	cliente.audiochatrecv = DISPONIBLE;
	cliente.audio_recv_thread_th = 0;

	/* Lanzamos el hilo de recepcion */	
	if(pthread_create(&cliente.recv_thread_th, NULL, recv_pthread, (void*) &cliente.socket) != 0){
		return NO_CONECTADO;
	}

	inicializar_comandos_cliente();

	return CONECTADO;
}

/**
 * @ingroup IRCInterfaceCallbacks
 *
 * @page IRCInterface_ActivateChannelKey IRCInterface_ActivateChannelKey
 *
 * @brief Llamada por el botón de activación de la clave del canal.
 *
 * @synopsis
 * @code
 *	#include <redes2/ircxchat.h>
 *
 * 	void IRCInterface_ActivateChannelKey (char *channel, char * key)
 * @endcode
 * 
 * @description 
 * Llamada por el botón de activación de la clave del canal. El segundo parámetro es
 * la clave del canal que se desea poner. Si es NULL deberá impedirse la activación
 * con la función implementada a tal efecto. En cualquier caso sólo se puede realizar si el servidor acepta la orden.
 * Las strings recibidas no deben ser manipuladas por el programador, sólo leídas.
 *
 * @param[in] channel canal sobre el que se va a activar la clave.
 * @param[in] key clave para el canal indicado.
 *
 * @warning Esta función debe ser implementada por el alumno.
 *
 * @author
 * Eloy Anguiano (eloy.anguiano@uam.es)
 *
 *<hr>
*/
 
void IRCInterface_ActivateChannelKey(char *channel, char *key)
{
	char *mensaje = NULL;
	IRCMsg_Mode(&mensaje, cliente.prefijo, channel, "+k", key);
	enviar_cliente(mensaje);
	free(mensaje);

}

/**
 * @ingroup IRCInterfaceCallbacks
 *
 * @page IRCInterface_ActivateExternalMessages IRCInterface_ActivateExternalMessages
 *
 * @brief Llamada por el botón de activación de la recepción de mensajes externos.
 *
 * @synopsis
 * @code
 *	#include <redes2/ircxchat.h>
 *
 * 	void IRCInterface_ActivateExternalMessages (char *channel)
 * @endcode
 * 
 * @description 
 * Llamada por el botón de activación de la recepción de mensajes externos.
 * 
 * En cualquier caso sólo se puede realizar si el servidor acepta la orden.
 * La string recibida no debe ser manipulada por el programador, sólo leída.
 *
 * @param[in] channel canal sobre el que se activará la recepción de mensajes externos.
 *
 * @warning Esta función debe ser implementada por el alumno.
 *
 * @author
 * Eloy Anguiano (eloy.anguiano@uam.es)
 *
 *<hr>
*/
 
void IRCInterface_ActivateExternalMessages(char *channel)
{
	char *mensaje = NULL;
	IRCMsg_Mode(&mensaje, cliente.prefijo, channel, "+n", NULL);
	enviar_cliente(mensaje);
	free(mensaje);
}

/**
 * @ingroup IRCInterfaceCallbacks
 *
 * @page IRCInterface_ActivateInvite IRCInterface_ActivateInvite
 *
 * @brief Llamada por el botón de activación de canal de sólo invitación.
 *
 * @synopsis
 * @code
 *	#include <redes2/ircxchat.h>
 *
 * 	void IRCInterface_ActivateInvite (char *channel)
 * @endcode
 * 
 * @description 
 * Llamada por el botón de activación de canal de sólo invitación.
 * 
 * En cualquier caso sólo se puede realizar si el servidor acepta la orden.
 * La string recibida no debe ser manipulada por el programador, sólo leída.
 *
 * @param[in] channel canal sobre el que se activará la invitación.
 *
 * @warning Esta función debe ser implementada por el alumno.
 *
 * @author
 * Eloy Anguiano (eloy.anguiano@uam.es)
 *
 *<hr>
*/
 
void IRCInterface_ActivateInvite(char *channel)
{
	char *mensaje = NULL;
	IRCMsg_Mode(&mensaje, cliente.prefijo, channel, "+i", NULL);
	enviar_cliente(mensaje);
	free(mensaje);
}

/**
 * @ingroup IRCInterfaceCallbacks
 *
 * @page IRCInterface_ActivateModerated IRCInterface_ActivateModerated
 *
 * @brief Llamada por el botón de activación de la moderación del canal.
 *
 * @synopsis
 * @code
 *	#include <redes2/ircxchat.h>
 *
 * 	void IRCInterface_ActivateModerated (char *channel)
 * @endcode
 * 
 * @description 
 * Llamada por el botón de activación de la moderación del canal.
 * 
 * En cualquier caso sólo se puede realizar si el servidor acepta la orden.
 * La string recibida no debe ser manipulada por el programador, sólo leída.
 *
 * @param[in] channel canal sobre el que se activará la moderación.
 *
 * @warning Esta función debe ser implementada por el alumno.
 *
 * @author
 * Eloy Anguiano (eloy.anguiano@uam.es)
 *
 *<hr>
*/
 
void IRCInterface_ActivateModerated(char *channel)
{
	char *mensaje = NULL;
	IRCMsg_Mode(&mensaje, cliente.prefijo, channel, "+m", NULL);
	enviar_cliente(mensaje);
	free(mensaje);
}

/**
 * @ingroup IRCInterfaceCallbacks
 *
 * @page IRCInterface_ActivateNicksLimit IRCInterface_ActivateNicksLimit
 *
 * @brief Llamada por el botón de activación del límite de usuarios en el canal.
 *
 * @synopsis
 * @code
 *	#include <redes2/ircxchat.h>
 *
 * 	void IRCInterface_ActivateNicksLimit (char *channel, int * limit)
 * @endcode
 * 
 * @description 
 * Llamada por el botón de activación del límite de usuarios en el canal. El segundo es el
 * límite de usuarios que se desea poner. Si el valor es 0 se sobrentiende que se desea eliminar
 * este límite.
 * 
 * En cualquier caso sólo se puede realizar si el servidor acepta la orden.
 * La string recibida no debe ser manipulada por el programador, sólo leída.
 *
 * @param[in] channel canal sobre el que se activará el límite de usuarios.
 * @param[in] limit límite de usuarios en el canal indicado.
 *
 * @warning Esta función debe ser implementada por el alumno.
 *
 * @author
 * Eloy Anguiano (eloy.anguiano@uam.es)
 *
 *<hr>
*/
 
void IRCInterface_ActivateNicksLimit(char *channel, int limit)
{
	char *mensaje = NULL;
	char limite[TAM_LIMIT_NICK] = {0};



	sprintf(limite, "%d", limit);
	IRCMsg_Mode(&mensaje, cliente.prefijo, channel, "+l", limite);
	enviar_cliente(mensaje);
	free(mensaje);
	

}

/**
 * @ingroup IRCInterfaceCallbacks
 *
 * @page IRCInterface_ActivatePrivate IRCInterface_ActivatePrivate
 *
 * @brief Llamada por el botón de activación del modo privado.
 *
 * @synopsis
 * @code
 *	#include <redes2/ircxchat.h>
 *
 * 	void IRCInterface_ActivatePrivate (char *channel)
 * @endcode
 * 
 * @description 
 * Llamada por el botón de activación del modo privado.
 * 
 * En cualquier caso sólo se puede realizar si el servidor acepta la orden.
 * La string recibida no debe ser manipulada por el programador, sólo leída.
 *
 * @param[in] channel canal sobre el que se va a activar la privacidad.
 *
 * @warning Esta función debe ser implementada por el alumno.
 *
 * @author
 * Eloy Anguiano (eloy.anguiano@uam.es)
 *
 *<hr>
*/
 
void IRCInterface_ActivatePrivate(char *channel)
{
	char *mensaje = NULL;
	IRCMsg_Mode(&mensaje, cliente.prefijo, channel, "+p", NULL);
	enviar_cliente(mensaje);
	free(mensaje);

}

/**
 * @ingroup IRCInterfaceCallbacks
 *
 * @page IRCInterface_ActivateProtectTopic IRCInterface_ActivateProtectTopic
 *
 * @brief Llamada por el botón de activación de la protección de tópico.
 *
 * @synopsis
 * @code
 *	#include <redes2/ircxchat.h>
 *
 * 	void IRCInterface_ActivateProtectTopic (char *channel)
 * @endcode
 * 
 * @description 
 * Llamada por el botón de activación de la protección de tópico.
 * 
 * En cualquier caso sólo se puede realizar si el servidor acepta la orden.
 * La string recibida no debe ser manipulada por el programador, sólo leída.
 *
 * @param[in] channel canal sobre el que se va a activar la protección de tópico.
 *
 * @warning Esta función debe ser implementada por el alumno.
 *
 * @author
 * Eloy Anguiano (eloy.anguiano@uam.es)
 *
 *<hr>
*/

void IRCInterface_ActivateProtectTopic(char *channel)
{
	char *mensaje = NULL;
	IRCMsg_Mode(&mensaje, cliente.prefijo, channel, "+t", NULL);
	enviar_cliente(mensaje);
	free(mensaje);
}

/**
 * @ingroup IRCInterfaceCallbacks
 *
 * @page IRCInterface_ActivateSecret IRCInterface_ActivateSecret
 *
 * @brief Llamada por el botón de activación de canal secreto.
 *
 * @synopsis
 * @code
 *	#include <redes2/ircxchat.h>
 *
 * 	void IRCInterface_ActivateSecret (char *channel)
 * @endcode
 * 
 * @description 
 * Llamada por el botón de activación de canal secreto.
 * 
 * En cualquier caso sólo se puede realizar si el servidor acepta la orden.
 * La string recibida no debe ser manipulada por el programador, sólo leída.
 *
 * @param[in] channel canal sobre el que se va a activar el estado de secreto.
 *
 * @warning Esta función debe ser implementada por el alumno.
 *
 * @author
 * Eloy Anguiano (eloy.anguiano@uam.es)
 *
 *<hr>
*/
 
void IRCInterface_ActivateSecret(char *channel)
{
	char *mensaje = NULL;
	IRCMsg_Mode(&mensaje, cliente.prefijo, channel, "+s", NULL);
	enviar_cliente(mensaje);
	free(mensaje);
}

/**
 * @ingroup IRCInterfaceCallbacks
 *
 * @page IRCInterface_BanNick IRCInterface_BanNick
 *
 * @brief Llamada por el botón "Banear".
 *
 * @synopsis
 * @code
 *	#include <redes2/ircxchat.h>
 *
 * 	void IRCInterface_BanNick (char *channel, char *nick)
 * @endcode
 * 
 * @description 
 * Llamada por el botón "Banear". Previamente debe seleccionarse un nick del
 * canal para darle voz a dicho usuario.
 * 
 * En cualquier caso sólo se puede realizar si el servidor acepta la orden.
 * Las strings recibidas no deben ser manipuladas por el programador, sólo leídas.
 *
 * @param[in] channel canal sobre el que se va a realizar el baneo. En principio es un valor innecesario.
 * @param[in] nick nick del usuario que va a ser baneado
 *
 * @warning Esta función debe ser implementada por el alumno.
 *
 * @author
 * Eloy Anguiano (eloy.anguiano@uam.es)
 *
 *<hr>
*/
 
void IRCInterface_BanNick(char *channel, char *nick)
{
	char *mensaje = NULL;
	IRCMsg_Mode(&mensaje, cliente.prefijo, channel, "+b", nick);
	enviar_cliente(mensaje);
	free(mensaje);
}

/**
 * @ingroup IRCInterfaceCallbacks
 *
 * @page IRCInterface_Connect IRCInterface_Connect
 *
 * @brief Llamada por los distintos botones de conexión.
 *
 * @synopsis
 * @code
 *	#include <redes2/ircxchat.h>
 *
 * 	long IRCInterface_Connect (char *nick, char * user, char * realname, char * password, char * server, int port, boolean ssl)
 * @endcode
 * 
 * @description 
 * Función a implementar por el programador.
 * Llamada por los distintos botones de conexión. Si implementará la comunicación completa, incluido
 * el registro del usuario en el servidor.
 *
 * En cualquier caso sólo se puede realizar si el servidor acepta la orden.
 * Las strings recibidas no deben ser manipuladas por el programador, sólo leída.
 *
 *
 * @param[in] nick nick con el que se va a realizar la conexíón.
 * @param[in] user usuario con el que se va a realizar la conexión.
 * @param[in] realname nombre real con el que se va a realizar la conexión.
 * @param[in] password password del usuario si es necesaria, puede valer NULL.
 * @param[in] server nombre o ip del servidor con el que se va a realizar la conexión.
 * @param[in] port puerto del servidor con el que se va a realizar la conexión.
 * @param[in] ssl puede ser TRUE si la conexión tiene que ser segura y FALSE si no es así.
 *
 * @retval IRC_OK si todo ha sido correcto (debe devolverlo).
 * @retval IRCERR_NOSSL si el valor de SSL es TRUE y no se puede activar la conexión SSL pero sí una 
 * conexión no protegida (debe devolverlo).
 * @retval IRCERR_NOCONNECT en caso de que no se pueda realizar la comunicación (debe devolverlo).
 *
 * @warning Esta función debe ser implementada por el alumno.
 *
 * @author
 * Eloy Anguiano (eloy.anguiano@uam.es)
 *
 *<hr>
*/
 
long IRCInterface_Connect(char *nick, char *user, char *realname, char *password, char *server, int port, boolean ssl)
{
	int socket;
	char *mensaje = NULL;

	/* Control de errores */
	if(!nick || !user || !realname || !server || port<0 )
		return IRCERR_NOCONNECT;

	/* Variable global ssl */
	ssl_active = ssl;

	/* Nos conectamos al servidor */
	if(crearSocketTCP_cliente(&socket, server, port) == RED_ERROR){
		return IRCERR_NOCONNECT;
	}


	/* Fijamos variable global socket */
	cliente.socket = socket;

	/* Inicializamos conexion SSL */
	if(ssl){

		inicializar_nivel_SSL();

		if(fijar_contexto_SSL(PATH_CLIENT_CERT, PATH_CLIENT_PKEY) < 0){
				close(cliente.socket);
				liberar_nivel_SSL();
				return IRCERR_NOSSL;
		}
		/* Conectamos canal seguro */
		if(conectar_canal_seguro_SSL(cliente.socket) < 0){
				close(cliente.socket);
				liberar_nivel_SSL();
				return IRCERR_NOSSL;
		}
	}

	/* Password */
	if(password && 0) {
		IRCMsg_Pass(&mensaje, NULL, password);
		enviar_cliente(mensaje);
		free(mensaje);
	}
	/* Mensaje de NICK */
	IRCMsg_Nick(&mensaje, NULL, nick, NULL);
	enviar_cliente(mensaje);
	free(mensaje);

	/* Mensaje de USER */
	IRCMsg_User(&mensaje, NULL, user, "*", realname);
	enviar_cliente(mensaje);
	free(mensaje);

	cliente.registro = CONECTADO;

	/* Creamos prefijo */
	cliente.prefijo = NULL;
	IRC_ComplexUser(&cliente.prefijo, nick, realname, NULL, server);

	/* Guardamos datos del usuario */
	if(inicializar_cliente(nick,realname,user,server,port) != 0){
		return IRCERR_NOCONNECT;
	}

	return IRC_OK;
}


/**
 * @ingroup IRCInterfaceCallbacks
 *
 * @page IRCInterface_DeactivateChannelKey IRCInterface_DeactivateChannelKey
 *
 * @brief Llamada por el botón de desactivación de la clave del canal.
 *
 * @synopsis
 * @code
 *	#include <redes2/ircxchat.h>
 *
 * 	void IRCInterface_DeactivateChannelKey (char *channel)
 * @endcode
 * 
 * @description 
 * Llamada por el botón de desactivación de la clave del canal.
 *
 * En cualquier caso sólo se puede realizar si el servidor acepta la orden.
 * La string recibida no debe ser manipulada por el programador, sólo leída.
 *
 * @param[in] channel canal sobre el que se va a desactivar la clave.
 *
 * @warning Esta función debe ser implementada por el alumno.
 *
 * @author
 * Eloy Anguiano (eloy.anguiano@uam.es)
 *
 *<hr>
*/
 
void IRCInterface_DeactivateChannelKey(char *channel)
{
	char *mensaje = NULL;
	IRCMsg_Mode(&mensaje, cliente.prefijo, channel, "-k", NULL);
	enviar_cliente(mensaje);
	free(mensaje);
}

/**
 * @ingroup IRCInterfaceCallbacks
 *
 * @page IRCInterface_DeactivateExternalMessages IRCInterface_DeactivateExternalMessages
 *
 * @brief Llamada por el botón de desactivación de la recepción de mensajes externos.
 *
 * @synopsis
 * @code
 *	#include <redes2/ircxchat.h>
 *
 * 	void IRCInterface_DeactivateExternalMessages (char *channel)
 * @endcode
 * 
 * @description 
 * Llamada por el botón de desactivación de la recepción de mensajes externos.
 *
 * En cualquier caso sólo se puede realizar si el servidor acepta la orden.
 * La string recibida no debe ser manipulada por el programador, sólo leída.
 *
 * @param[in] channel canal sobre el que se va a deactivar la recepción de mensajes externos.
 *
 * @warning Esta función debe ser implementada por el alumno.
 *
 * @author
 * Eloy Anguiano (eloy.anguiano@uam.es)
 *
 *<hr>
*/
 
void IRCInterface_DeactivateExternalMessages(char *channel)
{
	char *mensaje = NULL;
	IRCMsg_Mode(&mensaje, cliente.prefijo, channel, "-n", NULL);
	enviar_cliente(mensaje);
	free(mensaje);
}

/**
 * @ingroup IRCInterfaceCallbacks
 *
 * @page IRCInterface_DeactivateInvite IRCInterface_DeactivateInvite
 *
 * @brief Llamada por el botón de desactivación de canal de sólo invitación.
 *
 * @synopsis
 * @code
 *	#include <redes2/ircxchat.h>
 *
 * 	void IRCInterface_DeactivateInvite (char *channel)
 * @endcode
 * 
 * @description 
 * Llamada por el botón de desactivación de canal de sólo invitación.
 *
 * En cualquier caso sólo se puede realizar si el servidor acepta la orden.
 * La string recibida no debe ser manipulada por el programador, sólo leída.
 *
 * @param[in] channel canal sobre el que se va a desactivar la invitación.
 *
 * @warning Esta función debe ser implementada por el alumno.
 *
 * @author
 * Eloy Anguiano (eloy.anguiano@uam.es)
 *
 *<hr>
*/
 
void IRCInterface_DeactivateInvite(char *channel)
{
	char *mensaje = NULL;
	IRCMsg_Mode(&mensaje, cliente.prefijo, channel, "-i", NULL);
	enviar_cliente(mensaje);
	free(mensaje);
}

/**
 * @ingroup IRCInterfaceCallbacks
 *
 * @page IRCInterface_DeactivateModerated IRCInterface_DeactivateModerated
 *
 * @brief Llamada por el botón de desactivación  de la moderación del canal.
 *
 * @synopsis
 * @code
 *	#include <redes2/ircxchat.h>
 *
 * 	void IRCInterface_DeactivateModerated (char *channel)
 * @endcode
 * 
 * @description 
 * Llamada por el botón de desactivación  de la moderación del canal.
 *
 * En cualquier caso sólo se puede realizar si el servidor acepta la orden.
 * La string recibida no debe ser manipulada por el programador, sólo leída.
 *
 * @param[in] channel canal sobre el que se va a desactivar la moderación.
 *
 * @warning Esta función debe ser implementada por el alumno.
 *
 * @author
 * Eloy Anguiano (eloy.anguiano@uam.es)
 *
 *<hr>
*/
 
void IRCInterface_DeactivateModerated(char *channel)
{
	char *mensaje = NULL;
	IRCMsg_Mode(&mensaje, cliente.prefijo, channel, "-m", NULL);
	enviar_cliente(mensaje);
	free(mensaje);
}

/**
 * @ingroup IRCInterfaceCallbacks
 *
 * @page IRCInterface_DeactivateNicksLimit IRCInterface_DeactivateNicksLimit
 *
 * @brief Llamada por el botón de desactivación de la protección de tópico.
 *
 * @synopsis
 * @code
 *	#include <redes2/ircxchat.h>
 *
 * 	void IRCInterface_DeactivateNicksLimit (char *channel)
 * @endcode
 * 
 * @description 
 * Llamada por el botón de desactivación  del límite de usuarios en el canal.
 *
 * En cualquier caso sólo se puede realizar si el servidor acepta la orden.
 * La string recibida no debe ser manipulada por el programador, sólo leída.
 *
 * @param[in] channel canal sobre el que se va a desactivar el límite de usuarios.
 *
 * @warning Esta función debe ser implementada por el alumno.
 *
 * @author
 * Eloy Anguiano (eloy.anguiano@uam.es)
 *
 *<hr>
*/
 
void IRCInterface_DeactivateNicksLimit(char *channel)
{
	char *mensaje = NULL;
	IRCMsg_Mode(&mensaje, cliente.prefijo, channel, "-l", NULL);
	enviar_cliente(mensaje);
	free(mensaje);
}

/**
 * @ingroup IRCInterfaceCallbacks
 *
 * @page IRCInterface_DeactivatePrivate IRCInterface_DeactivatePrivate
 *
 * @brief Llamada por el botón de desactivación del modo privado.
 *
 * @synopsis
 * @code
 *	#include <redes2/ircxchat.h>
 *
 * 	void IRCInterface_DeactivatePrivate (char *channel)
 * @endcode
 * 
 * @description 
 * Llamada por el botón de desactivación del modo privado.
 *
 * En cualquier caso sólo se puede realizar si el servidor acepta la orden.
 * La string recibida no debe ser manipulada por el programador, sólo leída.
 *
 * @warning Esta función debe ser implementada por el alumno.
 *
 * @param[in] channel canal sobre el que se va a desactivar la privacidad.
 *
 * @warning Esta función debe ser implementada por el alumno.
 *
 * @author
 * Eloy Anguiano (eloy.anguiano@uam.es)
 *
 *<hr>
*/
 
void IRCInterface_DeactivatePrivate(char *channel)
{
	char *mensaje = NULL;
	IRCMsg_Mode(&mensaje, cliente.prefijo, channel, "-p", NULL);
	enviar_cliente(mensaje);
	free(mensaje);
}

/**
 * @ingroup IRCInterfaceCallbacks
 *
 * @page IRCInterface_DeactivateProtectTopic IRCInterface_DeactivateProtectTopic
 *
 * @brief Llamada por el botón de desactivación de la protección de tópico.
 *
 * @synopsis
 * @code
 *	#include <redes2/ircxchat.h>
 *
 * 	void IRCInterface_DeactivateProtectTopic (char *channel)
 * @endcode
 * 
 * @description 
 * Llamada por el botón de desactivación de la protección de tópico.
 *
 * En cualquier caso sólo se puede realizar si el servidor acepta la orden.
 * La string recibida no debe ser manipulada por el programador, sólo leída.
 *
 * @param[in] channel canal sobre el que se va a desactivar la protección de tópico.
 *
 * @warning Esta función debe ser implementada por el alumno.
 *
 * @author
 * Eloy Anguiano (eloy.anguiano@uam.es)
 *
 *<hr>
*/

void IRCInterface_DeactivateProtectTopic(char *channel)
{
	char *mensaje = NULL;
	IRCMsg_Mode(&mensaje, cliente.prefijo, channel, "-t", NULL);
	enviar_cliente(mensaje);
	free(mensaje);
}

/**
 * @ingroup IRCInterfaceCallbacks
 *
 * @page IRCInterface_DeactivateSecret IRCInterface_DeactivateSecret
 *
 * @brief Llamada por el botón de desactivación de canal secreto.
 *
 * @synopsis
 * @code
 *	#include <redes2/ircxchat.h>
 *
 * 	void IRCInterface_DeactivateSecret (char *channel)
 * @endcode
 * 
 * @description 
 * Llamada por el botón de desactivación de canal secreto.
 *
 * En cualquier caso sólo se puede realizar si el servidor acepta la orden.
 * La string recibida no debe ser manipulada por el programador, sólo leída.
 *
 * @param[in] channel canal sobre el que se va a desactivar la propiedad de canal secreto.
 *
 * @warning Esta función debe ser implementada por el alumno.
 *
 * @author
 * Eloy Anguiano (eloy.anguiano@uam.es)
 *
 *<hr>
*/
 
void IRCInterface_DeactivateSecret(char *channel)
{
	char *mensaje = NULL;
	IRCMsg_Mode(&mensaje, cliente.prefijo, channel, "-s", NULL);
	enviar_cliente(mensaje);
	free(mensaje);
}

/**
 * @ingroup IRCInterfaceCallbacks
 *
 * @page IRCInterface_DisconnectServer IRCInterface_DisconnectServer
 *
 * @brief Llamada por los distintos botones de desconexión.
 *
 * @synopsis
 * @code
 *	#include <redes2/ircxchat.h>
 *
 * 	boolean IRCInterface_DisconnectServer (char * server, int port)
 * @endcode
 * 
 * @description 
 * Llamada por los distintos botones de desconexión. Debe cerrar la conexión con el servidor.
 *
 * En cualquier caso sólo se puede realizar si el servidor acepta la orden.
 * La string recibida no debe ser manipulada por el programador, sólo leída.

 * @param[in] server nombre o ip del servidor del que se va a realizar la desconexión.
 * @param[in] port puerto sobre el que se va a realizar la desconexión.
 *
 * @retval TRUE si se ha cerrado la conexión (debe devolverlo).
 * @retval FALSE en caso contrario (debe devolverlo).
 *
 * @warning Esta función debe ser implementada por el alumno.
 *
 * @author
 * Eloy Anguiano (eloy.anguiano@uam.es)
 *
 *<hr>
*/
 
boolean IRCInterface_DisconnectServer(char *server, int port)
{

	char *mensaje;

	/* Comprobacion evitar seg fault en strcmp */
	if(!server || !cliente.server)
		return FALSE;

	if(strcmp(server, cliente.server) || port != cliente.puerto)
		return FALSE;

	/* Enviamos un quit al servidor */
	IRCMsg_Quit(&mensaje, cliente.prefijo, QUIT_MSG);
	enviar_cliente(mensaje);
	free(mensaje);

	/* Marcamos cliente como no conectado */
	cliente.registro = NO_CONECTADO;

	close(cliente.socket);

	IRCInterface_RemoveAllChannels();
	IRCInterface_ChangeConectionSelected();


	return TRUE;
}

/**
 * @ingroup IRCInterfaceCallbacks
 *
 * @page IRCInterface_ExitAudioChat IRCInterface_ExitAudioChat
 *
 * @brief Llamada por el botón "Cancelar" del diálogo de chat de voz.
 *
 * @synopsis
 * @code
 *	#include <redes2/ircxchat.h>
 *
 * 	void IRCInterface_ExitAudioChat (char *nick)
 * @endcode
 * 
 * @description 
 * Llamada por el botón "Parar" del diálogo de chat de voz. Previamente debe seleccionarse un nick del
 * canal para darle voz a dicho usuario. Esta función cierrala comunicación. Evidentemente tiene que
 * actuar sobre el hilo de chat de voz.
 *
 * En cualquier caso sólo se puede realizar si el servidor acepta la orden.
 * La string recibida no debe ser manipulada por el programador, sólo leída.
 *
 * @param[in] nick nick del usuario que solicita la parada del chat de audio.
 *
 * @retval TRUE si se ha cerrado la comunicación (debe devolverlo).
 * @retval FALSE en caso contrario (debe devolverlo).
 *
 * @warning Esta función debe ser implementada por el alumno.
 *
 * @author
 * Eloy Anguiano (eloy.anguiano@uam.es)
 *
 *<hr>
*/
 
boolean IRCInterface_ExitAudioChat(char *nick)
{
	char *mensaje = NULL;

	IRCMsg_Privmsg(&mensaje, cliente.prefijo, nick, "\001AUDIOEXIT");
	enviar_cliente(mensaje);
	free(mensaje);

	if(cliente.audio_recv_thread_th != 0){
		pthread_kill(cliente.audio_recv_thread_th, SIGUSR2);
	}

	return TRUE;
}

/**
 * @ingroup IRCInterfaceCallbacks
 *
 * @page IRCInterface_GiveOp IRCInterface_GiveOp
 *
 * @brief Llamada por el botón "Op".
 *
 * @synopsis
 * @code
 *	#include <redes2/ircxchat.h>
 *
 * 	void IRCInterface_GiveOp (char *channel, char *nick)
 * @endcode
 * 
 * @description 
 * Llamada por el botón "Op". Previamente debe seleccionarse un nick del
 * canal para darle "op" a dicho usuario.
 *
 * En cualquier caso sólo se puede realizar si el servidor acepta la orden.
 * Las strings recibidas no deben ser manipuladas por el programador, sólo leídas.
 *
 * @param[in] channel canal sobre el que se va dar op al usuario.
 * @param[in] nick nick al que se le va a dar el nivel de op.
 *
 * @warning Esta función debe ser implementada por el alumno.
 *
 * @author
 * Eloy Anguiano (eloy.anguiano@uam.es)
 *
 *<hr>
*/
 
void IRCInterface_GiveOp(char *channel, char *nick)
{
	char *mensaje = NULL;
	IRCMsg_Mode(&mensaje, cliente.prefijo, channel, "+o", nick);
	enviar_cliente(mensaje);
	free(mensaje);
}

/**
 * @ingroup IRCInterfaceCallbacks
 *
 * @page IRCInterface_GiveVoice IRCInterface_GiveVoice
 *
 * @brief Llamada por el botón "Dar voz".
 *
 * @synopsis
 * @code
 *	#include <redes2/ircxchat.h>
 *
 * 	void IRCInterface_GiveVoice (char *channel, char *nick)
 * @endcode
 * 
 * @description 
 * Llamada por el botón "Dar voz". Previamente debe seleccionarse un nick del
 * canal para darle voz a dicho usuario.
 *
 * En cualquier caso sólo se puede realizar si el servidor acepta la orden.
 * Las strings recibidas no deben ser manipuladas por el programador, sólo leídas.
 *
 * @param[in] channel canal sobre el que se va dar voz al usuario.
 * @param[in] nick nick al que se le va a dar voz.
 *
 * @warning Esta función debe ser implementada por el alumno.
 *
 * @author
 * Eloy Anguiano (eloy.anguiano@uam.es)
 *
 *<hr>
*/
 
void IRCInterface_GiveVoice(char *channel, char *nick)
{
	char *mensaje = NULL;
	IRCMsg_Mode(&mensaje, cliente.prefijo, channel, "+v", nick);
	enviar_cliente(mensaje);
	free(mensaje);
}

/**
 * @ingroup IRCInterfaceCallbacks
 *
 * @page IRCInterface_KickNick IRCInterface_KickNick
 *
 * @brief Llamada por el botón "Echar".
 *
 * @synopsis
 * @code
 *	#include <redes2/ircxchat.h>
 *
 * 	void IRCInterface_KickNick (char *channel, char *nick)
 * @endcode
 * 
 * @description 
 * Llamada por el botón "Echar". Previamente debe seleccionarse un nick del
 * canal para darle voz a dicho usuario.
 *
 * En cualquier caso sólo se puede realizar si el servidor acepta la orden.
 * Las strings recibidas no deben ser manipuladas por el programador, sólo leídas.
 *
 * @param[in] channel canal sobre el que se va a expulsar al usuario.
 * @param[in] nick nick del usuario que va a ser expulsado.
 *
 * @warning Esta función debe ser implementada por el alumno.
 *
 * @author
 * Eloy Anguiano (eloy.anguiano@uam.es)
 *
 *<hr>
*/
 
void IRCInterface_KickNick(char *channel, char *nick)
{
	char *mensaje = NULL;
	IRCMsg_Kick(&mensaje, cliente.prefijo, channel, nick, KICK_MSG);
	enviar_cliente(mensaje);
	free(mensaje);
}

/**
 * @ingroup IRCInterfaceCallbacks
 *
 * @page IRCInterface_NewCommandText IRCInterface_NewCommandText
 *
 * @brief Llamada la tecla ENTER en el campo de texto y comandos.
 *
 * @synopsis
 * @code
 *	#include <redes2/ircxchat.h>
 *
 * 	void IRCInterface_NewCommandText (char *command)
 * @endcode
 * 
 * @description
 * Llamada de la tecla ENTER en el campo de texto y comandos. El texto deberá ser
 * enviado y el comando procesado por las funciones de "parseo" de comandos de
 * usuario.
 *
 * En cualquier caso sólo se puede realizar si el servidor acepta la orden.
 * La string recibida no debe ser manipulada por el programador, sólo leída.
 *
 * @param[in] comando introducido por el usuario.
 *
 * @warning Esta función debe ser implementada por el alumno.
 *
 * @author
 * Eloy Anguiano (eloy.anguiano@uam.es)
 *
 *<hr>
*/
 
void IRCInterface_NewCommandText(char *command)
{

	char *mensaje = NULL;
	char error[TAM_ERROR_MSG] = {0};

	long comando = IRCUser_CommandQuery(command);

	switch(comando) {

		case IRCERR_NOSTRING:
			return;

		case IRCERR_NOUSERCOMMAND:

			/* No enviamos mensajes vacios */
			if(!strlen(command)){
				return;
			}
			/* Ni mensajes a la pantalla system */
			if(!strcmp(IRCInterface_ActiveChannelName(),"System"))
				break;

			IRCMsg_Privmsg(&mensaje, cliente.prefijo, IRCInterface_ActiveChannelName(), command);
			enviar_cliente(mensaje);
			free(mensaje);
			IRCInterface_WriteChannel(IRCInterface_ActiveChannelName(), cliente.nick, command);		
			break;

		case IRCERR_NOVALIDUSERCOMMAND:
			snprintf(error, TAM_ERROR_MSG, ERROR_COMMAND_MSG "%s", command);
			IRCInterface_ErrorDialog(error);
			break;

		default:		
			command++;
			comandos_cliente[comando](command);
			return;;
	}
}

/**
 * @ingroup IRCInterfaceCallbacks
 *
 * @page IRCInterface_NewTopicEnter IRCInterface_NewTopicEnter
 *
 * @brief Llamada cuando se pulsa la tecla ENTER en el campo de tópico.
 *
 * @synopsis
 * @code
 *	#include <redes2/ircxchat.h>
 *
 * 	void IRCInterface_NewTopicEnter (char * topicdata)
 * @endcode
 * 
 * @description 
 * Llamada cuando se pulsa la tecla ENTER en el campo de tópico.
 * Deberá intentarse cambiar el tópico del canal.
 * 
 * En cualquier caso sólo se puede realizar si el servidor acepta la orden.
 * La string recibida no debe ser manipulada por el programador, sólo leída.
 *
 * param[in] topicdata string con el tópico que se desea poner en el canal.
 *
 * @warning Esta función debe ser implementada por el alumno.
 *
 * @author
 * Eloy Anguiano (eloy.anguiano@uam.es)
 *
 *<hr>
*/
 
void IRCInterface_NewTopicEnter(char *topicdata)
{
	char *mensaje = NULL;
	char *channel = IRCInterface_ActiveChannelName();
	IRCMsg_Topic(&mensaje, cliente.prefijo, channel, topicdata);
	enviar_cliente(mensaje);
	free(mensaje);

}

/**
 * @ingroup IRCInterfaceCallbacks
 *
 * @page IRCInterface_SendFile IRCInterface_SendFile
 *
 * @brief Llamada por el botón "Enviar Archivo".
 *
 * @synopsis
 * @code
 *	#include <redes2/ircxchat.h>
 *
 * 	void IRCInterface_SendFile (char * filename, char *nick, char *data, long unsigned int length)
 * @endcode
 * 
 * @description 
 * Llamada por el botón "Enviar Archivo". Previamente debe seleccionarse un nick del
 * canal para darle voz a dicho usuario. Esta función como todos los demás callbacks bloquea el interface
 * y por tanto es el programador el que debe determinar si crea un nuevo hilo para enviar el archivo o
 * no lo hace.
 * 
 * En cualquier caso sólo se puede realizar si el servidor acepta la orden.
 * Las strings recibidas no deben ser manipuladas por el programador, sólo leídas.
 *
 * @param[in] filename nombre del fichero a enviar.
 * @param[in] nick nick del usuario que enviará el fichero.
 * @param[in] data datos a ser enviados.
 * @param[in] length longitud de los datos a ser enviados.
 *
 * @retval TRUE si se ha establecido la comunicación (debe devolverlo).
 * @retval FALSE en caso contrario (debe devolverlo).
 *
 * @warning Esta función debe ser implementada por el alumno.
 *
 * @author
 * Eloy Anguiano (eloy.anguiano@uam.es)
 *
 *<hr>
*/
 
boolean IRCInterface_SendFile(char *filename, char *nick, char *data, long unsigned int length)
{

	pEnvio envio;

	if(!filename || !nick || !data || length <= 0)
		return FALSE;

	if(cliente.envio != DISPONIBLE){
		IRCInterface_ErrorDialog(SEND_ERROR_BUSY);
		return TRUE;
	}

	cliente.envio = OCUPADO;
	
	envio = (pEnvio) calloc(1, sizeof(Envio));
	if(!envio){
		cliente.envio = DISPONIBLE;
		return FALSE;
	}

	envio->data = (char *) calloc (length, sizeof(char));
	if(!envio->data){
		free(envio);
		cliente.envio = DISPONIBLE;
		return FALSE;
	}

	memcpy(envio->data, data, length);

	envio->filename = (char *) calloc(strlen(filename) + 1, sizeof(char));
	if(!envio->filename){
		free(envio->data);
		free(envio);
		cliente.envio = DISPONIBLE;
		return FALSE;
	}

	strcpy(envio->filename, filename);
	normalize_file(envio->filename);

	envio->nick = (char *) calloc(strlen(nick) + 1, sizeof(char));
	if(!envio->nick){
		free(envio->filename);
		free(envio->data);
		free(envio);
		cliente.envio = DISPONIBLE;
		return FALSE;
	}
	strcpy(envio->nick, nick);
	envio->length = length;

	if(pthread_create(&cliente.file_thread_th, NULL, file_send_func, (void*) envio) != 0){
		free(envio->filename);
		free(envio->data);
		free(envio->nick);
		free(envio);
		cliente.envio = DISPONIBLE;
		return FALSE;
	}
	return TRUE;
}

/**
 * @ingroup IRCInterfaceCallbacks
 *
 * @page IRCInterface_StartAudioChat IRCInterface_StartAudioChat
 *
 * @brief Llamada por el botón "Iniciar" del diálogo de chat de voz.
 *
 * @synopsis
 * @code
 *	#include <redes2/ircxchat.h>
 *
 * 	void IRCInterface_StartAudioChat (char *nick)
 * @endcode
 * 
 * @description 
 * Llamada por el botón "Iniciar" del diálogo de chat de voz. Previamente debe seleccionarse un nick del
 * canal para darle voz a dicho usuario. Esta función como todos los demás callbacks bloquea el interface
 * y por tanto para mantener la funcionalidad del chat de voz es imprescindible crear un hilo a efectos
 * de comunicación de voz.
 * 
 * En cualquier caso sólo se puede realizar si el servidor acepta la orden.
 * La string recibida no debe ser manipulada por el programador, sólo leída.
 *
 * @param[in] nick nick del usuario con el que se desea conectar.
 *
 * @retval TRUE si se ha establecido la comunicación (debe devolverlo).
 * @retval FALSE en caso contrario (debe devolverlo).
 *
 * @warning Esta función debe ser implementada por el alumno.
 *
 * @author
 * Eloy Anguiano (eloy.anguiano@uam.es)
 *
 *<hr>
*/
 
boolean IRCInterface_StartAudioChat(char *nick)
{
	int socketudp;
	char *ip_local;
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);
	struct ifreq ifr;
	char *mensaje = NULL;
	char comando[COMANDO_LEN] = {0};
	pAudiorecepcion audior = NULL;

	if(cliente.audiochatrecv != DISPONIBLE){
		if(cliente.playing == 0){
			cliente.playing = 1;
			return TRUE;
		}
		return FALSE;
	}
	/* Ponemos como ocupado el envio de audio */
	cliente.audiochatrecv = OCUPADO;

	/* Creamos socket udp */
	if(socketUDP(&socketudp, 0) == RED_ERROR){
		cliente.audiochatrecv = DISPONIBLE;
		return FALSE;
	}
	/* Sacamos el puerto que se ha elegido */
	if(getsockname(socketudp,(struct sockaddr*) &addr, &addrlen) == -1){
		close(socketudp);
		cliente.audiochatrecv = DISPONIBLE;
		return FALSE;
	}
	/* Obtenemos IP local */
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, interfaz, IFNAMSIZ-1);
	ioctl(socketudp, SIOCGIFADDR, &ifr);
	ip_local = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);

	/* Enviamos AUDIOCHAT */
	snprintf(comando, COMANDO_LEN-1, "\001AUDIOCHAT %s %"PRIu16, ip_local, ntohs(addr.sin_port));
	IRCMsg_Privmsg(&mensaje, cliente.prefijo, nick,comando);
	enviar_cliente(mensaje);
	free(mensaje);
	cliente.playing = 1;

	audior = (pAudiorecepcion) malloc(sizeof(Audiorecepcion));
	audior->socket = socketudp;
	audior->direccion = malloc(addrlen);
	memcpy(audior->direccion, &ifr,addrlen);

	if(pthread_create(&cliente.audio_recv_thread_th, NULL, audiochatRecv, audior) < 0){
		cliente.audiochatrecv = DISPONIBLE;
		close(socketudp);
		return FALSE;
	}

	return TRUE;
}

/**
 * @ingroup IRCInterfaceCallbacks
 *
 * @page IRCInterface_StopAudioChat IRCInterface_StopAudioChat
 *
 * @brief Llamada por el botón "Parar" del diálogo de chat de voz.
 *
 * @synopsis
 * @code
 *	#include <redes2/ircxchat.h>
 *
 * 	void IRCInterface_StopAudioChat (char *nick)
 * @endcode
 * 
 * @description 
 * Llamada por el botón "Parar" del diálogo de chat de voz. Previamente debe seleccionarse un nick del
 * canal para darle voz a dicho usuario. Esta función sólo para la comunicación que puede ser reiniciada. 
 * Evidentemente tiene que actuar sobre el hilo de chat de voz.
 * 
 * En cualquier caso sólo se puede realizar si el servidor acepta la orden.
 * La string recibida no debe ser manipulada por el programador, sólo leída.
 *
 * @param[in] nick nick del usuario con el que se quiere parar el chat de voz.
 *
 * @retval TRUE si se ha parado la comunicación (debe devolverlo).
 * @retval FALSE en caso contrario (debe devolverlo).
 *
 * @warning Esta función debe ser implementada por el alumno.
 *
 * @author
 * Eloy Anguiano (eloy.anguiano@uam.es)
 *
 *<hr>
*/
 
boolean IRCInterface_StopAudioChat(char *nick)
{
	(void) nick;
	cliente.playing = 0;
	return TRUE;
}

/**
 * @ingroup IRCInterfaceCallbacks
 *
 * @page IRCInterface_TakeOp IRCInterface_TakeOp
 *
 * @brief Llamada por el botón "Quitar Op". 
 *
 * @synopsis
 * @code
 *	#include <redes2/ircxchat.h>
 *
 * 	void IRCInterface_TakeOp (char *channel, char *nick)
 * @endcode
 * 
 * @description 
 * Llamada por el botón "Quitar Op". Previamente debe seleccionarse un nick del
 * canal para quitarle "op" a dicho usuario.
 * 
 * En cualquier caso sólo se puede realizar si el servidor acepta la orden.
 * Las strings recibidas no deben ser manipuladas por el programador, sólo leídas.
 *
 * @param[in] channel canal sobre el que se va a quitar op al usuario.
 * @param[in] nick nick del usuario al que se le va a quitar op.
 *
 * @warning Esta función debe ser implementada por el alumno.
 *
 * @author
 * Eloy Anguiano (eloy.anguiano@uam.es)
 *
 *<hr>
*/
 
void IRCInterface_TakeOp(char *channel, char *nick)
{
	char *mensaje = NULL;
	IRCMsg_Mode(&mensaje, cliente.prefijo, channel, "-o", nick);
	enviar_cliente(mensaje);
	free(mensaje);
}

/**
 * @ingroup IRCInterfaceCallbacks
 *
 * @page IRCInterface_TakeVoice IRCInterface_TakeVoice
 *
 * @brief Llamada por el botón "Quitar voz". 
 *
 * @synopsis
 * @code
 *	#include <redes2/ircxchat.h>
 *
 * 	void IRCInterface_TakeVoice (char *channel, char *nick)
 * @endcode
 * 
 * @description 
 * Llamada por el botón "Quitar voz". Previamente debe seleccionarse un nick del
 * canal para darle voz a dicho usuario.
 * 
 * En cualquier caso sólo se puede realizar si el servidor acepta la orden.
 * Las strings recibidas no deben ser manipuladas por el programador, sólo leídas.
 *
 * @param[in] channel canal sobre el que se le va a quitar voz al usuario.
 * @param[in] nick nick del usuario al que se va a quitar la voz.
 *
 * @warning Esta función debe ser implementada por el alumno.
 *
 * @author
 * Eloy Anguiano (eloy.anguiano@uam.es)
 *
 *<hr>
*/
 
void IRCInterface_TakeVoice(char *channel, char *nick)
{
	char *mensaje = NULL;
	IRCMsg_Mode(&mensaje, cliente.prefijo, channel, "-v", nick);
	enviar_cliente(mensaje);
	free(mensaje);
}

/**
  * @brief Devuelve la IP de la interfaz que esta siendo utilizada para la conexion de internet
  * @return nombre del host o NULL si no ha conseguido obtenerla. Es necesario liberar la variable de retorno.
  */
char* get_interfaz() {
    FILE *f;
    char line[100] , *p , *c, *ret = NULL;

    f = fopen("/proc/net/route" , "r");
	if(!f)
		return NULL;

    while(fgets(line , 100 , f)){
        p = strtok(line , " \t");
        c = strtok(NULL , " \t");

        if(p!=NULL && c!=NULL){
            if(strcmp(c , "00000000") == 0){
                break;
            }
        }
    }

	if(p != NULL){
		ret = (char *) calloc(strlen(p) + 1, sizeof(char));
		if(!ret) return NULL;
		strcpy(ret, p);
	}

    return ret;
}

void IRCPrueba_NOInterface(char *cadena, int port){
	char buffer[1024] = {0};
	FILE *pf;

	/* Nos conectamos al servidor */
	if(crearSocketTCP_cliente(&cliente.socket, "localhost", port) == RED_ERROR){
		return;
	}

	/* Fijamos variable global socket */
	ssl_active = 1;

	/* Inicializamos conexion SSL */
	inicializar_nivel_SSL();

	if(fijar_contexto_SSL(PATH_CLIENT_CERT, PATH_CLIENT_PKEY) < 0){
			close(cliente.socket);
			liberar_nivel_SSL();
			return;
	}
	/* Conectamos canal seguro */
	if(conectar_canal_seguro_SSL(cliente.socket) < 0){
			close(cliente.socket);
			liberar_nivel_SSL();
			return;
	}
	pf = fopen("entrada.txt","r");
	if(!pf){
		enviar_datos_SSL(cliente.socket, cadena, strlen(cadena));

	} else {
		fread(buffer, 1023, 1, pf);
		enviar_datos_SSL(cliente.socket, buffer, strlen(buffer));
		fclose(pf);
	}

	recibir_datos_SSL(cliente.socket, buffer, 1023);
	printf("%s", buffer);
	close(cliente.socket);
	liberar_nivel_SSL();
	return;
}


/***************************************************************************************************/
/***************************************************************************************************/
/**                                                                                               **/
/** MMMMMMMMMM               MMMMM           AAAAAAA           IIIIIII NNNNNNNNNN          NNNNNN **/
/**  MMMMMMMMMM             MMMMM            AAAAAAAA           IIIII   NNNNNNNNNN          NNNN  **/
/**   MMPMM MMMM           MM MM            AAAAA   AA           III     NNNNN NNNN          NN   **/
/**   MMAMM  MMMM         MM  MM            AAAAA   AA           III     NNNNN  NNNN         NN   **/
/**   MMBMM   MMMM       MM   MM           AAAAA     AA          III     NNNNN   NNNN        NN   **/
/**   MMLMM    MMMM     MM    MM           AAAAA     AA          III     NNNNN    NNNN       NN   **/
/**   MMOMM     MMMM   MM     MM          AAAAA       AA         III     NNNNN     NNNN      NN   **/
/**   MMMMM      MMMM MM      MM          AAAAAAAAAAAAAA         III     NNNNN      NNNN     NN   **/
/**   MMAMM       MMMMM       MM         AAAAA         AA        III     NNNNN       NNNN    NN   **/
/**   MMRMM        MMM        MM         AAAAA         AA        III     NNNNN        NNNN   NN   **/
/**   MMCMM                   MM        AAAAA           AA       III     NNNNN         NNNN  NN   **/
/**   MMOMM                   MM        AAAAA           AA       III     NNNNN          NNNN NN   **/
/**  MMMSMMM                 MMMM     AAAAAA            AAAA    IIIII   NNNNNN           NNNNNNN  **/
/** MMMMMMMMM               MMMMMM  AAAAAAAA           AAAAAA  IIIIIII NNNNNNN            NNNNNNN **/
/**                                                                                               **/
/***************************************************************************************************/
/***************************************************************************************************/



int main (int argc, char *argv[])
{
	int long_index, flag_interfaz = 0, flag_c3po = 0;
	char c, *interface;
	int puerto = 0;
	char *cadena = NULL;

	static struct option options[] = {
		{"interfaz", required_argument,0,'i'},
		{"port", required_argument,0,'p'},
		{"ssldata", required_argument,0,'s'}
	};

	while ((c = getopt_long_only (argc, argv, "i:p:s:h", options, &long_index)) != -1){
		switch (c) {
			case 'i': /* Especifica interfaz por defecto */
				snprintf(interfaz,TAM_INTERFACE,"%s", optarg);
				flag_interfaz = 1;
				break;
			case 'p':
				puerto = atoi(optarg);
				break;
			case 's':
				flag_c3po = 1;
				cadena = optarg;
				break;

			case '?':
			default:
				usage();
				exit(EXIT_SUCCESS);
		}
	}

	/* Si no se ha especificado interfaz la obtiene de /proc/net/route */
	if(!flag_interfaz){
		interface = get_interfaz();
		if(interface){
			strncpy(interfaz,interface,TAM_INTERFACE);
			free(interface);
		} else { /* Si no somos capaces de obtenerla ponemos una por defecto */
			strcpy(interfaz,DEFAULT_INTERFACE);
		}
	}

	if(flag_c3po){
		/* Lanzamos prueba para el c3po */
		IRCPrueba_NOInterface(cadena,puerto?puerto:6669);

	} else {
		/* La función IRCInterface_Run debe ser llamada al final      */
		/* del main y es la que activa el interfaz gráfico quedándose */
		/* en esta función hasta que se pulsa alguna salida del       */
		/* interfaz gráfico.                                          */
		IRCInterface_Run(argc, argv);
	}

	exit(EXIT_SUCCESS);
}
