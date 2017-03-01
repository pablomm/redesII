#include "../includes/funciones_servidor.h"
#include "../includes/conexion_temp.h"

#include <redes2/irc.h>


void liberarEstructuras(void){

	printf("Llamada libera estructura\n");

}

void *manejaMensaje(void* pdesc){

	pDatosMensaje datos;
	char *next;
	char *comando;
	datos = (pDatosMensaje) pdesc;

	/* Informacion de debugeo */
    syslog(LOG_DEBUG, "Mensaje: [%s], len=%lu", (char*) datos->msg, datos->len);

	
    next = IRC_UnPipelineCommands (datos->msg, &comando);

    do {

        if(procesaComando(comando, datos) == COM_QUIT) {

			/* Se cierra la conexion */
			printf("Aqui ira llamada a cerrar conexion\n");

		    if(comando!= NULL) {
				free(comando);
			}
			break;
		}
		/* Libera memoria reservada comando */
        if(comando!= NULL) {
			free(comando);
		}

		/* Calcula siguiente comando */
        next = IRC_UnPipelineCommands(next, &comando);

    } while(next != NULL);
	
	/* Incluimos el descriptor de nuevo y mandamos SIGUSR1 */
	addFd(datos->sckfd);

	/* Liberamos estructura de datos */
    liberaDatosMensaje(datos);

	return NULL;
}

status procesaComando(char *comando, pDatosMensaje datos){
	
	long dev;	

	dev = IRC_CommandQuery(comando);

	switch(dev){
		case IRCERR_NOCOMMAND:

		case IRCERR_NOPARAMS:

		case IRCERR_UNKNOWNCOMMAND:
			return comandoVacio(comando,datos);
		
		return COM_ERROR;
	}

	/* Error en el comando */
	if(dev >= IRC_MAX_COMMANDS || dev < 0 ){
		
		return COM_ERROR;
	}

	return comandos[dev](comando, datos);
}

status nuevaConexion(int desc, struct sockaddr_in * address){
	status ret;
	struct hostent * host;
	char * ip_a = NULL;	


	deleteTempUser(desc);

	ip_a = inet_ntoa(address->sin_addr);
	if(ip_a == NULL){
		return COM_ERROR;
	}

	host = gethostbyaddr(address, sizeof(struct sockaddr_in), AF_INET);
	if(host != NULL){
		ret = newTempUser(desc,  ip_a, host->h_name);

	} else {
		ret = newTempUser(desc,  ip_a, "UNKNOWN_HOST");

	}

	addFd(desc);

	return ret;
}

void liberaDatosMensaje(pDatosMensaje datos){
	
	free(datos->msg);
	datos->msg = NULL;
	free(datos);
}

status crea_comandos(void) {
    int i;    
    for(i=0; i< IRC_MAX_COMMANDS; i++) {
        comandos[i] = comandoVacio; 
	}

    comandos[NICK]=nick;
    comandos[USER]=user; 

	return COM_OK;   
}



status liberarUserData(char *user, char *nick, char *real, char *host, char *IP, char *away){

	if(user){ free(user); user = NULL; }
	if(nick){ free(nick); nick = NULL; }
	if(real){ free(real); real = NULL; }
	if(host){ free(host); host = NULL; }
	if(IP){ free(IP); IP = NULL; }
	if(away){ free(away); away = NULL; }

	return COM_OK;
}

status nick(char* comando, pDatosMensaje datos){
	
	char *prefijo = NULL, *prefijo2 = NULL, *nickk = NULL, *msg = NULL;
	char *unknown_user = NULL, *unknown_nick = NULL, *unknown_real = NULL;
	char *host = NULL, *IP = NULL, *away = NULL;
	char * mensajeRespuesta = NULL;
	long unknown_id = 0, ret=0;
	long creationTS, actionTS;
	int sock;	
	pTempUser usuarioTemporal = NULL;

	if(!datos || !comando){
		return COM_ERROR;
	}

	sock =  datos->sckfd;

	/* Comprobamos si el socket esta utilizado */
	ret = IRCTADUser_GetData (&unknown_id, &unknown_user, &unknown_nick, &unknown_real, &host, &IP, &sock, &creationTS, &actionTS, &away);
	
	/* Caso no hay suficiente memoria */
	if(ret == IRCERR_NOENOUGHMEMORY) {
		syslog(LOG_ERR, "Error Nick NOENOUGHMEMORY");
		if(prefijo) free(prefijo);
		if(nickk) free(nickk);
		if(msg) free(msg);
		liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);

		return COM_ERROR;
	}

	/* Parseamos respuesta */
	switch(IRCParse_Nick(comando,&prefijo,&nickk, &msg)){
		case IRCERR_NOSTRING: /* Comando Invalido */
		case IRCERR_ERRONEUSCOMMAND:
			syslog(LOG_ERR, "Error en IRCParse_NICK");

			if(unknown_nick)
				IRCMsg_ErrNeedMoreParams(&mensajeRespuesta, SERVICIO, unknown_nick, comando);
			else
				IRCMsg_ErrNeedMoreParams(&mensajeRespuesta, SERVICIO, "*", comando);

			enviar(datos->sckfd, mensajeRespuesta);

			if(prefijo) {free(prefijo); prefijo=NULL;}
			if(msg) {free(msg); msg=NULL;}
			free(mensajeRespuesta);
			liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);

			return COM_OK;
	}



	/* Caso Nuevo usuario */
	if(unknown_id == 0) {

		//Comprobar si usuario ya utilizado


		 usuarioTemporal = pullTempUser(datos->sckfd);

		 /* Caso no se encuentra usuario temporal */
		 if(usuarioTemporal == NULL) {
			syslog(LOG_ERR, "Usuario %d temporal no encontrado", datos->sckfd);
			liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);
			return COM_ERROR;
		}

		setNickTemporal(usuarioTemporal, nickk);

	} else { /* Cambio de NICK */

		ret = IRCTADUser_Set(unknown_id, unknown_user, unknown_nick, unknown_real, NULL, nickk, NULL);
		switch(ret){
			/* no hay memoria suficiente */
			case IRCERR_NOENOUGHMEMORY:
				syslog(LOG_ERR, "No hay suficiente memoria llamada a nick");
				break;
	
			/* Usuario no encontrado */
			case IRCERR_INVALIDNICK:
			case IRCERR_INVALIDREALNAME:
			case IRCERR_INVALIDUSER:
				syslog(LOG_ERR,"Oops el usuario se ha perdido por el camino");
				break;
	
			/* Caso nuevo NICK en uso */
			case IRCERR_NICKUSED:
				IRCMsg_ErrNickNameInUse (&mensajeRespuesta, SERVICIO ,unknown_nick, nickk);
				enviar(datos->sckfd, mensajeRespuesta);
				break;

			case IRC_OK:
				// Complex USER para el prefijo???
				//IRCMsg_Nick();
				enviar(datos->sckfd, mensajeRespuesta);
				break;
		}
	}

	/* Liberamos estructuras */
	if(mensajeRespuesta) free(mensajeRespuesta);
	if(prefijo) free(prefijo);
	if(prefijo2) free(prefijo2);
	if(nickk) free(nickk);
	if(msg) free(msg);
	liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);

	return COM_OK;
}


