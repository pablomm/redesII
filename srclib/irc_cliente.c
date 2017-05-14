/**
 * @file irc_cliente.c
 * @brief Toda la parte de comandos de usuario
 * @author Pablo Marcos  <pablo.marcos@estudiante.uam.es>
 * @author Dionisio Perez  <dionisio.perez@estudiante.uam.es>
*/

/**
 * @defgroup ClienteIRC ClienteIRC
 *
 * <hr>
 */

#include "../includes/cliente.h"
#include "../includes/irc_cliente.h"
#include "../includes/red_cliente.h"
#include "../includes/comandos_noirc.h"
#include "../includes/ssl.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <redes2/irc.h>
#include <redes2/ircxchat.h>
#include <stdio.h>

typedef void (*comando_recepcion_t)(char *); 

comando_recepcion_t comandos_recv[IRC_MAX_COMMANDS];

/**
 * @addtogroup ClienteIRC
 * Comprende los comandos de respuestas y errores de clientes
 *
 * <hr>
 */

/**
 * @ingroup ClienteIRC
 *
 * @brief Funcion del hilo de recepcion de mensajes
 *
 * @synopsis
 * @code
 *	#include <irc_cliente.h>
 *
 * 	void *recv_pthread(void *sckfd)
 * @endcode
 * 
 * @param[in] sckfd puntero a numero de descriptor del socket
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *<hr>
*/
void *recv_pthread(void *sckfd){

	int socket = *((int*) sckfd);
	char buffer[TAM_RECV] = {0};
	char *next = NULL;
	char *comando = NULL;
	long ret;
	ssize_t tam;

	pthread_detach(pthread_self());

	inicializar_comandos_recv();

	tam = 1;

	while(tam) {

		if(ssl_active){
			tam = recibir_datos_SSL(socket, buffer, TAM_RECV-1);
		} else {
			tam = recv(socket, buffer, TAM_RECV-1, 0);
		}

		if(tam == -1){
			if(errno != EINTR)
				break;

		} else if(tam == 0){
		    IRCInterface_ErrorDialogThread("Conexion con el servidor cerrada. Cierre XCHAT");
			break;
		}

		next = buffer;
		while(next != NULL) {
        	next = IRC_UnPipelineCommands(next, &comando);

			IRCInterface_PlaneRegisterInMessageThread (comando);
			ret = IRC_CommandQuery (comando);

            switch(ret) {
                case IRCERR_NOCOMMAND:
                case IRCERR_UNKNOWNCOMMAND:
                    break;
                default:
                    comandos_recv[ret](comando);
            }

			free(comando);
		}
		memset(buffer, 0, tam);
	}
	pthread_exit(NULL);
}

/**
 * @ingroup ClienteIRC
 *
 * @brief Procesa el comando rplWelcome y ejecuta las acciones correspondientes
 *
 * @synopsis
 * @code
 *	#include <irc_cliente.h>
 *
 * 	void rplWelcome(char *comando)
 * @endcode
 * 
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *<hr>
*/
void rplWelcome(char *comando){
    char *prf=NULL, *nick=NULL, *msg=NULL;
	IRCParse_RplWelcome (comando, &prf, &nick, &msg);
	IRCInterface_WriteSystemThread("Welcome", msg);
	IRC_MFree(3, &nick, &msg, &prf);
}

/**
 * @ingroup ClienteIRC
 *
 * @brief Procesa el comando startMotd y ejecuta las acciones correspondientes
 *
 * @synopsis
 * @code
 *	#include <irc_cliente.h>
 *
 * 	void startMotd(char *comando)
 * @endcode
 * 
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *<hr>
*/
void startMotd(char *comando) {
    char *prf=NULL, *nick=NULL, *msg=NULL, *server=NULL;
	char mensaje[TAM_LINE];

    IRCParse_RplMotdStart (comando, &prf, &nick, &msg, &server);
	snprintf(mensaje, TAM_LINE, MOTD_START_MSG "%s", server?server:"");
    IRCInterface_WriteSystemThread(NULL, mensaje);
	IRC_MFree(4, &nick, &msg, &prf, &server);
}

/**
 * @ingroup ClienteIRC
 *
 * @brief Procesa el comando rplMotd y ejecuta las acciones correspondientes
 *
 * @synopsis
 * @code
 *	#include <irc_cliente.h>
 *
 * 	void rplMotd(char *comando)
 * @endcode
 * 
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *<hr>
*/
void rplMotd(char *comando) {
    char *prf=NULL, *nick=NULL, *msg=NULL;

    IRCParse_RplMotd (comando, &prf, &nick, &msg);
    IRCInterface_WriteSystemThread(NULL, msg);
	IRC_MFree(3, &nick, &msg, &prf);
}

/**
 * @ingroup ClienteIRC
 *
 * @brief Procesa el comando endMotd y ejecuta las acciones correspondientes
 *
 * @synopsis
 * @code
 *	#include <irc_cliente.h>
 *
 * 	void endMotd(char *comando)
 * @endcode
 * 
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *<hr>
*/
void endMotd(char *comando) {
	(void) comando;
	IRCInterface_WriteSystemThread(NULL, END_OF_MOTD_MSG);
}

