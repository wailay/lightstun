#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <poll.h>
#include <errno.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include "debug.h"
#include "stun.h"

#define MAX_EVENTS 1

typedef struct server {
	int tcp_listener;
	int udp_listener;
} server;


int handleNonBlocking(){
	if (errno == EAGAIN || errno == EWOULDBLOCK) {
		return 1;
	}
	return 0;
}

void handleError(int errnum, const char *msg) {
	if (handleNonBlocking()){
		return;
	}
	else if(errnum < 0) {
		perror(msg);
		exit(EXIT_FAILURE);
	}
}


void print_32(uint32_t* buf, int n){
	for (int i = 0; i < n; i++){
		printf("%08x ", buf[i]);
	}
	printf("\n");
}

///////////////////////////////////////////
// STUN FUNCTIONS
//////////////////////////////////////////



/////////////////////////////////////////

/*
   struct addrinfo {
        int              ai_flags;     // AI_PASSIVE, AI_CANONNAME, etc.
        int              ai_family;    // AF_INET, AF_INET6, AF_UNSPEC
        int              ai_socktype;  // SOCK_STREAM, SOCK_DGRAM
        int              ai_protocol;  // use 0 for "any"
        size_t           ai_addrlen;   // size of ai_addr in bytes
        struct sockaddr *ai_addr;      // struct sockaddr_in or _in6
        char            *ai_canonname; // full canonical hostname
    
        struct addrinfo *ai_next;      // linked list, next node
    };
*/ 

/*
returns a socket file descriptor ipv4 connection on TCP/PORT 
*/
void init_server(const char* PORT, struct server *s){

	struct addrinfo hints, *res, *p;
	int status;
	memset(&hints, 0, sizeof(struct addrinfo));
	
	hints.ai_family = AF_INET;
	hints.ai_socktype = 0;
	hints.ai_flags = AI_PASSIVE;

	status = getaddrinfo(NULL, PORT, &hints, &res);
	handleError(status, "getaddrinfo");

	//get the available socket fd
	int sfd;
	for (p = res; p != NULL; p = p->ai_next){
		
		if (p->ai_socktype != SOCK_DGRAM && p->ai_socktype != SOCK_STREAM) break;

		sfd = socket(p->ai_family, p->ai_socktype | SOCK_NONBLOCK, p->ai_protocol);
		if (sfd < 0) {
			perror("socket");
			continue;
		}
				
		status = setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
		handleError(status, "sockopt reuse");

		status = bind(sfd, p->ai_addr, p->ai_addrlen);
		handleError(status, "bind");

		
		if (p->ai_socktype == SOCK_STREAM) { //TCP socket
			
			status = listen(sfd, SOMAXCONN);
			handleError(status, "listen");

			s->tcp_listener = sfd;
		} 
		else if (p->ai_socktype == SOCK_DGRAM) { //UDP socket 
		
			s->udp_listener = sfd;
		}
	}

	freeaddrinfo(res);

}

void freeserver(struct server *s){
	close(s->udp_listener);
	close(s->tcp_listener);
	free(s);
}

void add_socket_to_epoll(int epollfd, struct epoll_event *ev, int sfd, int flags){
	
	ev->events = EPOLLIN | flags;
	ev->data.fd = sfd;
	int status = epoll_ctl(epollfd, EPOLL_CTL_ADD, sfd, ev);
	handleError(status, "epoll_ctl");

}
void setnonblocking(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	handleError(flags, "fcntl get flags");
	flags = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	handleError(flags, "fcntl set nonblock");
}

int main() {
	
	server *serverinfo = malloc(1 * sizeof(server));
	const char* port = "6677";
	init_server(port, serverinfo);

	int new_conn;

	struct sockaddr_in remote_conn;
	memset(&remote_conn, 0, sizeof(remote_conn));
	socklen_t remote_conn_len = sizeof(remote_conn);

	struct epoll_event epoll_ev, events[MAX_EVENTS];
	int epollfd, status, ready_fds;
	epollfd = epoll_create(1);
	handleError(epollfd, "epoll_create");

	add_socket_to_epoll(epollfd, &epoll_ev, serverinfo->tcp_listener, 0);
	add_socket_to_epoll(epollfd, &epoll_ev, serverinfo->udp_listener, 0);

	struct stun_header *sh;			
	init_stun_header(&sh);

	struct client_info *client_info;
	init_client_info(&client_info);

	struct stun_header *success_header;
	init_stun_header(&success_header);

    struct stun_attr *xor_map_attr;
	init_xor_map_attr(&xor_map_attr);
	
	uint8_t *header_buffer = calloc(STUN_HEADER_SIZE, sizeof(uint8_t));
	size_t message_len = STUN_HEADER_SIZE + XOR_MAP_IPV4_ATTR_SIZE;
	uint8_t *success_buffer = calloc(message_len, sizeof(uint8_t)); 

	for(;;){

		ready_fds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
		handleError(ready_fds, "epoll_wait");


		for (int i = 0; i < ready_fds; i++){

	
			if (events[i].data.fd == serverinfo->tcp_listener) {
				LOG("got tcp connection");
				
				new_conn = accept(events[i].data.fd , (struct sockaddr*) &remote_conn, &remote_conn_len);
				
				setnonblocking(new_conn); //in case client sends no data, do not block on read.
				add_socket_to_epoll(epollfd, &epoll_ev, new_conn, EPOLLET);
				
				
			} else if (events[i].data.fd == serverinfo->udp_listener){ // udp connection
				
				LOG("got udp connection");
				status = recvfrom(events[i].data.fd, header_buffer, STUN_HEADER_SIZE, 0, (struct sockaddr*) &remote_conn, &remote_conn_len);
				
				fill_client_info(client_info, &remote_conn);
				parse_stun_header(sh, header_buffer);
				if (!is_valid_stun_header(sh)) continue;
				process_request(success_buffer, sh, client_info, success_header, xor_map_attr);
		
				status = sendto(events[i].data.fd, success_buffer, message_len, 0, (struct sockaddr*) &remote_conn, remote_conn_len);
			
				
			} else { //data from tcp connection 

				char buf[5000];
				bzero(&buf, 5000);
				status = recv(events[i].data.fd, buf, 5000, 0);
				handleError(status, "recv");

				// check_stun_header();
				// send_stun_request();
				// close_connection();

				// check if client has sent a stun packet
				// send client xor map address back
				char str[INET_ADDRSTRLEN];
				inet_ntop(remote_conn.sin_family, &remote_conn.sin_addr, str, INET_ADDRSTRLEN);
				printf("ip from %s\n", str);
				
				status = send(events[i].data.fd, "response from server!\n", strlen("response from server!\n"), MSG_NOSIGNAL);
				
				if (status < 0) {
					if(errno == EPIPE) {
						printf("closing connection\n");
						close(events[i].data.fd);
					}
				}

			}
		}
	}

	free(&epoll_ev);
	freeserver(serverinfo);
	
	return 0;    
}
