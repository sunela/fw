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
#include <ctype.h>

#include "hal.h"
#include "rnd.h"
#include "timer.h"
#include "sha.h"
#include "bip39enc.h"
#include "bip39in.h"
#include "bip39dec.h"
#include "block.h"
#include "secrets.h"
#include "dbcrypt.h"
#include "db.h"
#include "rmt.h"
#include "rmt-db.h"
#include "version.h"
#include "ui.h"
#include "sim.h"
#include "script.h"

#include "debug.h"


static struct db_entry *moving = NULL;


/* --- Debugging dump ------------------------------------------------------ */


static void indent(unsigned level)
{
	while (level--)
		putchar('\t');
}


static void dump_entry(const struct db *db, const struct db_entry *de,
    unsigned level, bool pointers)
{
	const struct db_field *f;

	indent(level);
	if (pointers)
		printf("%p ", de);
	printf("%s@%u:0x%04x ", de->name, de->block, de->seq);
	printf("%s", de->db == db ? "db" : "DB MISMATCH");
	if (pointers)
		printf("%p\n", de->db);
	else
		printf("\n");
	for (f = de->fields; f; f = f->next) {
		indent(level);
		printf("    ");
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
		case ft_pw2:
			printf("pw2");
			break;
		case ft_dir:
			printf("dir");
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


static void dump_dir(const struct db *db, const struct db_entry *dir,
    unsigned level, bool pointers)
{
	while (dir) {
		dump_entry(db, dir, level, pointers);
		if (dir->children)
			dump_dir(db, dir->children, level + 1, pointers);
		dir = dir->next;
	}
}


void dump_db(const struct db *db, bool pointers)
{
	dump_dir(db, db->entries, 0, pointers);
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


static struct db_entry *find_entry(const char *name)
{
	struct db_entry *e;

	for (e = main_db.dir ? main_db.dir->children : main_db.entries; e;
	    e = e->next)
		if (!strcmp(e->name, name))
			return e;
	fprintf(stderr, "entry \"%s\" not found\n", name);
	exit(1);
}


static void dump_dir_short(const struct db_entry *dir, unsigned level)
{
	const struct db_field *f;
	unsigned i;

	while (dir) {
		for (i = 0; i != level; i++)
			putchar('\t');
		for (f = dir->fields; f; f = f->next)
			if (f->type == ft_prev)
				break;
		printf("%s %.*s\n", dir->name,
		    f ? f->len : 1,
		    f ? (char *) f->data : "-");
		if (dir->children)
			dump_dir_short(dir->children, level + 1);
		dir = dir->next;
	}
}


static void dump_db_short(const struct db *db)
{
	dump_dir_short(db->entries, 0);
}


static void dump_rmt(const char *s, void *data, unsigned len)
{
	printf("%s %u:", s, len);
	while (len--)
		printf(" %02x", *(uint8_t *) data++);
	putchar('\n');
}


static void dummy_ls(void)
{
	const struct db_entry *e;

	for (e = main_db.dir ? main_db.dir->children : main_db.entries; e;
	    e = e->next)
		printf("%s\n", e->name);
}


static void dummy_pwd(void)
{
	const char *pwd = db_pwd(&main_db);

	printf("%s\n", pwd ? pwd : "(top)");
}


static void dummy_cd(const char *name)
{
	if (name) {
		struct db_entry *e;

		for (e = main_db.dir ? main_db.dir->children : main_db.entries;
		    e; e = e->next)
			if (!strcmp(e->name, name))
				break;
		if (e) {
			db_chdir(&main_db, e);
		} else {
			fprintf(stderr, "\"%s\" not found\n", name);
			exit(1);
		}
	} else {
		db_chdir(&main_db, db_dir_parent(&main_db));
	}
}


static void rmt(const char *s)
{
	uint8_t buf[100]; /* @@@ */
	uint8_t *p = buf;
	char *end;

	while (*s) {
		end = strchr(s, ' ');
		if (!end)
			end = strchr(s, 0);
		if (end - s == 2 && isxdigit(s[0]) && isxdigit(s[1])) {
			*p++ = strtoul(s, NULL, 16);
		} else {
			memcpy(p, s, end - s);
			p += end - s;
		}
		if (!*end)
			break;
		s = end + 1;
	}

	/* send request */

	if (debugging)
		dump_rmt("S", buf, p - buf);
	if (!rmt_arrival(&rmt_usb, buf, p - buf)) {
		fprintf(stderr, "rmt_arrival failed\n");
		exit(1);
	}

	/* send end-of-request */
	do rmt_db_poll();
	while (!rmt_arrival(&rmt_usb, NULL, 0));

	if (debugging)
		dump_rmt("S", NULL, 0);
//fprintf(stderr, "S 0\n");

	/* retrieve response */
	while (1) {
		uint32_t got;
		uint8_t *res;

		rmt_db_poll();
		if (!rmt_query(&rmt_usb, &res, &got))
			continue;
		if (debugging)
			dump_rmt("R", res, got);
		if (!got)
			break;
		if (!*res) {
			printf("Error: ");
			res++;
			got--;
		}

		bool hex = 0;

		for (p = res; p != res + got; p++) {
			if (*p < ' ' || *p > '~') {
				if (p != res)
					putchar(' ');
				printf("%02X", *p);
				hex = 1;
			} else {
				if (hex)
					putchar(' ');
				putchar(*p);
				hex = 0;
			}
		}
		putchar('\n');
	}
}


static bool do_bip39_enc(const char *arg)
{
	struct bip39enc bip;
	uint8_t buf[(strlen(arg) + 1) / 2 + 1];
	unsigned n;

	n = 0;
	while (*arg) {
		char byte[3] = "??";
		unsigned long tmp;
		char *end;

		byte[0] = arg[0];
		byte[1] = arg[1];
		tmp = strtoul(byte, &end, 16);
		if (*end)
			return 0;
		buf[n++] = tmp;
		arg += arg[1] ? 2 : 1;
	}

	bip39_encode(&bip, buf, n);
	n = 0;
	while (1) {
		int i;

		i = bip39_next_word(&bip);
		if (i < 0)
			break;
		printf("%s%s", n ? " " : "", bip39_words[i]);
		n++;
	}
	printf("\n");
	return 1;
}


static bool do_bip39_dec(const char *arg)
{
	uint16_t words[BIP39_MAX_WORDS];
	uint8_t bytes[BIP39_MAX_BYTES];
	unsigned n = 0;
	int got;

	while (*arg) {
		const char *end;
		unsigned i;

		end = strchr(arg, ' ');
		if (!end)
			end = strchr(arg, 0);
		for (i = 0; i != BIP39_WORDS; i++) {
			const char *w = bip39_words[i];

			if (strlen(w) == (unsigned) (end - arg) &&
			    !strncmp(w, arg, end - arg))
				break;
		}
		if (i == BIP39_WORDS) {
			fprintf(stderr, "unrecognized word '%.*s'\n",
			    (int) (end - arg), arg);
			return 0;
		}
		words[n++] = i;
		if (!*end)
			break;
		arg = end + 1;
	}
	got = bip39_decode(bytes, BIP39_MAX_BYTES, words, n);
	switch (got) {
	case BIP39_DECODE_UNRECOGNIZED:
		fprintf(stderr, "unrecognized format\n");
		return 0;
	case BIP39_DECODE_OVERFLOW:
		fprintf(stderr, "output buffer overflow\n");
		return 0;
	case BIP39_DECODE_CHECKSUM:
		fprintf(stderr, "checksum error\n");
		return 0;
	default:
		for (int i = 0; i != got; i++)
			printf("%02x", bytes[i]);
		printf("\n");
		return 1;
	}
}


static void do_bip39_match(const char *arg)
{
	char next[10 + 1];
	unsigned n, i;

	n = bip39_match(arg, next, sizeof(next));
	printf("%u", n);
	for (i = 0; i != n && i != n; i++)
		printf("%s %s", i ? "" : ":", bip39_words[bip39_matches[i]]);
	printf("\n");
	printf("Next \"%s\"\n", next);
}


static void show_help(void)
{
	printf("Commands:\n\n"
"button\t\tpress the button long enough to debounce, then release it\n"
"bip39 decode WORD ...\n\t\tdecode the words to a hex string\n"
"bip39 encode HEXSTRING\n\t\tencode the hex string as words\n"
"bip39 match [KEYS]\n\t\tfind matching words for the key sequence\n"
"db dummy\tuse a dummy database. This must be the first command in the\n"
"\t\tscript.\n"
"db ls\t\tlist the entries of the current directory\n"
"db pwd\t\tshow the name of the current directory (not the whole path)\n"
"db cd [NAME]\tchange to a subdirectory, or, if omitted, go up\n"
"db add NAME [PREV]\n\t\tadd an entry to the dummy database\n"
"db mkdir NAME [PREV]\n\t\tlike \"db add\", but make a directory entry\n"
"db move NAME [BEFORE]\n\t\tmove an entry before another (to the bottom, if\n"
"\t\tBEFORE is omitted)\n"
"db move-from NAME\n\t\tfirst half of \"db move\"\n"
"db move-before [BEFORE]\n\t\tsecond half of \"db move\"\n"
"db dump\t\tprint the content of the database\n"
"db sort\t\tsort the database\n"
"db open\t\topen the database\n"
"db stats\tshow block statistics\n"
"db blocks\tdump block types\n"
"db new NAME\tcreate a new block\n"
"db delete NAME\tdelete a block\n"
"db change NAME\tchange a field in a block\n"
"db remove NAME\tremove a field from a block\n"
"down X Y\ttouch the touch screen\n"
"drag X0 Y0 X1 Y1\n"
"\t\tdrag gesture\n"
"echo MESSAGE\tdisplay a message, can contain spaces\n"
"help\t\tthis help text\n"
"interact\tshow the display and interact with the user\n"
"long X Y\tlong press the touch screen\n"
"master scramble\tdeterministically scamble the master secret\n"
"move X Y\tmove on the touch screen\n"
"press\t\tpress the side button\n"
"random\t\tenable random number generation\n"
"random BYTE\tset random number generator to fixed value\n"
"release\t\trelease the side button\n"
"rmt hex|string ...\n"
"\t\tsend a remote request and show the response (if any)\n"
"rmt open\tinitialize the remote control protocol\n"
"screen\t\ttake a screenshot (PPM)\n"
"screen PPMFILE\ttake a screenshot in a specific file, remember the name\n"
"static\t\tuse static dummy data where information may change\n"
"system COMMAND\trun a shell command\n"
"tap X Y\t\ttap the touch screen\n"
"tick\t\tgenerate one timer tick\n"
"tick N\t\tgenerate N timer ticks\n"
"time UNIX-TIME\tset the system time (and hold it until changed)\n"
"up\t\tstop touching the touch screen\n"
    );
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

	/* static build */

	if (!strcmp("static", cmd)) {
		build_override = 1;
		return 1;
	}

	/* rmt */

	arg = cmd_arg("rmt", cmd);
	if (arg) {
		if (!strcmp(arg, "open")) {
			rmt_db_init();
			return 1;
		} else {
			rmt(arg);
		}
		return 1;
	}

	/* database */

	arg = cmd_arg("db", cmd);
	if (arg) {
		int args;
		char op[MAX_NAME_LEN + 1];
		char name[MAX_NAME_LEN + 1];
		char prev[MAX_NAME_LEN + 1];

		args = sscanf(arg, "%s %s %s", op, name, prev);
		if (args < 1)
			goto fail;
		if (!strcmp(op, "dummy") && args == 1) {
			struct dbcrypt *c;

			secrets_init();
			c = dbcrypt_init(master_secret, sizeof(master_secret));
			db_open_empty(&main_db, c);
			return 1;
		}
		if (!strcmp(op, "sort") && args == 1) {
			db_tsort(&main_db);
			return 1;
		}
		if (!strcmp(op, "dump") && args == 1) {
			dump_db_short(&main_db);
			return 1;
		}
		if (!strcmp(op, "ls") && args == 1) {
			dummy_ls();
			return 1;
		}
		if (!strcmp(op, "pwd") && args == 1) {
			dummy_pwd();
			return 1;
		}
		if (!strcmp(op, "cd")) {
			switch (args) {
			case 1:
				dummy_cd(NULL);
				return 1;
			case 2:
				dummy_cd(name);
				return 1;
			default:
				goto fail;
			}
		}
		if (!strcmp(op, "add"))
			switch (args) {
			case 2:
				db_dummy_entry(&main_db, name, NULL);
				return 1;
			case 3:
				db_dummy_entry(&main_db, name, prev);
				return 1;
			default:
				goto fail;
			}
		if (!strcmp(op, "mkdir"))
			switch (args) {
				struct db_entry *de;

			case 2:
				de = db_dummy_entry(&main_db, name, NULL);
				db_mkdir(de);
				return 1;
			case 3:
				de = db_dummy_entry(&main_db, name, prev);
				db_mkdir(de);
				return 1;
			default:
				goto fail;
			}
		if (!strcmp(op, "move")) {
			struct db_entry *a, *b;

			switch (args) {
			case 2:
				a = find_entry(name);
				db_move_before(a, NULL);
				return 1;
			case 3:
				a = find_entry(name);
				b = find_entry(prev);
				db_move_before(a, b);
				return 1;
			default:
				goto fail;
			}
		}
		if (!strcmp(op, "move-from") && args == 2) {
			assert(!moving);
			moving = find_entry(name);
			return 1;
		}
		if (!strcmp(op, "move-before")) {
			struct db_entry *before = NULL;

			assert(moving);
			switch (args) {
			case 2:
				before = find_entry(name);
				/* fall through */
			case 1:
				db_move_before(moving, before);
				return 1;
			default:
				goto fail;
			}
			moving = find_entry(name);
			return 1;
		}
		if (!strcmp(op, "open")) {
			struct dbcrypt *c;

			secrets_init();
			c  = dbcrypt_init(master_secret, sizeof(master_secret));
			if (!c) {
				fprintf(stderr, "dbcrypt_init failed\n");
				exit(1);
			}
			db_open(&main_db, c);
			return 1;
		}
		if (!strcmp(op, "stats")) {
			printf("total %u invalid %u data %u\n",
			    main_db.stats.total, main_db.stats.invalid,
			    main_db.stats.data);
			printf("erased %u deleted %u empty %u\n",
			    main_db.stats.erased, main_db.stats.deleted,
			    main_db.stats.empty);
			return 1;
		}
		if (!strcmp(arg, "blocks")) {
			bool first = 1;
			unsigned i;

			for (i = RESERVED_BLOCKS; i != main_db.stats.total; i++)
				switch (block_read(main_db.c,
				    NULL, NULL, NULL, i)) {
				case bt_data:
					printf("%sD%u", first ? "" : " ", i);
					first = 0;
					break;
				case bt_deleted:
					printf("%sX%u", first ? "" : " ", i);
					first = 0;
					break;
				case bt_erased:
					continue;
				case bt_invalid:
					printf("%sI%u", first ? "" : " ", i);
					first = 0;
					break;
				default:
					abort();
				}
			if (!first)
				printf("\n");
			return 1;
		}

		const char *arg2;

		arg2 = cmd_arg("new", arg);
		if (arg2) {
			struct db_entry *de;

			de = db_new_entry(&main_db, arg2);
			if (de)
				printf("%u\n", de->block);
			else
				printf("failed\n");
			return 1;
		}
		arg2 = cmd_arg("delete", arg);
		if (arg2) {
			struct db_entry *de = find_entry(arg2);

			if (!db_delete_entry(de))
				printf("failed");
			return 1;
		}
		arg2 = cmd_arg("change", arg);
		if (arg2) {
			struct db_entry *de = find_entry(arg2);

			if (db_change_field(de, ft_user, "foo", 3))
				printf("%u\n", de->block);
			else
				printf("failed\n");
			return 1;
		}
		arg2 = cmd_arg("remove", arg);
		if (arg2) {
			struct db_entry *de = find_entry(arg2);
			struct db_field *f = db_field_find(de, ft_user);

			if (!f)
				printf("field no found\n");
			else if (db_delete_field(de, f))
				printf("%u\n", de->block);
			else
				printf("failed\n");
			return 1;
		}
		goto fail;
	}

	/* BIP39 */

	arg = cmd_arg("bip39", cmd);
	if (arg) {
		const char *arg2;

		arg2 = cmd_arg("encode", arg);
		if (arg2) {
			if (!do_bip39_enc(arg2))
				goto fail;
			return 1;
		}
		arg2 = cmd_arg("decode", arg);
		if (arg2) {
			if (!do_bip39_dec(arg2))
				goto fail;
			return 1;
		}
		if (!strcmp("match", arg)) {
			do_bip39_match("");
			return 1;
		}
		arg2 = cmd_arg("match", arg);
		if (arg2) {
			do_bip39_match(arg2);
			return 1;
		}
		goto fail;
	}

	/* master*/

	arg = cmd_arg("master", cmd);
	if (arg) {
		if (!strcmp(arg, "scramble")) {
			sha256_begin();
			sha256_hash(master_secret, sizeof(master_secret));
			sha256_end(master_secret);
			return 1;
		}
		goto fail;
	}

	/* help */

	if (!strcmp("help", cmd)) {
		show_help();
		return 1;
	}

fail:
	fprintf(stderr, "bad command \"%s\"\n", cmd);
	return 0;
}


bool run_script(char **args, int n_args)
{
	if (!n_args || strncmp(args[0], "db ", 3)) {
		struct dbcrypt *c;

		secrets_init();
		c  = dbcrypt_init(master_secret, sizeof(master_secret));
		db_open(&main_db, c);
	}
	while (n_args-- && headless)
		if (!process_cmd(*args++))
			return 0;
	return 1;
}