status user(char* comando, pDatosMensaje datos){
	char *prefijo = NULL, *prefijo2 = NULL, *user = NULL;
	char *modehost = NULL, *server = NULL, *realname = NULL;
	char *unknown_user = NULL, *unknown_nick = NULL, *unknown_real = NULL;
	char *host = NULL, *IP = NULL, *away = NULL;
	char * mensajeRespuesta = NULL;
	long unknown_id = 0, ret=0;
	long creationTS, actionTS;
	int sock;	
	pTempUser usuarioTemporal = NULL;

	ret = IRCTADUser_GetData (&unknown_id, &unknown_user, &unknown_nick, &unknown_real, &host, &IP, &sock, &creationTS, &actionTS, &away);

	/* Caso no hay suficiente memoria */
	if(ret == IRCERR_NOENOUGHMEMORY) {
		syslog(LOG_ERR, "Error Nick NOENOUGHMEMORY");
		return COM_ERROR;
	}

	/* Creamos prefijo para mensajes respuesta */
	if(IRC_ComplexUser1459(&prefijo2, unknown_nick, unknown_user, host, NULL) != IRC_OK){
		if(prefijo2){
			free(prefijo2);
			prefijo2 = NULL;
		}
	}

	/* Parseamos comando */
	switch(IRCParse_User(comando,&prefijo,&user,&modehost,&server,&realname)){
		
		case IRCERR_NOSTRING:
		case IRCERR_ERRONEUSCOMMAND:

		syslog(LOG_ERR, "Error en IRCParse_User");

		if(unknown_nick != NULL)
			IRCMsg_ErrNeedMoreParams (&mensajeRespuesta, SERVICIO ,unknown_nick, comando);
		else
			IRCMsg_ErrNeedMoreParams (&mensajeRespuesta, SERVICIO ,"***", comando);

		enviar(datos->sckfd, mensajeRespuesta);
		if(mensajeRespuesta) free(mensajeRespuesta);
	
		return COM_OK;
	}

	/* Caso ErrAlreadyRegistred */
	if(unknown_id != 0) {
		IRCMsg_ErrAlreadyRegistred(&mensajeRespuesta, prefijo2, unknown_nick);
		enviar(datos->sckfd, mensajeRespuesta);
		if(mensajeRespuesta) free(mensajeRespuesta);
		liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);
		return COM_OK;
	}

	return COM_OK;
}
status comandoVacio(char* comando, pDatosMensaje datos){
	char *mensajeRespuesta = NULL;
	char *unknown_user = NULL, *unknown_nick = NULL, *unknown_real = NULL;
	char *host = NULL, *IP = NULL, *away = NULL;
	long unknown_id = 0;
	long creationTS, actionTS;
	int sock;

	if(!comando || !datos){
		return COM_ERROR;
	}

	sock = datos->sckfd;

	/* Obtenemos identificador del usuario */
	IRCTADUser_GetData(&unknown_id, &unknown_user, &unknown_nick, &unknown_real, &host, &IP, &sock, &creationTS, &actionTS, &away);
    syslog(LOG_DEBUG, "Comando No reconocido %s", comando);

	/* Variante si el usuario esta registrado */
	if(unknown_nick){
    	IRCMsg_ErrUnKnownCommand (&mensajeRespuesta, SERVICIO, unknown_nick, comando);
	    enviar(datos->sckfd, mensajeRespuesta);
	}

	/* Liberamos estructuras */
    if(mensajeRespuesta) free(mensajeRespuesta);
	liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);

	return COM_OK;
}

