/*
 * script.c - Functions for scripted tests and other debugging
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#define	_GNU_SOURCE	/* for vasprintf */
#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hal.h"
#include "rnd.h"
#include "timer.h"
#include "db.h"
#include "ui.h"
#include "sim.h"
#include "script.h"


/* --- Debugging dump ------------------------------------------------------ */


void dump_db(const struct db *db, bool pointers)
{
	const struct db_entry *de;
	const struct db_field *f;

	for (de = db->entries; de; de = de->next) {
		if (pointers)
			printf("%p ", de);
		printf("%s@%u:0x%04x ", de->name, de->block, de->seq);
		printf("%s", de->db == db ? "db" : "DB MISMATCH");
		if (pointers)
			printf("%p\n", de->db);
		else
			printf("\n");
		for (f = de->fields; f; f = f->next) {
			printf("\t");
			if (pointers)
				printf("%p ", f);
			switch (f->type) {
			case ft_id:
				printf("id");
				break;
			case ft_prev:
				printf("prev");
				break;
			case ft_user:
				printf("user");
				break;
			case ft_email:
				printf("email");
				break;
			case ft_pw:
				printf("pw");
				break;
			case ft_hotp_secret:
				printf("hotp_secret");
				break;
			case ft_hotp_counter:
				printf("hotp_counter");
				break;
			case ft_totp_secret:
				printf("totp_secret");
				break;
			case ft_comment:
				printf("Comment");
				break;
			default:
				printf("%u", f->type);
				break;
			}
			if (pointers)
				printf(" %p+", f->data);
			else
				printf(" ");
			printf("%u", f->len);
			if (f->data) {
				const uint8_t *s;

				for (s = f->data; s != f->data + f->len; s++)
					if (*s < 32 || *s > 126)
						break;
				if (s == f->data + f->len) {
					printf(" \"%.*s\"",
					    f->len, (const char *) f->data);
				} else {
					for (s = f->data;
					    s != f->data + f->len; s++)
						printf(" %02x", *s);
				}
			}
			printf("\n");
		}
	}
}


/* --- Screen dump --------------------------------------------------------- */


static bool write_ppm(const struct gfx_drawable *da, const char *name)
{
	FILE *file;
	const gfx_color *p;

	file = fopen(name, "wb");
	if (!file)
		return 0;
	if (fprintf(file, "P6\n%u %u\n%u\n", da->w, da->h, 255) < 0)
		return 0;
	for (p = da->fb; p != da->fb + da->h * da->w; p++) {
		uint8_t rgb[3] = {
			gfx_decode_r(*p),
			gfx_decode_g(*p),
			gfx_decode_b(*p)
		};

		if (fwrite(rgb, 1, 3, file) != 3)
			return 0;
	}
	if (fclose(file) < 0)
		return 0;
	return 1;
}


bool screenshot(const struct gfx_drawable *da, const char *fmt, ...)
{
	char *s = (char *) fmt;
	bool ok;

	if (strchr(fmt, '%')) {
		va_list ap;

		va_start(ap, fmt);
		if (vasprintf(&s, fmt, ap) < 0) {
			perror(fmt);
			return 0;
		}
		va_end(ap);
	}
	ok = write_ppm(da, s);
	if (!ok)
		perror(s);
	if (s != fmt)
		free(s);
	return ok;
}


/* --- Scripting actions --------------------------------------------------- */


static void ticks(unsigned n)
{
	static unsigned uptime = 1;
	unsigned i;

	for (i = 0; i != n; i++) {
		timer_tick(uptime);
		tick_event();
		uptime += 10;
	}
}


static void show_help(void)
{
	printf("Commands:\n\n"
"echo MESSAGE\tdisplay a message, can contain spaces\n"
"system COMMAND\trun a shell command\n"
"interact\tshow the display and interact with the user\n"
"random\t\tenable random number generation\n"
"random BYTE\t\tset random number generator to fixed value\n"
"screen\t\ttake a screenshot (PPM)\n"
"screen PPMFILE\ttake a screenshot in a specific file, remember the name\n"
"press\t\tpress the side button\n"
"release\t\trelease the side button\n"
"button\t\tpress the button long enough to debounce, then release it\n"
"down X Y\ttouch the touch screen\n"
"move X Y\tmove on the touch screen\n"
"up\t\tstop touching the touch screen\n"
"tap X Y\ttap the touch screen\n"
"long X Y\tlong press the touch screen\n"
"drag X0 Y0 X1 Y1\tdrag gesture\n"
"tick\t\tgenerate one timer tick\n"
"tick N\t\tgenerate N timer ticks\n" 
"time UNIX-TIME\tset the system time (and hold it until changed)\n" 
"help\t\tthis help text\n");
}


