/**
 * @file ssl.c
 * @brief Soporte SSL
 * @author Pablo Marcos  <pablo.marcos@estudiante.uam.es>
 * @author Dionisio Perez  <dionisio.perez@estudiante.uam.es>
 */

/**
 * @defgroup SoporteSSL SoporteSSL
 *
 * <hr>
 */

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "../includes/ssl.h"

#define CA_FILE_PATH "./certs/ca.pem"

/**
 * @addtogroup SoporteSSL
 * Comprende todas las funciones para dar soporte SSL
 *
 * <hr>
 */
 
/* Contexto SSL: var global */
SSL_CTX *context;
 
/* Array para almacenar sockets SSL */
typedef SSL* pSSL;
pSSL SSL_sockets[MAX_SSL] = {0};

/**
 * @ingroup SoporteSSL
 *
 * @brief Imprime mensaje de error si una funcion SSL falla
 *
 * @synopsis
 * @code
 * 	status ssl_error(char *msg)
 * @endcode
 *
 * @param[in] msg mensaje de error a imprimir
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
 */
void ssl_error(char *msg){
  fprintf (stderr, "ERROR: %s (errno %d, %s)\n", msg, errno, strerror(errno));
  /* Print SSL errors */
  ERR_print_errors_fp(stderr);
}

/**
 * @ingroup SoporteSSL
 *
 * @brief Inicializa nivel SSL
 *
 * @synopsis
 * @code
 * 	int inicializar_nivel_SSL()
 * @endcode
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
 */
int inicializar_nivel_SSL() {
	context = NULL;
    SSL_load_error_strings();
    SSL_library_init(); 
	return SSL_OK;
}

/**
 * @ingroup SoporteSSL
 *
 * @brief Crea contexto SSL y comprueba certificados
 *
 * @synopsis
 * @code
 * 	int fijar_contexto_SSL(const char *pcert, const char *pkey)
 * @endcode
 *
 * @param[in] pcert ruta del certificado del cliente/servidor a comprobar
 * @param[in] pkey ruta de la clave privada del cliente/servidor a comprobar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
 */
int fijar_contexto_SSL(const char *pcert, const char *pkey) {
 
	/* Creamos contexto SSL */
    context = SSL_CTX_new(SSLv23_method());
    if(!context) {
		ssl_error("No SSL context"); 
		return SSL_ERR;
	}
	/* Cargamos localizacion CA certificado */
    if(SSL_CTX_load_verify_locations(context, CA_FILE_PATH, NULL) != 1){
		ssl_error("verify locations"); 
		return SSL_ERR;
	}

    if(SSL_CTX_set_default_verify_paths(context) != 1){
		ssl_error("Verify paths"); 
		return SSL_ERR;
	}
	/*
    if (SSL_CTX_use_certificate_chain_file(context, pcert) != 1){
		ssl_error("Chain file"); 
		return SSL_ERR;
	}*/

    if (SSL_CTX_use_certificate_file(context, pcert, SSL_FILETYPE_PEM) != 1){
		ssl_error("Chain file"); 
		return SSL_ERR;
	}

    if (SSL_CTX_use_PrivateKey_file(context, pkey, SSL_FILETYPE_PEM) != 1){ 
		ssl_error("PrivateKey"); 
		return SSL_ERR;
	}

    SSL_CTX_set_verify(context,SSL_VERIFY_PEER , NULL);


    return SSL_OK;
}
 
/**
 * @ingroup SoporteSSL
 *
 * @brief Se conecta a un socket con seguridad SSL
 *
 * @synopsis
 * @code
 * 	int conectar_canal_seguro_SSL(int sckfd)
 * @endcode
 *
 * @param[in] sckfd socket desde el que conectarse
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
 */ 
int conectar_canal_seguro_SSL(int sckfd) {

	/* Guardamos nuevo contexto en array de sockets ssl */
 	SSL_sockets[sckfd] = SSL_new(context);

	/* Asociamso el socket a la estructura en la posicion */
    if(SSL_set_fd(SSL_sockets[sckfd], sckfd)!=1){
		ssl_error("SSL_set_fd"); 
		return SSL_ERR;
	}
	/* Inicializamos handshake con servidor */
    if(SSL_connect(SSL_sockets[sckfd]) != 1){
		ssl_error("SSL_connect"); 
		return SSL_ERR;
	}    
	return SSL_OK;
}
 
