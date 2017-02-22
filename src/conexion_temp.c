#include "../includes/conexion_temp.h"


/*
typedef struct _TempUser {

	int socket;
	char * nick;
	char *host;
	char IP[INET6_ADDRSTRLEN];
	pTempUser previous;
	pTempUser next;

} TempUser, *pTempUser;
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

	usuario->nick = NULL;
	usuario-> socket = socket;

	strcpy(usuario->IP, ip);
	usuario->previous = NULL;
	usuario->next = NULL;

	if( usuarioPrimero == NULL){
		usuarioPrimero = usuario;
		usuarioUltimo = usuario;
	}

	usuarioUltimo->next = usuario;
	usuarioUltimo = usuario;

	return CON_OK;
}

pTempUser pullTempUser(int socket){

	pTempUser useri;

	useri = usuarioPrimero;
	while (useri != NULL){
		if(useri->socket == socket){
			return useri; 
		}

		useri = useri->next;

	}

	return NULL;
}

status deleteTempUser(int socket){

	pTempUser tuser;
	tuser = pullTempUser (socket);
	
	if(tuser == NULL) return CON_ERROR;
	if(tuser->previous == NULL) { /* Caso primero*/
		(tuser->next)->previous = NULL;
		liberaTempUser(tuser);
	}	

	if(tuser->next == NULL) { /* Caso último */
		(tuser->previous)->next = NULL;
		liberaTempUser(tuser);
	}

	/*Caso medio*/


}

status liberaTempUser (pTempUser usuario) {

	if(usuario == NULL) return CON_ERROR;
	if(usuario->host != NULL) free(usuario->host);
	if(usuario->nick != NULL) free(usuario->nick);
	free(usuario);
}