/**
 * @ingroup ClienteIRC
 *
 * @brief Procesa el comando rplNotice y ejecuta las acciones correspondientes
 *
 * @synopsis
 * @code
 *	#include <irc_cliente.h>
 *
 * 	void rplNotice(char *comando)
 * @endcode
 * 
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *<hr>
*/
void rplNotice(char *comando){
   	char *prf=NULL, *nick=NULL, *msg=NULL;
	IRCParse_Notice(comando, &prf, &nick, &msg);
	IRCInterface_WriteSystemThread(NOTICE_TYPE, msg);
	IRC_MFree(3, &nick, &msg, &prf);
}

/**
 * @ingroup ClienteIRC
 *
 * @brief Procesa el comando rplPing y ejecuta las acciones correspondientes
 *
 * @synopsis
 * @code
 *	#include <irc_cliente.h>
 *
 * 	void rplPing(char *comando)
 * @endcode
 * 
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *<hr>
*/
void rplPing(char* comando) {
    char* prf = NULL, *server = NULL, *server2 = NULL;
    char* mensaje = NULL, *msg = NULL;

    IRCParse_Ping (comando, &prf, &server, &server2, &msg);
    IRCMsg_Pong(&mensaje, NULL, server, NULL, server); 
    enviar_clienteThread(mensaje);
	IRC_MFree(5, &prf, &server, &server2, &msg, &mensaje);
}

/**
 * @ingroup ClienteIRC
 *
 * @brief Procesa el comando rplJoin y ejecuta las acciones correspondientes
 *
 * @synopsis
 * @code
 *	#include <irc_cliente.h>
 *
 * 	void rplJoin(char *comando)
 * @endcode
 * 
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *<hr>
*/
void rplJoin(char *comando){

	char *prf = NULL, *canal=NULL, *clave=NULL, *msg = NULL;
    char *nick = NULL, *user = NULL, *host = NULL, *server = NULL;
	char mensaje[TAM_LINE];

    IRCParse_Join (comando, &prf, &canal, &clave, &msg); 
    IRCParse_ComplexUser (prf, &nick, &user, &host, &server);

	/* Si el canal no existe lo incluimos */
	if(!IRCInterface_QueryChannelExist(msg)) 
		IRCInterface_AddNewChannelThread(msg, IRCInterface_ModeToIntMode("w"));

	snprintf(mensaje, TAM_LINE, "%s" USER_IN_CHANNEL_MSG, nick);
    IRCInterface_WriteChannelThread (msg, nick, mensaje);
    IRCInterface_AddNickChannelThread (msg, nick, nick, nick, nick, NONE);

	IRC_MFree(8, &prf, &canal, &clave, &msg, &nick, &user, &host, &server);
}

/**
 * @ingroup ClienteIRC
 *
 * @brief Procesa el comando rplPrivMsg y ejecuta las acciones correspondientes
 *
 * @synopsis
 * @code
 *	#include <irc_cliente.h>
 *
 * 	void rplPrivMsg(char *comando)
 * @endcode
 * 
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *<hr>
*/
void rplPrivMsg(char* comando) {
    char *prf=NULL, *tar=NULL, *msg=NULL;
	char *nick=NULL, *user=NULL, *host=NULL, *server=NULL;

    IRCParse_Privmsg(comando, &prf, &tar, &msg);
    IRCParse_ComplexUser (prf, &nick, &user, &host, &server);

	if(!tar || ! msg)
		return;

	/* Caso mensaje especial */
	if(*msg == 1 ){
		parse_noIRC(msg+1,nick);

	/* Caso mensaje estandar */
	} else {


		if(*tar == '#' || *tar == '&') {
		if(!IRCInterface_QueryChannelExist(tar)) 
			IRCInterface_AddNewChannelThread(tar, IRCInterface_ModeToIntMode("w"));

		IRCInterface_WriteChannelThread(tar, nick, msg);
	} else {
		if(!IRCInterface_QueryChannelExist(nick)) {
			IRCInterface_AddNewChannelThread(nick, IRCInterface_ModeToIntMode("w"));
    		IRCInterface_AddNickChannelThread (nick, nick, nick, nick, nick, NONE);
			IRCInterface_AddNickChannelThread (nick, cliente.nick, cliente.nick, cliente.nick, cliente.nick, NONE);

		}

		IRCInterface_WriteChannelThread(nick, nick, msg);


	}

	}
    IRC_MFree(7, &prf, &tar, &msg, &nick, &user, &host, &server);
}

/**
 * @ingroup ClienteIRC
 *
 * @brief Procesa el comando rplNamReply y ejecuta las acciones correspondientes
 *
 * @synopsis
 * @code
 *	#include <irc_cliente.h>
 *
 * 	void rplNamReply(char *comando)
 * @endcode
 * 
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *<hr>
*/
void rplNamReply(char *comando){
	char *prf = NULL, *nick = NULL, *type = NULL;
    char *usr = NULL, *canal = NULL, *msg = NULL, *name=NULL; 
	char systemMsg[TAM_LINE_MED] = {0};

    IRCParse_RplNamReply(comando, &prf, &nick, &type, &canal, &msg);

    snprintf(systemMsg, TAM_LINE_MED-1, NAMREPLY_MSG"%s: %s", canal, msg);
    IRCInterface_WriteSystemThread(NULL, systemMsg);
    
	usr = strtok(msg, DELIMITERS);

    do { 
		name = (*usr=='@'||*usr=='+') ? usr+1 :usr;
        IRCInterface_AddNickChannel(canal, name, name,name, name, *usr=='@'?OPERATOR:*usr=='+'?VOICE:NONE);

    } while((usr = strtok(NULL, DELIMITERS)));

	IRC_MFree(5, &prf, &nick, &type, &canal, &msg); 
}

