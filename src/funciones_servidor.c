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


	//IRCTAD_ShowAll();

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
	ret = newTempUser(desc,  ip_a, host ? host->h_name : "*");

	addFd(desc);

	return ret;
}

/**
  @brief libera informacion del mensaje
  @param datos: estructura con la informacion del mensaje
  @return nada
*/
status cerrarConexion(int socket){
	char *unknown_user = NULL, *unknown_nick = NULL, *unknown_real = NULL;
	char *host = NULL, *IP = NULL, *away = NULL;
	long unknown_id = 0;
	long creationTS=0, actionTS=0;
	int sock =socket;
	char **lista = NULL;
	long num=0,i;

	IRCTADUser_GetData(&unknown_id, &unknown_user, &unknown_nick, &unknown_real, &host, &IP, &sock, &creationTS, &actionTS, &away);

	/* Abandona todos los canales */
	/*
	IRCTAD_ListChannelsOfUserArray(unknown_user, unknown_nick, &lista, &num);
	for(i=0; i<num; i++){
		IRCTAD_Part(lista[i], unknown_nick);
	}*/

	/* Eliminamos de la base de datos */
	IRCTADUser_Delete(unknown_id, unknown_user, unknown_nick, unknown_real);
	liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);

	if(lista){
		for(i=0; i<num; i++){
			if(lista[i]) free(lista[i]);
		}
		free(lista);
	}
	deleteTempUser(socket);
	close(socket);
	deleteFd(socket);

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

    comandos[NICK] = nick;
    comandos[USER] = user; 
	comandos[JOIN] = join;
	comandos[LIST] = list;
	comandos[WHOIS] = whois;
	comandos[NAMES] = names;
	comandos[PRIVMSG] = privmsg;
	comandos[PING] = ping;
	comandos[PART] = part;
	comandos[TOPIC] = topic;
	comandos[KICK] = kick;
	comandos[AWAY] = away;
	comandos[MODE] = mode;
	comandos[QUIT] = quit;

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
	
	char *prefijo = NULL, *nickk = NULL, *msg = NULL;
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

				IRCMsg_Nick(&mensajeRespuesta, SERVICIO, NULL, nickk);
				enviar(datos->sckfd, mensajeRespuesta);
				break;
		}
	}

	/* Liberamos estructuras */
	if(mensajeRespuesta) free(mensajeRespuesta);
	if(prefijo) free(prefijo);
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
	char *prefijo = NULL, *user = NULL;
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

			IRCMsg_RplWelcome(&mensajeRespuesta, SERVICIO, usuarioTemporal->nick, realname, user, SERVICIO);
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
	char *prefijo = NULL;
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

			/* Primero respondemos JOIN */
			IRCMsg_Join(&mensajeRespuesta,SERVICIO, NULL, NULL, canal);
			enviar(datos->sckfd, mensajeRespuesta);
			if(mensajeRespuesta){ free(mensajeRespuesta); mensajeRespuesta=NULL; }


			// Enviamos el topic 
			//enviar lista usuariaros canal
			//IRCMsg_RplNamReply(&mensajeRespuesta, SERVICIO, unknown_nick, "=", canal,
			break;



	}

	enviar(datos->sckfd, mensajeRespuesta);
	if(prefijo) free(prefijo);
	if(canal) free(canal);
	if(msg) free(msg);	
	if(clave) free(clave);
	liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);
	return COM_OK;

}


void listarCanalesUsuario(char *canal, int sckfd, char *prefijo, char *nicku) {
	char *mensajeRespuesta = NULL;
	char *topico = NULL;
	char *mode=NULL;
	int modeInt=0;

	/* Si el modo de canal es secreto no se muestra */
	modeInt = IRCTADChan_GetModeInt(canal);
	if(modeInt&IRCMODE_SECRET){
		return;
	}

	mode = IRCTADChan_GetModeChar(canal);
	IRCTAD_GetTopic(canal, &topico);
	IRCMsg_RplList(&mensajeRespuesta, SERVICIO, nicku, canal, mode, topico?topico:"");
	enviar(sckfd, mensajeRespuesta);
	
	if(mode) free(mode);
	if(mensajeRespuesta) free(mensajeRespuesta);
	if(topico) free(topico);

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
	long creationTS=0, actionTS=0;
	int sock=0;	
	char *canal=NULL, *objetivo=NULL;
	char **lista = NULL;
	long num=0;
    int i = 0;
	char *next=NULL;

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

	switch(IRCParse_List(comando, &prefijo, &canal, &objetivo)){
		case IRCERR_NOSTRING:
		case IRCERR_ERRONEUSCOMMAND:
			IRCMsg_ErrNeedMoreParams (&mensajeRespuesta, SERVICIO ,unknown_nick, comando);
			enviar(datos->sckfd, mensajeRespuesta);
			if(mensajeRespuesta) free(mensajeRespuesta);
			if(prefijo) free(prefijo);
			if(canal) free(canal);
			if(objetivo) free(objetivo);	
			liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);

			return COM_OK;
	}

	/* Caso listar todos los canales */
	if(canal == NULL){

    	IRCTADChan_GetList(&lista, &num, NULL);
		for(i=0; i<num; i++){
			listarCanalesUsuario(lista[i], datos->sckfd, SERVICIO, unknown_nick);
		}
    
		/* Liberamos lista y mensaje respuesta */
		if(lista){
			for(i=0; i < num; i++){
				if(lista[i]) free(lista[i]);
			}
			free(lista);
		}

	/* Caso una lista de canales */
    } else {
    
		next = strtok(canal,",");
		while(next != NULL){
			listarCanalesUsuario(next, datos->sckfd, SERVICIO, unknown_nick);
			next = strtok(NULL,",");
		}
		free(canal);
	}

		IRCMsg_RplListEnd(&mensajeRespuesta, SERVICIO,unknown_nick);
		enviar(datos->sckfd, mensajeRespuesta);

		if(mensajeRespuesta) free(mensajeRespuesta);
		if(prefijo) free(prefijo);
		if(objetivo) free(objetivo);
		liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);
	return COM_OK;
}


