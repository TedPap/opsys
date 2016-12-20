#include "tinyos.h"
#include "kernel_socket.h"
#include "kernel_cc.h"
#include "kernel_streams.h"
#include "kernel_sched.h"
#include "kernel_dev.h"

void* socket_init() {

	void* ptr = xmalloc(sizeof(SCB));
	return ptr;
}

int socket_read(void* ptr, char *buf, unsigned int size) {

    return 0;
}


int socket_write(void* ptr, const char* buf, unsigned int size) {

    return 0;
}


int socket_close(void* ptr) {

  	return 0;
}


void* socket_open(uint minor) {

  	return NULL;
}


file_ops socket_fops = {
  .Open = socket_open,
  .Read = socket_read,
  .Write = socket_write,
  .Close = socket_close
};


Fid_t Socket(port_t port)
{
	bool err = false;
	Fid_t fid;
	FCB* fcb;
	SCB* scb;

  	Mutex_Lock(& kernel_mutex);

  	if (port < NOPORT && port > MAX_PORT) {
  		err = true;
  	}
  	else {
		if (FCB_reserve(1, &fid, &fcb) == 1) {
			scb = (SCB*) socket_init();
			scb->refcount = 0;
			scb->type = UNBOUND;
			scb->portnum = port;
			fcb->streamobj = scb;
			fcb->streamfunc = &socket_fops;
		}
		else {
			err = true;
		}
	}

	Mutex_Unlock(& kernel_mutex);

	switch (err) {
		case false:
			return fid;
		case true:
			return NOFILE;
	}
	return NOFILE;
}

int Listen(Fid_t sock)
{
	bool err = false;
	SCB* scb;
	FCB* fcb;

  	Mutex_Lock(& kernel_mutex);

	if (sock<MAX_FILEID && CURPROC->FIDT[sock]!=NULL) {
		fcb = get_fcb(sock);
		scb = (SCB*) fcb->streamobj;
		if (scb->port == NOPORT || scb->type != UNBOUND) {
			err = true;
		}
		else {
			
		}
	}
	else {
		err = true;
	}

	Mutex_Unlock(& kernel_mutex);

	switch (err) {
		case false:
			return 0;
		case true:
			return -1;
	}
	return -1;
}


Fid_t Accept(Fid_t lsock)
{
	return NOFILE;
}


int Connect(Fid_t sock, port_t port, timeout_t timeout)
{
	return -1;
}


int ShutDown(Fid_t sock, shutdown_mode how)
{
	return -1;
}

