/**
 * @file funciones_registro.c
 * @brief funciones referidas a los comandos
 * @author Pablo Marcos  <pablo.marcos@estudiante.uam.es>
 * @author Dionisio Perez  <dionisio.perez@estudiante.uam.es>
*/

/** 
 * @defgroup ComandosRegistro ComandosRegistro
 *
 */

#include "../includes/funciones_servidor.h"
#include "../includes/conexion_temp.h"

#include <redes2/irc.h>


/**
 * @addtogroup ComandosRegistro
 * Comprende los comandos para iniciarse en un servidor IRC
 *
 * <hr>
 */

/**
 * @ingroup ComandosRegistro
 *
 * @brief Ejecuta el comando NICK
 *
 * @synopsis
 * @code
 * 	status nick(char* comando, pDatosMensaje datos)
 * @endcode
 *
 * @param[in] comando comando a ejecutar
 * @param[in] datos estructura con la informacion del mensaje
 *
 * @return COM_OK si todo va bien. Error en otro caso
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
status nick(char* comando, pDatosMensaje datos){
	
	char *prefijo = NULL, *nickk = NULL, *msg = NULL;
	char *unknown_user = NULL, *unknown_nick = NULL, *unknown_real = NULL;
	char *host = NULL, *IP = NULL, *away = NULL;
	char * mensajeRespuesta = NULL;
	long unknown_id = 0;
	unsigned long ret = 0;
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

		/* Actualizamos tiempo action ts */
		IRCTADUser_SetActionTS(unknown_id, unknown_user, unknown_nick, unknown_real);

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
 * @ingroup ComandosRegistro
 *
 * @brief Ejecuta el comando USER
 *
 * @synopsis
 * @code
 * 	status user(char* comando, pDatosMensaje datos)
 * @endcode
 *
 * @param[in] comando comando a ejecutar
 * @param[in] datos estructura con la informacion del mensaje
 *
 * @return COM_OK si todo va bien. Error en otro caso
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
status user(char* comando, pDatosMensaje datos){
	char *prefijo = NULL, *user = NULL;
	char *modehost = NULL, *server = NULL, *realname = NULL;
	char *unknown_user = NULL, *unknown_nick = NULL, *unknown_real = NULL;
	char *host = NULL, *IP = NULL, *away = NULL;
	char * mensajeRespuesta = NULL;
	long unknown_id = 0;
	unsigned long ret = 0;
	long creationTS, actionTS;
	int sock;	
	pTempUser usuarioTemporal = NULL;

	if(!datos || !comando){
		return COM_ERROR;
	}
	sock = datos->sckfd;
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

		/* Actualizamos tiempo action ts */
		IRCTADUser_SetActionTS(unknown_id, unknown_user, unknown_nick, unknown_real);

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
			/* Mandamos motd inicio de conexion */
			/*motd("motd", datos);*/

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
 * @ingroup ComandosRegistro
 *
 * @brief Ejecuta el comando JOIN
 *
 * @synopsis
 * @code
 * 	status join(char *comando, pDatosMensaje datos)
 * @endcode
 *
 * @param[in] comando comando a ejecutar
 * @param[in] datos estructura con la informacion del mensaje
 *
 * @return COM_OK si todo va bien. Error en otro caso
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
status join(char *comando, pDatosMensaje datos){
	char *prefijo = NULL, *prefijo2=NULL, *comandonames=NULL;
	char *unknown_user = NULL, *unknown_nick = NULL, *unknown_real = NULL;
	char *host = NULL, *IP = NULL, *away = NULL;
	char * mensajeRespuesta = NULL;
	unsigned long ret = 0;
	char *canal, *clave, *msg;
	long unknown_id = 0;
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

	/* Actualizamos tiempo action ts */
	IRCTADUser_SetActionTS(unknown_id, unknown_user, unknown_nick, unknown_real);

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

			IRC_ComplexUser(&prefijo2, unknown_nick, unknown_real, host, SERVICIO);

			/* Primero respondemos JOIN */
			IRCMsg_Join(&mensajeRespuesta,prefijo2, NULL, NULL, canal);
			enviarMensajeACanal(datos->sckfd, mensajeRespuesta, canal, unknown_nick);
			enviar(datos->sckfd, mensajeRespuesta);
			if(mensajeRespuesta){ free(mensajeRespuesta); mensajeRespuesta=NULL; }
			/* Mandamos names al usuario tras unirse */
			IRCMsg_Names(&comandonames,prefijo2,canal,"*");
			names(comandonames, datos);
			free(comandonames); comandonames=NULL;
			free(prefijo2); prefijo2=NULL;
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

/**
 * @ingroup ComandosRegistro
 *
 * @brief Funcion auxiliar relacionada con el comando LIST
 *
 * @synopsis
 * @code
 * 	void listarCanalesUsuario(char *canal, int sckfd, char *prefijo, char *nicku) 
 * @endcode
 *
 * @param[in] canal canal sobre el que trabajar
 * @param[in] sckfd socket desde el que enviar respuesta
 * @param[in] prefijo prefijo del usuario
 * @param[in] nicku nick del usuario
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
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
	IRCMsg_RplList(&mensajeRespuesta, prefijo, nicku, canal, mode, topico?topico:"");
	enviar(sckfd, mensajeRespuesta);
	
	if(mode) free(mode);
	if(mensajeRespuesta) free(mensajeRespuesta);
	if(topico) free(topico);

}

/**
 * @ingroup ComandosRegistro
 *
 * @brief Ejecuta el comando LIST
 *
 * @synopsis
 * @code
 * 	status list(char *comando, pDatosMensaje datos)
 * @endcode
 *
 * @param[in] comando comando a ejecutar
 * @param[in] datos estructura con la informacion del mensaje
 *
 * @return COM_OK si todo va bien. Error en otro caso
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
status list(char *comando, pDatosMensaje datos){
	char *prefijo = NULL;
	char *unknown_user = NULL, *unknown_nick = NULL, *unknown_real = NULL;
	char *host = NULL, *IP = NULL, *away = NULL;
	char * mensajeRespuesta = NULL;
	long unknown_id = 0;
	long creationTS=0, actionTS=0;
	int sock=0;	
	char *canal=NULL, *objetivo=NULL;
	char **lista = NULL;
	long num=0;
    int i = 0;
	unsigned long ret = 0;
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

	/* Actualizamos tiempo action ts */
	IRCTADUser_SetActionTS(unknown_id, unknown_user, unknown_nick, unknown_real);

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

/**
 * @ingroup ComandosRegistro
 *
 * @brief Ejecuta el comando WHO
 *
 * @synopsis
 * @code
 * 	status who(char *comando, pDatosMensaje datos)
 * @endcode
 *
 * @param[in] comando comando a ejecutar
 * @param[in] datos estructura con la informacion del mensaje
 *
 * @return COM_OK si todo va bien. Error en otro caso
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
status who(char *comando, pDatosMensaje datos){
	char *prefijo = NULL;
	char *unknown_user = NULL, *unknown_nick = NULL, *unknown_real = NULL;
	char *host = NULL, *IP = NULL, *away = NULL;
	char * mensajeRespuesta = NULL;
	long unknown_id = 0;
	long creationTS=0, actionTS=0;
	int sock=0;	
	char *canal=NULL, *objetivo=NULL;
	char **lista = NULL;
	long num=0;
    int i = 0;
	unsigned long ret = 0;
	int sock1=0;
	long id1=0;
	char *user1=NULL,*real1=NULL,*host1=NULL,*ip1=NULL,*away1=NULL;

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

	/* Actualizamos tiempo action ts */
	IRCTADUser_SetActionTS(unknown_id, unknown_user, unknown_nick, unknown_real);

	switch(IRCParse_Who(comando, &prefijo, &canal, &objetivo)){
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

	/* No hacemos nada */
	if(canal == NULL){

    	/* ************************************
		   Animo por haber llegado hasta aqui
		          creemos en ti!!
		   ************************************ */
	/* Caso una lista de canales */
    } else {
    
			IRCTAD_ListNicksOnChannelArray(canal, &lista, &num);
	
			for(i=0; i< num; i++){
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
				IRCTADUser_GetData(&id1, &user1, &(lista[i]), &real1, &host1, &ip1, &sock1, &creationTS, &actionTS, &away1);
				IRCMsg_RplWhoReply(&mensajeRespuesta, SERVICIO, lista[i], canal, user1, host1,"*", "*", "*", 0, real1);
				enviar(datos->sckfd, mensajeRespuesta);
				if(mensajeRespuesta) {free(mensajeRespuesta); mensajeRespuesta=NULL; }
				liberarUserData(user1, NULL, real1, host1, ip1, away1);
			}
	}
		IRCTADUser_FreeList(lista,num);
		IRCMsg_RplEndOfWho(&mensajeRespuesta, SERVICIO,unknown_nick, canal);
		enviar(datos->sckfd, mensajeRespuesta);
		free(canal);
		if(mensajeRespuesta) free(mensajeRespuesta);
		if(prefijo) free(prefijo);
		if(objetivo) free(objetivo);
		liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);
	return COM_OK;
}

