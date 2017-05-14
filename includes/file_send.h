/**
 * @file file_send.h
 * @brief Funciones para el envio de archivos
 * @author Pablo Marcos  <pablo.marcos@estudiante.uam.es>
 * @author Dionisio Perez  <dionisio.perez@estudiante.uam.es>
*/

#ifndef FILE_SEND_H
#define FILE_SEND_H

void * file_send_func(void * envioVoid);

void * file_recv_func(void * recvVoid);

void normalize_file(char* f);

#endif /* FILE_SEND_H */
