#include "stun.h"

int init_stun_header(struct stun_header **sh)
{
    *sh = calloc(1, sizeof(struct stun_header));
    (*sh)->transaction_id = calloc(TRANSACTION_ID_SIZE / 4, sizeof(uint32_t));
    if (*sh == NULL || (*sh)->transaction_id == NULL)
    {
        return 0;
    }
    return 1;
}

int init_xor_map_attr(struct stun_attr **xma)
{

    *xma = calloc(1, sizeof(struct stun_attr));
    (*xma)->value = calloc(sizeof(uint32_t) * 2, sizeof(uint32_t)); //ipv4 (port + family) 4byte + ip4addr 4byte

    if (*xma == NULL || (*xma)->value == NULL)
    {
        return 0;
    }
    return 1;
}

int init_client_info(struct client_info **ci)
{
    *ci = calloc(1, sizeof(struct client_info));
    if (*ci == NULL)
        return 0;
    return 1;
}

void fill_client_info(struct client_info *ci, struct sockaddr_in const *peer_addr)
{
    ci->family = IPV4_FAMILY;
    ci->port = ntohs(peer_addr->sin_port);
    ci->ip_addr = ntohl(peer_addr->sin_addr.s_addr);
}

int is_valid_stun_header(struct stun_header const *sh)
{

    if (!((sh->message_type & 0xC000) == 0) )
        return 0;
    if (!(sh->magic_cookie == MAGIC_COOKIE))
        return 0;

    return 1;
}

void process_request(uint8_t *buf, struct stun_header const *sh, struct client_info const *ci, struct stun_header *success_header, struct stun_attr *sa)
{
    

    switch (sh->message_type)
    {
    case BINDING_REQUEST:

        form_success_header(success_header, sh->transaction_id);
        form_xmap_attr(sa, ci);

        pack_stun_message(buf, success_header, sa);
        
        break;

    default: //discard request if message type is other than binding
        break;
    }
}

void form_success_header(struct stun_header *success_header, uint32_t const *t_id)
{
    success_header->message_type = BINDING_SUCESS_RESPONSE;
    success_header->magic_cookie = MAGIC_COOKIE;
    success_header->transaction_id = (uint32_t *)t_id;
    success_header->message_length = TRANSACTION_ID_SIZE;

}

void form_xmap_attr(struct stun_attr *xmap, struct client_info const *ci) {

    xmap->type = XOR_MAPPED_ADDRESS_TYPE;
    xmap->length = 8; //ipv4 (port + family) 4byte + ip4addr 4byte
    xor_map_attr_value_encode(xmap->value, ci);

}

void xor_map_attr_value_encode(uint32_t *value, struct client_info const *ci)
{
    value[0] = (ci->family << 16) | ((ci->port) ^ (MAGIC_COOKIE >> 16));
    value[1] = ci->ip_addr ^ MAGIC_COOKIE;
}

void print_byte(uint8_t* buf, int n){
	for (int i = 0; i < n; i++){
		printf("%02x ", buf[i]);
	}
	printf("\n");
}

/*
Pack a stun message containing a stun header and a stun attribute in a 8bit buffer
*/
void pack_stun_message(uint8_t *buf, struct stun_header const *sh, struct stun_attr const *sa) {
    

    pack_u16(buf, &sh->message_type);
    pack_u16(buf + 2,  &sh->message_length);
    pack_u32(buf + 4,  &sh->magic_cookie);
    pack_uvlength(buf + 8, sh->transaction_id, 3);

    pack_u16(buf + 20,  &sa->type);
    pack_u16(buf + 22,  &sa->length);
    pack_uvlength(buf + 24, sa->value, 2);



}

/* 
Pack a 16bit integer in a 8bit buffer 
*/
void pack_u16(uint8_t *buf, uint16_t const *value){
    buf[0] = *value >> 8; 
    buf[1] = *value; 

}

/* 
Pack a 32bit integer in a 8bit buffer 
*/
void pack_u32(uint8_t *buf, uint32_t const *value){
    buf[0] = *value >> 24; 
    buf[1] = *value >> 16; 
    buf[2] = *value >> 8; 
    buf[3] = *value; 


}

/* 
Pack multiple 32bit integer in a 8bit buffer 
*/
void pack_uvlength(uint8_t *buf, uint32_t const *value, int len) {
    for (int i = 0; i < len; i++) {
        pack_u32(buf + i * 4, &value[i]);
    }

}


void parse_stun_header(struct stun_header *sh, uint8_t *byte_data)
{

    int offset = 0;

    parse_u16(byte_data, &sh->message_type);
    parse_u16(byte_data + 2, &sh->message_length);
    parse_u32(byte_data + 4, &sh->magic_cookie);
    parse_u32_vlength(byte_data + 8, sh->transaction_id, 3);
}

void parse_message_method(uint16_t message_type)
{
}
void parse_message_class();

void parse_u16(uint8_t *data, uint16_t *attr)
{
    *attr = (data[0] << 8) | data[1];
}

void parse_u32(uint8_t *data, uint32_t *attr)
{
    *attr = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
}

void parse_u32_vlength(uint8_t *data, uint32_t *attr, int len)
{

    for (int i = 0; i < len; i++)
    {
        parse_u32(data + i * 4, &attr[i]);
    }
}

void free_stun_header(struct stun_header *sh)
{
    free(sh->transaction_id);
    free(sh);
}

void free_stun_attr(struct stun_attr *sa) {
    free(sa->value);
    free(sa);
}

