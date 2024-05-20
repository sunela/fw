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

#include "db.h"
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
