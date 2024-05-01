/*
 * demo.h - Demos/tests
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef DEMO_H
#define	DEMO_H

#include "mbox.h"


extern struct mbox demo_mbox;


void demo(char **args, unsigned n_args);
void poll_demo_mbox(void);
void demo_init(void);

#endif /* !DEMO_H */
