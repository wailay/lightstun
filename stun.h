#ifndef STUN_H
#define STUN_H

#include <stdint.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>


#define MAGIC_COOKIE 0x2112A442

#define STUN_HEADER_SIZE 20
#define TRANSACTION_ID_SIZE 12
#define BINDING_REQUEST 0x0001
#define BINDING_SUCESS_RESPONSE 0x0101
#define BINDING_ERROR_RESPONSE 0x0111

#define MAPPED_ADDRESS_TYPE 0x0001
#define XOR_MAPPED_ADDRESS_TYPE 0x0020
#define IPV4_FAMILY 0x01
#define XOR_MAP_IPV4_ATTR_SIZE 12

struct stun_header {
	uint16_t message_type;
	uint16_t message_length;
	uint32_t magic_cookie;
	uint32_t *transaction_id;
};

struct stun_attr {
	uint16_t type;
	uint16_t length;
	uint32_t *value;
};

struct client_info {
	uint16_t family;
	uint16_t port;
	uint32_t ip_addr;
};

int init_stun_header(struct stun_header **sh);
int init_xor_map_attr(struct stun_attr **xma);

int init_client_info(struct client_info **ci);
void fill_client_info(struct client_info *ci, struct sockaddr_in const *peer_addr);

int is_valid_stun_header(struct stun_header const *sh);
void parse_stun_header(struct stun_header *sh, uint8_t* byte_data);

void parse_u16(uint8_t *data, uint16_t *attr);
void parse_u32(uint8_t *data, uint32_t *attr);
void parse_u32_vlength(uint8_t *data, uint32_t *attr, int len);

void process_request(uint8_t *buf, struct stun_header const *sh, struct client_info const *ci, struct stun_header *success_header, struct stun_attr *sa);
void form_success_header(struct stun_header *success_header, uint32_t const *t_id);
void form_xmap_attr(struct stun_attr *xmap, struct client_info const *ci);
void xor_map_attr_value_encode( uint32_t *value, struct client_info const *ci);

void pack_stun_message(uint8_t *buf, struct stun_header const *sh, struct stun_attr const *sa);
void pack_u16(uint8_t *buf, uint16_t const *value);
void pack_u32(uint8_t *buf, uint32_t const *value);
void pack_uvlength(uint8_t *buf, uint32_t const *value, int len);

void free_stun_header(struct stun_header *sh);
void free_stun_attr(struct stun_attr *sa);

void print_byte(uint8_t* buf, int n);

#endif
