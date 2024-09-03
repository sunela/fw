/*
 * fake-rmt.h - Remote control protocol transported over Unix-domain sockets
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef FAKE_RMT_H
#define	FAKE_FMT_H

void fake_rmt_poll(void);
void fake_rmt_init(const char *path);

#endif /* !FAKE_FMT_H */
