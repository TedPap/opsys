#include "tinyos.h"
#include "kernel_cc.h"
#include "kernel_streams.h"
#include "kernel_sched.h"
#include "kernel_pipe.h"
#include "kernel_dev.h"

/** Aquire space for the pipe_control_block */
void* pipe_init() {

	void* ptr = xmalloc(sizeof(PICB));
	return ptr;
}


void* pipe_open(uint minor)
{
  return NULL;
}


/** Read size amount of bytes from the pipe buffer to the buf */
int pipe_read(void* ptr, char *buf, unsigned int size)
{
	int i = 0;
	bool err = false;
	PICB* picb = (PICB*) ptr;

	Mutex_Lock(& picb->spinlock);

	/** Error check */
	if (picb->pfcb[1]->refcount == 0) {
		if (picb->balance <= 0)
			err = true;
	}
	
	/** While buffer doesn't have enough bytes to read, wait on picb's cv */
	while (picb->balance < size && picb->pfcb[1]->refcount != 0)
    	Cond_Wait(& picb->spinlock, & picb->buff_full);

    /** Read size amount of bytes into buf */
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

/** Write size amount of bytes from buf to pipe's buffer. */
int pipe_write(void* ptr, const char* buf, unsigned int size)
{
	int i = 0;
	bool err = false;
	PICB* picb = (PICB*) ptr;

	/** Error check */
	if (picb->pfcb[0]->refcount == 0)
		err = true;

	/** When buffer is full signal reader */
	if (picb->balance >= buffer_size) {
		Cond_Broadcast(& picb->buff_full);
	}

	/** Write the bytes */
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


/** Free the space of the picb */
int pipe_close(void* ptr) 
{
	free(ptr);
  	return 0;
}


file_ops pipe_fops = {
  .Open = pipe_open,
  .Read = pipe_read,
  .Write = pipe_write,
  .Close = pipe_close
};


/** Contstruct the picb */
int Pipe(pipe_t* pipe)
{
	Fid_t Fids[2];
	bool err = false;
	PICB* picb;


  	Mutex_Lock(& kernel_mutex);

	picb = (PICB*) pipe_init();

	/** If there are enough fids in the proc's table proceed */
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

