/*
 * ui_accounts.h - User interface: show account list
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef UI_ACCOUNTS_H
#define	UI_ACCOUNTS_H

/*
 * Any operation that may invalidate an on-going moving operation needs to call
 * ui_accounts_cancel_move. For example, account deletion.
 */

void ui_accounts_cancel_move(void);

#endif /* !UI_ACCOUNTS_H */
