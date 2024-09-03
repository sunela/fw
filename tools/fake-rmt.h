/*
 * fake-rmt.h - Remote control protocol transported over Unix-domain sockets
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef FAKE_RMT_H
#define	FAKE_RMT_H

int fake_rmt_recv(void *buf, unsigned size);
void fake_rmt_send(const void *buf, unsigned size);
void fake_rmt_open(const char *path);

#endif /* !FAKE_RMT_H */
