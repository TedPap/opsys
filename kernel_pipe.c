#include "tinyos.h"
#include "kernel_cc.h"
#include "kernel_streams.h"
#include "kernel_sched.h"
#include "kernel_pipe.h"
#include "kernel_dev.h"


void* pipe_init() {

	void* ptr = xmalloc(sizeof(PICB));
	return ptr;
}

void* pipe_open(uint minor)
{
  return NULL;
}

int pipe_read(void* ptr, char *buf, unsigned int size)
{
	int i = 0;
	bool err = false;
	PICB* picb = (PICB*) ptr;

	Mutex_Lock(& picb->spinlock);

	if (picb->pfcb[1]->refcount == 0) {
		if (picb->balance <= 0)
			err = true;
	}
	
	while (picb->balance < size && picb->pfcb[1]->refcount != 0)
    	Cond_Wait(& picb->spinlock, & picb->buff_full);

  	while ( i < size && i < buffer_size && picb->balance > 0) {
  		buf[i] = picb->puffer[i];
  		i++;
  		picb->balance--;
  	}

    Mutex_Unlock(& picb->spinlock);

  	switch (err){
    	case true:
    		return 0;
    	case false:
    		return i;
    }
    return 0;
}

int pipe_write(void* ptr, const char* buf, unsigned int size)
{
	int i = 0;
	bool err = false;
	PICB* picb = (PICB*) ptr;

	if (picb->pfcb[0]->refcount == 0)
		err = true;

	if (picb->balance >= buffer_size) {
		Cond_Broadcast(& picb->buff_full);
	}

    while (i < size && i < buffer_size) {
    	picb->puffer[i] = buf[i];
    	i++;
    	picb->balance++;
    }

    Mutex_Unlock(& picb->spinlock);

    switch (err){
    	case true:
    		return -1;
    	case false:
    		return i;
    }
    return -1;
}


int pipe_close(void* ptr) 
{
	PICB* picb = (PICB*) ptr;
	//fprintf(stderr, "%s\n", "pipe_close");
	if (picb->pfcb[0]->refcount == 0 && picb->pfcb[1]->refcount == 0) {
		//release pipe;
	}
  	return 0;
}

file_ops pipe_fops = {
  .Open = pipe_open,
  .Read = pipe_read,
  .Write = pipe_write,
  .Close = pipe_close
};

int Pipe(pipe_t* pipe)
{
	Fid_t Fids[2];
	bool err = false;


  	Mutex_Lock(& kernel_mutex);

	PICB* picb = (PICB*) pipe_init();

	if(FCB_reserve(2, Fids, picb->pfcb) == 1) {
		err = false;
			pipe->read = Fids[0];
			pipe->write = Fids[1];

			picb->pipe = pipe;
			picb->pipe_ops = pipe_fops;
			picb->pfcb[0]->streamobj = picb;
			picb->pfcb[1]->streamobj = picb;
			picb->pfcb[0]->streamfunc = &picb->pipe_ops;
			picb->pfcb[1]->streamfunc = &picb->pipe_ops;
			picb->balance = 0;
			picb->spinlock = MUTEX_INIT;
			picb->buff_full = COND_INIT;
	}
	else {
		err = true;
	}

  	Mutex_Unlock(& kernel_mutex);  

	switch(err) {
		case true:
			return -1;
		case false:
			return 0;
	}
	return -1;
}

