#include "tinyos.h"
#include "kernel_socket.h"
#include "kernel_cc.h"
#include "kernel_streams.h"
#include "kernel_sched.h"
#include "kernel_proc.h"
#include "kernel_dev.h"
#include <unistd.h>
#include "assert.h"

bool OccupiedPorts[MAX_PORT] = { false };
Fid_t PortSockets[MAX_PORT] = { NOFILE };

void* socket_init() {

	void* ptr = xmalloc(sizeof(SCB));
	return ptr;
}


void* request_init() {

	void* ptr = xmalloc(sizeof(RCB));
	return ptr;
}


/** Call to the read pipe's Read fileop. */
int socket_read(void* ptr, char *buf, unsigned int size) {
	SCB* temp = (SCB*) ptr;
	FCB* fcb;
	int ret;
	if (temp->type == PEER){
		fcb = get_fcb(temp->peer.fid_receive);
		ret = fcb->streamfunc->Read(fcb->streamobj, buf, size);
		return ret;
	}
	else{
		return 0;
	}

    return 0;
}


/** Call to the write pipe's Write fileop. */
int socket_write(void* ptr, const char* buf, unsigned int size) {
	SCB* temp = (SCB*) ptr;
	FCB* fcb;
	int ret;
	if (temp->type == PEER){
		fcb = get_fcb(temp->peer.fid_send);
		ret = fcb->streamfunc->Write(fcb->streamobj, buf, size);
		return ret;
	}
	else{
		return 0;
	}

    return 0;
}


