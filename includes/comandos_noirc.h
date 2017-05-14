/**
 * @file comandos_noirc.h
 * @brief Funciones para el parseo de mensajes de envio de ficheros y audio
 * @author Pablo Marcos  <pablo.marcos@estudiante.uam.es>
 * @author Dionisio Perez  <dionisio.perez@estudiante.uam.es>
*/

#ifndef COMANDOS_NOIRC_H
#define COMANDOS_NOIRC_H

void parse_noIRC(char * comando, char *nick);

void parse_fsend(char * comando, char *nick);
void parse_fcancel(char * comando, char *nick);
void parse_audiochat(char * comando, char *nick);
void parse_audioexit(char * comando, char *nick);

#endif /* COMANDOS_NOIRC_H */
