#include "util.h"
#include "bios.h"
#include "tinyos.h"

typedef enum socket_type_e {
	UNBOUND,
	LISTENER,
	PEER
} socket_type;

typedef struct listener_control_block LCB;
typedef struct peer_control_block PECB;

typedef struct request_control_block {
	rlnode node;
	LCB* lcb;
	CondVar cv;
} RCB;

typedef struct listener_control_block {
	CondVar cv;
	rlnode request_list;
} LCB;

typedef struct peer_control_block {
	PECB* peer;
	Fid_t pipe_send, pipe_receive;
} PECB;

typedef struct socket_control_block {
	socket_type type;
	port_t portnum;
	int refcount;
	union {
		LCB listener;
		PECB peer;
	};
} SCB;

Fid_t Socket(port_t port);

int Listen(Fid_t sock);


Fid_t Accept(Fid_t lsock);


int Connect(Fid_t sock, port_t port, timeout_t timeout);


int ShutDown(Fid_t sock, shutdown_mode how);