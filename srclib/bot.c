/**
 * @file bot.c
 * @brief Lanza el bot
 * @author Pablo Marcos  <pablo.marcos@estudiante.uam.es>
 * @author Dionisio Perez  <dionisio.perez@estudiante.uam.es>
*/

/**
 * @defgroup BotIRC Bot
 *
 * <hr>
 */

/** 
 * @addtogroup BotIRC
 * Funciones que permiten el lanzamiento del bot
 *
 * <hr>
 */

#include <unistd.h>
#include <redes2/irc.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../includes/ssl.h"
#include "../includes/bot.h"

#define TAM_BUFFER_COMMAND 120
#define DEFAULT_SERVER "localhost"
#define DEFAULT_BOT_PORT 6667
#define DEFAULT_SSL_BOT_PORT 6669

#define PATH_CLIENT_CERT "certs/cliente.pem"
#define PATH_CLIENT_PKEY "certs/clientkeypri.pem"

/**
 * @ingroup BotIRC
 *
 * @brief Imprime por stderr la ayuda de usuario
 *
 * @synopsis
 * @code
 * 	void usage_bot(void)
 * @endcode
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
void usage_bot(void){
	fprintf(stderr, "usage: [-hS -s <server> -p <port> -c <channel>]\n");
	/* fprintf(stderr, " -S --ssl Realiza conexion segura con el servidor\n"); */
	fprintf(stderr, " -p --port Especifica un puerto para conectarse al servidor. Por defecto 6667 o 6669 con SSL\n");
	fprintf(stderr, " -s --server Especifica un servidor al que conectarse. Por defecto localhost\n");
	fprintf(stderr, " -c --channel Especifica un canal IRC al que unirse al conectarse\n");
	fprintf(stderr, " -h --help Muestra la ayuda y finaliza la ejecucion del programa\n");
}

/**
 * @ingroup BotIRC
 *
 * @brief Funcion para crear un socket TCP para el bot
 *
 * @synopsis
 * @code
 * 	int crearSocketTCP_cliente_bot(int *sckfd, char *server, unsigned short port)
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
int crearSocketTCP_cliente_bot(int *sckfd, char *server, unsigned short port){

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

/**
 * @ingroup BotIRC
 *
 * @brief Funcion para enviar mensajes desde el bot
 *
 * @synopsis
 * @code
 * 	int enviar_bot(char *mensaje, int socket)
 * @endcode
 *
 * @param[in] mensaje mensaje a enviar
 * @param[in] socket socket al que enviar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
int enviar_bot(char *mensaje, int socket){
	if(socket < 0 || mensaje==NULL){
		return -1;
	}

	if(ssl_active) {
		return enviar_datos_SSL(socket, mensaje, strlen(mensaje));
	} else {
		return send(socket, mensaje, strlen(mensaje), 0);
	}

	return -1;
}

/**
 * @ingroup BotIRC
 *
 * @brief Funcion para recibir mensajes desde el bot
 *
 * @synopsis
 * @code
 * 	ssize_t recibir_bot(int sckfd, char *buffer, int max)
 * @endcode
 *
 * @param[in] sckfd socket en el que recibir
 * @param[in] buffer mensaje a recibir
 * @param[in] max tamanno maximo a leer
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
ssize_t recibir_bot(int sckfd, char *buffer, int max){

	if (ssl_active)
		return recibir_datos_SSL(sckfd, buffer, max);
	
	return recv(sckfd, buffer, max, 0);
}

/**
 * @ingroup BotIRC
 *
 * @brief Funcion para tratar los mensajes privados desde el bot
 *
 * @synopsis
 * @code
 * 	void procesar_privmsg_bot(int sckfd, int (*procesar)(char*,char*))
 * @endcode
 *
 * @param[in] sckfd socket en el que recibir
 * @param[in] procesar funcion para procesar mensajes recibidos
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
void procesar_privmsg_bot(int sckfd, int (*procesar)(char*,char*)){
	ssize_t tam = 0;
	char buffer[1024] = {0};
	char *next = NULL, *comando = NULL;
    char *tar=NULL, *msg=NULL, *prf=NULL;

	while((tam = recibir_bot(sckfd,buffer, 1024))){
		if(tam == -1)
			break;

		/* Procesamos paquete recibido solo si es un privmsg */
		
		next = buffer;
		while(next != NULL) {
        	next = IRC_UnPipelineCommands(next, &comando);

			switch(IRC_CommandQuery (comando)) {
				case PRIVMSG:
					/* Procesamos los privmsg */
					IRCParse_Privmsg(comando, &prf, &tar, &msg);
					procesar(msg,tar);
					IRC_MFree(3, &prf, &tar, &msg);
					break;

                case IRCERR_NOCOMMAND:
                case IRCERR_UNKNOWNCOMMAND:
                    continue;
            }

			if(comando) { free(comando); comando=NULL; }

		}	
		memset(buffer, 0, 1024);
	}
}

