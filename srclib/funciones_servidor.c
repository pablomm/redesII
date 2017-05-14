/**
 * @file funciones_servidor.c
 * @brief funciones referidas a los comandos
 * @author Pablo Marcos  <pablo.marcos@estudiante.uam.es>
 * @author Dionisio Perez  <dionisio.perez@estudiante.uam.es>
*/

/** 
 * @defgroup ComandosResto ComandosResto
 *
 */

#include "../includes/funciones_servidor.h"
#include "../includes/conexion_temp.h"
#include "../includes/ssl.h"

#include <redes2/irc.h>

/**
 * @addtogroup ComandosResto
 * Comprende los comandos restantes pedidos en la asignatura
 *
 * <hr>
 */

/**
 * @ingroup ComandosResto
 *
 * @brief Libera estructuras antes de cerrar servidor
 *
 * @synopsis
 * @code
 * 	void liberarEstructuras(void)
 * @endcode
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
void liberarEstructuras(void){

	liberaTodosTempUser();
	pthread_mutex_destroy(&mutexDescr);
	pthread_mutex_destroy(&mutexTempUser);

}

/**
 * @ingroup ComandosResto
 *
 * @brief Parsea el mensaje recibido
 *
 * @synopsis
 * @code
 * 	void manejaMensaje(void* pdesc)
 * @endcode
 *
 * @param[in] pdesc estructura con la informacion del mensaje
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
void *manejaMensaje(void* pdesc){

	pDatosMensaje datos;
	char *next;
	char *comando;

	pthread_detach(pthread_self());

	datos = (pDatosMensaje) pdesc;

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

	pthread_exit(NULL);
}

/**
 * @ingroup ComandosResto
 *
 * @brief Selecciona comando del array a ejecutar
 *
 * @synopsis
 * @code
 * 	status procesaComando(char *comando, pDatosMensaje datos)
 * @endcode
 *
 * @param[in] comando comando a ejecutar
 * @param[in] datos estructura con la informacion del mensaje
 *
 * @return la funcion que ejecuta el comando
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
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

void enviarMensajePrivado(int sckfd, char *mensaje, char *nick, char * nickorigin, int sndaway);

/**
 * @ingroup ComandosResto
 *
 * @brief Crea una nueva conexion
 *
 * @synopsis
 * @code
 * 	status nuevaConexion(int desc, struct sockaddr_in * address)
 * @endcode
 *
 * @param[in] desc socket del usuario
 * @param[in] address estructura que almacena informacion de la direccion de internet
 *
 * @return el usuario temporal
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
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
 * @ingroup ComandosResto
 *
 * @brief Cierra una conexion
 *
 * @synopsis
 * @code
 * 	status cerrarConexion(int socket)
 * @endcode
 *
 * @param[in] socket socket del usuario
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
status cerrarConexion(int socket){
	char *unknown_user = NULL, *unknown_nick = NULL, *unknown_real = NULL;
	char *host = NULL, *IP = NULL, *away = NULL;
	long unknown_id = 0;
	long creationTS=0, actionTS=0;
	int sock =socket;

	#ifdef PRUEBAS_IRC
	char **lista=NULL, *mensajequit=NULL, *prefijo=NULL;
	long num;
	int i;
	#endif

	IRCTADUser_GetData(&unknown_id, &unknown_user, &unknown_nick, &unknown_real, &host, &IP, &sock, &creationTS, &actionTS, &away);

	/*
		Documentado en la memoria
		Problema en algunos ordenadores, creemos que por saturacion
		kernel: [ 2901.143828] ieee80211 phy0: rt2x00queue_flush_queue: Warning - Queue 2 failed to flush
		Para compilar añadir a flags de gcc -D PRUEBAS_IRC
	*/
	#ifdef PRUEBAS_IRC
	if(unknown_user) { /* Version por usuarios */
		IRC_ComplexUser(&prefijo, unknown_nick, unknown_real, host, SERVICIO);
		IRCMsg_Quit(&mensajequit, prefijo,"Abandono el servidor");

		IRCTADUser_GetUserList (&lista, &num);
	
		for(i=0; i<num; i++){

			if(strcmp(unknown_user,lista[i])){
				/* printf("%s -- (%s)\n",lista[i], unknown_user); */
				enviarMensajePrivado(socket, mensajequit, lista[i], NULL, 0);
			}
		}

		free(prefijo);
		free(mensajequit);
		IRCTADUser_FreeList(lista,num);
	}

	/* // Version canales compartidos, problema que si comparte varios canales llega varias veces
	if(unknown_id) {
		IRC_ComplexUser(&prefijo, unknown_nick, unknown_real, host, SERVICIO);
		IRCMsg_Quit(&mensajequit, prefijo,"Abandono el servidor");

		IRCTAD_ListChannelsOfUserArray (unknown_user, unknown_nick, &lista, &num);
	
		for(i=0; i<num; i++){
			printf("Canal: %s\n", lista[i]);
			printf("Mensaje %s\n", mensajequit);

			enviarMensajeACanal(socket, mensajequit, lista[i], NULL);
		}

		free(prefijo);
		free(mensajequit);
		IRCTADUser_FreeList(lista,num);

	} */
	#endif

	/* Eliminamos de la base de datos */
	if(unknown_nick)
		IRCTAD_Quit(unknown_nick);

	liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);

	deleteTempUser(socket);
	
	if(ssl_active){
		cerrar_canal_SSL(socket);

	}

	close(socket);
	deleteFd(socket);

	return COM_OK;
}

