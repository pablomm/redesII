/**
 * @file cliente.h
 * @brief Macros y estructuras necesarias para el tratamiento del cliente
 * @author Pablo Marcos  <pablo.marcos@estudiante.uam.es>
 * @author Dionisio Perez  <dionisio.perez@estudiante.uam.es>
*/

#ifndef CLIENTE_H
#define CLIENTE_H

#include <pthread.h>

#define CONECTADO 0
#define NO_CONECTADO -1
#define PENDIENTE -2
#define OCUPADO -3
#define DISPONIBLE 0
#define ESPERANDO_ACCEPT 4
#define ENVIANDO_ARCHIVO 5
#define ENVIO_COMPLETADO 6
#define ENVIO_CANCELADO -4
#define TIMEOUT_ACCEPT 15
#define BUFFERAUDIO 2560
#define TAMPACKETAUDIO 800

#define RECV_FILE "files"

#define TAM_LIMIT_NICK 10
#define TAM_ERROR_MSG 40
#define TAM_RECV 16384
#define TAM_COM_USER 256
#define TAM_DEST 128
#define TAM_LINE 80
#define TAM_LINE_MED 240
#define DELIMITERS ", "
#define COMANDO_LEN 200
#define TAM_INTERFACE 15

#define TAM_PACKAGE 8192

#define DEFAULT_INTERFACE "wlan0"
#define FCANCEL "\001FCANCEL"

#define NOTICE_TYPE "NOTICE"
#define WHOIS_TYPE "  WHOIS  "
#define TOPIC_TYPE "TOPIC"

#define KICK_MSG "Fuera de aqui, rata!"
#define QUIT_MSG "Asas"
#define ERROR_COMMAND_MSG "Comando no reconocido: "
#define USER_IN_CHANNEL_MSG " se ha unido al canal"
#define JOIN_CHANNEL_MSG "Te has unido al canal"
#define END_OF_MOTD_MSG "End of MOTD"
#define MOTD_START_MSG "Message of the day of "
#define NAMREPLY_MSG "Usuarios en "
#define NICK_CHANGE_SELF_MSG "Cambio de nombre efectuado: "
#define RPL_QUIT_MSG "%s ha abandonado el servidor: %s"
#define RPL_WHOISUSER_MSG "Usuario %s, nombre %s, host %s, realname %s"
#define RPL_WHOISCHANNEL_MSG "(%s) Canales: %s"
#define RPL_WHOISOPERATOR_MSG "(%s) Es operador del servidor"
#define RPL_WHOISSERVER_MSG "(%s) conectado al servidor %s: %s"
#define RPL_WHOISIDLE_MSG "(%s) inactivo durante %d(s)"
#define RPL_LIST_MSG "Canal: %s (%s): %s"
#define BAN_RPL_MSG "%s ha sido baneado del canal %s: %s"
#define RPL_TOPIC_MSG "Nuevo topic (cambiado por %s): %s"
#define NO_TOPIC_MSG "Topic del canal sin definir"
#define TOPIC_MSG "<%s>"
#define ERR_OP_PRIV_MSG "No tienes permisos para realizar la accion"
#define ERR_NO_SUCH_NICK_MSG "El usuario %s no existe"
#define ERR_NO_SUCH_CHANNEL_MSG "El canal %s no existe"
#define RPL_PART_MSG "El usuario %s ha abandonado el canal (%s)"
#define RPL_KICK_SELF_MSG "%s te ha expulsado del canal (%s)"
#define RPL_KICK_MSG "%s ha expulsado a %s (%s)"
#define AWAY_MSG "Ahora estás ausente"
#define UNAWAY_MSG "Ya no estás ausente"
#define RPL_AWAY_MSG "El usuario está ausente: %s"
#define CHANNEL_MODE_IS_MSG "Modo del canal %s"
#define RPL_WHO_MSG "%s %s %s %s %s %s"
#define ERR_NICK_NAME_IN_USE_MSG "El nick ya está siendo utilizado. Intente conectarse con otro nick"
#define ERR_PASS_MISMATCH_MSG "La contraseña es incorrecta. Intente conectarse con otra contraseña o cambie de usuario"
#define ERR_ALREADY_REGISTRED_MSG "Ya está registrado. Debe desconectarse para volver a registrarse"
#define ERR_NO_NICKNAME_GIVEN_MSG "Debe introducir un nick"
#define ERR_ERRONEUS_NICKNAME_MSG "Nick invalido. Pruebe con otro nick"
#define SEND_ERROR_BUSY "Solo puede enviar un archivo a la vez. Espere a que el envio se complete"
#define N_ACCEPT_FILE_MSG "El receptor no ha aceptado el envio del archivo"
#define CANCEL_SEND_MSG "Envio de archivo cancelado"
#define SEND_COMPLETED_MSG "Envio completado de %s"
#define ERR_CON_RECV_MSG "Error antes de aceptar conexion en la recepcion"
#define ERR_RECV_MSG "Error durante la recepcion de fichero"

#ifndef STATUS
#define STATUS
typedef int status;
#endif

typedef struct _cliente {

	int socket;
	int puerto;
	char *server;
	char *nick;
	char *realname;
	char *user;
	char *prefijo;
	char *interfaz;
	int playing;
	status registro;
	pthread_t recv_thread_th;

	status envio;
	pthread_t file_thread_th;

	status recepcion;
	pthread_t file_recv_thread_th;

	pthread_mutex_t envio_thread_mutex;

	int audiochatsend;
	pthread_t audio_send_thread_th;

	int audiochatrecv;
	pthread_t audio_recv_thread_th;

} Cliente, *pCliente;


typedef struct _envio {
	char *filename;
	char *nick;
	char *data;
	long unsigned int length;

} Envio, *pEnvio;

typedef struct _recepcion {
	char *ip;
	char *filename;
	short puerto;
	long unsigned int len;
} Recepcion, *pRecepcion;

typedef struct _audioenvio {
	char *ip;
	char *nick;
	short puerto;
} Audioenvio, *pAudioenvio;

typedef struct _audiorecepcion {
	int socket;
	void *direccion;
} Audiorecepcion, *pAudiorecepcion;


char interfaz[TAM_INTERFACE];

Cliente cliente;
pEnvio envio;
pRecepcion recepcion;

#endif /* CLIENTE_H */
