/**
  @file funciones_servidor.c
  @brief funciones referidas a los comandos
  @author Pablo Marcos  <pablo.marcos@estudiante.uam.es>
  @author Dionisio Perez  <dionisio.perez@estudiante.uam.es>
*/

#include "../includes/funciones_servidor.h"
#include "../includes/conexion_temp.h"

#include <redes2/irc.h>

/**
  @brief libera estructuras antes de cerrar servidor
  @return nada
*/
void liberarEstructuras(void){

	printf("Llamada libera estructura sin implementar\n");
	/* Liberar lista usuarios temporales */
	/* Liberar tads de eloy */
	/* Liberar mutex creado (descriptores y usuarios temporales */

}

/**
  @brief parsea el mensaje recibido
  @param pdesc: estructura con la informacion del mensaje
  @return NULL
*/
void *manejaMensaje(void* pdesc){

	pDatosMensaje datos;
	char *next;
	char *comando;
	datos = (pDatosMensaje) pdesc;

	/* Informacion de debugeo */
	next = datos->msg;

	while(next != NULL) {
        next = IRC_UnPipelineCommands(next, &comando);

        if(procesaComando(comando, datos) == COM_QUIT) {
			/* Se cierra la conexion */
		    if(comando!= NULL) {
				free(comando);
			}
			cerrarConexion(datos->sckfd);
			return NULL;
		}
		/* Libera memoria reservada comando */
        if(comando!= NULL) {
			free(comando);
		}
    }
	
	/* Incluimos el descriptor de nuevo y mandamos SIGUSR1 */
	addFd(datos->sckfd);

	/* Liberamos estructura de datos */
    liberaDatosMensaje(datos);

	return NULL;
}

/**
  @brief selecciona comandon del array a ejecutar
  @param comando: comando a ejecutar
  @param datos: estructura con la informacion del mensaje
  @return la funcion que ejecuta el comando
*/
status procesaComando(char *comando, pDatosMensaje datos){
	
	long dev;	

	dev = IRC_CommandQuery(comando);

	switch(dev){
		case IRCERR_NOCOMMAND:
		case IRCERR_NOPARAMS:
		case IRCERR_UNKNOWNCOMMAND:
			return comandoVacio(comando,datos);
		
		return COM_OK;
	}

	/* Comprobacion extra por si acaso */
	if(dev >= IRC_MAX_COMMANDS || dev < 0 ){
		return COM_ERROR;
	}

	return comandos[dev](comando, datos);
}

/**
  @brief crea una nueva conexion
  @param desc: socket del usuario
  @param address: estructura que almacena informacion de la direccion de internet
  @return el usuario temporal
*/
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
		ret = newTempUser(desc,  ip_a, "*");

	}

	addFd(desc);

	return ret;
}

/**
  @brief libera informacion del mensaje
  @param datos: estructura con la informacion del mensaje
  @return nada
*/
status cerrarConexion(int socket){

	syslog(LOG_DEBUG,"Cerrada conexion del socket %d", socket);
	deleteFd(socket);
	deleteTempUser(socket);
	close(socket);
	
	//Borrar posible usuario de la base de datos

	return COM_OK;
}

/**
  @brief inicializa el array de comandos
  @param void
  @return COM_OK
*/
void liberaDatosMensaje(pDatosMensaje datos){
	
	free(datos->msg);
	datos->msg = NULL;
	free(datos);
}

/**
  @brief inicializa el array de comandos
  @param void
  @return COM_OK
*/
status crea_comandos(void) {
    int i;    
    for(i=0; i< IRC_MAX_COMMANDS; i++) {
        comandos[i] = comandoVacio; 
	}

    comandos[NICK]=nick;
    comandos[USER]=user; 
	comandos[JOIN]=join;
	comandos[LIST]=list;

	return COM_OK;   
}

/**
  @brief libera informacion del usuario
  @param user: campo user de la estructura
  @param nick: campo nick de la estructura
  @param real: campo realname de la estructura
  @param host: campo host de la estructura
  @param IP: campo dir_IP de la estructura
  @param away: campo away de la estructura
  @return COM_OK
*/
status liberarUserData(char *user, char *nick, char *real, char *host, char *IP, char *away){

	if(user){ free(user); user = NULL; }
	if(nick){ free(nick); nick = NULL; }
	if(real){ free(real); real = NULL; }
	if(host){ free(host); host = NULL; }
	if(IP){ free(IP); IP = NULL; }
	if(away){ free(away); away = NULL; }

	return COM_OK;
}