/**
 * @ingroup ClienteIRC
 *
 * @brief Procesa el comando rplNick y ejecuta las acciones correspondientes
 *
 * @synopsis
 * @code
 *	#include <irc_cliente.h>
 *
 * 	void rplNick(char *comando)
 * @endcode
 * 
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *<hr>
*/
void rplNick(char* comando) {

    char *prefix=NULL, *nick=NULL, *msg=NULL;
	char *old=NULL, *user=NULL, *host=NULL, *server=NULL;
    char mensaje[2*TAM_LINE] = {0};

    IRCParse_Nick(comando, &prefix, &nick, &msg);
    IRCParse_ComplexUser (prefix, &old, &user, &host, &server);
	snprintf(mensaje, 2*TAM_LINE - 1, NICK_CHANGE_SELF_MSG "%s -> %s", old, msg);
	IRCInterface_WriteSystemThread(nick, mensaje);
    IRCInterface_ChangeNickThread(old, msg);
	

	if(!strcmp(cliente.nick, old)){
		free(cliente.nick);
		cliente.nick = msg;
		IRC_MFree(6, &prefix, &nick, &old, &user, &host, &server);
	} else {
		IRC_MFree(7, &prefix, &nick, &msg, &old, &user, &host, &server);
	}
}

/**
 * @ingroup ClienteIRC
 *
 * @brief Procesa el comando rplQuit y ejecuta las acciones correspondientes
 *
 * @synopsis
 * @code
 *	#include <irc_cliente.h>
 *
 * 	void rplQuit(char *comando)
 * @endcode
 * 
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *<hr>
*/
void rplQuit(char* command) {
	int i,num = 0;
    char *prefix=NULL, *msg=NULL;
    char *nick=NULL, *user=NULL, *host=NULL, *server=NULL;
    char mensaje[2*TAM_LINE];
    char **canales;

    IRCParse_Quit(command, &prefix, &msg);
    IRCParse_ComplexUser (prefix, &nick, &user, &host, &server);
    snprintf(mensaje, 2*TAM_LINE, RPL_QUIT_MSG, nick, msg);
    IRCInterface_WriteSystemThread(NULL, mensaje);

	IRCInterface_ListAllChannelsThread (&canales, &num);
	
	for(i=0; i< num; i++){
    	IRCInterface_DeleteNickChannelThread(canales[i], nick);
	}

	IRC_MFree(6, &prefix, &msg, &nick, &user, &host, &server);
	IRCInterface_FreeListAllChannelsThread (canales, num);

}

/**
 * @ingroup ClienteIRC
 *
 * @brief Procesa el comando rplWhoisUser y ejecuta las acciones correspondientes
 *
 * @synopsis
 * @code
 *	#include <irc_cliente.h>
 *
 * 	void rplWhoisUser(char *comando)
 * @endcode
 * 
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *<hr>
*/
void rplWhoisUser(char* comando) {
    char *prefix= NULL, *nick=NULL, *dest=NULL, *user=NULL, *host=NULL, *realname=NULL;
    char mensaje[4*TAM_LINE]={0};

    IRCParse_RplWhoIsUser (comando, &prefix, &nick, &dest, &user, &host, &realname);
    snprintf(mensaje, 4*TAM_LINE -1, RPL_WHOISUSER_MSG, dest, user, host, realname); 
    IRCInterface_WriteSystemThread(WHOIS_TYPE, mensaje);   

  	IRC_MFree(6, &prefix, &nick, &dest, &user, &host, &realname);
}

/**
 * @ingroup ClienteIRC
 *
 * @brief Procesa el comando rplWhoisChannels y ejecuta las acciones correspondientes
 *
 * @synopsis
 * @code
 *	#include <irc_cliente.h>
 *
 * 	void rplWhoisChannels(char *comando)
 * @endcode
 * 
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *<hr>
*/
void rplWhoisChannels(char* comando) {
    char *prefix=NULL, *nick=NULL, *dest=NULL, *canales=NULL;
    char mensaje[TAM_LINE_MED] = {0};

    IRCParse_RplWhoIsChannels (comando, &prefix, &nick, &dest, &canales);
    snprintf(mensaje, TAM_LINE_MED -1 ,RPL_WHOISCHANNEL_MSG, dest, canales);
    IRCInterface_WriteSystemThread(NULL, mensaje);   
    IRC_MFree(4, &prefix, &nick, &dest, &canales);
}

/**
 * @ingroup ClienteIRC
 *
 * @brief Procesa el comando rplWhoisOperator y ejecuta las acciones correspondientes
 *
 * @synopsis
 * @code
 *	#include <irc_cliente.h>
 *
 * 	void rplWhoisOperator(char *comando)
 * @endcode
 * 
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *<hr>
*/
void rplWhoisOperator(char* comando) {
    char *prefix=NULL, *nick=NULL, *dest=NULL, *msg=NULL;
    char mensaje[TAM_LINE_MED] = {0};
    IRCParse_RplWhoIsOperator (comando, &prefix, &nick, &dest, &msg);
    snprintf(mensaje, TAM_LINE_MED-1, RPL_WHOISOPERATOR_MSG, dest);
    IRCInterface_WriteSystemThread(NULL, mensaje);   
    IRC_MFree(4, &prefix, &nick, &dest, &msg);
}