/**
 * @ingroup BotIRC
 *
 * @brief Funcion para conectar el botIRC
 *
 * @synopsis
 * @code
 * 	int conectar_bot(char *server, int port, char *channel, char *nick, char *user, char *realname, int ssl)
 * @endcode
 *
 * @param[in] server servidor al que conectar
 * @param[in] port puerto al que conectar
 * @param[in] channel canal al que unirse
 * @param[in] nick nick a utilizar
 * @param[in] user user a utiliazar
 * @param[in] realname realname a utilizar
 * @param[in] ssl para activar comunicacion segura SSL
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
int conectar_bot(char *server, int port, char *channel, char *nick, char *user, char *realname, int ssl){

	int socket;
	char *mensaje = NULL, *mensaje2 = NULL, *mensaje3 = NULL;
	char mensajejoin[120] = {0};

	/* Control de errores */
	if(!nick || !user || !realname || !server || port<0 )
		return -1;


	/* Nos conectamos al servidor */
	if(crearSocketTCP_cliente_bot(&socket, server, port) == -1){
		return -1;
	}

	/* Inicializamos conexion SSL */
	if(ssl){
		inicializar_nivel_SSL();

		if(fijar_contexto_SSL(PATH_CLIENT_CERT, PATH_CLIENT_PKEY) < 0){
				close(socket);
				liberar_nivel_SSL();
				return -1;
		}
		/* Conectamos canal seguro */
		if(conectar_canal_seguro_SSL(socket) < 0){
				close(socket);
				liberar_nivel_SSL();
				return -1;
		}
	}

	/* Mensaje de NICK */
	IRCMsg_Nick(&mensaje, NULL, nick, NULL);
	IRCMsg_User(&mensaje2, NULL, user, "*", realname);

	if(channel){

		snprintf(mensajejoin,120,"JOIN #%s\r\n",channel);
		IRC_PipelineCommands (&mensaje3, mensaje, mensaje2, mensajejoin, NULL);
	} else {
		IRC_PipelineCommands (&mensaje3, mensaje, mensaje2,"\r\n ", NULL);
	}
	enviar_bot(mensaje3,socket);

	IRC_MFree(3,&mensaje3, &mensaje, &mensaje2);

	return socket;
}

/**
 * @ingroup BotIRC
 *
 * @brief Funcion para enviar un mensaje privado desde el bot
 *
 * @synopsis
 * @code
 * 	int privmsg_bot(int argc, char **argv,int (*procesar)(char*,char*), char *name)
 * @endcode
 *
 * @param[in] argc argument count
 * @param[in] argv argument vector
 * @param[in] procesar funcion para procesar mensajes recibidos
 * @param[in] name nombre del bot
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
int privmsg_bot(int argc, char **argv,int (*procesar)(char*,char*), char *name){

	int long_index;
	char c;
	int sckfd = -1, puerto = 0;
	char canal[TAM_BUFFER_COMMAND]={0}, server[TAM_BUFFER_COMMAND]={0};

	/* Estructura con opciones */
	static struct option options[] = {
		{"help", no_argument,0,'h'},
		{"ssl", no_argument,0,'S'},
		{"server", required_argument,0,'s'},
		{"port", required_argument,0,'p'},
		{"channel", required_argument,0,'c'}
	};

	/* Variable global libreria de SSL */
	ssl_active = 0;

	/* Valores por defecto */
	strcpy(server, DEFAULT_SERVER);

	/* Parseamos las opciones */
	while ((c = getopt_long_only (argc, argv, "i:p:s:h", options, &long_index)) != -1){
		switch (c) {
			case 'c': /* Especifica un canal al que autounirse */
				strncpy(canal,optarg,120);
				break;
			case 'p': /* Especifica un puerto */
				puerto = atoi(optarg);
				break;
			case 's': /* Especifica un servidor */
				strncpy(server,optarg,120);
				break;
			case 'S': /* Especifica conexion segura */
				ssl_active = 1;
				break;
			case '?':
			default:
				usage_bot();
				return 0;
		}
	}

	/* Ponemos el puerto por defecto si no se ha elegido */
	if(!puerto)
		puerto = (ssl_active) ? DEFAULT_SSL_BOT_PORT : DEFAULT_BOT_PORT;

	/* printf("Canal: %s Server: %s Puerto: %d SSL: %s\n", canal, server, puerto, ssl_active?"Activo":"Inactivo"); */

	/* Nos conectamos al servidor */
	sckfd = conectar_bot(server, puerto, strlen(canal)?canal:NULL, name, name,name, ssl_active);
	
	if(sckfd < 0){
		fprintf(stderr, "No se ha podido establecer conexion con el servidor\n");
		return -1;
	}

	socket_bot = sckfd;

	/* Llamamos a procesar privmsg con la funcion de nuestro bot */
	procesar_privmsg_bot(sckfd, procesar);

	/* Cerramos socket  y ssl*/
	close(sckfd);
	if(ssl_active)
		liberar_nivel_SSL();

	return 0;
}