/**
  @brief ejecuta el comando NICK
  @param comando: comando a ejecutar
  @param datos: estructura con la informacion del mensaje
  @return COM_OK si todo va bien. Error en otro caso
*/
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
			if(mensajeRespuesta) free(mensajeRespuesta);
			liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);

			return COM_OK;
	}

	/* Caso Nuevo usuario */
	if(unknown_id == 0) {

		sock = 0;
		/* Comprobar si el NICK esta utilizado */
		liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);
		ret = IRCTADUser_GetData (&unknown_id, &unknown_user, &nickk, &unknown_real, &host, &IP, &sock, &creationTS, &actionTS, &away);
		
		if(unknown_id != 0){
			syslog(LOG_DEBUG, "NICK NAME IN USE");
			IRCMsg_ErrNickNameInUse(&mensajeRespuesta, SERVICIO, "*", nickk);
			enviar(datos->sckfd, mensajeRespuesta);
			liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);
			if(mensajeRespuesta) free(mensajeRespuesta);
			return COM_OK;
		}


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

				/* Creamos prefijo para mensajes respuesta */
				if(IRC_ComplexUser1459(&prefijo2, nickk, unknown_user, host, NULL) != IRC_OK){
					if(prefijo2){
						free(prefijo2);
						prefijo2 = NULL;
					}
				}
				IRCMsg_Nick(&mensajeRespuesta, prefijo2, nickk, NULL);
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

	syslog(LOG_DEBUG, "Comando nick completado");

	return COM_OK;
}

