/*
 * io.c - Test tool for USB communication with the Sunela device
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#define	_XOPEN_SOURCE 500	/* for strptime; 500 needed for usleep */
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <usb.h>
#include <errno.h>
#include <assert.h>

#include "../rmt/rmt-db.h"
#include "../sdk/sdk-usb.h"
#include "../db/db.h" /* for enum field_type */

#include "usbopen.h"
#include "fake-rmt.h"


#define	TIMEOUT_MS	1000


static bool fake_rmt = 0;


static void old_set_time(usb_dev_handle *dev)
{
	uint64_t t = time(NULL);
	int res;

	res = usb_control_msg(dev, TO_DEV, SUNELA_TIME, 0, 0,
	    (char *)  &t, sizeof(t), TIMEOUT_MS);
	if (res < 0) {
		fprintf(stderr, "SUNELA_TIME: %d\n", res);
		exit(1);
	}
}


static void query(usb_dev_handle *dev, uint8_t req, const char *name)
{
	char buf[1024];
	int res, i;

	res = usb_control_msg(dev, FROM_DEV, req, 0, 0, buf, sizeof(buf),
	    TIMEOUT_MS);
	if (res < 0) {
		fprintf(stderr, "%s: %d\n", name, res);
		exit(1);
	}
	printf("%d:", res);
	for (i = 0; i != res; i++)
		printf(" %02x", buf[i]);
	printf("\n");
}


static void demo(usb_dev_handle *dev, char *const *argv, int args)
{
	char buf[1024] = "";
	unsigned n = 0;
	int i;

	for (i = 0; i != args; i++) {
		strcpy(buf+ n, argv[i]);
		n += strlen(argv[i]) + 1;
	}
	int res;

	res = usb_control_msg(dev, TO_DEV, SUNELA_DEMO, 0, 0, buf, n,
	    TIMEOUT_MS);
	if (res < 0) {
		fprintf(stderr, "SUNELA_DEMO: %d\n", res);
		exit(1);
	}
}


static void rmt_io(usb_dev_handle *dev, uint8_t op, const void *arg,
    size_t len, void (*print)(const char *buf, unsigned len))
{
	int res;
	char buf[256] = { op, };

	memcpy(buf + 1, arg, len);
//fprintf(stderr, "TO_DEV SUNELA_RMT %u\n", (unsigned) len + 1);
	if (fake_rmt) {
		fake_rmt_send(buf, len + 1);
	} else {
		res = usb_control_msg(dev, TO_DEV, SUNELA_RMT, 0, 0,
		    buf, len + 1, TIMEOUT_MS);
		if (res < 0) {
			fprintf(stderr, "SUNELA_RMT (req): %d\n", res);
			exit(1);
		}
	}
	while (1) {
		usleep(10 * 1000);
//fprintf(stderr, "TO_DEV SUNELA_RMT %u\n", 0);
		if (fake_rmt) {
			fake_rmt_send(NULL, 0);
		} else {
			res = usb_control_msg(dev, TO_DEV, SUNELA_RMT, 0, 0,
			    NULL, 0, TIMEOUT_MS);
			if (res == -EPIPE)
				continue;
			if (res < 0) {
				fprintf(stderr, "SUNELA_RMT (req-end): %d\n",
				    res);
				exit(1);
			}
		}
		break;
	}
	while (1) {
		usleep(10 * 1000);
//fprintf(stderr, "FROM_DEV SUNELA_RMT %u\n", (unsigned) sizeof(buf));
		if (fake_rmt) {
			res = fake_rmt_recv(buf, sizeof(buf));
		} else {
			res = usb_control_msg(dev, FROM_DEV, SUNELA_RMT, 0, 0,
			    buf, sizeof(buf), TIMEOUT_MS);
//fprintf(stderr, "got %d\n", res);
			if (res == -EPIPE)
				continue;
			if (res < 0) {
				fprintf(stderr, "SUNELA_RMT (res): %d\n", res);
				exit(1);
			}
		}
		if (!res)
			return;
		if (!*buf) {
			fprintf(stderr, "%.*s\n", res - 1, buf + 1);
			continue;
		}
		print(buf, res);
	}
}


static void print_ls(const char *buf, unsigned len)
{
	printf("%.*s\n", len , buf);
}


static void print_show(const char *buf, unsigned len)
{
	printf("%u: %.*s\n", buf[0], len - 1, buf + 1);
}


static void rmt_bin(usb_dev_handle *dev, uint8_t op, const void *arg,
    size_t len)
{
	void (*print)(const char *buf, unsigned len);

	switch (op) {
	case RDOP_LS:
		print = print_ls;
		break;
	case RDOP_SHOW:
		print = print_show;
		break;
	default:
		abort();
	}
	rmt_io(dev, op, arg, len, print);
}


static void rmt(usb_dev_handle *dev, uint8_t op, const char *arg)
{
	rmt_bin(dev, op, arg, strlen(arg));
}


