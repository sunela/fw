/*
 * fake-rmt.c - Remote control protocol transported over Unix-domain sockets
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */


#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/uio.h>

#include "fake-rmt.h"


static int rmt_s = -1;


int fake_rmt_recv(void *buf, unsigned size)
{
	ssize_t got;

	got = recv(rmt_s, buf, size, 0);
	if (got < 0) {
		perror("recv");
		exit(1);
	}
	return got;
}


void fake_rmt_send(const void *buf, unsigned size)
{
	const uint8_t zero = 0;
	struct iovec iov[] = {
		{
			.iov_base	= (void *) &zero,
			.iov_len	= 1,
		},
		{
			.iov_base	= (void *) buf,
			.iov_len	= size,
		}
	};
	ssize_t sent;

	sent = writev(rmt_s, iov, 2);
	if (sent < 0) {
		perror("sent");
		exit(1);
	}
	if (sent != size + 1) {
		fprintf(stderr, "short send: %u < %u\n",
		    (unsigned) sent, size + 1);
		exit(1);
	}
}


void fake_rmt_open(const char *path)
{
	struct sockaddr_un addr;

	rmt_s = socket(PF_UNIX, SOCK_SEQPACKET, 0);
	if (rmt_s < 0) {
		perror("socket");
		exit(1);
	}
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, path);

        if (connect(rmt_s, (const struct sockaddr *) &addr, sizeof(addr)) < 0) {
                perror(addr.sun_path);
                exit(1);
        }
}