void whoischannels(int sckfd, char *nick1, char *nick2, char* user, char *target){
	char *mensajeRespuesta=NULL;
	char *lista = NULL;
	long num=0;

	IRCTAD_ListChannelsOfUser (user, nick2, &lista, &num);
	IRCMsg_RplWhoIsChannels (&mensajeRespuesta, SERVICIO, nick1, nick2, lista);
	enviar(sckfd, mensajeRespuesta);

	if(mensajeRespuesta) free(mensajeRespuesta);
	if(lista) free(lista);
}

void sendAway(int sckfd, char *away, char *nick, char *nickorig){
	char *msgAway;

	if(away!=NULL){
		IRCMsg_RplAway(&msgAway, SERVICIO, nick, nickorig, away);
		enviar(sckfd, msgAway);
		if(msgAway) free(msgAway);
	}
}

/**
  @brief ejecuta el comando WHOIS
  @param comando: comando a ejecutar
  @param datos: estructura con la informacion del mensaje
  @return COM_OK si todo va bien. Error en otro caso
*/
status whois(char *comando, pDatosMensaje datos){
	char *mensajeRespuesta = NULL;
	char *unknown_user = NULL, *unknown_nick = NULL, *unknown_real = NULL;
	char *host = NULL, *IP = NULL, *away = NULL;
	long unknown_id = 0, ret=0;
	long creationTS=0, actionTS=0;
	int sock;
	char *prefijo=NULL,*target=NULL,*mask=NULL;
	char *next;
	int sock1;
	long id1;
	char *user1=NULL,*real1=NULL,*host1=NULL,*ip1=NULL,*away1=NULL;

	if(!comando || !datos){
		return COM_ERROR;
	}

	sock = datos->sckfd;

	/* Obtenemos identificador del usuario */
	IRCTADUser_GetData(&unknown_id, &unknown_user, &unknown_nick, &unknown_real, &host, &IP, &sock, &creationTS, &actionTS, &away);

	/* Caso no hay suficiente memoria */
	if(ret == IRCERR_NOENOUGHMEMORY) {
		syslog(LOG_ERR, "Error WHOIS NOENOUGHMEMORY");
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

	/* Parseamos el comando */
	switch(IRCParse_Whois (comando, &prefijo, &target, &mask)){
		case IRCERR_NOSTRING:
		case IRCERR_ERRONEUSCOMMAND:
			IRCMsg_ErrNoNickNameGiven(&mensajeRespuesta, SERVICIO, unknown_nick);
			enviar(datos->sckfd, mensajeRespuesta);
			if(mensajeRespuesta) free(mensajeRespuesta);
			if(prefijo) free(prefijo);
			if(target) free(target);
			if(mask) free(mask);
			liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);
			return COM_OK;
			break;
	}

	next = strtok(mask,",");
	while(next != NULL){

		id1 =0;
		user1 = NULL;
		real1 = NULL;
		host1 = NULL;
		ip1 = NULL;
		sock1 = 0;
		creationTS = 0;
		actionTS = 0;
		away1 = NULL;

		/* Sacamos informacion del usuario especifico */
		IRCTADUser_GetData(&id1, &user1, &next, &real1, &host1, &ip1, &sock1, &creationTS, &actionTS, &away1);
		if(id1 == 0){
			IRCMsg_ErrNoSuchNick (&mensajeRespuesta, SERVICIO, unknown_nick, next);
			enviar(datos->sckfd, mensajeRespuesta);
			if(mensajeRespuesta) { free(mensajeRespuesta); mensajeRespuesta=NULL;}
		} else {
			/* Envio whois channels */
			whoischannels(datos->sckfd, unknown_nick, next, user1, target);
			/* Who is user */
			IRCMsg_RplWhoIsUser(&mensajeRespuesta, SERVICIO, unknown_nick, next, user1, host1, real1);
			enviar(datos->sckfd, mensajeRespuesta);
			if(mensajeRespuesta) {free(mensajeRespuesta); mensajeRespuesta=NULL; }
			/* Away info */
			sendAway(datos->sckfd, away1, next, unknown_nick);
			/* Who is server */
			IRCMsg_RplWhoIsServer (&mensajeRespuesta, SERVICIO, unknown_nick, next, SERVICIO, "Servidor IRC Redes II");
			enviar(datos->sckfd, mensajeRespuesta);
			if(mensajeRespuesta) {free(mensajeRespuesta); mensajeRespuesta=NULL; }
		}
	
		IRCMsg_RplEndOfWhoIs (&mensajeRespuesta, SERVICIO, unknown_nick, next);
		enviar(datos->sckfd, mensajeRespuesta);
		if(mensajeRespuesta) {free(mensajeRespuesta); mensajeRespuesta=NULL; }
		liberarUserData(user1, NULL, real1, host1, ip1, away1);

		next = strtok(NULL, ",");
	}
	
	if(prefijo) free(prefijo);
	if(target) free(target);
	if(mask) free(mask);
	liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);

	return COM_OK;
}


