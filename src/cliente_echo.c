/**
 * @file cliente_echo.c
 * @brief main sencillo que prueba el cliente echo con SSL
 * @author Pablo Marcos  <pablo.marcos@estudiante.uam.es>
 * @author Dionisio Perez  <dionisio.perez@estudiante.uam.es>
*/

/**
 * @defgroup Servidor-Cliente-echo
 *
 * <hr>
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#include "../includes/ssl.h"

#define MAX_BUFFER_ECHO 2048
#define PUERTO_ECHO 6668
#define SERVER_ECHO "localhost"
#define EXIT_WORD "exit"
#define EXIT_WORD_LEN 4
#define ERROR -1;
#define STOP 0
#define FLAGS 0
#define OK 0

#define PATH_CLIENT_CERT "certs/cliente.pem"
#define PATH_CLIENT_PKEY "certs/clientkeypri.pem"

/**
 * @addtogroup Servidor-Cliente-echo
 * Comprende los mains para probar tanto el cliente como el servidor echo
 *
 * <hr>
 */

/**
 * @ingroup Servidor-Cliente-echo
 *
 * @brief Funcion para crear un socket TCP para el cliente echo
 *
 * @synopsis
 * @code
 * 	int crearSocketTCP_cliente(int *sckfd, char *server, unsigned short port)
 * @endcode
 *
 * @param[in] sckfd socket en el que conectar
 * @param[in] server servidor del que obtener direccion IP
 * @param[in] port puerto al que conectar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
int crearSocketTCP_cliente(int *sckfd, char *server, unsigned short port){

    struct sockaddr_in addr;
    struct hostent* host;

    /* Creamos socket TCP */
    if ((*sckfd=socket(AF_INET, SOCK_STREAM, 0)) == -1 ) {  
        return -1;
    }
	/* Obtenemos IP del servidor */
    if((host = gethostbyname(server)) == NULL) {
        return -1;
    }

    addr.sin_family = AF_INET;         
    addr.sin_port = htons(port); 
    memcpy(&addr.sin_addr.s_addr, (char *)host->h_addr , host->h_length); 
    memset(&addr.sin_zero,0,8); 
	/* Nos conectamos al servidor */
    if(connect(*sckfd, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
        return -1;
    }
	return 0;
}

int main(void){


	int desc, run = 1;
	char buffer[MAX_BUFFER_ECHO] = {0};
	ssize_t tam;

	/* Creamos socket cliente*/
	if(crearSocketTCP_cliente(&desc, SERVER_ECHO, PUERTO_ECHO) < 0){
		return ERROR;
	}

	/* Inicializamos libreria ssl */
	inicializar_nivel_SSL();
 
	if(fijar_contexto_SSL(PATH_CLIENT_CERT, PATH_CLIENT_PKEY) < 0){
		perror("Error al fijar contexto");
		close(desc);
		liberar_nivel_SSL();
		return ERROR;
	}

	/* Conectamos canal seguro */
	if(conectar_canal_seguro_SSL(desc) < 0){
		perror("Error conectar canal seguro");
		close(desc);
		liberar_nivel_SSL();
		return ERROR;
	}


	while(run){
		/* Recibimos de forma segura */
		tam = scanf("%2048s", buffer);
		/* Caso de error */
		if(tam <= 0){
			cerrar_canal_SSL(desc);
			close(desc);
			liberar_nivel_SSL();
			return ERROR;
		/* Case de salida */
		} else if(!strncmp(EXIT_WORD, buffer,EXIT_WORD_LEN)) {
			run = STOP;

			enviar_datos_SSL(desc, buffer, strnlen(buffer, MAX_BUFFER_ECHO));
		/* Enviamos de forma segura lo mismo recibido */
		} else {
			enviar_datos_SSL(desc, buffer, strnlen(buffer, MAX_BUFFER_ECHO));
			recibir_datos_SSL(desc, buffer, MAX_BUFFER_ECHO);
			printf("%s\n", buffer);
		}
	}

	cerrar_canal_SSL(desc);
	close(desc);
	liberar_nivel_SSL();

	return OK;
}
