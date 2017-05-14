/**
 * @file ssl.h
 * @brief Soporte SSL
 * @author Pablo Marcos  <pablo.marcos@estudiante.uam.es>
 * @author Dionisio Perez  <dionisio.perez@estudiante.uam.es>
 */

#ifndef SSL_H
#define SSL_H

#define MAX_SSL 1024

#define SSL_ERR -1
#define SSL_OK 0

int ssl_active;

int inicializar_nivel_SSL();
 
int fijar_contexto_SSL(const char *pcert, const char *pkey);
 
int conectar_canal_seguro_SSL(int sckfd);
 
int aceptar_canal_seguro_SSL(int sckfd);
 
int evaluar_post_conectar_SSL(int sckfd);
 
int enviar_datos_SSL(int sckfd, void* mensaje, int tam);
 
int recibir_datos_SSL(int sckfd, void* mensaje, int max);

void cerrar_canal_SSL(int sckfd);

void liberar_nivel_SSL();


#endif
