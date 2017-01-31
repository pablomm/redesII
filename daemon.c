#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <syslog.h>

#define IDENT "REDESII"
#define ERROR -1

daemonizar(){

	/* Creamos proceso hijo */
	switch(fork()){
		case -1: /* Caso de error */
			perror("fork failed");
			exit(EXIT_FAILURE);
		
		case 0: /* Caso del hijo  */
			break;

		default: /* Caso del padre */
			exit(EXIT_SUCCESS);
	}

	/* Creamos nueva sesion */
	if(setsid() < 0){
		exit(EXIT_FAILURE);
	}

	/* Cambiamos mascara */
	umask(0);

	/* Cambiamos a directorio raiz */
	chdir("/");

	/* Cerramos descriptores */
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	/* Abrimos log */
	openlog(IDENT, LOG_PID, LOG_USER);

	return (0);
}

int main(void){

	daemonizar();
	exit(EXIT_SUCCESS);

}
