#include "tinyos.h"

#define buffer_size 16384

typedef struct Pipe_control_block {
	pipe_t* pipe;				/**< The pipe file ids */
	FCB* pfcb[2];
	file_ops pipe_ops;			/**< The file_ops of pipes */
	char puffer[buffer_size];	/**< The pipe buffer */
	int balance;				/**< Point to where we have writen/read this far */
	Mutex spinlock;				
  	CondVar buff_full;			/**< Condition variable of pipe */
}PICB;
/**
	@brief Construct and return a pipe.

	A pipe is a one-directional buffer accessed via two file ids,
	one for each end of the buffer. The size of the buffer is 
	implementation-specific, but can be assumed to be between 4 and 16 
	kbytes. 

	Once a pipe is constructed, it remains operational as long as both
	ends are open. If the read end is closed, the write end becomes 
	unusable: calls on @c Write to it return error. On the other hand,
	if the write end is closed, the read end continues to operate until
	the buffer is empty, at which point calls to @c Read return 0.

	@param pipe a pointer to a pipe_t structure for storing the file ids.
	@returns 0 on success, or -1 on error. Possible reasons for error:
		- the available file ids for the process are exhausted.
*/
int Pipe(pipe_t* pipe);


void* pipe_open(uint minor);


int pipe_read(void* picb, char* buf, unsigned int size);


int pipe_write(void* picb, const char* buf, unsigned int size);


int pipe_close(void* picb);
