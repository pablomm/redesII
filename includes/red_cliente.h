/**
 * @file red_cliente.h
 * @brief crear sockets y enviar referido al cliente
 * @author Pablo Marcos  <pablo.marcos@estudiante.uam.es>
 * @author Dionisio Perez  <dionisio.perez@estudiante.uam.es>
 */

#ifndef RED_CLIENTE_H
#define RED_CLIENTE_H

#define RED_ERROR -1
#define RED_OK 0

#include "cliente.h"

status crearSocketTCP_cliente(int *sckfd, char *server, unsigned short port);
status enviar_cliente(char *mensaje);
status enviar_clienteThread(char *mensaje);
status socketUDP(int *sckfd, short puerto);


#endif /* RED_CLIENTE_H */