/** Free the alocated space of the socket. */
int socket_close(void* ptr) {
	SCB* temp = (SCB*) ptr;
	temp->refcount--;
	if (temp->refcount == 0)
		OccupiedPorts[temp->portnum] = false;
		free(ptr);
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


/** Create and initialize an UNBOUND socket control block. */
Fid_t Socket(port_t port)
{
	bool err = false;
	Fid_t fid;
	FCB* fcb;
	SCB* scb;

  	Mutex_Lock(& kernel_mutex);

  	/** Error check. */
  	if (port < NOPORT || port > MAX_PORT) {
  		err = true;
  	}
  	else {
		if (FCB_reserve(1, &fid, &fcb) == 1) {
			/** Check for enough fid's */
			scb = (SCB*) socket_init();
			scb->refcount = 1;
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


/** Further initialize an UNBOUND socket as a LISTENER. */
int Listen(Fid_t sock)
{
	bool err = false;
	SCB* scb;
	FCB* fcb;
	PCB* curr = CURPROC;

  	Mutex_Lock(& kernel_mutex);

  	/** Error checks. */
	if (sock < MAX_FILEID && curr->FIDT[sock] != NULL && sock != NOFILE) {
		fcb = get_fcb(sock);
		scb = (SCB*) fcb->streamobj;

		if (scb->portnum == NOPORT || scb->type != UNBOUND || OccupiedPorts[scb->portnum] == true) {
			err = true;
		}
		else {
			scb->type = LISTENER;
			scb->listener.cv = COND_INIT;
			rlnode_init(& scb->listener.request_list, NULL);

			/** Hold info about which port we occupie. */
			OccupiedPorts[scb->portnum] = true;
			PortSockets[scb->portnum] = sock;
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


/** Check if all is ok and create a PEER socket to receive the I/O 
of the connecting PEER. */
Fid_t Accept(Fid_t lsock)
{
	bool err = false;
	Fid_t pfid;
	SCB* lscb, * pscb;
	FCB* lfcb, *pfcb;
	PCB* curr = CURPROC;
	RCB* rcb;
	pipe_t pipe_1, pipe_2;

  	Mutex_Lock(& kernel_mutex);
  	/** Error checks. */
  	if (lsock<MAX_FILEID && curr->FIDT[lsock]!=NULL) {
  		lfcb = get_fcb(lsock);
		lscb = (SCB*) lfcb->streamobj;
  		/** Error checks. */
		if (lscb->type == LISTENER ) {
			/** While request list is empty, wait for connections. */
			while (is_rlist_empty(& lscb->listener.request_list) == 1)
				Cond_Wait(& kernel_mutex, &lscb->listener.cv);

			/** Check if there are enough fids. */
			if (FCB_reserve(1, &pfid, &pfcb) == 1){

				/** Initiate the PEER socket. */
				pscb = (SCB*) socket_init();
				pscb->refcount = 0;
				pscb->type = PEER;
				pscb->portnum = 0;
				pfcb->streamobj = pscb;
				pfcb->streamfunc = &socket_fops;

				Mutex_Unlock(& kernel_mutex);

				/** Create the pipes to be used by the peers. */
				if (Pipe(& pipe_1) == 0 && Pipe(& pipe_2) == 0) {

					Mutex_Lock(& kernel_mutex);

					/** Link everything up. */
					rcb = rlist_pop_front(& lscb->listener.request_list)->rcb;
					pscb->peer.peer2 = rcb->scb;
					pscb->peer.fid_send = pipe_1.write;
					pscb->peer.fid_receive = pipe_2.read;
					rcb->scb->peer.peer2 = pscb;
					rcb->scb->peer.fid_send = pipe_1.read;
					rcb->scb->peer.fid_receive = pipe_2.write;
					rcb->flag = true;
					Cond_Signal(& rcb->cv);
				}
				else{
					Mutex_Lock(& kernel_mutex);
					err = true;
				}
			}
			else {
				err = true;
			}
		}
		else {
			err = true;
		}
  	}
  	else {
  		err = true;
  	}

	Mutex_Unlock(& kernel_mutex);

	switch (err) {
		case false:
			return pfid;
		case true:
			return NOFILE;
	}
	return NOFILE;
}

/** Create a connection request to the listening socket occupying the given port. */
int Connect(Fid_t sock, port_t port, timeout_t timeout)
{
	bool err = false;
	SCB* lscb, * pscb;
	RCB* rcb;

	Mutex_Lock(& kernel_mutex);

	/** Error checks. */
	if (port != NOPORT && port <= MAX_PORT && OccupiedPorts[port] == true ){
		lscb = get_fcb(PortSockets[port])->streamobj;
		pscb = get_fcb(sock)->streamobj;

		/** Error checks. */
		if(pscb->type == UNBOUND){

			/** Initialize the RCB to bw pushed into the Listener's list. */
			rcb = (RCB*) request_init();
			rlnode_init(& rcb->node, rcb);
			rcb->scb = pscb;
			rcb->cv = COND_INIT;
			rcb->flag = false;
			rlist_push_back(& lscb->listener.request_list, & rcb->node);

			/** Signal the listener that it has a request. */
			Cond_Signal(& lscb->listener.cv);

			/** Wait to be processed by Accept. */
			Cond_Wait(& kernel_mutex, & rcb->cv);
			usleep(timeout*10);

			/** If not processed by the end of timeout return error. */
			if (rcb->flag == false){
				err = true;
			}
		}
		else {

			err = true;
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

/** Close the right fids depending on the shutdown mode*/
int ShutDown(Fid_t sock, shutdown_mode how)
{
	Fid_t soc_read, soc_write;
	FCB* fcb;
	SCB* scb;

	Mutex_Lock(& kernel_mutex);

	fcb = get_fcb(sock);
	scb = fcb->streamobj;
	soc_read = scb->peer.fid_receive;
	soc_write = scb->peer.fid_send;

	switch (how) {
		case SHUTDOWN_READ:
			Mutex_Unlock(& kernel_mutex);
			Close(soc_read);
		case SHUTDOWN_WRITE:
			Mutex_Unlock(& kernel_mutex);
			Close(soc_write);
		case SHUTDOWN_BOTH:
			Mutex_Unlock(& kernel_mutex);
			Close(soc_read);
			Close(soc_write);
			Close(sock);
	}

	return -1;
}

