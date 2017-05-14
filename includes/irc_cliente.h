/**
 * @file irc_cliente.h
 * @brief Toda la parte de comandos de usuario
 * @author Pablo Marcos  <pablo.marcos@estudiante.uam.es>
 * @author Dionisio Perez  <dionisio.perez@estudiante.uam.es>
*/

#ifndef IRC_CLIENTE_H
#define IRC_CLIENTE_H


void *recv_pthread(void *sckfd);
void inicializar_comandos_recv(void);

#endif /* IRC_CLIENTE_H */
