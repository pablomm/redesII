/**
 * @file comandos_cliente.h
 * @brief Funciones para el tratamiento de los comandos de cliente
 * @author Pablo Marcos  <pablo.marcos@estudiante.uam.es>
 * @author Dionisio Perez  <dionisio.perez@estudiante.uam.es>
*/

#ifndef COMANDOS_CLIENTE_H
#define COMANDOS_CLIENTE_H

typedef void (*comando_cliente_t)(char *); 

comando_cliente_t comandos_cliente[IRC_MAX_USER_COMMANDS];

void inicializar_comandos_cliente(void);

#endif /* COMANDOS_CLIENTE_H */

