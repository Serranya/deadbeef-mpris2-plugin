#ifndef MPRISSERVER_H_
#define MPRISSERVER_H_

void* startServer(void*);
void stopServer();

void emitVolumeChanged(float);

#endif
