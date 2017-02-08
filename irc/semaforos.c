/**
 * Biblioteca de semaforos 
 * @brief Biblioteca de semaforos
 * @file semaforos.c
 * @author Pablo Marcos Manchon pablo.marcosm@estudiante.uam.es
 * @date 11/04/2016
 */

#include "semaforos.h"

/**
 * Inicializa los semaforos indicados.
 * @brief Funcion Inicializar_Semaforo.
 * @file semaforos.c
 * @date 11/04/2016
 * @param semid: Identificador del semaforo.
 * @param *array: Valores iniciales.
 * @return int: OK si todo fue correcto, ERROR en caso de error.
 */
int Inicializar_Semaforo(int semid, unsigned short *array){
    /* Creamos union semun */
    union semun{
        int val;
        struct semid_ds *semstat;
        unsigned short *array;
    } arg;

    /* Control de errores */
    if(array == NULL){
        return ERROR;
    }
    arg.array=array;

    return semctl(semid, 0, SETALL, arg);
}

/**
 * Borra un semaforo.
 * @brief Borrar_Semaforo.
 * @file semaforos.c
 * @date 11/04/2016
 * @param semid: Identificador del semaforo.
 * @return int: OK si todo fue correcto, ERROR en caso de error.
 */
int Borrar_Semaforo(int semid){
    return semctl(semid, 0, IPC_RMID);
}

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
int Crear_Semaforo(key_t key, int size, int *semid){
    
	/* Control de Errores */    
    if( !semid || size < 1){
        return ERROR;
    }
    *semid=semget(key, size, IPC_CREAT | IPC_EXCL | SHM_R | SHM_W);

    /* Caso semaforo ya creado */
    if(*semid == ERROR && errno == EEXIST){
        /* Quitamos flags IPC_CREAT y IPC_EXCL */
        *semid = semget(key, size,  SHM_R | SHM_W);
        return 1;
    }
    /* Caso de Error */
    if(*semid == ERROR){
        return ERROR;
    }

    return 0;
 }

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
int Down_Semaforo(int id, int num_sem, int undo){
    struct sembuf sem_oper;
    
    /* Control de errors */    
    if(num_sem<0){
        return ERROR;
    }
    /* Establecemos valores de la estructura */
    sem_oper.sem_num = num_sem;
    sem_oper.sem_op = -1; /* Decrementamos 1 */
    sem_oper.sem_flg = undo;

    return semop(id, &sem_oper, 1);
}

/**
 * Baja todos los semaforos del array indicado por active.
 * @brief DownMultiple_Semaforo
 * @file semaforos.c
 * @date 11/04/2016
 * @param semid: Identificador del semaforo.
 * @param size: Numero de semaforos del array.
 * @param undo: Flag de modo persistente pese a finalización abrupta.
 * @param *active: Semaforos involucrados.
 * @return int: OK si todo fue correcto, ERROR en caso de error.
 */
int DownMultiple_Semaforo(int id,int size, int undo,int * active){
    struct sembuf sem_oper[size];
    int i;

    /* Control de errores */    
    if( !active || size < 1){
        return ERROR;
    }
    /* Bucle para inicializar los sembuf */
    for(i=0; i<size; i++){
        sem_oper[i].sem_num = active[i];
        sem_oper[i].sem_op = -1; /* Decrementamos 1 */
        sem_oper[i].sem_flg = undo;
    }

    return semop(id, sem_oper, size);
}

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
int Up_Semaforo(int id, int num_sem, int undo){
    struct sembuf sem_oper;
    
    /* Control de errors */    
    if(num_sem < 0){
        return ERROR;
    }
    /* Establecemos valores de la estructura */
    sem_oper.sem_num = num_sem;
    sem_oper.sem_op = 1; /* Aumentamos 1 */
    sem_oper.sem_flg = undo;

    return semop(id, &sem_oper, 1);
}

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
int UpMultiple_Semaforo(int id,int size,int undo, int *active){
    struct sembuf sem_oper[size];
    int i;

    /* Control de errores */    
    if( !active || size < 1){
        return ERROR;
    }
    /* Bucle para inicializar los sembuf */
    for(i=0; i<size; i++){
        sem_oper[i].sem_num = active[i];
        sem_oper[i].sem_op = 1; /* Aumentamos 1 */
        sem_oper[i].sem_flg = undo;
    }

    return semop(id, sem_oper, size);
}