void listarNamesCanal(char *canal, int sckfd, char *prefijo, char *nicku) {
	char *mensajeRespuesta = NULL;
	char *lista;
	long num=0;

	if(IRCTAD_ListNicksOnChannel(canal, &lista, &num) == IRC_OK){
		IRCMsg_RplNamReply (&mensajeRespuesta, SERVICIO, nicku, "=", canal, lista);
		enviar(sckfd, mensajeRespuesta);
	}
	if(mensajeRespuesta) free(mensajeRespuesta);
	if(lista) free(lista);

}

/**
  @brief ejecuta el comando NAMES
  @param comando: comando a ejecutar
  @param datos: estructura con la informacion del mensaje
  @return COM_OK si todo va bien. Error en otro caso
*/
status names(char *comando, pDatosMensaje datos){
	char *mensajeRespuesta = NULL;
	char *unknown_user = NULL, *unknown_nick = NULL, *unknown_real = NULL;
	char *host = NULL, *IP = NULL, *away = NULL;
	long unknown_id = 0, ret=0;
	long creationTS=0, actionTS=0;
	int sock;
	char *canal=NULL, *prefijo=NULL, *objetivo=NULL;
	char **lista =NULL;
	int i;
	char *next = NULL;
	long num;

	if(!comando || !datos){
		return COM_ERROR;
	}

	sock = datos->sckfd;

	/* Obtenemos identificador del usuario */
	IRCTADUser_GetData(&unknown_id, &unknown_user, &unknown_nick, &unknown_real, &host, &IP, &sock, &creationTS, &actionTS, &away);

	/* Caso no hay suficiente memoria */
	if(ret == IRCERR_NOENOUGHMEMORY) {
		syslog(LOG_ERR, "Error NAMES NOENOUGHMEMORY");
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


	switch(IRCParse_Names (comando, &prefijo, &canal, &objetivo)){
		case IRCERR_NOSTRING:
		case IRCERR_ERRONEUSCOMMAND:
			IRCMsg_ErrNeedMoreParams (&mensajeRespuesta, SERVICIO ,unknown_nick, comando);
			enviar(datos->sckfd, mensajeRespuesta);
			if(mensajeRespuesta) free(mensajeRespuesta);
			if(prefijo) free(prefijo);
			if(canal) free(canal);
			if(objetivo) free(objetivo);	
			liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);

			return COM_OK;
	}

	/* Caso listar todos los canales */
	if(canal == NULL){

    	IRCTADChan_GetList(&lista, &num, NULL);
		for(i=0; i<num; i++){
			listarNamesCanal(lista[i], datos->sckfd, SERVICIO, unknown_nick);
		}
    
		/* Liberamos lista y mensaje respuesta */
		if(lista){
			for(i=0; i < num; i++){
				if(lista[i]) free(lista[i]);
			}
			free(lista);
		}

		// Habria que mandar lista usuarios privados

		IRCMsg_RplEndOfNames (&mensajeRespuesta, SERVICIO, unknown_nick, "*");
		enviar(datos->sckfd, mensajeRespuesta);
		if(mensajeRespuesta) { free(mensajeRespuesta); mensajeRespuesta=NULL;}

	/* Caso una lista de canales */
    } else {
    
		next = strtok(canal,",");
		while(next != NULL){
			listarNamesCanal(next, datos->sckfd, SERVICIO, unknown_nick);
			IRCMsg_RplEndOfNames (&mensajeRespuesta, SERVICIO, unknown_nick, next);
			enviar(datos->sckfd, mensajeRespuesta);
			if(mensajeRespuesta) { free(mensajeRespuesta); mensajeRespuesta=NULL;}

			next = strtok(NULL,",");
		}
		free(canal);
	}

		if(mensajeRespuesta) free(mensajeRespuesta);
		if(prefijo) free(prefijo);
		if(objetivo) free(objetivo);
		liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);
	return COM_OK;
}