/**
 * @ingroup ComandosResto
 *
 * @brief Libera informacion del mensaje
 *
 * @synopsis
 * @code
 * 	void liberaDatosMensaje(pDatosMensaje datos)
 * @endcode
 *
 * @param[in] datos estructura con la informacion del mensaje
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
void liberaDatosMensaje(pDatosMensaje datos){
	
	free(datos->msg);
	datos->msg = NULL;
	free(datos);
}

/**
 * @ingroup ComandosResto
 *
 * @brief Inicializa el array de comandos
 *
 * @synopsis
 * @code
 * 	void liberaDatosMensaje(pDatosMensaje datos)
 * @endcode
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
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
	comandos[MOTD] = motd;
	comandos[PONG] = pong;
	comandos[WHO] = who;

	return COM_OK;   
}

/**
 * @ingroup ComandosResto
 *
 * @brief Inicializa rutina ping-pong
 *
 * @synopsis
 * @code
 * 	status rutinaPingPong(void)
 * @endcodeje
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
status rutinaPingPong(void){

	long nelements = 0;
	long *ids = NULL;
	char **users = NULL;
	char **nicks = NULL;
	char **realnames = NULL;
	char **passwords = NULL;
	char **hosts = NULL;
	char **IPs = NULL;
	int *sockets = NULL;
	long *modes = NULL;
	long *creationTSs = NULL;
	long *actionTSs = NULL;
	long i=0;


	
	/* printf("LLAMADA A RUTINA PING PONG\n"); */
	IRCTADUser_GetAllLists (&nelements,&ids,&users,&nicks,&realnames,&passwords,&hosts,&IPs,&sockets,&modes,&creationTSs,&actionTSs);

	for(i=0; i< nelements; i++){

		/* printf("socket %d -> %ld\n", sockets[i], actionTSs[i]); */

	}

	IRCTADUser_FreeAllLists (nelements,ids,users,nicks,realnames,passwords,hosts,IPs,sockets,modes,creationTSs,actionTSs);

	return COM_OK;
}

