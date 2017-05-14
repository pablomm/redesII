

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "../includes/bot.h"
#include <redes2/irc.h>

int bot_galletas(char *mensaje3, char *tar){

	char *palabra = NULL, *mensaje2 = NULL;
	char line[2560];
	FILE *fp;

	char * mensaje;

	if(!mensaje3 || ! tar)
		return 1;

	mensaje = (char *) calloc(strlen(mensaje3)+1, sizeof(char));
	strcpy(mensaje, mensaje3);

	palabra = strtok(mensaje," \r\n\t?!,.;:-_");

	/* Detectamos si aluien ha dicho la palabra galletas */
	while(palabra){
		if(!strcmp(palabra,"galletas")){

			system("./misc/motd.bash");
			
		fp = fopen("misc/motdoffensive","r");
		if(!fp)
			return -1;

		while(fgets(line,256,fp)) {
			line[strlen(line)-1] = '\0';
			IRCMsg_Privmsg(&mensaje2, "*", tar, line);
			enviar_bot(mensaje2,socket_bot);
			if(mensaje2) { free(mensaje2); mensaje2=NULL; }
		}
			fclose(fp);
			break;
		}
		palabra = strtok(NULL," \r\n\t?!,.;:-_");
	}
	if(mensaje) free(mensaje);


	return 1;
}


int main(int argc, char *argv[]){

	privmsg_bot(argc, argv, bot_galletas,"galletas");

	return 0;
}