/**
 * @ingroup ClienteIRC
 *
 * @brief Procesa el comando rplWhoisServer y ejecuta las acciones correspondientes
 *
 * @synopsis
 * @code
 *	#include <irc_cliente.h>
 *
 * 	void rplWhoisServer(char *comando)
 * @endcode
 * 
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *<hr>
*/
void rplWhoisServer(char* comando) {
    char *prefix=NULL, *nick=NULL, *dest=NULL, *server=NULL, *info=NULL;
    char mensaje[TAM_LINE_MED] = {0};
    IRCParse_RplWhoIsServer(comando, &prefix, &nick, &dest, &server, &info);

    snprintf(mensaje, TAM_LINE_MED -1, RPL_WHOISSERVER_MSG, dest, server, info);
    IRCInterface_WriteSystemThread(NULL, mensaje);   

    IRC_MFree(5,&prefix, &nick, &dest, &server, &info);
}

/**
 * @ingroup ClienteIRC
 *
 * @brief Procesa el comando rplWhoisIdle y ejecuta las acciones correspondientes
 *
 * @synopsis
 * @code
 *	#include <irc_cliente.h>
 *
 * 	void rplWhoisIdle(char *comando)
 * @endcode
 * 
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *<hr>
*/
void rplWhoisIdle(char* comando) {
    char *prefix=NULL, *nick=NULL, *dest=NULL, *msg=NULL;
    char mensaje[TAM_LINE_MED] = {0};
    int secs1, secs2;

    IRCParse_RplWhoIsIdle(comando, &prefix, &nick, &dest, &secs1, &secs2, &msg);
   
    snprintf(mensaje, TAM_LINE_MED -1, RPL_WHOISIDLE_MSG, dest, secs1); 
    IRCInterface_WriteSystemThread(NULL, mensaje);   
    IRC_MFree(4, &prefix, &nick, &dest, &msg);
}

/**
 * @ingroup ClienteIRC
 *
 * @brief Procesa el comando rplList y ejecuta las acciones correspondientes
 *
 * @synopsis
 * @code
 *	#include <irc_cliente.h>
 *
 * 	void rplList(char *comando)
 * @endcode
 * 
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *<hr>
*/
void rplList(char* comando) {
    char* prefix=NULL, *nick=NULL, *canal=NULL, *visible=NULL, *topic=NULL;
    char mensaje[TAM_LINE_MED] = {0};

    IRCParse_RplList (comando, &prefix, &nick, &canal, &visible, &topic); 
    snprintf(mensaje, TAM_LINE_MED-1, RPL_LIST_MSG, canal, visible, topic?topic:"");
    IRCInterface_WriteSystemThread(NULL, mensaje);
	IRC_MFree(5, &prefix, &nick, &canal, &visible, &topic);
}

/**
 * @ingroup ClienteIRC
 *
 * @brief Procesa el comando errBannedFromChannel y ejecuta las acciones correspondientes
 *
 * @synopsis
 * @code
 *	#include <irc_cliente.h>
 *
 * 	void errBannedFromChannel(char *comando)
 * @endcode
 * 
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *<hr>
*/
void errBannedFromChannel(char* comando) {
    char *prefix=NULL, *nick=NULL, *canal=NULL, *msg=NULL;
    char mensaje[TAM_LINE_MED] = {0};

    IRCParse_ErrBannedFromChan (comando, &prefix, &nick, &canal, &msg);
    snprintf(mensaje, TAM_LINE_MED -1, BAN_RPL_MSG, nick, canal, msg?msg:"");
    IRCInterface_WriteChannelThread(canal, NULL, mensaje);

	IRCInterface_DeleteNickChannelThread(canal, nick);

	IRC_MFree(4, &prefix, &nick, &canal, &msg);

}

/**
 * @ingroup ClienteIRC
 *
 * @brief Procesa el comando rplNewTopic y ejecuta las acciones correspondientes
 *
 * @synopsis
 * @code
 *	#include <irc_cliente.h>
 *
 * 	void rplNewTopic(char *comando)
 * @endcode
 * 
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *<hr>
*/
void rplNewTopic(char* comando) {
    char *prefix=NULL, *topic=NULL, *canal=NULL;
	char *nick=NULL, *user=NULL, *host=NULL, *server=NULL;
    char mensaje[TAM_LINE_MED] = {0};

    IRCParse_Topic (comando, &prefix, &canal, &topic);
	IRCParse_ComplexUser (prefix, &nick, &user, &host, &server);
    snprintf(mensaje, TAM_LINE_MED - 1, RPL_TOPIC_MSG, nick, topic);

    IRCInterface_WriteChannelThread(canal, TOPIC_TYPE, mensaje);
    IRC_MFree(7, &prefix, &canal, &topic, &nick, &user, &host, &server);
}