/**
 * @ingroup ComandosResto
 *
 * @brief Libera informacion del usuario
 *
 * @synopsis
 * @code
 * 	void liberaDatosMensaje(pDatosMensaje datos)
 * @endcode
 *
 * @param[in] user campo user de la estructura
 * @param[in] nick campo nick de la estructura
 * @param[in] real campo realname de la estructura
 * @param[in] host campo host de la estructura
 * @param[in] IP campo dir_IP de la estructura
 * @param[in] away campo away de la estructura
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
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
 * @ingroup ComandosResto
 *
 * @brief Funcion auxiliar para el comando WHOIS
 *
 * @synopsis
 * @code
 * 	void whoischannels(int sckfd, char *nick1, char *nick2, char* user, char *target)
 * @endcode
 *
 * @param[in] sckfd socket desde el que enviar
 * @param[in] nick1 nick necesario numero 1
 * @param[in] nick2 nick necesario numero 2
 * @param[in] user usuario del canal
 * @param[in] target objetivo
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
void whoischannels(int sckfd, char *nick1, char *nick2, char* user, char *target){
	char *mensajeRespuesta=NULL;
	char *lista = NULL;
	long num=0;

	(void) target;

	IRCTAD_ListChannelsOfUser (user, nick2, &lista, &num);
	IRCMsg_RplWhoIsChannels (&mensajeRespuesta, SERVICIO, nick1, nick2, lista);
	enviar(sckfd, mensajeRespuesta);

	if(mensajeRespuesta) free(mensajeRespuesta);
	if(lista) free(lista);
}

/**
 * @ingroup ComandosResto
 *
 * @brief Funcion auxiliar para el comando WHOIS
 *
 * @synopsis
 * @code
 * 	void sendAway(int sckfd, char *away, char *nick, char *nickorig)
 * @endcode
 *
 * @param[in] sckfd socket desde el que enviar
 * @param[in] away mensaje away
 * @param[in] nick nick del usuario
 * @param[in] nickorig nick original
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
void sendAway(int sckfd, char *away, char *nick, char *nickorig){
	char *msgAway;

	if(away!=NULL){
		IRCMsg_RplAway(&msgAway, SERVICIO, nick, nickorig, away);
		enviar(sckfd, msgAway);
		if(msgAway) free(msgAway);
	}
}

/**
 * @ingroup ComandosResto
 *
 * @brief Ejecuta el comando WHOIS
 *
 * @synopsis
 * @code
 * 	status whois(char *comando, pDatosMensaje datos)
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
status whois(char *comando, pDatosMensaje datos){
	char *mensajeRespuesta = NULL;
	char *unknown_user = NULL, *unknown_nick = NULL, *unknown_real = NULL;
	char *host = NULL, *IP = NULL, *away = NULL;
	long unknown_id = 0;
	unsigned long ret = 0;
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

	/* Actualizamos tiempo action ts */
	IRCTADUser_SetActionTS(unknown_id, unknown_user, unknown_nick, unknown_real);

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

/**
 * @ingroup ComandosResto
 *
 * @brief Funcion auxiliar para el comando NAMES
 *
 * @synopsis
 * @code
 * 	void listarNamesCanal(char *canal, int sckfd, char *prefijo, char *nicku)
 * @endcode
 *
 * @param[in] canal canal del que obtener users
 * @param[in] sckfd socket desde el que enviar
 * @param[in] prefijo prefijo a utilizar
 * @param[in] nicku nick del user
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
void listarNamesCanal(char *canal, int sckfd, char *prefijo, char *nicku) {
	char *mensajeRespuesta = NULL;
	char *lista = NULL;
	long num=0;

	if(IRCTAD_ListNicksOnChannel(canal, &lista, &num) == IRC_OK){
		IRCMsg_RplNamReply (&mensajeRespuesta, prefijo, nicku, "=", canal, lista);
		enviar(sckfd, mensajeRespuesta);
	}
	if(mensajeRespuesta) free(mensajeRespuesta);
	if(lista) free(lista);

}

/**
 * @ingroup ComandosResto
 *
 * @brief Ejecuta el comando NAMES
 *
 * @synopsis
 * @code
 * 	status names(char *comando, pDatosMensaje datos)
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
status names(char *comando, pDatosMensaje datos){
	char *mensajeRespuesta = NULL;
	char *unknown_user = NULL, *unknown_nick = NULL, *unknown_real = NULL;
	char *host = NULL, *IP = NULL, *away = NULL;
	long unknown_id = 0;
	unsigned long ret = 0;
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

	/* Actualizamos tiempo action ts */
	IRCTADUser_SetActionTS(unknown_id, unknown_user, unknown_nick, unknown_real);

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

