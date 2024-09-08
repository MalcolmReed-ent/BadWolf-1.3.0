// BadWolf: Minimalist and privacy-oriented WebKitGTK+ browser
// SPDX-FileCopyrightText: 2019-2023 Badwolf Authors <https://hacktivis.me/projects/badwolf>
// SPDX-License-Identifier: BSD-3-Clause

#include "userscripts.h"

#include "badwolf.h"
#include "config.h"

#include <assert.h>
#include <glib/gi18n.h> /* _() and other internationalization/localization helpers */
#include <glob.h>

void
load_userscripts(WebKitUserContentManager *content_manager)
{
	glob_t scripts_path_glob;
	gchar *scripts_path = g_build_filename(g_get_user_data_dir(), "badwolf", "scripts", "*.js", NULL);
	unsigned int loaded = 0, failed = 0;

	fprintf(stderr, _("badwolf: Checking for userscripts matching %s\n"), scripts_path);

	switch(glob(scripts_path, GLOB_NOSORT, NULL, &scripts_path_glob))
	{
	case 0:
		fprintf(stderr, _("badwolf: Notice: Found %zd userscripts\n"), scripts_path_glob.gl_pathc);
		break;
	case GLOB_NOMATCH:
		fprintf(stderr, _("badwolf: Notice: No userscripts found\n"));
		goto clean;
		break;
	case GLOB_NOSPACE:
		fprintf(stderr, _("badwolf: Failed to list userscripts: Out of Memory\n"));
		goto clean;
		break;
	case GLOB_ABORTED:
		fprintf(stderr, _("badwolf: Failed to list userscripts: Read Error\n"));
		goto clean;
		break;
	}

	for(size_t i = 0; i < scripts_path_glob.gl_pathc; i++)
	{
		char *filename = scripts_path_glob.gl_pathv[i];
		gchar *contents;
		GError *err = NULL;

		if(g_file_get_contents(filename, &contents, NULL, &err) && err == NULL)
		{

			WebKitUserScript *userscript =
			    webkit_user_script_new(contents,
			                           WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
			                           WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START,
			                           NULL,
			                           NULL);
			webkit_user_content_manager_add_script(content_manager, userscript);
			loaded++;
		}
		else
		{
			fprintf(stderr, _("badwolf: Error reading userscript: %s\n"), err->message);
			g_error_free(err);
			failed++;
		}
	}

	fprintf(stderr,
	        _("badwolf: Notice: Userscript loading: %d loaded, %d failed to load\n"),
	        loaded,
	        failed);

clean:
	g_free(scripts_path);
	globfree(&scripts_path_glob);
}