static const char *cmd_arg(const char *c, const char *arg)
{
	int len = strlen(c);

	if (strncmp(c, arg, len))
		return NULL;
	if (arg[len] != ' ')
		return NULL;
	return arg + len + 1;
}


static bool process_cmd(const char *cmd)
{
	const char *arg;
	unsigned x, y;
	unsigned n;

	/* system interaction */

	arg = cmd_arg("echo", cmd);
	if (arg) {
		printf("%s\n", arg);
		return 1;
	}
	arg = cmd_arg("system", cmd);
	if (arg) {
		system(arg);
		return 1;
	}
	if (!strcmp("interact", cmd)) {
		headless = 0;
		return 1;
	}
	if (!strcmp("random", cmd)) {
		rnd_fixed = 0;
		return 1;
	}
	arg = cmd_arg("random", cmd);
	if (arg) {
		char *end;

		rnd_fixed = strtoul(arg, &end, 0);
		return !*end;
	}

	/* screenshots */

	if (!strcmp("screen", cmd)) {
		if (!screenshot(&main_da, screenshot_name, screenshot_number))
			return 0;
		screenshot_number++;
		return 1;
	}
	arg = cmd_arg("screen", cmd);
	if (arg) {
		screenshot_name = arg;
		if (!screenshot(&main_da, screenshot_name, screenshot_number))
			return 0;
		screenshot_number++;
		return 1;
	}

	/* button: low-level */

	if (!strcmp("press", cmd)) {
		button_event(1);
		return 1;
	}
	if (!strcmp("release", cmd)) {
		button_event(0);
		return 1;
	}

	/* button: high-level */

	if (!strcmp("button", cmd)) {
		button_event(1);
		ticks(2);
		button_event(0);
		return 1;
	}

	/* touch screen: low-level */

	arg = cmd_arg("down", cmd);
	if (arg) {
		if (sscanf(arg, "%u %u", &x, &y) != 2)
			goto fail;
		touch_down_event(x, y);
		return 1;
	}
	arg = cmd_arg("move", cmd);
	if (arg) {
		if (sscanf(arg, "%u %u", &x, &y) != 2)
			goto fail;
		touch_move_event(x, y);
		return 1;
	}
	if (!strcmp("up", cmd)) {
		touch_up_event();
		return 1;
	}

	/* touch screen: high-level */

	arg = cmd_arg("tap", cmd);
	if (arg) {
		if (sscanf(arg, "%u %u", &x, &y) != 2)
			goto fail;
		touch_down_event(x, y);
		touch_up_event();
		return 1;
	}
	arg = cmd_arg("long", cmd);
	if (arg) {
		if (sscanf(arg, "%u %u", &x, &y) != 2)
			goto fail;
		touch_down_event(x, y);
		ticks(50);
		touch_up_event();
		return 1;
	}
	arg = cmd_arg("drag", cmd);
	if (arg) {
		unsigned x0, y0, x1, y1;

		if (sscanf(arg, "%u %u %u %u", &x0, &y0, &x1, &y1) != 4)
			goto fail;
		touch_down_event(x0, y0);
		touch_move_event(x1, y1);
		touch_up_event();
		return 1;
	}

	/* timer ticks */

	if (!strcmp("tick", cmd)) {
		ticks(1);
		return 1;
	}
	arg = cmd_arg("tick", cmd);
	if (arg) {
		if (sscanf(arg, "%u", &n) != 1)
			goto fail;
		ticks(n);
		return 1;
	}

	/* time override */

	arg = cmd_arg("time", cmd);
	if (arg) {
		unsigned long long t;

		if (sscanf(arg, "%llu", &t) != 1)
			goto fail;
		time_override = t;
		return 1;
	}

	/* help */

	if (!strcmp("help", cmd)) {
		show_help();
		return 1;
	}

fail:
	fprintf(stderr, "bad command \"%s\n", cmd);
	return 0;
}


bool run_script(char **args, int n_args)
{
	db_open(&main_db, NULL);
	while (n_args-- && headless)
		if (!process_cmd(*args++))
			return 0;
	return 1;
}
