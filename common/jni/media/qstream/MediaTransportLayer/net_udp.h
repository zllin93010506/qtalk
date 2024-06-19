/*
 * FILE:    net_udp.h
 * AUTHORS: Colin Perkins
 * 
 * Copyright (c) 1998-2000 University College London
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, is permitted provided that the following conditions 
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the Computer Science
 *      Department at University College London
 * 4. Neither the name of the University nor of the Department may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * $Revision: 1.4 $
 * $Date: 2005/06/05 23:05:13 $
 */

#ifndef _NET_UDP
#define _NET_UDP

#include <sys/socket.h>

#define	UDP_SOCKET_OPTION_TOS	0

typedef struct _socket_udp socket_udp; 

typedef struct msg_ctrl {
        struct cmsghdr cm;
        char control[512];
}msg_ctrl_t; 

int         udp_addr_valid(const char *addr);
socket_udp *udp_init(const char *addr, uint16_t rx_port, uint16_t tx_port, int ttl, int loop, const char* local_addr, int tos);
socket_udp *udp_init_if(const char *addr, const char *iface, uint16_t rx_port, uint16_t tx_port, int ttl, int loop, const char* local_addr, int tos);
void        udp_exit(socket_udp *s);

int         udp_peek(socket_udp *s, uint8_t *buffer, int buflen);
int         udp_recv(socket_udp *s, uint8_t *buffer, int buflen);
int         udp_send(socket_udp *s, uint8_t *buffer, int buflen);

//int         udp_recvv(socket_udp *s, struct msghdr *m);
int         udp_sendv(socket_udp *s, struct iovec *vector, int count);

const char *udp_host_addr(socket_udp *s);
int         udp_fd(socket_udp *s);

int         udp_select(struct timeval *timeout);
void	    udp_fd_zero(void);
void        udp_fd_set(socket_udp *s);
int         udp_fd_isset(socket_udp *s);


/* set socket option, added by willy, 2010-11-10 */
int	    udp_set_socket_option(int option_name, const void *option_value);

int     udp_get_socket_send_buf_size(int sockfd);



void udp_msg_init(struct msghdr *msg, struct iovec *entry, struct sockaddr_in *from_addr, struct msg_ctrl *control, uint8_t *buffer, int buflen);
struct timeval udp_recvtime_obtain(struct msghdr *msg);

#endif

