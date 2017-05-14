/**
 * @file comandos_cliente.c
 * @brief Funciones para el tratamiento de los comandos de cliente
 * @author Pablo Marcos  <pablo.marcos@estudiante.uam.es>
 * @author Dionisio Perez  <dionisio.perez@estudiante.uam.es>
*/

/**
 * @defgroup ComandosCliente ComandosCliente
 *
 * <hr>
 */
#include <redes2/ircxchat.h>
#include <redes2/irc.h>
#include <stdio.h>
#include <string.h>


#include "../includes/cliente.h"
#include "../includes/red_cliente.h"
#include "../includes/comandos_cliente.h"

/**
 * @addtogroup ComandosCliente
 * Comprende funciones para el tratamiento de los comandos de cliente
 *
 * <hr>
 */

/* No esta incluido IRCMsg_GeneralCommand en los .h de redes2/xchat
   linka bien pero da warning al compilar */
extern long IRCMsg_GeneralCommand ( char **command, char *prefix, char *type, char *msg, ...);

/**
 * @ingroup ComandosCliente
 *
 * @brief Funcion de procesamiento por defecto del comando del cliente
 *
 * @synopsis
 * @code
 * 	void comando_general(char *comando)
 * @endcode
 *
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
void comando_general(char *comando){

	char *mensaje = NULL;
	IRCMsg_GeneralCommand (&mensaje, cliente.prefijo, comando, NULL, NULL);
	enviar_cliente(mensaje);
	free(mensaje);
}

/**
 * @ingroup ComandosCliente
 *
 * @brief Funcion de procesamiento del comando del cliente Part
 *
 * @synopsis
 * @code
 * 	void clientePart(char *comando)
 * @endcode
 *
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
void clientePart(char* comando) {
    char *msg = NULL, *mensaje = NULL, *active;

	active = IRCInterface_ActiveChannelName();
	if(!strcmp(active, "System")) return;

    IRCUserParse_Part(comando, &msg);
    IRCMsg_Part(&mensaje, NULL, active, msg ? msg: "Ta lue"); 
    enviar_cliente(mensaje);
    IRC_MFree(2,&mensaje, &msg);
}

/**
 * @ingroup ComandosCliente
 *
 * @brief Funcion de procesamiento del comando del cliente PartAll
 *
 * @synopsis
 * @code
 * 	void clientePartAll(char *comando)
 * @endcode
 *
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
void clientePartAll(char* comando) {
    char *mensaje = NULL;
	(void) comando;
	
	IRCMsg_Join (&mensaje, cliente.prefijo, "0", NULL, NULL);
    enviar_cliente(mensaje);
    free(mensaje);

}

/**
 * @ingroup ComandosCliente
 *
 * @brief Funcion de procesamiento del comando del cliente Cycle
 *
 * @synopsis
 * @code
 * 	void clienteCycle(char *comando)
 * @endcode
 *
 * @param[in] comando comando a procesar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
void clienteCycle(char* comando) {
    char **canales=NULL, *active;
    int n,i;
    char mensaje[256]={0};

    IRCUserParse_Cycle(comando, &canales, &n);

    for(i=0;i<n;i++) {
        snprintf(mensaje, 255, "PART %s\r\nJOIN %s\r\n", canales[i], canales[i]);
        enviar_cliente(mensaje);
    }


    if(!n) {
		active = IRCInterface_ActiveChannelName();
		if(strcmp(active,"System")){
		    snprintf(mensaje, 255, "PART %s\r\nJOIN %s\r\n", active, active);
		    enviar_cliente(mensaje);
		}
    }

	IRCTADUser_FreeList (canales, n);
}

/**
 * @ingroup ComandosCliente
 *
 * @brief Funcion de inicializacion del array de comandos de cliente
 *
 * @synopsis
 * @code
 * 	void inicializar_comandos_cliente(void)
 * @endcode
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
void inicializar_comandos_cliente(void){
	int i;

	for(i=0; i< IRC_MAX_USER_COMMANDS ; i++){
		comandos_cliente[i] = comando_general;
	}

	/* Comandos necesitan procesado especial */
	comandos_cliente[UPART] = clientePart;
	comandos_cliente[UPARTALL] = clientePartAll;
	comandos_cliente[UCYCLE] = clienteCycle;
	
}
