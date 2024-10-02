/*
 * ui_show_pubkey.c - User interface: show the public key
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include <stdint.h>
#include <string.h>

#include "db.h"
#include "dbcrypt.h"
#include "tweetnacl.h"
#include "base32.h"
#include "ui.h"
#include "ui_notice.h"


/* --- Open/close ---------------------------------------------------------- */


static void ui_show_pubkey_open(void *ctx, void *params)
{
	char buf[60];	/* needs 56 characters */

	base32_encode(buf, sizeof(buf), dbcrypt_pubkey(main_db.c),
	    crypto_box_PUBLICKEYBYTES);
	notice_idle(nt_info, IDLE_PUBKEY_S, "Public key", "%s", buf);
}


/* --- Interface ----------------------------------------------------------- */


const struct ui ui_show_pubkey = {
	.name		= "show pubkey",
	.open		= ui_show_pubkey_open,
};