/**
  @brief ejecuta el comando USER
  @param comando: comando a ejecutar
  @param datos: estructura con la informacion del mensaje
  @return COM_OK si todo va bien. Error en otro caso
*/
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

	if(!datos || !comando){
		return COM_ERROR;
	}

	ret = IRCTADUser_GetData (&unknown_id, &unknown_user, &unknown_nick, &unknown_real, &host, &IP, &sock, &creationTS, &actionTS, &away);

	/* Caso no hay suficiente memoria */
	if(ret == IRCERR_NOENOUGHMEMORY) {
		syslog(LOG_ERR, "Error Nick NOENOUGHMEMORY");
		liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);
		return COM_ERROR;
	}

	/* ErrAlreadyRegistred */
	if(unknown_id!= 0){
		IRCMsg_ErrAlreadyRegistred(&mensajeRespuesta, SERVICIO, unknown_nick);
		enviar(datos->sckfd, mensajeRespuesta);
		if(mensajeRespuesta) free(mensajeRespuesta);
		liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);
		return COM_OK;
	}

	liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);

	/* Parseamos comando */
	switch(IRCParse_User(comando,&prefijo,&user,&modehost,&server,&realname)){
		case IRCERR_NOSTRING:
		case IRCERR_ERRONEUSCOMMAND:
			syslog(LOG_ERR, "Error en IRCParse_User");
			IRCMsg_ErrNeedMoreParams (&mensajeRespuesta, SERVICIO ,"*", comando);
			enviar(datos->sckfd, mensajeRespuesta);

			if(mensajeRespuesta) free(mensajeRespuesta);
			if(prefijo) free(prefijo);
			if(user) free(user);
			if(modehost) free(modehost);
			if(server) free(server);
			if(realname) free(realname);

			return COM_OK;
	}

	/* Extraemos datos extructura temporal */
	usuarioTemporal = pullTempUser(datos->sckfd);
	if(usuarioTemporal == NULL){
		syslog(LOG_ERR, "Fallo en extraccion usuario temporal en USER");
		return COM_ERROR;
	}

	/* Caso NoNickNameGiven */
	if(usuarioTemporal->nick == NULL){
		IRCMsg_ErrNoNickNameGiven(&mensajeRespuesta, SERVICIO, "*");
		enviar(datos->sckfd, mensajeRespuesta);
		if(mensajeRespuesta) free(mensajeRespuesta);
		if(prefijo) free(prefijo);
		if(user) free(user);
		if(modehost) free(modehost);
		if(server) free(server);
		if(realname) free(realname);

		return COM_OK;
	}

	ret = IRCTADUser_New(user,usuarioTemporal->nick,realname,NULL,usuarioTemporal->host, usuarioTemporal->IP, datos->sckfd);
	switch(ret){
		case IRCERR_NOENOUGHMEMORY:
		case IRCERR_INVALIDHOST:
		case IRCERR_INVALIDIP:
		case IRCERR_INVALIDID:
		case IRCERR_INVALIDSOCKET:
		case IRCERR_NOMUTEX:

			syslog(LOG_ERR,"Error interno al incluir usuario en comando USER");
			return COM_OK;

		case IRCERR_NICKUSED:
			IRCMsg_ErrNickNameInUse(&mensajeRespuesta, SERVICIO ,"*", usuarioTemporal->nick);
			enviar(datos->sckfd, mensajeRespuesta);
			break;

		case IRCERR_INVALIDUSER:
			IRCMsg_ErrErroneusNickName (&mensajeRespuesta, SERVICIO, "*", user);
			enviar(datos->sckfd, mensajeRespuesta);
			break;

		case IRCERR_INVALIDNICK:
			IRCMsg_ErrErroneusNickName (&mensajeRespuesta, SERVICIO, "*", usuarioTemporal->nick);
			enviar(datos->sckfd, mensajeRespuesta);
			break;
		case IRCERR_INVALIDREALNAME:
			IRCMsg_ErrErroneusNickName (&mensajeRespuesta, SERVICIO, "*", realname);
			enviar(datos->sckfd, mensajeRespuesta);
			break;

		case IRC_OK:
			/* Creamos prefijo para mensajes respuesta */
			if(IRC_ComplexUser1459(&prefijo2,  usuarioTemporal->nick, user, usuarioTemporal->host, NULL) != IRC_OK){
				if(prefijo2){
					free(prefijo2);
					prefijo2 = NULL;
				}
			}

			IRCMsg_RplWelcome(&mensajeRespuesta, prefijo2, usuarioTemporal->nick, realname, user, SERVICIO);
			enviar(datos->sckfd, mensajeRespuesta);
			usuarioTemporal = NULL;
			deleteTempUser(datos->sckfd);
			break;
	}

	if(mensajeRespuesta) free(mensajeRespuesta);
	if(prefijo) free(prefijo);
	if(user) free(user);
	if(modehost) free(modehost);
	if(server) free(server);
	if(realname) free(realname);

	syslog(LOG_DEBUG, "Comando user completado");

	return COM_OK;
}

