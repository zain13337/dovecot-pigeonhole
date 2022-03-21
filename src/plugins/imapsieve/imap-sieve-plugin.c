/* Copyright (c) 2016-2018 Pigeonhole authors, see the included COPYING file */

#include "imap-common.h"
#include "str.h"

#include "imap-sieve.h"
#include "imap-sieve-storage.h"

#include "imap-sieve-plugin.h"

static struct module *imap_sieve_module;
static imap_client_created_func_t *next_hook_client_created;

/*
 * Client
 */

static void imap_sieve_client_created(struct client **clientp)
{
	struct client *client = *clientp;
	struct mail_user *user = client->user;
	const char *url = NULL;

	if (mail_user_is_plugin_loaded(user, imap_sieve_module)) {
		url = mail_user_plugin_getenv(user, "imapsieve_url");
		// FIXME: parse the URL and report error if it is bad
		if (url != NULL && str_begins_icase_with(url, "sieve:")) {
			client_add_capability(client, t_strconcat(
				"IMAPSIEVE=", url, NULL));
		} else {
			url = NULL;
		}

		imap_sieve_storage_client_created(client, (url != NULL));
	}

	if (next_hook_client_created != NULL)
		next_hook_client_created(clientp);
}

/*
 * Plugin
 */

const char *imap_sieve_plugin_version = DOVECOT_ABI_VERSION;
const char imap_sieve_plugin_binary_dependency[] = "imap";

void imap_sieve_plugin_init(struct module *module)
{
	imap_sieve_module = module;
	next_hook_client_created =
		imap_client_created_hook_set(imap_sieve_client_created);
	imap_sieve_storage_init(module);
}

void imap_sieve_plugin_deinit(void)
{
	imap_sieve_storage_deinit();
	imap_client_created_hook_set(next_hook_client_created);
}