/**
 * @ingroup ClienteIRC
 *
 * @brief Procesa el comando rplTopic y ejecuta las acciones correspondientes
 *
 * @synopsis
 * @code
 *	#include <irc_cliente.h>
 *
 * 	void rplTopic(char *comando)
 * @endcode
 * 
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *<hr>
*/
void rplTopic(char* comando) {
    char *prefix=NULL, *nick=NULL, *canal=NULL, *topic=NULL;
    char mensaje[TAM_LINE_MED] = {0};

    IRCParse_RplTopic(comando, &prefix, &nick, &canal, &topic);
    snprintf(mensaje, TAM_LINE_MED -1, TOPIC_MSG, topic);
    IRCInterface_WriteChannelThread(canal, TOPIC_TYPE, mensaje);
	IRC_MFree(4, &prefix, &nick, &canal, &topic);

}

/**
 * @ingroup ClienteIRC
 *
 * @brief Procesa el comando rplNoTopic y ejecuta las acciones correspondientes
 *
 * @synopsis
 * @code
 *	#include <irc_cliente.h>
 *
 * 	void rplNoTopic(char *comando)
 * @endcode
 * 
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *<hr>
*/
void rplNoTopic(char* comando) {
    char *prefix=NULL, *nick=NULL, *canal=NULL, *topic=NULL;
    IRCParse_RplNoTopic(comando, &prefix, &nick, &canal, &topic);
    IRCInterface_WriteChannelThread(canal, TOPIC_TYPE, NO_TOPIC_MSG);
    IRC_MFree(4, &prefix, &nick, &canal, &topic);
}

/**
 * @ingroup ClienteIRC
 *
 * @brief Procesa el comando errChanOpPrivIsNeeded y ejecuta las acciones correspondientes
 *
 * @synopsis
 * @code
 *	#include <irc_cliente.h>
 *
 * 	void errChanOpPrivIsNeeded(char *comando)
 * @endcode
 * 
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *<hr>
*/
void errChanOpPrivIsNeeded(char* comando) {
    char *prefix=NULL, *nick=NULL, *canal=NULL, *msg=NULL;
    IRCParse_ErrChanOPrivsNeeded(comando, &prefix, &nick, &canal, &msg);
    IRCInterface_WriteChannelThread(canal, NULL, ERR_OP_PRIV_MSG);
    IRC_MFree(4, &prefix, &nick, &canal, &msg);
}

/**
 * @ingroup ClienteIRC
 *
 * @brief Procesa el comando errNoSuchNick y ejecuta las acciones correspondientes
 *
 * @synopsis
 * @code
 *	#include <irc_cliente.h>
 *
 * 	void errNoSuchNick(char *comando)
 * @endcode
 * 
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *<hr>
*/
void errNoSuchNick(char* comando) {
    char *prefix=NULL, *nick=NULL, *dest=NULL, *msg=NULL;
    char mensaje[TAM_LINE_MED] = {0};

    IRCParse_ErrNoSuchNick(comando, &prefix, &nick,  &dest, &msg);
    snprintf(mensaje, TAM_LINE_MED -1, ERR_NO_SUCH_NICK_MSG, dest);
    IRCInterface_WriteSystemThread(NULL, mensaje);
	IRC_MFree(4, &prefix, &nick,  &dest, &msg);
    
}

/**
 * @ingroup ClienteIRC
 *
 * @brief Procesa el comando errNoSuchChannel y ejecuta las acciones correspondientes
 *
 * @synopsis
 * @code
 *	#include <irc_cliente.h>
 *
 * 	void errNoSuchChannel(char *comando)
 * @endcode
 * 
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *<hr>
*/
void errNoSuchChannel(char* comando) {
    char *prefix=NULL, *nick=NULL, *dest=NULL, *msg=NULL;
    char mensaje[TAM_LINE_MED] = {0};

    IRCParse_ErrNoSuchChannel(comando, &prefix, &nick,  &dest, &msg);
    snprintf(mensaje, TAM_LINE_MED -1, ERR_NO_SUCH_CHANNEL_MSG, dest);
    IRCInterface_WriteSystemThread(NULL, mensaje);
	IRC_MFree(4, &prefix, &nick,  &dest, &msg);
}

/**
 * @ingroup ClienteIRC
 *
 * @brief Procesa el comando rplPart y ejecuta las acciones correspondientes
 *
 * @synopsis
 * @code
 *	#include <irc_cliente.h>
 *
 * 	void rplPart(char *comando)
 * @endcode
 * 
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *<hr>
*/
void rplPart(char *comando){

    char *prefix=NULL, *canal=NULL, *msg=NULL;
    char *nick=NULL, *user=NULL, *host=NULL, *server=NULL;
    char mensaje[TAM_LINE_MED] = {0};

    IRCParse_Part(comando, &prefix, &canal, &msg); 
    IRCParse_ComplexUser (prefix, &nick, &user, &host, &server);

	/* Caso partimos nosotros */
    if(!strcmp(cliente.nick, nick)) {
       IRCInterface_RemoveChannelThread(canal); 

	/* Se va otro usuario */
    } else {
        snprintf(mensaje, TAM_LINE_MED -1, RPL_PART_MSG, nick, msg?msg:"");
        IRCInterface_WriteChannelThread(canal, NULL, mensaje);
        IRCInterface_DeleteNickChannelThread (canal, nick);
    }

	IRC_MFree(7, &prefix, &canal, &msg, &nick, &user, &host, &server);

}