/**
  @brief ejecuta el comando JOIN
  @param comando: comando a ejecutar
  @param datos: estructura con la informacion del mensaje
  @return COM_OK si todo va bien. Error en otro caso
*/
status join(char *comando, pDatosMensaje datos){
	char *prefijo = NULL, *prefijo2 = NULL;
	char *unknown_user = NULL, *unknown_nick = NULL, *unknown_real = NULL;
	char *host = NULL, *IP = NULL, *away = NULL;
	char * mensajeRespuesta = NULL;
	char *canal, *clave, *msg;
	long unknown_id = 0, ret=0;
	long creationTS, actionTS;
	int sock;	


	sock = datos->sckfd;	

	ret = IRCTADUser_GetData (&unknown_id, &unknown_user, &unknown_nick, &unknown_real, &host, &IP, &sock, &creationTS, &actionTS, &away);

	/* Caso no hay suficiente memoria */
	if(ret == IRCERR_NOENOUGHMEMORY) {
		syslog(LOG_ERR, "Error JOIN NOENOUGHMEMORY");
		liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);
		return COM_ERROR;
	}

	/* ERRNOTREGISTERD */
	if(unknown_id == 0){
		IRCMsg_ErrNotRegisterd(&mensajeRespuesta, SERVICIO, "*");
		enviar(datos->sckfd, mensajeRespuesta);
		if(mensajeRespuesta) free(mensajeRespuesta);
		liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);
		return COM_OK;
	}

	switch(IRCParse_Join(comando, &prefijo, &canal, &clave, &msg)){
		case IRCERR_NOSTRING:
		case IRCERR_ERRONEUSCOMMAND:
			syslog(LOG_ERR, "Error en IRCParse_Join");
			IRCMsg_ErrNeedMoreParams (&mensajeRespuesta, SERVICIO ,unknown_nick, comando);
			enviar(datos->sckfd, mensajeRespuesta);

			if(mensajeRespuesta) free(mensajeRespuesta);
			if(prefijo) free(prefijo);
			if(canal) free(canal);
			if(msg) free(msg);	
			if(clave) free(clave);
			liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);
			return COM_OK;
	}

	/* Generamos prefijo usuario */
	
	switch(IRCTAD_Join(canal, unknown_nick, NULL, clave)){

		case IRCERR_NOENOUGHMEMORY:
			syslog(LOG_ERR, "Error IRCTAD_JOIN NOENOUGHMEMORY");
			if(prefijo) free(prefijo);
			if(canal) free(canal);
			if(msg) free(msg);	
			if(clave) free(clave);
			liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);
			return COM_ERROR;

		case IRCERR_NOVALIDCHANNEL:
			IRCMsg_ErrNoSuchChannel(&mensajeRespuesta, SERVICIO, unknown_nick, canal);
			break;

		case IRCERR_USERSLIMITEXCEEDED:
			IRCMsg_ErrChannelIsFull(&mensajeRespuesta, SERVICIO, unknown_nick, canal);
			break;
		
		case IRCERR_BANEDUSERONCHANNEL:
			IRCMsg_ErrBannedFromChan(&mensajeRespuesta, SERVICIO, unknown_nick, canal);
			break;

		case IRCERR_NOINVITEDUSER:
			IRCMsg_ErrInviteOnlyChan(&mensajeRespuesta, SERVICIO, unknown_nick, canal);
			break;

		case IRCERR_NOVALIDUSER:
			syslog(LOG_DEBUG, "NOT VALID USER");

		case IRCERR_YETINCHANNEL:
			IRCMsg_ErrUserOnChannel(&mensajeRespuesta, SERVICIO, unknown_nick, unknown_user, canal);
			break;

		case IRCERR_FAIL:
			IRCMsg_ErrBadChannelKey(&mensajeRespuesta, SERVICIO, unknown_nick, canal);
			break;

		case IRC_OK:
			/* Creamos prefijo para mensajes respuesta */
			if(IRC_ComplexUser1459(&prefijo2,  unknown_nick, unknown_user, host, NULL) != IRC_OK){
				if(prefijo2){
					free(prefijo2);
					prefijo2 = NULL;
				}
			}

			/* Primero respondemos JOIN */
			IRCMsg_Join(&mensajeRespuesta,prefijo2, NULL, NULL, canal);
			enviar(datos->sckfd, mensajeRespuesta);
			if(mensajeRespuesta){ free(mensajeRespuesta); mensajeRespuesta=NULL; }


			// Enviamos el topic 
			//enviar lista usuariaros canal
			//IRCMsg_RplNamReply(&mensajeRespuesta, SERVICIO, unknown_nick, "=", canal,
			break;



	}

	enviar(datos->sckfd, mensajeRespuesta);
	if(prefijo) free(prefijo);
	if(prefijo2) free(prefijo2);
	if(canal) free(canal);
	if(msg) free(msg);	
	if(clave) free(clave);
	liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);
	return COM_OK;

}