/**
 * @ingroup ComandosResto
 *
 * @brief Funcion auxilar del comando PRIVMSG
 *
 * @synopsis
 * @code
 * 	void enviarMensajePrivado(int sckfd, char *mensaje, char *nick, char * nickorigin, int sndaway)
 * @endcode
 *
 * @param[in] sckfd socket desde el que enviar
 * @param[in] mensaje mensaje a enviar
 * @param[in] nick nick del usuario
 * @param[in] nickorig nick original
 * @param[in] sndaway send away del usuario
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
void enviarMensajePrivado(int sckfd, char *mensaje, char *nick, char * nickorigin, int sndaway){
	char *mensajeError = NULL;
	char *unknown_user = NULL, *unknown_real = NULL;
	char *host = NULL, *IP = NULL, *away = NULL;
	long unknown_id = 0;
	long creationTS=0, actionTS=0;
	int sock = 0;

	/* Obtenemos datos usuario target con el nick */
	IRCTADUser_GetData(&unknown_id, &unknown_user, &nick, &unknown_real, &host, &IP, &sock, &creationTS, &actionTS, &away);
	if(unknown_id == 0 && nickorigin){
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

/**
 * @ingroup ComandosResto
 *
 * @brief Funcion auxilar del comando PRIVMSG
 *
 * @synopsis
 * @code
 * 	void enviarMensajeACanal(int sckfd, char *mensaje, char *canal, char * nickorigin)
 * @endcode
 *
 * @param[in] sckfd socket desde el que enviar
 * @param[in] mensaje mensaje a enviar
 * @param[in] canal canal al que enviar el mensaje
 * @param[in] nickorig nick original
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
void enviarMensajeACanal(int sckfd, char *mensaje, char *canal, char * nickorigin){
	char *mensajeError = NULL;
	char **lista = NULL;
	long num = 0;
	long i;

	/* Obtenemos datos usuario target con el nick */
	if(IRCTAD_ListNicksOnChannelArray(canal, &lista, &num) != IRC_OK){

		IRCMsg_ErrNoSuchNick (&mensajeError, SERVICIO, nickorigin, canal);
		enviar(sckfd, mensajeError);
		if(mensajeError) free(mensajeError);
		return;	
	}

	/* Enviamos mensajes a usuarios del canal */	
	for(i=0; i<num; i++) {
		/* No volvemos a mandarselo al que lo ha enviado */
		if(nickorigin){
			if(!strcmp(nickorigin,lista[i])){
				continue;
			}
		}


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
 * @ingroup ComandosResto
 *
 * @brief Ejecuta el comando PRIVMSG
 *
 * @synopsis
 * @code
 * 	status privmsg(char *comando, pDatosMensaje datos)
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
status privmsg(char *comando, pDatosMensaje datos){
	char *mensajeRespuesta = NULL;
	char *unknown_user = NULL, *unknown_nick = NULL, *unknown_real = NULL;
	char *host = NULL, *IP = NULL, *away = NULL;
	long unknown_id = 0;
	unsigned long ret = 0;
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

	/* Actualizamos tiempo action ts */
	IRCTADUser_SetActionTS(unknown_id, unknown_user, unknown_nick, unknown_real);

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
 * @ingroup ComandosResto
 *
 * @brief Ejecuta el comando PING
 *
 * @synopsis
 * @code
 * 	status ping(char *comando, pDatosMensaje datos)
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
status ping(char *comando, pDatosMensaje datos){
	char *mensajeRespuesta = NULL;
	char *unknown_user = NULL, *unknown_nick = NULL, *unknown_real = NULL;
	char *host = NULL, *IP = NULL, *away = NULL;
	long unknown_id = 0;
	unsigned long ret = 0;
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

	/* Actualizamos tiempo action ts */
	IRCTADUser_SetActionTS(unknown_id, unknown_user, unknown_nick, unknown_real);

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

/**
 * @ingroup ComandosResto
 *
 * @brief Ejecuta el comando PONG
 *
 * @synopsis
 * @code
 * 	status pong(char *comando, pDatosMensaje datos)
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
status pong(char *comando, pDatosMensaje datos){
	char *unknown_user = NULL, *unknown_nick = NULL, *unknown_real = NULL;
	char *host = NULL, *IP = NULL, *away = NULL;
	long unknown_id = 0;
	unsigned long ret = 0;
	long creationTS=0, actionTS=0;
	int sock;

	if(!comando || !datos){
		return COM_ERROR;
	}

	sock = datos->sckfd;

	/* Obtenemos identificador del usuario */
	IRCTADUser_GetData(&unknown_id, &unknown_user, &unknown_nick, &unknown_real, &host, &IP, &sock, &creationTS, &actionTS, &away);

	/* Caso no hay suficiente memoria */
	if(ret == IRCERR_NOENOUGHMEMORY) {
		syslog(LOG_ERR, "Error PONG NOENOUGHMEMORY");
		liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);
		return COM_ERROR;
	}

	/* ERRNOTREGISTERD */
	if(unknown_id == 0){
		liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);
		return COM_OK;
	}

	/* Actualizamos tiempo action ts */
	IRCTADUser_SetActionTS(unknown_id, unknown_user, unknown_nick, unknown_real);

	liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);

	return COM_OK;
}

