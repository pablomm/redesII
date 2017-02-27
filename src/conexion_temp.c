#include "../includes/conexion_temp.h"


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
	
	if(tuser == NULL) 
		return CON_ERROR;

	/* Caso unico */
	if(tuser->previous == NULL && tuser->next == NULL) {

		usuarioPrimero = NULL;
		usuarioUltimo = NULL;

		return liberaTempUser(tuser);

	}


	/* Caso primero o ultimo (no unico)*/
	if(tuser->previous == NULL || tuser->next == NULL) {

		if(tuser->previous == NULL) { /* Caso primero*/
			(tuser->next)->previous = NULL;
			usuarioPrimero = tuser->next;
		
		} else { /* Caso último */
			(tuser->previous)->next = NULL;
			usuarioUltimo = tuser->next;
		}
		
	} else { /*Caso medio*/

		(tuser->previous)->next = tuser->next;
		(tuser->next)->previous = tuser->previous;

	}

	return liberaTempUser(tuser);

}

status liberaTempUser(pTempUser usuario) {

	if(usuario == NULL) return CON_ERROR;
	if(usuario->host != NULL) free(usuario->host);
	if(usuario->nick != NULL) free(usuario->nick);
	free(usuario);

	return CON_OK;
}



status liberaTodosTempUser(void){

	pTempUser aux = NULL;
	pTempUser t = usuarioPrimero;

	while(t != NULL){
		aux = t->next;
		liberaTempUser(t);
		t = aux;
	}

	usuarioPrimero = NULL;
	usuarioUltimo = NULL;

	return CON_OK;

}





