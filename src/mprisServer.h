#ifndef MPRISSERVER_H_
#define MPRISSERVER_H_

void* startServer(void*);
void stopServer(void);

void emitVolumeChanged(float);
void emitSeeked(float);
void emitMetadataChanged(int, DB_functions_t*);

#endif