/**
  @brief Muestra los usuarios del canal
  @param
  @param
  @return 
*/
void listarUsuariosCanal(char** lista, ) {



}
/**
  @brief ejecuta el comando LIST
  @param comando: comando a ejecutar
  @param datos: estructura con la informacion del mensaje
  @return COM_OK si todo va bien. Error en otro caso
*/
status list(char *comando, pDatosMensaje datos){
	char *prefijo = NULL;
	char *unknown_user = NULL, *unknown_nick = NULL, *unknown_real = NULL;
	char *host = NULL, *IP = NULL, *away = NULL;
	char * mensajeRespuesta = NULL;
	long unknown_id = 0, ret=0;
	long creationTS, actionTS;
	int sock;	
	char *canal=NULL, *objetivo=NULL;
	char **lista = NULL;
	long num=0;
    char mascara=0;
    int i = 0;

	printf("LIST\n");
	sock = datos->sckfd;	

	ret = IRCTADUser_GetData (&unknown_id, &unknown_user, &unknown_nick, &unknown_real, &host, &IP, &sock, &creationTS, &actionTS, &away);

	/* Caso no hay suficiente memoria */
	if(ret == IRCERR_NOENOUGHMEMORY) {
		syslog(LOG_ERR, "Error JOIN NOENOUGHMEMORY");
		liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);
		return COM_ERROR;
	}

	/* ERRNOTREGISTERD */
	if(unknown_id == 0){
		IRCMsg_ErrNotRegisterd(&mensajeRespuesta, SERVICIO, "*");
		enviar(datos->sckfd, mensajeRespuesta);
		if(mensajeRespuesta) free(mensajeRespuesta);
		liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);
		return COM_OK;
	}
	printf("antes del parse list\n");
	switch(IRCParse_List(comando, &prefijo, &canal, &objetivo)){
		case IRCERR_NOSTRING:
		case IRCERR_ERRONEUSCOMMAND:
			printf( "Error en IRCLIST");
			IRCMsg_ErrNeedMoreParams (&mensajeRespuesta, SERVICIO ,unknown_nick, comando);
			enviar(datos->sckfd, mensajeRespuesta);
			if(mensajeRespuesta) free(mensajeRespuesta);
			if(prefijo) free(prefijo);
			if(canal) free(canal);
			if(objetivo) free(objetivo);	
			liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);

			return COM_OK;
	}

	printf("Despues del parse list\n");

   /* Obtenemos listas de canales */ 
    IRCTADChan_GetList(&lista, &num, NULL);
        
    /* Caso usuarios un canal concreto */
    if(canal!=NULL) {
        for (i=0; i<num; i++) {
            if(lista[i] == canal) {
                listarUsuariosCanal(lista[i], canal);
                break;
            }
        }
    

    }
    
	if(canal != NULL) {
		printf("Dentro de canal\n");
		switch(IRCTAD_ListNicksOnChannel(canal, &lista, &num)){
			case IRCERR_NOENOUGHMEMORY:
				syslog(LOG_ERR, "Error IRCTAD_JOIN NOENOUGHMEMORY");
				if(prefijo) free(prefijo);
				if(canal) free(canal);
				if(objetivo) free(objetivo);
				if(lista) free(lista);
				liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);
				return COM_ERROR;

			case IRCERR_NOVALIDCHANNEL:
				IRCMsg_ErrNoSuchChannel(&mensajeRespuesta, SERVICIO, unknown_nick, canal);
                printf("No valid channel\n");
				enviar(datos->sckfd, mensajeRespuesta);
				break;

			case IRC_OK:
				if(lista != NULL){
					IRCMsg_RplList(&mensajeRespuesta, SERVICIO, unknown_nick, canal, NULL, NULL);
					enviar(datos->sckfd, mensajeRespuesta);
                    if(mensajeRespuesta) printf("%s\n", mensajeRespuesta);
					printf("Lista no null\n");
                    break;
				}
                printf ("Lista null\n");
				break;
            default:
                printf("Caso no reconocido\n");

		}
	}
/*
		if(mensajeRespuesta) free(mensajeRespuesta);
		if(prefijo) free(prefijo);
		if(canal) free(canal);
		if(objetivo) free(objetivo);
		if(lista) free(lista);
		liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);*/
	return COM_OK;
}

/**
  @brief ejecuta el comando NO_COMMAND
  @param comando: comando a ejecutar
  @param datos: estructura con la informacion del mensaje
  @return COM_OK si todo va bien. Error en otro caso
*/
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
   	IRCMsg_ErrUnKnownCommand (&mensajeRespuesta, SERVICIO, unknown_nick ? unknown_nick : "*", comando);
    enviar(datos->sckfd, mensajeRespuesta);

	/* Liberamos estructuras */
    if(mensajeRespuesta) free(mensajeRespuesta);
	liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);

	return COM_OK;
}