/**
 * @ingroup ClienteIRC
 *
 * @brief Procesa el comando rplKick y ejecuta las acciones correspondientes
 *
 * @synopsis
 * @code
 *	#include <irc_cliente.h>
 *
 * 	void rplKick(char *comando)
 * @endcode
 * 
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *<hr>
*/
void rplKick(char* comando) {
    char *prefix=NULL, *canal=NULL, *msg=NULL; 
	char *nick=NULL, *user=NULL, *host=NULL, *server=NULL, *dest=NULL;
    char mensaje[TAM_LINE_MED] = {0};

    IRCParse_Kick (comando, &prefix, &canal, &dest, &msg);
    IRCParse_ComplexUser (prefix, &nick, &user, &host, &server);

	/* Caso nos han expulsado a nosotros */
    if(!strcmp(cliente.nick, dest)) {
       IRCInterface_RemoveChannelThread(canal); 
       snprintf(mensaje, TAM_LINE_MED-1, RPL_KICK_SELF_MSG, nick, msg);
       IRCInterface_WriteSystemThread(NULL, mensaje);
    } else {
       snprintf(mensaje, TAM_LINE_MED-1, RPL_KICK_MSG, nick, dest, msg);
       IRCInterface_DeleteNickChannelThread (canal, dest);
       IRCInterface_WriteChannelThread(canal, NULL, mensaje);
    }

	IRC_MFree(8, &prefix, &canal, &dest, &msg, &nick, &user, &host, &server);
}

/**
 * @ingroup ClienteIRC
 *
 * @brief Procesa el comando rplNowAway y ejecuta las acciones correspondientes
 *
 * @synopsis
 * @code
 *	#include <irc_cliente.h>
 *
 * 	void rplNowAway(char *comando)
 * @endcode
 * 
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *<hr>
*/
void rplNowAway(char* comando) { 
	(void) comando;
    IRCInterface_WriteSystemThread(NULL, AWAY_MSG);
}

/**
 * @ingroup ClienteIRC
 *
 * @brief Procesa el comando rplUnAway y ejecuta las acciones correspondientes
 *
 * @synopsis
 * @code
 *	#include <irc_cliente.h>
 *
 * 	void rplUnAway(char *comando)
 * @endcode
 * 
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *<hr>
*/
void rplUnAway(char* comando) {
	(void) comando;
	IRCInterface_WriteSystemThread(NULL, UNAWAY_MSG);
}

/**
 * @ingroup ClienteIRC
 *
 * @brief Procesa el comando rplAway y ejecuta las acciones correspondientes
 *
 * @synopsis
 * @code
 *	#include <irc_cliente.h>
 *
 * 	void rplAway(char *comando)
 * @endcode
 * 
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *<hr>
*/
void rplAway(char *comando) {
    char *prefix=NULL, *nick=NULL, *dest=NULL, *msg=NULL;
    char mensaje[TAM_LINE_MED] = {0};

    IRCParse_RplAway(comando, &prefix, &nick, &dest, &msg);
    snprintf(mensaje, TAM_LINE_MED -1, RPL_AWAY_MSG, msg);
    IRCInterface_WriteChannelThread(dest,dest,mensaje);
    IRC_MFree(4,&prefix, &nick, &dest, &msg);
}

/**
 * @ingroup ClienteIRC
 *
 * @brief Procesa el comando rplChannelModeIs y ejecuta las acciones correspondientes
 *
 * @synopsis
 * @code
 *	#include <irc_cliente.h>
 *
 * 	void rplChannelModeIs(char *comando)
 * @endcode
 * 
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *<hr>
*/
void rplChannelModeIs(char *comando) {
    char *prefix=NULL, *nick=NULL, *canal=NULL, *modo=NULL;
    char mensaje[TAM_LINE_MED] = {0};
    IRCParse_RplChannelModeIs(comando, &prefix, &nick, &canal, &modo);

    IRCInterface_SetModeChannelThread(canal, IRCInterface_ModeToIntModeThread(modo));
    snprintf(mensaje,TAM_LINE_MED -1, CHANNEL_MODE_IS_MSG, modo);
    IRCInterface_WriteChannelThread(canal, NULL, mensaje);
	IRC_MFree(4, &prefix, &nick, &canal, &modo);
}

/**
 * @ingroup ClienteIRC
 *
 * @brief Procesa el comando rplWhoReply y ejecuta las acciones correspondientes
 *
 * @synopsis
 * @code
 *	#include <irc_cliente.h>
 *
 * 	void rplWhoReply(char *comando)
 * @endcode
 * 
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *<hr>
*/
void rplWhoReply(char* comando) {
    char* prefix, *me, *canal, *user, *host, *server, *nick, *type, *msg, *realname;
    char mensaje[TAM_LINE_MED] = {0};
    int hops;
    IRCParse_RplWhoReply (comando, &prefix, &me, &canal, &user, &host, &server, &nick, &type, &msg, &hops, &realname);
	snprintf(mensaje, TAM_LINE_MED -1, RPL_WHO_MSG, nick, user, realname, host, msg?msg:"", canal);
    IRCInterface_WriteSystemThread(NULL, mensaje);
	IRC_MFree(10, &prefix, &me, &canal, &user, &host, &server, &nick, &type, &msg, &realname);
}