static void reveal(usb_dev_handle *dev, const char *entry, const char *field)
{
	unsigned len = strlen(entry);
	uint8_t buf[len + 1];
	enum field_type type;

	if (!strcmp(field, "password") || !strcmp(field, "pw"))
		type = ft_pw;
	else if (!strcmp(field, "password2") || !strcmp(field, "pw2"))
		type = ft_pw2;
	else if (!strcmp(field, "hotp_secret"))
		type = ft_hotp_secret;
	else if (!strcmp(field, "totp_secret"))
		type = ft_totp_secret;
	else {
		fprintf(stderr, "unrecognized field \"%s\"\n", field);
		exit(1);
	}
	memcpy(buf, entry, len);
	buf[len] = type;
	rmt_bin(dev, RDOP_REVEAL, buf, len + 1);
}



static void print_time(const char *buf, unsigned len)
{
	uint64_t t_raw;
	time_t t;
	struct tm tm;
	char buf2[100];

	assert(len == sizeof(t_raw) + 1);
	memcpy(&t_raw, buf + 1, len - 1);
	t = t_raw;
	gmtime_r(&t, &tm);
	strftime(buf2, sizeof(buf2), "%Y-%m-%d %H:%M:%S", &tm);

	printf("%s\n", buf2);
}


static void get_time(usb_dev_handle *dev)
{
	rmt_io(dev, RDOP_GET_TIME, NULL, 0, print_time);
}


static void set_time(usb_dev_handle *dev, const char *s)
{
	time_t t;
	uint64_t t_buf;

	time(&t);
	if (s) {
		struct tm tm = *gmtime(&t);
		char *next;

		next = strptime(s, "%Y-%m-%dT%H:%M:%S", &tm);
		if (!next)
			next = strptime(s, "%H:%M:%S", &tm);
		if (!next)
			next = strptime(s, "%Y-%m-%dT%H:%M", &tm);
		if (!next) {
			tm.tm_sec = 0;
			next = strptime(s, "%H:%M", &tm);
		}
		if (!next || *next) {
			fprintf(stderr, "invalid date\n");
			exit(1);
		}
		t = mktime(&tm);
	}
	t_buf = t;
	rmt_io(dev, RDOP_SET_TIME, &t_buf, sizeof(t_buf), NULL);
}


static void usage(const char *name)
{
	fprintf(stderr,
"usage: %s [-d vid:pid] [-R path] [command [args ...]]\n\n"
"Options:\n"
"-R path\topen a Unix domain SEQPACKET socket for RMT communication\n"
"\n"
"Commands:\n"
"  bad-query\n"
"  demo name [args ...]\n"
"  get-time\n"
"  ls\n"
"  query\n"
"  reveal entry-name field-name\n"
"  set-time [[YYYY-MM-DDT]HH:MM[:SS]]\n"
"  show entry-name\n"
"  time\n"
    , name);
	exit(1);
}


int main(int argc, char *const *argv)
{
	usb_dev_handle *dev;
	int c, n_args;

	while ((c = getopt(argc, argv, "R:")) != EOF)
		switch (c) {
		case 'R':
			fake_rmt_open(optarg);
			fake_rmt = 1;
			break;
		default:
			usage(*argv);
		}

	usb_unrestrict();
#if 0
	if (device)
		restrict_usb_path(device);
#endif
	if (!fake_rmt) {
		dev = open_usb(USB_VENDOR, USB_PRODUCT);
		if (!dev) {
			perror("usb");
			exit(1);
		}
	}

	n_args = argc - optind;
	if (n_args == 1 && !strcmp(argv[optind], "time"))
		old_set_time(dev);
	else if (n_args == 1 && !strcmp(argv[optind], "query"))
		query(dev, SUNELA_QUERY, "SUNELA_QUERY");
	else if (n_args == 1 && !strcmp(argv[optind], "bad-query"))
		query(dev, SUNELA_DEMO, "SUNELA_DEMO");
	else if (n_args > 1 && !strcmp(argv[optind], "demo"))
		demo(dev, argv + optind + 1, n_args - 1);
	else if (n_args == 1 && !strcmp(argv[optind], "ls"))
		rmt(dev, RDOP_LS, "");
	else if (n_args == 2 && !strcmp(argv[optind], "show"))
		rmt(dev, RDOP_SHOW, argv[optind + 1]);
	else if (n_args == 3 && !strcmp(argv[optind], "reveal"))
		reveal(dev, argv[optind + 1], argv[optind + 2]);
	else if (n_args == 1 && !strcmp(argv[optind], "get-time"))
		get_time(dev);
	else if (n_args == 1 && !strcmp(argv[optind], "set-time"))
		set_time(dev, NULL);
	else if (n_args == 2 && !strcmp(argv[optind], "set-time"))
		set_time(dev, argv[optind + 1]);
	else
		usage(*argv);
	return 0;
}
