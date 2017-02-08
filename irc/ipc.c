
#include "ipc.h"



int crear_cola_mensajes(int key){

	key_t clave;
	int msqid;

    /* Obtenemos clave para generar la cola de mensajes */
    clave = ftok("/bin/ls", key);
    if (clave == (key_t) -1){
        syslog(LOG_ERR, "Error creando cola de mensajes en llamada a ftok()");
        return ERROR;
    }
    
    /* Creamos la cola de mensajes */
    msqid = msgget(clave, 0666 | IPC_CREAT);
    if (msqid == -1){
		syslog(LOG_ERR, "Error creando cola de mensajes en llamada a msgget()");
        return ERROR;
    }

	return msqid;
}

int borrar_cola_mensajes(int msqid){
	
	return msgctl(msqid, IPC_RMID, (struct msqid_ds *)NULL);
}

int enviar_mensaje(int msqid, struct msgbuf *msg) {
	
	int ret;

	do {
		/* Envia un mensaje bloqueante */
		ret = msgsnd (msqid, msg, sizeof(*msg) - sizeof(long), 0);

		/* Caso de error */
		if(ret == -1 &&  errno == EIDRM){
			syslog(LOG_ERR, "Error en envio de mensaje");
			return ERROR;
		}

	/* Si el bloqueo es interrumpido por una señal vuelve a enviarse */
	} while(ret == -1 && errno == EINTR);

	return OK;
}

int recibir_mensaje(int msqid, struct msgbuf *msg, int id){
	
	int ret;

	do {
		/* Recepcion bloqueante de un mensaje */
		ret = msgrcv(msqid, msg, sizeof(*msg) - sizeof(long), id, 0);

		/* Caso de error */
		if(ret == -1 && ret == EIDRM){
			syslog(LOG_ERR, "Error en envio de mensaje");
			return ERROR;
		}

	/* Si el bloqueo es interrumpido por una señal vuelve a bloquearse a la espera */
	} while(ret == -1 && ret == EINTR);

	return OK;
}



