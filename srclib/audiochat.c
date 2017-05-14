/**
 * @file audiochat.c
 * @brief funciones relacionadas con el envio de audio
 * @author Pablo Marcos  <pablo.marcos@estudiante.uam.es>
 * @author Dionisio Perez  <dionisio.perez@estudiante.uam.es>
*/

/**
 * @defgroup AudioIRC AudioIRC
 *
 * <hr>
 */

#include "../includes/cliente.h"
#include "../includes/irc_cliente.h"
#include "../includes/red_cliente.h"
#include "../includes/comandos_noirc.h"
#include "../includes/audiochat.h"

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <redes2/ircsound.h>

#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <redes2/irc.h>
#include <redes2/ircxchat.h>

/**
 * @addtogroup AudioIRC
 * Comprende las funciones relacionadas con el envio de audio
 *
 * <hr>
 */

pAudioenvio audiodata;
pAudiorecepcion audiodatarecv;
int socketsendaudio;
int flagsendaudio;

int socketrecvaudio;
int flagrecvaudio;

/**
 * @ingroup AudioIRC
 *
 * @brief Manejador para terminar audio
 *
 * @synopsis
 * @code
 * 	void manejadorexitaudio(int sig)
 * @endcode
 *
 * @param[in] sig sennal recibida
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
void manejadorexitaudio(int sig){
	(void) sig;

	cerrarAudioRecv();
	pthread_exit(NULL);
}

/**
 * @ingroup AudioIRC
 *
 * @brief Manejador para terminar envio
 *
 * @synopsis
 * @code
 * 	void manejadorexitsend(int sig)
 * @endcode
 *
 * @param[in] sig sennal recibida
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
void manejadorexitsend(int sig){
	(void) sig;

	cerrarAudioSend();
	pthread_exit(NULL);
}

/**
 * @ingroup AudioIRC
 *
 * @brief Termina el envio de audio
 *
 * @synopsis
 * @code
 * 	void cerrarAudioSend(void)
 * @endcode
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
void cerrarAudioSend(void){

	if((audiodata != NULL ) &&flagsendaudio >= 0){
	   IRC_MFree(3,&(audiodata->ip), &(audiodata->nick),&audiodata);
	}

	if(flagsendaudio >= 1){
		if(socketsendaudio != -1){
			close(socketsendaudio);
			socketsendaudio = -1;
		}
	}

	flagsendaudio = -1;
	cliente.audio_send_thread_th = 0;
	cliente.audiochatsend = DISPONIBLE;
	cliente.audiochatrecv = DISPONIBLE;

}

/**
 * @ingroup AudioIRC
 *
 * @brief Se encarga del envio de audio
 *
 * @synopsis
 * @code
 * 	void *audiochatSend(void *audio)
 * @endcode
 *
 * @param[in] *audio estructura con informacion del audio
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
void *audiochatSend(void *audio){

	struct sockaddr_in si_other;
    int s, slen=sizeof(si_other);
    char buf[2560];

	pthread_detach(pthread_self());

	audiodata = (pAudioenvio) audio;

	signal(SIGUSR1, manejadorexitaudio);
	signal(SIGUSR2, manejadorexitsend);
 
	flagsendaudio = 0;

    if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
		cerrarAudioSend();
        pthread_exit(NULL);
    }
	socketsendaudio = s;
 
	/* */
    memset((char *) &si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(audiodata->puerto);
     
	flagsendaudio = 1;

    if (inet_aton(audiodata->ip, &si_other.sin_addr) == 0) 
    {
		cerrarAudioSend();
        pthread_exit(NULL);
    }

	IRCSound_RecordFormat(PA_SAMPLE_S16BE,2);
 	if(IRCSound_OpenRecord()){
		cerrarAudioSend();
        pthread_exit(NULL);		
	}

	flagsendaudio = 2;

    while(1)
    {

         IRCSound_RecordSound(buf,TAMPACKETAUDIO);
        /* Enviamos el mensaje */
        if (sendto(s, buf, TAMPACKETAUDIO, MSG_DONTWAIT, (struct sockaddr *) &si_other, slen)<= 0) {
            cerrarAudioSend();
        	pthread_exit(NULL);
        }
	}
	cerrarAudioSend();
    pthread_exit(NULL);
}

/**
 * @ingroup AudioIRC
 *
 * @brief Termina recepcion del audio
 *
 * @synopsis
 * @code
 * 	void cerrarAudioRecv(void)
 * @endcode
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
void cerrarAudioRecv(void){

	cliente.audiochatsend = DISPONIBLE;
	cliente.audiochatrecv = DISPONIBLE;
	if((audiodatarecv!=NULL) && flagrecvaudio >= 1){
		close(audiodatarecv->socket);
		IRC_MFree(1,&audiodatarecv);
	}

	if(flagrecvaudio >= 2)
  		IRCSound_ClosePlay();

	flagrecvaudio = -1;
	cliente.audio_recv_thread_th = 0;
}

/**
 * @ingroup AudioIRC
 *
 * @brief Se encarga de la recepcion de audio
 *
 * @synopsis
 * @code
 * 	void *audiochatRecv(void *audio)
 * @endcode
 *
 * @param[in] *audio estructura con informacion del audio
 *
 * @author
 * Pablo Marcos Manchon
 * Dionisio Perez Alvear
 *
 *<hr>
*/
void *audiochatRecv(void *audio){

	char buffer[BUFFERAUDIO] = {0};
	ssize_t tam;
	socklen_t slen = sizeof(struct sockaddr_in);

	pthread_detach(pthread_self());

	signal(SIGUSR1, manejadorexitaudio);
	signal(SIGUSR2, manejadorexitsend);

	flagrecvaudio = 0;

	audiodatarecv = audio;

	if(audiodatarecv == NULL){
		cerrarAudioRecv();
		pthread_exit(NULL);
	}

	flagrecvaudio = 1;

	IRCSound_PlayFormat(PA_SAMPLE_S16BE,2); 
	if(IRCSound_OpenPlay()) {
		cerrarAudioRecv();
		pthread_exit(NULL);
	}

	flagrecvaudio = 2;

	while((tam = (recvfrom(audiodatarecv->socket, buffer, BUFFERAUDIO, 0, (struct sockaddr *) &(audiodatarecv->direccion), &slen)))) {
		if(tam == -1){
			if(errno != EINTR)
				break;
		}
		if(cliente.playing){
		   IRCSound_PlaySound(buffer,tam);
		}

	}

	cerrarAudioRecv();
	pthread_exit(NULL);
}





