/**
 * Cabecera de la biblioteca de semaforos 
 * @brief Biblioteca de semaforos
 * @file semaforos.h
 * @author Pablo Marcos Manchon pablo.marcosm@estudiante.uam.es
 * @date 11/04/2016
 */

/* Nota: se debe compilar en c99 o cambiar la
 * reserva estatica de algunas funciones en funcion
 * de size por una reserva dinamica
*/

#ifndef SEMAFOROS_H
#define	SEMAFOROS_H


#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <errno.h>


     

/**
 * @brief Valor de retorno en caso exitoso
 */
#define OK 0

/**
 * @brief Valor de retorno en caso error
 */
#define ERROR -1

/**
 * Inicializa los semaforos indicados.
 * @brief Funcion Inicializar_Semaforo.
 * @file semaforos.c
 * @date 11/04/2016
 * @param semid: Identificador del semaforo.
 * @param *array: Valores iniciales.
 * @return int: OK si todo fue correcto, ERROR en caso de error.
 */
int Inicializar_Semaforo(int semid, unsigned short *array);

/**
 * Borra un semaforo.
 * @brief Borrar_Semaforo.
 * @file semaforos.c
 * @date 11/04/2016
 * @param semid: Identificador del semaforo.
 * @return int: OK si todo fue correcto, ERROR en caso de error.
 */
int Borrar_Semaforo(int semid);

/**
 * Crea un semaforo con la clave y el tamaño especificado. 
 * Lo inicializa a 0.
 * @brief Crear_Semaforo.
 * @file semaforos.c
 * @date 11/04/2016
 * @param key: Clave precompartida del semaforo.
 * @param size: Tamaño del semaforo.
 * @return *semid: Identificador del semaforo.
 * @return int: ERROR en caso de error,
 *           0 si ha creado el semaforo,
 *           1 si ya estaba creado.
 */
int Crear_Semaforo(key_t key, int size, int *semid);

/**	
 * Baja el semaforo indicado
 * @brief Down_Semaforo
 * @file semaforos.c
 * @date 11/04/2016
 * @param semid: Identificador del semaforo.
 * @param num_sem: Semaforo dentro del array.
 * @param undo: Flag de modo persistente pese a finalización abrupta.
 * @return int: OK si todo fue correcto, ERROR en caso de error.
 */
int Down_Semaforo(int id, int num_sem, int undo);

/**
 * Baja todos los semaforos del array indicado por active.
 * @brief DownMultiple_Semaforo
 * @file semaforos.c
 * @date 11/04/2016
 * @param semid: Identificador del semaforo.
 * @param size: Numero de semaforos del array.
 * @param undo: Flag de modo persistente pese a finalización abrupta.
 * @param *active: Semaforos involucrados.
 *  @return int: OK si todo fue correcto, ERROR en caso de error.
 */
int DownMultiple_Semaforo(int id,int size, int undo,int * active);

/**
 * Sube el semaforo indicado
 * @brief Up_Semaforo
 * @file semaforos.c
 * @date 11/04/2016
 * @param semid: Identificador del semaforo.
 * @param num_sem: Semaforo dentro del array.
 * @param undo: Flag de modo persistente pese a finalizacion abupta.
 * @return int: OK si todo fue correcto, ERROR en caso de error.
 */
int Up_Semaforo(int id, int num_sem, int undo);

/**
 * Sube todos los semaforos del array indicado por active. 
 * @brief UpMultiple_Semaforo
 * @file semaforos.c
 * @date 11/04/2016
 * @param semid: Identificador del semaforo.
 * @param size: Numero de semaforos del array.
 * @param undo: Flag de modo persistente pese a finalización abrupta.
 * @param *active: Semaforos involucrados.
 * @return int: OK si todo fue correcto, ERROR en caso de error.
 */
int UpMultiple_Semaforo(int id,int size,int undo, int *active);

#endif /* SEMAFOROS_H */
