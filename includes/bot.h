/**
 * @file bot.h
 * @brief Funciones para lanzar el bot
 * @author Pablo Marcos  <pablo.marcos@estudiante.uam.es>
 * @author Dionisio Perez  <dionisio.perez@estudiante.uam.es>
*/

#ifndef BOT_H
#define BOT_H


int conectar_bot(char *server, int port, char *channel, char *nick, char *user, char *realname, int ssl);

int socket_bot;

ssize_t recibir_bot(int sckfd, char *buffer, int max);

int enviar_bot(char *mensaje, int socket);

void procesar_privmsg_bot(int sckfd, int (*procesar)(char*,char*));

void usage_bot(void);

int privmsg_bot(int argc, char **argv,int (*procesar)(char*,char*), char *name);

#endif /* BOT_H */
