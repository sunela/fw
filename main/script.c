/*
 * script.c - Functions for scripted tests and other debugging
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */


#include <stdbool.h>
#include <stdio.h>

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
