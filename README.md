# lightstun

Implementation of a basic stand-alone STUN server like described in [Section 12](https://tools.ietf.org/html/rfc8489#section-12) from [RFC8489](https://tools.ietf.org/html/rfc8489) 

Supports UDP/TCP and IPV4 

Not currently supporting IPV6, STUN will not be really needed when IPV6 becomes the standard.

This is an attempt at learning C Networking. Using `epoll` for TCP and UDP multiplexing.

The code is not really portable, will work only on Linux for now.


TODOS:
 
 * Implement TLS over TCP - read more about dtls
 * Better error handling and error response.