/**
 * @ingroup ClienteIRC
 *
 * @brief Procesa el comando rplMode y ejecuta las acciones correspondientes
 *
 * @synopsis
 * @code
 *	#include <irc_cliente.h>
 *
 * 	void rplMode(char *comando)
 * @endcode
 * 
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *<hr>
*/
void rplMode(char* comando) {
    char *prefix=NULL, *canal=NULL, *mode=NULL, *user=NULL;

    IRCParse_Mode (comando, &prefix, &canal, &mode, &user);
    if(*canal=='#' || *canal=='&') {
        if(user) {
            if(*mode=='+') {
                switch(mode[1]) {
                    case 'o':
                        IRCInterface_ChangeNickStateChannelThread(canal, user, OPERATOR);
                        break;
                    case 'v':
                        IRCInterface_ChangeNickStateChannelThread(canal, user, VOICE);
                        break;
                }
            } else if (*mode=='-') {
                switch(mode[1]) {
                    case 'o':
                        IRCInterface_ChangeNickStateChannelThread(canal, user, NONE);
                        break;
                    case 'v':
                        IRCInterface_ChangeNickStateChannelThread(canal, user, NONE);
                        break;
                }
            } else {
                if(strpbrk(mode, "o")!=NULL)
                    IRCInterface_ChangeNickStateChannelThread(canal, user, OPERATOR);
                else if(strpbrk(mode, "v")!=NULL)
                    IRCInterface_ChangeNickStateChannelThread(canal, user, VOICE);
                else IRCInterface_ChangeNickStateChannelThread(canal, user, NONE);
            }
        } else {
            if(*mode=='+') 
                IRCInterface_AddModeChannelThread (canal, IRCInterface_ModeToIntModeThread(mode+1));
            else if(*mode=='-')
                IRCInterface_DeleteModeChannelThread (canal, IRCInterface_ModeToIntModeThread(mode+1));
        }
    }

	IRC_MFree(4, &prefix, &canal, &mode, &user);

}

/**
 * @ingroup ClienteIRC
 *
 * @brief Procesa el comando errNickNameInUse y ejecuta las acciones correspondientes
 *
 * @synopsis
 * @code
 *	#include <irc_cliente.h>
 *
 * 	void errNickNameInUse(char *comando)
 * @endcode
 * 
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *<hr>
*/
void errNickNameInUse(char *comando){
	(void) comando;
    IRCInterface_ErrorDialogThread(ERR_NICK_NAME_IN_USE_MSG);
}

/**
 * @ingroup ClienteIRC
 *
 * @brief Procesa el comando errPassMismatch y ejecuta las acciones correspondientes
 *
 * @synopsis
 * @code
 *	#include <irc_cliente.h>
 *
 * 	void errPassMismatch(char *comando)
 * @endcode
 * 
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *<hr>
*/
void errPassMismatch(char *comando){
	(void) comando;
    IRCInterface_ErrorDialogThread(ERR_PASS_MISMATCH_MSG);
}

/**
 * @ingroup ClienteIRC
 *
 * @brief Procesa el comando errAlreadyRegistred y ejecuta las acciones correspondientes
 *
 * @synopsis
 * @code
 *	#include <irc_cliente.h>
 *
 * 	void errAlreadyRegistred(char *comando)
 * @endcode
 * 
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *<hr>
*/
void errAlreadyRegistred(char *comando){
	(void) comando;
    IRCInterface_WriteSystemThread(NULL, ERR_ALREADY_REGISTRED_MSG);
}

/**
 * @ingroup ClienteIRC
 *
 * @brief Procesa el comando errNoNickNameGiven y ejecuta las acciones correspondientes
 *
 * @synopsis
 * @code
 *	#include <irc_cliente.h>
 *
 * 	void errNoNickNameGiven(char *comando)
 * @endcode
 * 
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *<hr>
*/
void errNoNickNameGiven(char *comando){
	(void) comando;
    IRCInterface_ErrorDialogThread(ERR_NO_NICKNAME_GIVEN_MSG);
}

/**
 * @ingroup ClienteIRC
 *
 * @brief Procesa el comando errErroneusNickName y ejecuta las acciones correspondientes
 *
 * @synopsis
 * @code
 *	#include <irc_cliente.h>
 *
 * 	void errErroneusNickName(char *comando)
 * @endcode
 * 
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *<hr>
*/
void errErroneusNickName(char *comando){
	(void) comando;
    IRCInterface_ErrorDialogThread(ERR_ERRONEUS_NICKNAME_MSG);
}

/**
 * @ingroup ClienteIRC
 *
 * @brief Procesa el comando rplYourHost y ejecuta las acciones correspondientes
 *
 * @synopsis
 * @code
 *	#include <irc_cliente.h>
 *
 * 	void rplYourHost(char *comando)
 * @endcode
 * 
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *<hr>
*/
void rplYourHost(char *comando){
	char *prefix=NULL, *nick=NULL, *msg=NULL, *servername=NULL, *versionname=NULL;
    IRCParse_RplYourHost (comando, &prefix, &nick, &msg, &servername, &versionname);

	IRCInterface_WriteSystemThread("Host", msg);
	IRCInterface_WriteSystemThread("Server", servername);
	IRCInterface_WriteSystemThread("Version", versionname);

	IRC_MFree(5,&prefix, &nick, &msg, &servername, &versionname);

}

