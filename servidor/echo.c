
#include "redes2.h"




int main(void){


	int sckt, desc;
	struct sockaddr_in direccion;
	char echo[100] = {0};

	//printf("Vale\n");
	abrirLog(LOG_DEBUG);


	sckt = crearSocketTCP(1234, 10);
	printf("Socket creado\n");

	desc = aceptar_conexion(sckt);
	printf("Conexion aceptada\n");

	for(;;){	
		recv(desc, echo, 100*sizeof(char), 0);

		printf("%s\n", echo);
	}
	exit(EXIT_SUCCESS);
}