void enviarMensajePrivado(int sckfd, char *mensaje, char *nick, char * nickorigin, int sndaway){
	char *mensajeError = NULL;
	char *unknown_user = NULL, *unknown_real = NULL;
	char *host = NULL, *IP = NULL, *away = NULL;
	long unknown_id = 0;
	long creationTS=0, actionTS=0;
	int sock = 0;

	/* Obtenemos datos usuario target con el nick */
	IRCTADUser_GetData(&unknown_id, &unknown_user, &nick, &unknown_real, &host, &IP, &sock, &creationTS, &actionTS, &away);
	if(unknown_id == 0){
		IRCMsg_ErrNoSuchNick (&mensajeError, SERVICIO, nickorigin, nick);
		enviar(sckfd, mensajeError);	
		if(mensajeError) free(mensajeError);
	} else {
		enviar(sock, mensaje);
		if(away!=NULL && sndaway!=0){
			sendAway(sckfd, away, nick, nickorigin);
		}
	}
	liberarUserData(unknown_user, NULL, unknown_real, host, IP, away);
}


void enviarMensajeACanal(int sckfd, char *mensaje, char *canal, char * nickorigin){
	char *mensajeError = NULL;
	char **lista = NULL;
	long num = 0;
	long i;

	// Habria que comprobar si tiene permisos

	/* Obtenemos datos usuario target con el nick */
	if(IRCTAD_ListNicksOnChannelArray(canal, &lista, &num) != IRC_OK){

		IRCMsg_ErrNoSuchNick (&mensajeError, SERVICIO, nickorigin, canal);
		enviar(sckfd, mensajeError);
		if(mensajeError) free(mensajeError);
		return;	
	}

	/* Enviamos mensajes a usuarios del canal */	
	for(i=0; i<num; i++) {
		enviarMensajePrivado(sckfd, mensaje, lista[i], nickorigin,0);
	}

	/* Liberamos la lista de usuarios */
	if(lista){
		for(i=0; i<num; i++){
			if(lista[i]) free(lista[i]);
		}
		free(lista);
	}
}

/**
  @brief ejecuta el comando PRIVMSG
  @param comando: comando a ejecutar
  @param datos: estructura con la informacion del mensaje
  @return COM_OK si todo va bien. Error en otro caso
*/
status privmsg(char *comando, pDatosMensaje datos){
	char *mensajeRespuesta = NULL;
	char *unknown_user = NULL, *unknown_nick = NULL, *unknown_real = NULL;
	char *host = NULL, *IP = NULL, *away = NULL;
	long unknown_id = 0, ret=0;
	long creationTS=0, actionTS=0;
	int sock;
	char *msgtarget=NULL, *msg=NULL,*prefijo=NULL, *target=NULL;
	char *mensaje=NULL, *prefijo2=NULL;

	if(!comando || !datos){
		return COM_ERROR;
	}

	sock = datos->sckfd;

	/* Obtenemos identificador del usuario */
	IRCTADUser_GetData(&unknown_id, &unknown_user, &unknown_nick, &unknown_real, &host, &IP, &sock, &creationTS, &actionTS, &away);

	/* Caso no hay suficiente memoria */
	if(ret == IRCERR_NOENOUGHMEMORY) {
		syslog(LOG_ERR, "Error WHOIS NOPRIV");
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

	switch(IRCParse_Privmsg (comando, &prefijo, &msgtarget, &msg)){
		case IRCERR_NOSTRING:
		case IRCERR_ERRONEUSCOMMAND:
			IRCMsg_ErrNoTextToSend(&mensajeRespuesta, SERVICIO ,unknown_nick);
			enviar(datos->sckfd, mensajeRespuesta);
			if(mensajeRespuesta) free(mensajeRespuesta);
			if(prefijo) free(prefijo);
			if(msgtarget) free(msgtarget);
			if(msg) free(msg);
			liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);

			return COM_OK;
	}


	IRC_ComplexUser(&prefijo2, unknown_nick, unknown_real, host, SERVICIO);
		
	target = strtok(msgtarget,",");
	while(target != NULL){

		/* Creamos el mensaje */
		IRCMsg_Privmsg (&mensaje, prefijo2, target, msg);

		/* Caso mensaje a canal */
		if(*target == '#' || *target == '&'){
			enviarMensajeACanal(datos->sckfd, mensaje, target,unknown_nick );
		} else { /* Caso mensaje a usuario */
			enviarMensajePrivado(datos->sckfd, mensaje, target,unknown_nick,1);
		}

		target = strtok(NULL,",");
		if(mensaje) { free(mensaje); mensaje=NULL;}
	}
	if(prefijo) free(prefijo);
	if(prefijo2) free(prefijo2);
	if(msgtarget) free(msgtarget);
	if(msg) free(msg);
	liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);

	return COM_OK;
}