/**
 * @ingroup SoporteSSL
 *
 * @brief Permite conexion SSL segura
 *
 * @synopsis
 * @code
 * 	int aceptar_canal_seguro_SSL(int sckfd)
 * @endcode
 *
 * @param[in] sckfd socket que aceptar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
 */ 
int aceptar_canal_seguro_SSL(int sckfd) {

	/* Guardamos nuevo contexto en array de sockets ssl */
 	SSL_sockets[sckfd] = SSL_new(context);

	/* Asociamso el socket a la estructura en la posicion */
    if(SSL_set_fd(SSL_sockets[sckfd], sckfd)!=1){
		ssl_error("SSL_set_fd"); 
		return SSL_ERR;
	}
	/* Esperando handshake cliente */
    if(SSL_accept(SSL_sockets[sckfd]) != 1){
		ssl_error("SSL_connect"); 
		return SSL_ERR;
	}    
	return SSL_OK;
}
 
/**
 * @ingroup SoporteSSL
 *
 * @brief evalua seguridad de la conexion
 *
 * @synopsis
 * @code
 * 	int evaluar_post_conectar_SSL(int sckfd)
 * @endcode
 *
 * @param[in] sckfd socket a evaluar
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
 */ 
int evaluar_post_conectar_SSL(int sckfd) {
    return SSL_get_peer_certificate(SSL_sockets[sckfd]) && SSL_get_verify_result(SSL_sockets[sckfd]);
}
 
/**
 * @ingroup SoporteSSL
 *
 * @brief envia mensaje de forma segura por medio de SSL
 *
 * @synopsis
 * @code
 * 	int enviar_datos_SSL(int sckfd, void* mensaje, int tam)
 * @endcode
 *
 * @param[in] sckfd socket desde el que enviar
 * @param[in] mensaje mensaje a enviar
 * @param[in] tam tamaño del mensaje
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
 */ 
int enviar_datos_SSL(int sckfd, void* mensaje, int tam) {
	if(!mensaje || tam < 0 || sckfd < 0){
		return SSL_ERR;
	}
  	return SSL_write(SSL_sockets[sckfd], mensaje, tam);
}

/**
 * @ingroup SoporteSSL
 *
 * @brief Recibe mensaje de forma segura por medio de SSL
 *
 * @synopsis
 * @code
 * 	int recibir_datos_SSL(int sckfd, void* mensaje, int max)
 * @endcode
 *
 * @param[in] sckfd socket del que recibir
 * @param[in] mensaje mensaje a enviar
 * @param[in] max tamaño maximo a leer
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
 */ 
int recibir_datos_SSL(int sckfd, void* mensaje, int max) {
    if(!SSL_sockets[sckfd]) 
		return SSL_ERR;
    memset(mensaje,0, max);
    return SSL_read(SSL_sockets[sckfd], mensaje, max);
}
 
/**
 * @ingroup SoporteSSL
 *
 * @brief Cierra conexion segura SSL
 *
 * @synopsis
 * @code
 * 	void cerrar_canal_SSL(int sckfd)
 * @endcode
 *
 * @param[in] sckfd socket del que terminar conexion
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
 */ 
void cerrar_canal_SSL(int sckfd) {
   if(SSL_sockets[sckfd]){
		SSL_shutdown(SSL_sockets[sckfd]); 
		SSL_free(SSL_sockets[sckfd]);
		SSL_sockets[sckfd]=NULL;
	}
}

/**
 * @ingroup SoporteSSL
 *
 * @brief Elimina contexto SSL creado
 *
 * @synopsis
 * @code
 * 	void liberar_nivel_SSL()
 * @endcode
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
 */ 
void liberar_nivel_SSL() {
    SSL_CTX_free(context);
    context = NULL;
}








