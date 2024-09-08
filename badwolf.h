// SPDX-FileCopyrightText: 2019-2022 Badwolf Authors <https://hacktivis.me/projects/badwolf>
// SPDX-License-Identifier: BSD-3-Clause

#ifndef BADWOLF_H_INCLUDED
#define BADWOLF_H_INCLUDED

#include <gtk/gtk.h>
#include <inttypes.h> /* uint64_t */
#include <webkit2/webkit2.h>

#if !WEBKIT_CHECK_VERSION(2, 32, 0)
#error WebkitGTK 2.32.0 is the latest supported version for badwolf.
#endif

#ifdef UNUSED
#error UNUSED is already defined
#elif defined(__GNUC__)
#define UNUSED(x) UNUSED_##x __attribute__((unused))
#else
#define UNUSED(x) x
#endif

extern const gchar *homepage;
extern const gchar *version;

struct Window
{
	GtkWidget *main_window;
	GtkWidget *notebook;
	GtkWidget *new_tab;
	GtkWidget *downloads_tab;
	WebKitUserContentManager *content_manager;
	WebKitUserContentFilterStore *content_store;
};

struct Client
{
	GtkWidget *box;

	GtkWidget *toolbar;
	GtkWidget *back;
	GtkWidget *forward;
	GtkWidget *javascript;
	GtkWidget *auto_load_images;
	GtkWidget *location;

	uint64_t context_id;
	WebKitWebView *webView;
	struct Window *window;

	GtkWidget *statusbar;
	GtkWidget *statuslabel;
	GtkWidget *search;
};

GtkWidget *badwolf_new_tab_box(const gchar *title, struct Client *browser);
void webView_tab_label_change(struct Client *browser, const gchar *title);
struct Client *
new_browser(struct Window *window, const gchar *target_url, struct Client *old_browser);
int badwolf_new_tab(GtkNotebook *notebook, struct Client *browser, bool auto_switch);
gint badwolf_get_tab_position(GtkContainer *notebook, GtkWidget *child);
#endif /* BADWOLF_H_INCLUDED */