/**
  @brief ejecuta el comando PING
  @param comando: comando a ejecutar
  @param datos: estructura con la informacion del mensaje
  @return COM_OK si todo va bien. Error en otro caso
*/
status ping(char *comando, pDatosMensaje datos){
	char *mensajeRespuesta = NULL;
	char *unknown_user = NULL, *unknown_nick = NULL, *unknown_real = NULL;
	char *host = NULL, *IP = NULL, *away = NULL;
	long unknown_id = 0, ret=0;
	long creationTS=0, actionTS=0;
	int sock;
	char *prefijo=NULL, *server=NULL, *server2=NULL,*msg=NULL;

	if(!comando || !datos){
		return COM_ERROR;
	}

	sock = datos->sckfd;

	/* Obtenemos identificador del usuario */
	IRCTADUser_GetData(&unknown_id, &unknown_user, &unknown_nick, &unknown_real, &host, &IP, &sock, &creationTS, &actionTS, &away);

	/* Caso no hay suficiente memoria */
	if(ret == IRCERR_NOENOUGHMEMORY) {
		syslog(LOG_ERR, "Error WHOIS NOENOUGHMEMORY");
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

	/* Parseamos el comando */
	switch(IRCParse_Ping (comando,&prefijo, &server, &server2, &msg)){
		case IRCERR_NOSTRING:
		case IRCERR_ERRONEUSCOMMAND:
			IRCMsg_ErrNoOrigin(&mensajeRespuesta, SERVICIO, unknown_nick, "No origin specified");
			break;
		default:	
			IRCMsg_Pong (&mensajeRespuesta, SERVICIO, SERVICIO, msg, server);
		break;
	}

	enviar(datos->sckfd, mensajeRespuesta);
	if(mensajeRespuesta) free(mensajeRespuesta);
	if(prefijo) free(prefijo);
	if(server) free(server);
	if(server2) free(server2);
	if(msg) free(msg);
	liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);

	return COM_OK;
}


void partirCanal(int sckfd, char * canal, char *nick, char *real, char *msg){

	char *mensaje=NULL;

	switch(IRCTAD_Part (canal, nick)){
		case IRCERR_NOVALIDUSER:
			IRCMsg_ErrNotOnChannel (&mensaje, SERVICIO, nick, real, canal);
			break;
		case IRCERR_NOVALIDCHANNEL:
			IRCMsg_ErrNoSuchChannel (&mensaje, SERVICIO, nick, canal);
			break;
		case IRCERR_UNDELETABLECHANNEL:
			syslog(LOG_INFO, "No se ha podido borrar el canal %s", canal);
		case IRC_OK:
			IRCMsg_Part (&mensaje, SERVICIO, canal, msg);
			break;
	}

	enviar(sckfd, mensaje);
	if(mensaje) free(mensaje);

}

/**
  @brief ejecuta el comando PART
  @param comando: comando a ejecutar
  @param datos: estructura con la informacion del mensaje
  @return COM_OK si todo va bien. Error en otro caso
*/
status part(char *comando, pDatosMensaje datos){
	char *mensajeRespuesta = NULL;
	char *unknown_user = NULL, *unknown_nick = NULL, *unknown_real = NULL;
	char *host = NULL, *IP = NULL, *away = NULL;
	long unknown_id = 0, ret=0;
	long creationTS=0, actionTS=0;
	int sock;
	char *prefijo=NULL, *canal=NULL, *mensaje=NULL, *next=NULL;


	if(!comando || !datos){
		return COM_ERROR;
	}

	sock = datos->sckfd;

	/* Obtenemos identificador del usuario */
	IRCTADUser_GetData(&unknown_id, &unknown_user, &unknown_nick, &unknown_real, &host, &IP, &sock, &creationTS, &actionTS, &away);

	/* Caso no hay suficiente memoria */
	if(ret == IRCERR_NOENOUGHMEMORY) {
		syslog(LOG_ERR, "Error PART NOENOUGHMEMORY");
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

	switch(IRCParse_Part(comando, &prefijo, &canal, &mensaje)){
		case IRCERR_NOSTRING:
		case IRCERR_ERRONEUSCOMMAND:
			IRCMsg_ErrNeedMoreParams(&mensajeRespuesta, SERVICIO ,unknown_nick, comando);
			enviar(datos->sckfd, mensajeRespuesta);
			if(mensajeRespuesta) free(mensajeRespuesta);
			break;

		case IRC_OK:
		default:			
			next = strtok(canal,",");
			while(next != NULL){
				partirCanal(datos->sckfd, next, unknown_nick, unknown_real, mensaje);
				next = strtok(NULL,",");
			}
			break;
	}

	if(prefijo) free(prefijo);
	if(canal) free(canal);
	if(mensaje) free(mensaje);
	liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);
	return COM_OK;
}

