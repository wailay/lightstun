# lightstun

Implementation of a basic stand-alone STUN server like described in [Section 12](https://tools.ietf.org/html/rfc8489#section-12) from [RFC8489](https://tools.ietf.org/html/rfc8489) 

Supports UDP/TCP and IPV4 

This is an attempt at learning C Networking. Using `epoll` for TCP and UDP multiplexing.

The code is not really portable, will work only on Linux for now.


TODOS:
 
 * implement tls and dtls.
 * better error handling and logging.
 * automate build process.
