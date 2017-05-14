/**
 * @file servidor_echo.c
 * @brief main sencillo que prueba el servidor echo con SSL
 * @author Pablo Marcos  <pablo.marcos@estudiante.uam.es>
 * @author Dionisio Perez  <dionisio.perez@estudiante.uam.es>
*/

#include <stdio.h>

#include "../includes/red_servidor.h"
#include "../includes/ssl.h"

#define MAX_BUFFER_ECHO 2048
#define PUERTO_ECHO 6668
#define EXIT_WORD "exit"
#define EXIT_WORD_LEN 4
#define ERROR -1;
#define STOP 0
#define FLAGS 0
#define OK 0

#define PATH_SERV_CERT "certs/servidor.pem"
#define PATH_SERV_PKEY "certs/serverkeypri.pem"

int main(void){

	int sckt, desc, run = 1;
	char buffer[MAX_BUFFER_ECHO] = {0};
	struct sockaddr_in address;
	ssize_t tam;
	
	/* Creamos socket TCP/SSL*/
	if(crearSocketTCP(&sckt, PUERTO_ECHO) < 0){
		return ERROR;
	}

	/* Aceptamos conexion */
	if(aceptarConexion(sckt, &desc, &address) < 0){
		perror("Error aceptar conexion");
		close(sckt);
		return ERROR;
	}

	inicializar_nivel_SSL();
 
	if(fijar_contexto_SSL(PATH_SERV_CERT, PATH_SERV_PKEY) < 0){
		perror("Error al fijar contexto");
		cerrar_canal_SSL(desc);
		close(desc);
		close(sckt);
		liberar_nivel_SSL();
		return ERROR;
	}

	/* Aceptamos conexion de forma segura */
	if(aceptar_canal_seguro_SSL(desc) < 0){
		perror("Error aceptar canal seguro");
		cerrar_canal_SSL(desc);
		close(desc);
		close(sckt);
		liberar_nivel_SSL();
		return ERROR;
	}

	while(run){
			/* Recibimos de forma segura */
			tam = recibir_datos_SSL(desc, buffer, MAX_BUFFER_ECHO);
			/* Caso de error */
			if(tam <= 0){
				cerrar_canal_SSL(desc);
				close(desc);
				close(sckt);
				liberar_nivel_SSL();
				return ERROR;
			/* Case de salida */
			} else if(!strncmp(EXIT_WORD, buffer,EXIT_WORD_LEN)) {
				run = STOP;
			/* Enviamos de forma segura lo mismo recibido */
			} else {
				enviar_datos_SSL(desc, buffer, tam);
				memset(buffer,0, tam);
			}
		}

	cerrar_canal_SSL(desc);
	close(desc);
	close(sckt);
	liberar_nivel_SSL();

	return OK;
}