/**
  @brief ejecuta el comando TOPIC
  @param comando: comando a ejecutar
  @param datos: estructura con la informacion del mensaje
  @return COM_OK si todo va bien. Error en otro caso
*/
status topic(char *comando, pDatosMensaje datos){
	char *mensajeRespuesta = NULL;
	char *unknown_user = NULL, *unknown_nick = NULL, *unknown_real = NULL;
	char *host = NULL, *IP = NULL, *away = NULL;
	long unknown_id = 0, ret=0;
	long creationTS=0, actionTS=0;
	int sock;
	char *prefijo=NULL, *canal=NULL, *topico=NULL;

	if(!comando || !datos){
		return COM_ERROR;
	}

	sock = datos->sckfd;

	/* Obtenemos identificador del usuario */
	IRCTADUser_GetData(&unknown_id, &unknown_user, &unknown_nick, &unknown_real, &host, &IP, &sock, &creationTS, &actionTS, &away);

	/* Caso no hay suficiente memoria */
	if(ret == IRCERR_NOENOUGHMEMORY) {
		syslog(LOG_ERR, "Error TOPIC NOENOUGHMEMORY");
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

	switch(IRCParse_Topic (comando, &prefijo, &canal, &topico)){
		case IRCERR_NOSTRING:
		case IRCERR_ERRONEUSCOMMAND:
			IRCMsg_ErrNeedMoreParams(&mensajeRespuesta, SERVICIO ,unknown_nick, comando);
			enviar(datos->sckfd, mensajeRespuesta);
			if(mensajeRespuesta) free(mensajeRespuesta);
			if(canal)free(canal);
			if(prefijo)free(prefijo);
			if(topico)free(topico);
			liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);
			return COM_OK;
			
	}

	if(!topico){ /* Caso obtener topic */
		IRCTAD_GetTopic (canal,&topico);
		if(!topico){ /* El canal no tiene topico */
			IRCMsg_RplNoTopic (&mensajeRespuesta, SERVICIO, unknown_nick, canal);
		} else { /* Devolvemos el topico */
			IRCMsg_RplTopic (&mensajeRespuesta, SERVICIO, unknown_nick, canal,topico);
		}
	} else { /* Cambio de topic */
		ret = IRCTAD_SetTopic (canal, unknown_nick, topico);
		if(ret == IRC_OK) /* Cambio realizado */
			IRCMsg_Topic (&mensajeRespuesta, SERVICIO, canal, topico);
		else /* Error al cambiar topico */
			IRCMsg_ErrChanOPrivsNeeded(&mensajeRespuesta, SERVICIO, unknown_nick, canal);
	}

	enviar(datos->sckfd, mensajeRespuesta);
	if(mensajeRespuesta) free(mensajeRespuesta);
	if(canal)free(canal);
	if(prefijo)free(prefijo);
	if(topico)free(topico);
	liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);
	return COM_OK;
}

