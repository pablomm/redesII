/**
 * @file audiochat.h
 * @brief funciones relacionadas con el envio de audio
 * @author Pablo Marcos  <pablo.marcos@estudiante.uam.es>
 * @author Dionisio Perez  <dionisio.perez@estudiante.uam.es>
*/

#ifndef AUDIOCHAT_H
#define AUDIOCHAT_H

void cerrarAudioSend(void);
void *audiochatSend(void *audio);
void cerrarAudioRecv(void);
void *audiochatRecv(void *audio);

#endif /* AUDIOCHAT_H */

