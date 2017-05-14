/**
 * @file conexion_temp.c
 * @brief estructura y funciones del usuario temporal
 * @author Pablo Marcos  <pablo.marcos@estudiante.uam.es>
 * @author Dionisio Perez  <dionisio.perez@estudiante.uam.es>
*/

/**
 * @defgroup TempUser TempUser
 *
 * <hr>
 */

#include "../includes/conexion_temp.h"

/**
 * @addtogroup TempUser
 * Comprende funciones para el tratamiento de usuarios que no han sido añadidos a la base de datos de metis
 *
 * <hr>
 */

/**
 * @ingroup TempUser
 *
 * @brief Crea un nuevo usuario temporal
 *
 * @synopsis
 * @code
 * 	status newTempUser(int socket,  char *ip, char *host)
 * @endcode
 *
 * @param[in] socket el socket
 * @param[in] ip la direccion ip
 * @param[in] host el host
 *
 * @return CON_OK si todo va bien. CON_ERROR en caso contrario
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
status newTempUser(int socket,  char *ip, char *host){
	pTempUser usuario;

	if(!ip || !host){
		return CON_ERROR;
	}


	usuario = (pTempUser) calloc(1, sizeof(TempUser));

	if(usuario == NULL){
		return CON_ERROR;
	}

	usuario->host = (char*) calloc(strlen(host) + 1, sizeof(char));
	if(usuario->host == NULL){
		free(usuario);
		return CON_ERROR;
	}
	strcpy(usuario->host, host);

	usuario->nick = NULL;
	usuario-> socket = socket;

	strcpy(usuario->IP, ip);
	usuario->previous = NULL;
	usuario->next = NULL;

	pthread_mutex_lock(&mutexTempUser);

	if( usuarioPrimero == NULL){
		usuarioPrimero = usuario;
		usuarioUltimo = usuario;
	} else {
		usuarioUltimo->next = usuario;
		usuario->previous = usuarioUltimo;
		usuarioUltimo = usuario;
	}

	pthread_mutex_unlock(&mutexTempUser);

	return CON_OK;
}

/**
 * @ingroup TempUser
 *
 * @brief Modifica el nick del usuario temporal
 *
 * @synopsis
 * @code
 * 	status setNickTemporal(pTempUser usuario, char* nick)
 * @endcode
 *
 * @param[in] usuario el usuario
 * @param[in] nick el nuevo nick
 *
 * @return CON_OK si todo va bien. CON_ERROR en caso contrario
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
status setNickTemporal(pTempUser usuario, char* nick){

	if(usuario == NULL || nick == NULL){
		return CON_ERROR;
	}

	if(usuario->nick){
		free(usuario->nick);
	}

	usuario->nick = (char*) calloc(strlen(nick) + 1,  sizeof(char));
	if(usuario->nick == NULL){
		return CON_ERROR;
	}

	strcpy(usuario->nick, nick);

	return CON_OK;
}

/**
 * @ingroup TempUser
 *
 * @brief Busca por socket un usuario temporal en la lista
 *
 * @synopsis
 * @code
 * 	pTempUser pullTempUser(int socket)
 * @endcode
 *
 * @param[in] socket el socket
 *
 * @return el usuario econtrado, o NULL
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
pTempUser pullTempUser(int socket){

	pTempUser useri;

	pthread_mutex_lock(&mutexTempUser);

	useri = usuarioPrimero;
	while (useri != NULL){
		if(useri->socket == socket){
			pthread_mutex_unlock(&mutexTempUser);
			return useri; 
		}

		useri = useri->next;

	}
	pthread_mutex_unlock(&mutexTempUser);
	return NULL;
}

/**
 * @ingroup TempUser
 *
 * @brief Elimina un usario temporal
 *
 * @synopsis
 * @code
 * 	status deleteTempUser(int socket)
 * @endcode
 *
 * @param[in] socket el socket
 *
 * @return CON_OK si todo va bien. CON_ERROR en caso contrario
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
status deleteTempUser(int socket){

	pTempUser tuser;
	tuser = pullTempUser (socket);
	
	if(tuser == NULL) 
		return CON_ERROR;

	pthread_mutex_lock(&mutexTempUser);

	/* Caso unico */
	if(tuser->previous == NULL && tuser->next == NULL) {

		usuarioPrimero = NULL;
		usuarioUltimo = NULL;

		pthread_mutex_unlock(&mutexTempUser);
		return liberaTempUser(tuser);

	}


	/* Caso primero o ultimo (no unico)*/
	if(tuser->previous == NULL || tuser->next == NULL) {

		if(tuser->previous == NULL) { /* Caso primero*/
			(tuser->next)->previous = NULL;
			usuarioPrimero = tuser->next;
		
		} else { /* Caso último */
			(tuser->previous)->next = NULL;
			usuarioUltimo = tuser->previous;
		}
		
	} else { /*Caso medio*/
		(tuser->previous)->next = tuser->next;
		(tuser->next)->previous = tuser->previous;

	}

	pthread_mutex_unlock(&mutexTempUser);
	return liberaTempUser(tuser);

}

/**
 * @ingroup TempUser
 *
 * @brief Elimina de memoria un usuario temporal
 *
 * @synopsis
 * @code
 * 	status liberaTempUser(pTempUser usuario)
 * @endcode
 *
 * @param[in] usuario el usuario
 *
 * @return CON_OK 
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
status liberaTempUser(pTempUser usuario) {

	if(usuario == NULL) return CON_ERROR;
	if(usuario->host != NULL) free(usuario->host);
	if(usuario->nick != NULL) free(usuario->nick);
	free(usuario);

	return CON_OK;
}


/**
 * @ingroup TempUser
 *
 * @brief Elimina de memoria a todos los usuarios temporales
 *
 * @synopsis
 * @code
 * 	status liberaTodosTempUser(void)
 * @endcode
 *
 * @return CON_OK 
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
status liberaTodosTempUser(void){

	pTempUser aux = NULL;
	pTempUser t = NULL;

	pthread_mutex_lock(&mutexTempUser);

	t = usuarioPrimero;

	while(t != NULL){
		aux = t->next;
		liberaTempUser(t);
		t = aux;
	}

	usuarioPrimero = NULL;
	usuarioUltimo = NULL;

	pthread_mutex_unlock(&mutexTempUser);
	return CON_OK;

}


/**
 * @ingroup TempUser
 *
 * @brief Funcion para debbugear usuarios temporales
 *
 * @synopsis
 * @code
 * 	status printDebugUsers(void)
 * @endcode
 *
 * @return CON_OK 
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
status printDebugUsers(void){
	pTempUser aux = NULL;
	pTempUser t = usuarioPrimero;

	printf("Temp users:\n");
	while(t != NULL){
		aux = t->next;
		printf("Socket: %d ", t->socket);
		if(t->nick)
			printf("Nick: %s ", t->nick);	
		if(t->host)
			printf("Host: %s", t->host);

		printf("\n");

		t = aux;
	}

	return CON_OK;

}