/**
  @brief ejecuta el comando KICK
  @param comando: comando a ejecutar
  @param datos: estructura con la informacion del mensaje
  @return COM_OK si todo va bien. Error en otro caso
*/
status kick(char *comando, pDatosMensaje datos){
	char *mensajeRespuesta = NULL;
	char *unknown_user = NULL, *unknown_nick = NULL, *unknown_real = NULL;
	char *host = NULL, *IP = NULL, *away = NULL;
	long unknown_id = 0, ret=0;
	long creationTS=0, actionTS=0;
	int sock;
	char *prefijo=NULL, *canal=NULL, *usuario=NULL,*comentario=NULL;
	char *prefijo2=NULL;


	if(!comando || !datos){
		return COM_ERROR;
	}

	sock = datos->sckfd;

	/* Obtenemos identificador del usuario */
	IRCTADUser_GetData(&unknown_id, &unknown_user, &unknown_nick, &unknown_real, &host, &IP, &sock, &creationTS, &actionTS, &away);

	/* Caso no hay suficiente memoria */
	if(ret == IRCERR_NOENOUGHMEMORY) {
		syslog(LOG_ERR, "Error KICK NOENOUGHMEMORY");
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

	switch(IRCParse_Kick (comando, &prefijo, &canal, &usuario, &comentario)){
		case IRCERR_NOSTRING:
		case IRCERR_ERRONEUSCOMMAND:
			IRCMsg_ErrNeedMoreParams(&mensajeRespuesta, SERVICIO ,unknown_nick, comando);
			enviar(datos->sckfd, mensajeRespuesta);
			if(mensajeRespuesta) free(mensajeRespuesta);
			if(canal)free(canal);
			if(prefijo)free(prefijo);
			if(usuario)free(usuario);
			if(comentario) free(comentario);
			liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);
			return COM_OK;
	}

	//Falta comprobar permisos para echar al usuario
	
	IRC_ComplexUser(&prefijo2, unknown_nick, unknown_real, host, SERVICIO);
	ret = IRCTAD_GetUserModeOnChannel(canal, unknown_nick);

	if(ret == IRCERR_NOVALIDCHANNEL) { /* Canal no valido */
			IRCMsg_ErrNoSuchChannel(&mensajeRespuesta, SERVICIO, unknown_nick, canal);
			enviar(datos->sckfd, mensajeRespuesta);

	/* No privilegios suficientes */
	} else if (!(ret&(IRCUMODE_CREATOR|IRCUMODE_OPERATOR|IRCUMODE_LOCALOPERATOR))){
		IRCMsg_ErrChanOPrivsNeeded(&mensajeRespuesta, SERVICIO, unknown_nick, canal);
		enviar(datos->sckfd, mensajeRespuesta);


	} else { /* Caso se puede kickear al usuario */
		switch(IRCTAD_KickUserFromChannel (canal, usuario)){
			case IRCERR_NOVALIDUSER:
				IRCMsg_ErrNotOnChannel (&mensajeRespuesta, SERVICIO, usuario, unknown_real, canal);
				enviar(datos->sckfd, mensajeRespuesta);
				break;
			case IRCERR_NOVALIDCHANNEL:
				IRCMsg_ErrNoSuchChannel (&mensajeRespuesta, SERVICIO, usuario, canal);
				enviar(datos->sckfd, mensajeRespuesta);
				break;
			case IRCERR_UNDELETABLECHANNEL:
				syslog(LOG_INFO, "No se ha podido borrar el canal %s", canal);
				IRCMsg_Kick (&mensajeRespuesta, prefijo2, canal,usuario, comentario);
				enviar(datos->sckfd, mensajeRespuesta);
				enviarMensajePrivado(datos->sckfd, mensajeRespuesta, canal, unknown_nick,0);
				break;

			case IRC_OK:
				IRCMsg_Kick (&mensajeRespuesta, prefijo2, canal,usuario, comentario);
				enviarMensajeACanal(datos->sckfd, mensajeRespuesta, canal, usuario);
				enviarMensajePrivado(datos->sckfd, mensajeRespuesta, usuario, usuario,0);

				break;
		}
	}

	if(mensajeRespuesta) free(mensajeRespuesta);
	if(canal)free(canal);
	if(prefijo)free(prefijo);
	if(prefijo2)free(prefijo2);
	if(usuario)free(usuario);
	if(comentario) free(comentario);
	liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);

	return COM_OK;
}

/**
  @brief ejecuta el comando AWAY
  @param comando: comando a ejecutar
  @param datos: estructura con la informacion del mensaje
  @return COM_OK si todo va bien. Error en otro caso
*/
status away(char *comando, pDatosMensaje datos){
	char *mensajeRespuesta = NULL;
	char *unknown_user = NULL, *unknown_nick = NULL, *unknown_real = NULL;
	char *host = NULL, *IP = NULL, *away = NULL;
	long unknown_id = 0, ret=0;
	long creationTS=0, actionTS=0;
	int sock;
	char *prefijo=NULL, *mensaje=NULL;

	if(!comando || !datos){
		return COM_ERROR;
	}

	sock = datos->sckfd;

	/* Obtenemos identificador del usuario */
	IRCTADUser_GetData(&unknown_id, &unknown_user, &unknown_nick, &unknown_real, &host, &IP, &sock, &creationTS, &actionTS, &away);

	/* Caso no hay suficiente memoria */
	if(ret == IRCERR_NOENOUGHMEMORY) {
		syslog(LOG_ERR, "Error AWAY NOENOUGHMEMORY");
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

	switch(IRCParse_Away(comando,&prefijo, &mensaje)){
		case IRCERR_NOSTRING:
		case IRCERR_ERRONEUSCOMMAND:
			IRCMsg_ErrNeedMoreParams(&mensajeRespuesta, SERVICIO ,unknown_nick, comando);
			enviar(datos->sckfd, mensajeRespuesta);
			if(mensaje) free(mensaje);
			if(prefijo)free(prefijo);
			liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);
			return COM_OK;
	}

	/* Caso Poner away */
	if(mensaje){
		IRCTADUser_SetAway(unknown_id,unknown_user, unknown_nick, unknown_real, mensaje);
		IRCMsg_RplNowAway(&mensajeRespuesta,SERVICIO, unknown_nick);

	} else { /* Caso salir de away */
		IRCTADUser_SetAway(unknown_id,unknown_user, unknown_nick, unknown_real, NULL);
		IRCMsg_RplUnaway(&mensajeRespuesta,SERVICIO, unknown_nick);
	}

	enviar(datos->sckfd, mensajeRespuesta);
	if(mensaje) free(mensaje);
	if(prefijo)free(prefijo);
	liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);

	return COM_OK;
}



