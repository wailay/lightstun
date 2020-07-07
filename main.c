#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include "debug.h"
#include "stun.h"
#include "ossl.h"

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



/////////////////////////////////////////

/*
fill server struct with TCP and UDP socket file descriptor
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

int main(int argc, char **argv) {
	
	char* port = "3478";

	if (argc > 1) {
		port = argv[1];
	} 

	printf("listening on port %s\n", port);
	server *serverinfo = malloc(1 * sizeof(server));
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
				LOG("%s\n", "got tcp connection");

				new_conn = accept(events[i].data.fd , (struct sockaddr*) &remote_conn, &remote_conn_len);
				handleError(new_conn, "accept tcp");
				setnonblocking(new_conn); //in case client sends no data, do not block on read.
				
				
				add_socket_to_epoll(epollfd, &epoll_ev, new_conn, 0);
				
				
				
			} else if (events[i].data.fd == serverinfo->udp_listener){ // udp connection
				
				LOG("%s\n", "got udp connection");
				bzero(header_buffer, STUN_HEADER_SIZE);

				status = recvfrom(events[i].data.fd, header_buffer, STUN_HEADER_SIZE, 0, (struct sockaddr*) &remote_conn, &remote_conn_len);
			

				fill_client_info(client_info, &remote_conn);
				parse_stun_header(sh, header_buffer);
				if (!is_valid_stun_header(sh)) continue;
				process_request(success_buffer, sh, client_info, success_header, xor_map_attr);

				status = sendto(events[i].data.fd, success_buffer, message_len, 0, (struct sockaddr*) &remote_conn, remote_conn_len);

				
			} else { //data from tcp connection 
				LOG("%s\n", "got tcp data");

				bzero(header_buffer, STUN_HEADER_SIZE);
				status = recv(events[i].data.fd, header_buffer, STUN_HEADER_SIZE, 0);
				handleError(status, "recv");

				fill_client_info(client_info, &remote_conn);
				parse_stun_header(sh, header_buffer);
				if (!is_valid_stun_header(sh)) continue;
				process_request(success_buffer, sh, client_info, success_header, xor_map_attr);
				
				status = send(events[i].data.fd, success_buffer, message_len, MSG_NOSIGNAL);

				if (status < 0) {
					if(errno == EPIPE) {
						close(events[i].data.fd);
					}
				}

				close(events[i].data.fd);
			}
		}
	}

	free(&epoll_ev);
	freeserver(serverinfo);
	free_stun_header(sh);
	free_stun_header(success_header);
	free_stun_attr(xor_map_attr);
	free(&success_buffer);
	free(&header_buffer);

	return 0;    
}
