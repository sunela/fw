/*
 * fake-rmt.c - Remote control protocol transported over Unix-domain sockets
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */


#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "sdk-hal.h"
#include "../sdk/sdk-usb.h"
#include "rmt.h"
#include "fake-rmt.h"

static int listen_s = -1;
static int rmt_s = -1;


static int rmt_socket(void)
{
	int s;

	s = accept(listen_s, NULL, NULL);
	if (s < 0) {
		if (errno == EAGAIN)
			return -1;
		perror("accept");
		exit(1);
	}
	if (fcntl(s, F_SETFL, O_NONBLOCK) < 0) {
		perror("F_SETFL(O_NONBLOCK)");
		exit(1);
	}
	return s;
}


static int listen_socket(const char *path)
{
	struct sockaddr_un addr;
	int s;

	s = socket(PF_UNIX, SOCK_SEQPACKET, 0);
	if (s < 0) {
		perror("socket");
		exit(1);
	}
	if (fcntl(s, F_SETFL, O_NONBLOCK) < 0) {
		perror("F_SETFL(O_NONBLOCK)");
		exit(1);
	}

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, path);

	unlink(path);
	if (bind(s, (const struct sockaddr *) &addr, sizeof(addr)) < 0) {
		perror(addr.sun_path);
		exit(1);
	}
	if (listen(s, 5) < 0) {
		perror("listen");
		exit(1);
	}
	return s;
}


static void receive(void)
{
	static uint8_t buf[256 + 1];
	static ssize_t got = -1;

	/*
	 * If USB cannot deliver a request, it simply stalls and the host tries
	 * again. With SEQPACKET, we would have to tell the host somehow.
	 * Instead, we just buffer any undelivered messages until our stack is
	 * ready.
	 */
	if (got < 0) {
		got = recv(rmt_s, buf, sizeof(buf), MSG_DONTWAIT);
		if (got < 0) {
			if (errno == EAGAIN)
				return;
			perror("recv");
			exit(1);
		}
		if (!got) { /* EOF */
			close(rmt_s);
			rmt_s = -1;
			got = -1;
			return;
		}
	}
	if (usb_arrival(SUNELA_RMT, buf + 1, got - 1))
		got = -1;
	else
		fprintf(stderr, "usb_arrival: refused\n");
}


static void poll_send(void)
{
	uint8_t *data;
	uint32_t len;
	ssize_t sent;

	if (!rmt_query(&rmt_usb, &data, &len))
		return;
	sent = send(rmt_s, data, len, 0);
	if (sent < 0) {
		perror("send");
		exit(1);
	}
	if (sent != len)
		fprintf(stderr, "short send: %u < %u\n",
		    (unsigned) sent, (unsigned) len);
}


void fake_rmt_poll(void)
{
	if (rmt_s < 0)
		rmt_s = rmt_socket();
	if (rmt_s < 0)
		return;
	receive();
	poll_send();
}


void fake_rmt_init(const char *path)
{
	listen_s = listen_socket(path);
	rmt_s = rmt_socket();
}