/**
 * @ingroup ClienteIRC
 *
 * @brief Procesa el comando rplMyInfo y ejecuta las acciones correspondientes
 *
 * @synopsis
 * @code
 *	#include <irc_cliente.h>
 *
 * 	void rplMyInfo(char *comando)
 * @endcode
 * 
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *<hr>
*/
void rplMyInfo(char *comando){
	char *prefix=NULL, *nick=NULL, *servername=NULL, *version=NULL, *usermodes=NULL, *channelmodes=NULL, *addedg=NULL;

	IRCParse_RplMyInfo(comando,&prefix, &nick, &servername, &version, &usermodes, &channelmodes, &addedg);

	IRCInterface_WriteSystemThread("User Modes", usermodes);
	IRCInterface_WriteSystemThread("Channel Modes", channelmodes);

	IRC_MFree(7,&prefix, &nick, &servername, &version, &usermodes, &channelmodes, &addedg);

}

/**
 * @ingroup ClienteIRC
 *
 * @brief Ignora el comando 
 *
 * @synopsis
 * @code
 *	#include <irc_cliente.h>
 *
 * 	void ignorar(char *comando)
 * @endcode
 * 
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *<hr>
*/
void ignorar(char *comando){
	(void) comando;
}

/**
 * @ingroup ClienteIRC
 *
 * @brief Procesa un comando desconocido, imprime el comando en system
 * @synopsis
 * @code
 *	#include <irc_cliente.h>
 *
 * 	void desconocido(char *comando)
 * @endcode
 * 
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *<hr>
*/
void desconocido(char *comando){
	IRCInterface_WriteSystemThread(NULL, comando);
}

/**
 * @ingroup ClienteIRC
 *
 * @brief Inicializa el array de comandos
 *
 * @synopsis
 * @code
 *	#include <irc_cliente.h>
 *
 * 	void inicializar_comandos_recv(void)
 * @endcode
 * 
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *<hr>
*/
void inicializar_comandos_recv(void){
	int i;

	for(i=0; i< IRC_MAX_COMMANDS ; i++){
		comandos_recv[i] = desconocido;
	}

	comandos_recv[RPL_WELCOME] = rplWelcome;
	comandos_recv[RPL_MOTDSTART] = startMotd;
	comandos_recv[RPL_MOTD] = rplMotd;
	comandos_recv[RPL_ENDOFMOTD] = endMotd;
	comandos_recv[NOTICE] = rplNotice;
	comandos_recv[PING] = rplPing;
	comandos_recv[JOIN] = rplJoin;
	comandos_recv[PRIVMSG] = rplPrivMsg;
	comandos_recv[RPL_NAMREPLY] = rplNamReply;
	comandos_recv[RPL_ENDOFNAMES] = ignorar;
	comandos_recv[PONG] = ignorar;
	comandos_recv[NICK] = rplNick;
	comandos_recv[QUIT] = rplQuit;
    comandos_recv[RPL_WHOISUSER] = rplWhoisUser;
    comandos_recv[RPL_WHOISCHANNELS] = rplWhoisChannels;
    comandos_recv[RPL_WHOISOPERATOR] = rplWhoisOperator;
    comandos_recv[RPL_WHOISSERVER] = rplWhoisServer;
    comandos_recv[RPL_WHOISIDLE] = rplWhoisIdle;
    comandos_recv[RPL_ENDOFWHOIS] = ignorar;
    comandos_recv[RPL_WHOREPLY] = rplWhoReply;
    comandos_recv[RPL_ENDOFWHO] = ignorar;
    comandos_recv[RPL_TOPICWHOTIME] = ignorar;
    comandos_recv[RPL_CREATIONTIME] = ignorar;
    comandos_recv[RPL_LIST] = rplList;
    comandos_recv[RPL_LISTEND] = ignorar;
	comandos_recv[ERR_BANNEDFROMCHAN]= errBannedFromChannel;
    comandos_recv[TOPIC] = rplNewTopic;
    comandos_recv[RPL_NOTOPIC] = rplNoTopic;
    comandos_recv[RPL_TOPIC] = rplTopic;
    comandos_recv[ERR_CHANOPRIVSNEEDED] = errChanOpPrivIsNeeded;
    comandos_recv[ERR_NOSUCHNICK]= errNoSuchNick;
    comandos_recv[ERR_NOSUCHCHANNEL] = errNoSuchChannel;
    comandos_recv[PART] = rplPart;
    comandos_recv[KICK] = rplKick;
    comandos_recv[MODE] = rplMode;
    comandos_recv[RPL_NOWAWAY] = rplNowAway;
	comandos_recv[RPL_UNAWAY] = rplUnAway;
	comandos_recv[RPL_AWAY] = rplAway;
    comandos_recv[RPL_CHANNELMODEIS] = rplChannelModeIs;
    comandos_recv[ERR_NICKNAMEINUSE] = errNickNameInUse;
    comandos_recv[ERR_NICKCOLLISION] = errNickNameInUse;
    comandos_recv[ERR_PASSWDMISMATCH] = errPassMismatch;
	comandos_recv[ERR_ALREADYREGISTRED] = errAlreadyRegistred;
	comandos_recv[ERR_NONICKNAMEGIVEN] = errNoNickNameGiven;
	comandos_recv[ERR_ERRONEUSNICKNAME] = errNoNickNameGiven;
	comandos_recv[SERVICE] = ignorar;
	comandos_recv[PASS] = ignorar;
	comandos_recv[RPL_YOURHOST] = rplYourHost;
	comandos_recv[RPL_MYINFO] = rplMyInfo;
}