/**
  @brief ejecuta el comando MODE
  @param comando: comando a ejecutar
  @param datos: estructura con la informacion del mensaje
  @return COM_OK si todo va bien. Error en otro caso
*/
status mode(char *comando, pDatosMensaje datos){
	char *mensajeRespuesta = NULL;
	char *unknown_user = NULL, *unknown_nick = NULL, *unknown_real = NULL;
	char *host = NULL, *IP = NULL, *away = NULL;
	long unknown_id = 0, ret=0;
	long creationTS=0, actionTS=0;
	int sock;
	char *canal=NULL,*mode=NULL,*user=NULL,*prefijo=NULL;

	if(!comando || !datos){
		return COM_ERROR;
	}

	sock = datos->sckfd;

	/* Obtenemos identificador del usuario */
	IRCTADUser_GetData(&unknown_id, &unknown_user, &unknown_nick, &unknown_real, &host, &IP, &sock, &creationTS, &actionTS, &away);

	/* Caso no hay suficiente memoria */
	if(ret == IRCERR_NOENOUGHMEMORY) {
		syslog(LOG_ERR, "Error MODE NOENOUGHMEMORY");
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

	switch(IRCParse_Mode(comando, &prefijo, &canal, &mode, &user)){
		case IRCERR_NOSTRING:
		case IRCERR_ERRONEUSCOMMAND:
			IRCMsg_ErrNeedMoreParams(&mensajeRespuesta, SERVICIO ,unknown_nick, comando);
			enviar(datos->sckfd, mensajeRespuesta);
			if(mensajeRespuesta) free(mensajeRespuesta);
			if(canal)free(canal);
			if(prefijo)free(prefijo);
			if(user)free(user);
			if(mode) free(mode);
			liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);
			return COM_OK;
	}

	if(canal){
			ret = IRCTAD_GetUserModeOnChannel(canal, unknown_nick);	

			if(ret == IRCERR_NOVALIDCHANNEL) { /* Canal no valido */
				IRCMsg_ErrNoSuchChannel(&mensajeRespuesta, SERVICIO, unknown_nick, canal);
	

			} else if (!(ret&(IRCUMODE_CREATOR|IRCUMODE_OPERATOR|IRCUMODE_LOCALOPERATOR))){
				IRCMsg_ErrChanOPrivsNeeded(&mensajeRespuesta, SERVICIO, unknown_nick, canal);

			} else { /* Caso comandos necesitan permisos */
				if(!strcmp("\\+k", mode)){
					IRCTADChan_SetPassword(canal,user);
				}

				IRCTAD_Mode(canal, unknown_nick, mode);
				IRCMsg_Mode(&mensajeRespuesta, SERVICIO, canal, mode, unknown_nick);
			}
	} else {
		IRCMsg_ErrChanOPrivsNeeded(&mensajeRespuesta, SERVICIO, unknown_nick, "*");
	}

	enviar(datos->sckfd, mensajeRespuesta);
	if(mensajeRespuesta) free(mensajeRespuesta);
	if(canal)free(canal);
	if(prefijo)free(prefijo);
	if(user)free(user);
	if(mode) free(mode);
	liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);

	return COM_OK;
}

/**
  @brief ejecuta el comando QUIT
  @param comando: comando a ejecutar
  @param datos: estructura con la informacion del mensaje
  @return COM_OK si todo va bien. Error en otro caso
*/
status quit(char *comando, pDatosMensaje datos){
	char *mensajeRespuesta = NULL;
	char *unknown_user = NULL, *unknown_nick = NULL, *unknown_real = NULL;
	char *host = NULL, *IP = NULL, *away = NULL;
	long unknown_id = 0;
	long creationTS=0, actionTS=0;
	int sock;

	if(!comando || !datos){
		return COM_ERROR;
	}

	sock = datos->sckfd;
	IRCTADUser_GetData(&unknown_id, &unknown_user, &unknown_nick, &unknown_real, &host, &IP, &sock, &creationTS, &actionTS, &away);
	IRCMsg_Notice(&mensajeRespuesta, SERVICIO, unknown_nick, "Ta lue");
	enviar(datos->sckfd, mensajeRespuesta);
	if(mensajeRespuesta) free(mensajeRespuesta);

	/* Se cierra la conexion */
	IRCTADUser_Delete(unknown_id, unknown_user, unknown_nick, unknown_real);
	liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);

	return COM_QUIT;
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
	long creationTS=0, actionTS=0;
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