/**
 * @ingroup ComandosResto
 *
 * @brief Funcion auxilar del comando PART
 *
 * @synopsis
 * @code
 * 	void partirCanal(int sckfd, char * canal, char *nick, char *real, char *msg)
 * @endcode
 *
 * @param[in] sckfd socket desde el que enviar
 * @param[in] canal canal a abandonar
 * @param[in] nick nick del usuario
 * @param[in] real realname del usuario
 * @param[in] msg mensaje a enviar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
void partirCanal(int sckfd, char * canal, char *nick, char *real, char *msg){

	char *mensaje=NULL, *prefijo2=NULL;

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
			IRC_ComplexUser(&prefijo2,nick, real, "*", SERVICIO);
			IRCMsg_Part (&mensaje, prefijo2, canal, msg);
			free(prefijo2);
			break;
	}

	enviar(sckfd, mensaje);
	enviarMensajeACanal(sckfd,mensaje,canal, NULL);
	if(mensaje) free(mensaje);

}

/**
 * @ingroup ComandosResto
 *
 * @brief Ejecuta el comando PART
 *
 * @synopsis
 * @code
 * 	status part(char *comando, pDatosMensaje datos)
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
status part(char *comando, pDatosMensaje datos){
	char *mensajeRespuesta = NULL;
	char *unknown_user = NULL, *unknown_nick = NULL, *unknown_real = NULL;
	char *host = NULL, *IP = NULL, *away = NULL;
	long unknown_id = 0;
	unsigned long ret = 0;
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

	/* Actualizamos tiempo action ts */
	IRCTADUser_SetActionTS(unknown_id, unknown_user, unknown_nick, unknown_real);

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
 * @ingroup ComandosResto
 *
 * @brief Ejecuta el comando TOPIC
 *
 * @synopsis
 * @code
 * 	status topic(char *comando, pDatosMensaje datos)
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
status topic(char *comando, pDatosMensaje datos){
	char *mensajeRespuesta = NULL;
	char *unknown_user = NULL, *unknown_nick = NULL, *unknown_real = NULL;
	char *host = NULL, *IP = NULL, *away = NULL;
	long unknown_id = 0;
	unsigned long ret = 0;
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

	/* Actualizamos tiempo action ts */
	IRCTADUser_SetActionTS(unknown_id, unknown_user, unknown_nick, unknown_real);

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
 * @ingroup ComandosResto
 *
 * @brief Ejecuta el comando KICK
 *
 * @synopsis
 * @code
 * 	status kick(char *comando, pDatosMensaje datos)
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
status kick(char *comando, pDatosMensaje datos){
	char *mensajeRespuesta = NULL;
	char *unknown_user = NULL, *unknown_nick = NULL, *unknown_real = NULL;
	char *host = NULL, *IP = NULL, *away = NULL;
	long unknown_id = 0;
	unsigned long ret = 0;
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

	/* Actualizamos tiempo action ts */
	IRCTADUser_SetActionTS(unknown_id, unknown_user, unknown_nick, unknown_real);

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
 * @ingroup ComandosResto
 *
 * @brief Ejecuta el comando AWAY
 *
 * @synopsis
 * @code
 * 	status away(char *comando, pDatosMensaje datos)
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
status away(char *comando, pDatosMensaje datos){
	char *mensajeRespuesta = NULL;
	char *unknown_user = NULL, *unknown_nick = NULL, *unknown_real = NULL;
	char *host = NULL, *IP = NULL, *away = NULL;
	long unknown_id = 0;
	unsigned long ret = 0;
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

	/* Actualizamos tiempo action ts */
	IRCTADUser_SetActionTS(unknown_id, unknown_user, unknown_nick, unknown_real);

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
 * @ingroup ComandosResto
 *
 * @brief Ejecuta el comando MODE
 *
 * @synopsis
 * @code
 * 	status mode(char *comando, pDatosMensaje datos)
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
status mode(char *comando, pDatosMensaje datos){
	char *mensajeRespuesta = NULL;
	char *unknown_user = NULL, *unknown_nick = NULL, *unknown_real = NULL;
	char *host = NULL, *IP = NULL, *away = NULL;
	long unknown_id = 0;
	unsigned long ret = 0;
	long creationTS=0, actionTS=0;
	int sock;
	char *canal=NULL,*mode=NULL,*user=NULL,*prefijo=NULL, *mcanal=NULL;

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

	/* Actualizamos tiempo action ts */
	IRCTADUser_SetActionTS(unknown_id, unknown_user, unknown_nick, unknown_real);

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

			} else if(mode) { /* Caso comandos necesitan permisos */
				if(!strcmp("\\+k", mode)){
					IRCTADChan_SetPassword(canal,user);
				}

				IRCTAD_Mode(canal, unknown_nick, mode);
				IRCMsg_Mode(&mensajeRespuesta, SERVICIO, canal, mode, unknown_nick);
			} else {
				mcanal = IRCTADChan_GetModeChar(canal);
				IRCMsg_RplChannelModeIs(&mensajeRespuesta, SERVICIO, unknown_nick, canal, mcanal?mcanal:"+");
				free(mcanal);
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
 * @ingroup ComandosResto
 *
 * @brief Ejecuta el comando QUIT
 *
 * @synopsis
 * @code
 * 	status quit(char *comando, pDatosMensaje datos)
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
	IRCMsg_Notice(&mensajeRespuesta, SERVICIO, unknown_nick, "Me piro vampiro");
	enviar(datos->sckfd, mensajeRespuesta);
	if(mensajeRespuesta) free(mensajeRespuesta);

	/* Se cierra la conexion */
	/*IRCTAD_Quit(unknown_nick);*/
	liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);

	return COM_QUIT;
}

/**
 * @ingroup ComandosResto
 *
 * @brief Obtiene ruta del motd a mostrar 
 *
 * @synopsis
 * @code
 * 	char * getMOTD(void)
 * @endcode
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
char * getMOTD(void){
	char comando[1040];
	/* Para que funcione daemonizado */
	snprintf(comando,1040,".%s/%s",abspath,MOTD_SCRIPT);

	if(offensive) {
		system(comando);
		return FMOTDOFFENSIVE;
	}
	
	return FMOTD;
}

/**
 * @ingroup ComandosResto
 *
 * @brief Ejecuta el comando MOTD
 *
 * @synopsis
 * @code
 * 	status motd(char *comando, pDatosMensaje datos)
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
status motd(char *comando, pDatosMensaje datos){
	char *mensajeRespuesta = NULL;
	char *unknown_user = NULL, *unknown_nick = NULL, *unknown_real = NULL;
	char *host = NULL, *IP = NULL, *away = NULL;
	long unknown_id = 0;
	unsigned long ret = 0;
	long creationTS=0, actionTS=0;
	int sock;
	char *prefijo=NULL, *target=NULL;
	char *mtd=NULL;
	FILE *fp=NULL;
	char line[256];
	char motd_file[1040];

	if(!comando || !datos){
		return COM_ERROR;
	}

	sock = datos->sckfd;

	/* Obtenemos identificador del usuario */
	IRCTADUser_GetData(&unknown_id, &unknown_user, &unknown_nick, &unknown_real, &host, &IP, &sock, &creationTS, &actionTS, &away);

	/* Caso no hay suficiente memoria */
	if(ret == IRCERR_NOENOUGHMEMORY) {
		syslog(LOG_ERR, "Error MOTD NOENOUGHMEMORY");
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

	IRCParse_Motd(comando, &prefijo, &target);

	IRCMsg_RplMotdStart(&mensajeRespuesta, SERVICIO,  unknown_nick, SERVICIO);
	enviar(datos->sckfd, mensajeRespuesta);
	if(mensajeRespuesta) { free(mensajeRespuesta); mensajeRespuesta=NULL; }

	/* Para que funcione aunque sea daemon */
	snprintf(motd_file,1040,"%s/%s",abspath, getMOTD());
	fp = fopen(motd_file,"r");
	if(!fp) {
		IRCMsg_RplMotd(&mensajeRespuesta, SERVICIO,  unknown_nick,  "Oops algo ha ocurrido");
		enviar(datos->sckfd, mensajeRespuesta);
		if(mensajeRespuesta) { free(mensajeRespuesta); mensajeRespuesta=NULL; }

	} else {
		while(fgets(line,256,fp)) {
			line[strlen(line)-1] = '\0';
			IRCMsg_RplMotd(&mensajeRespuesta, SERVICIO,  unknown_nick,  line);
			enviar(datos->sckfd, mensajeRespuesta);
			if(mensajeRespuesta) { free(mensajeRespuesta); mensajeRespuesta=NULL; }
		}
		fclose(fp);
	}
	IRCMsg_RplEndOfMotd(&mensajeRespuesta, SERVICIO,  unknown_nick);
	enviar(datos->sckfd, mensajeRespuesta);
	if(mensajeRespuesta) { free(mensajeRespuesta); mensajeRespuesta=NULL; }

	if(mtd) free(mtd);
	if(prefijo) free(prefijo);
	if(target) free(target);
	liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);

	return COM_OK;
}

/**
 * @ingroup ComandosResto
 *
 * @brief Ejecuta el comando NO_COMMAND
 *
 * @synopsis
 * @code
 * 	status comandoVacio(char *comando, pDatosMensaje datos)
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
	
	if(unknown_id != 0) {
		/* Actualizamos tiempo action ts */
		IRCTADUser_SetActionTS(unknown_id, unknown_user, unknown_nick, unknown_real);
	}

	/* Variante si el usuario esta registrado */
   	IRCMsg_ErrUnKnownCommand (&mensajeRespuesta, SERVICIO, unknown_nick ? unknown_nick : "*", comando);
    enviar(datos->sckfd, mensajeRespuesta);

	/* Liberamos estructuras */
    if(mensajeRespuesta) free(mensajeRespuesta);
	liberarUserData(unknown_user, unknown_nick, unknown_real, host, IP, away);

	return COM_OK;
}

