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


int socket_close(void* ptr) {
	//fprintf(stderr, "%s\n", "PAPA JOHN'S");
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


Fid_t Socket(port_t port)
{
	bool err = false;
	Fid_t fid;
	FCB* fcb;
	SCB* scb;

  	Mutex_Lock(& kernel_mutex);
	//fprintf(stderr, "%s\n", "SOCKET TEST 1");

  	if (port < NOPORT || port > MAX_PORT) {
		//fprintf(stderr, "%s\n", "SOCKET TEST 2");
  		err = true;
  	}
  	else {
		if (FCB_reserve(1, &fid, &fcb) == 1) {
			//fprintf(stderr, "%s, %d\n", "SOCKET TEST 3", fid);
			scb = (SCB*) socket_init();
			scb->refcount = 1;
			scb->type = UNBOUND;
			scb->portnum = port;
			fcb->streamobj = scb;
			fcb->streamfunc = &socket_fops;
		}
		else {
			//fprintf(stderr, "%s\n", "SOCKET TEST 4");
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
	PCB* curr = CURPROC;
	//fprintf(stderr, "%s, %d\n", "LISTEN TEST 1", sock);

  	Mutex_Lock(& kernel_mutex);

	if (sock < MAX_FILEID && curr->FIDT[sock] != NULL && sock != NOFILE) {
		//fprintf(stderr, "%s\n", "LISTEN TEST 2");
		fcb = get_fcb(sock);
		scb = (SCB*) fcb->streamobj;

		if (scb->portnum == NOPORT || scb->type != UNBOUND || OccupiedPorts[scb->portnum] == true) {
			err = true;
		}
		else {
			//fprintf(stderr, "%s\n", "LISTEN TEST 3");
			scb->type = LISTENER;
			scb->listener.cv = COND_INIT;
			rlnode_init(& scb->listener.request_list, NULL);
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
  	//fprintf(stderr, "%s\n", "ACCEPT TEST 1");
  	if (lsock<MAX_FILEID && curr->FIDT[lsock]!=NULL) {
  		//fprintf(stderr, "%s\n", "ACCEPT TEST 2");
  		lfcb = get_fcb(lsock);
		lscb = (SCB*) lfcb->streamobj;
		if (lscb->type == LISTENER ) {
  			//fprintf(stderr, "%s\n", "ACCEPT TEST 3");
			while (is_rlist_empty(& lscb->listener.request_list) == 1)
				Cond_Wait(& kernel_mutex, &lscb->listener.cv);

			if (FCB_reserve(1, &pfid, &pfcb) == 1){
  				//fprintf(stderr, "%s\n", "ACCEPT TEST 4");
				pscb = (SCB*) socket_init();
				pscb->refcount = 0;
				pscb->type = PEER;
				pscb->portnum = 0;
				pfcb->streamobj = pscb;
				pfcb->streamfunc = &socket_fops;

				Mutex_Unlock(& kernel_mutex);

				if (Pipe(& pipe_1) == 0 && Pipe(& pipe_2) == 0) {
  					//fprintf(stderr, "%s\n", "ACCEPT TEST 5");

					Mutex_Lock(& kernel_mutex);

					rcb = rlist_pop_front(& lscb->listener.request_list)->rcb;
					pscb->peer.peer2 = rcb->scb;
					pscb->peer.fid_send = pipe_1.write;
					pscb->peer.fid_receive = pipe_2.read;
					rcb->scb->peer.peer2 = pscb;
					rcb->scb->peer.fid_send = pipe_1.read;
					rcb->scb->peer.fid_receive = pipe_2.write;
					rcb->flag = true;
					//fprintf(stderr, "%s\n", "finished with Accept");
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


int Connect(Fid_t sock, port_t port, timeout_t timeout)
{
	bool err = false;
	SCB* lscb, * pscb;
	//FCB* lfcb;
	RCB* rcb;

	Mutex_Lock(& kernel_mutex);
	//fprintf(stderr, "%s\n", "CONNECT TEST 1");
	if (port != NOPORT && port <= MAX_PORT && OccupiedPorts[port] == true ){
		lscb = get_fcb(PortSockets[port])->streamobj;
		pscb = get_fcb(sock)->streamobj;

		//fprintf(stderr, "%s\n", "CONNECT TEST 2");
		if(pscb->type == UNBOUND){

			//fprintf(stderr, "%s\n", "CONNECT TEST 3");
			rcb = (RCB*) request_init();
			rlnode_init(& rcb->node, rcb);
			rcb->scb = pscb;
			rcb->cv = COND_INIT;
			rcb->flag = false;
			rlist_push_back(& lscb->listener.request_list, & rcb->node);

			Cond_Signal(& lscb->listener.cv);

			//fprintf(stderr, "%s\n", "Waiting...");
			Cond_Wait(& kernel_mutex, & rcb->cv);
  			//fprintf(stderr, "%s\n", "Returned to Connect");

			usleep(timeout*10);
			if (rcb->flag == false){
				//fprintf(stderr, "%s\n", "CONNECT TEST 4");
				err = true;
			}
		}
		else {

			//fprintf(stderr, "%s\n", "CONNECT TEST 5");
			err = true;
		}
	}
	else {

		//fprintf(stderr, "%s\n", "CONNECT TEST 6");
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

