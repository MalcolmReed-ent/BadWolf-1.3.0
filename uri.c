// BadWolf: Minimalist and privacy-oriented WebKitGTK+ browser
// SPDX-FileCopyrightText: 2019-2022 Badwolf Authors <https://hacktivis.me/projects/badwolf>
// SPDX-License-Identifier: BSD-3-Clause

#include "uri.h"

#include <glib.h>   /* g_strcmp0(), g_uri_parse_scheme(), g_strdup_printf */
#include <stdlib.h> /* realpath(), free() */
#include <unistd.h> /* access() */

const gchar *
badwolf_ensure_uri_scheme(const gchar *text, gboolean try_file)
{
	const gchar *fallback = "about:blank";
	char *path            = NULL;

	if(g_strcmp0(text, "") <= 0) return fallback;

	if(g_uri_parse_scheme(text)) return text;

	if(try_file)
	{
		/* flawfinder: ignore. `path` is allocated by realpath itself */
		path     = realpath(text, NULL);
		gchar *f = NULL;

		if(path != NULL)
		{
			f = g_strdup_printf("file://%s", path);

			free(path);

			return f;
		}
	}

	return g_strdup_printf("http://%s", text);
}
