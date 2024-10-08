// BadWolf: Minimalist and privacy-oriented WebKitGTK+ browser
// SPDX-FileCopyrightText: 2019-2023 Badwolf Authors <https://hacktivis.me/projects/badwolf>
// SPDX-License-Identifier: BSD-3-Clause

#include "badwolf.h"

#include "config.h"
#include "downloads.h"
#include "fmt.h"
#include "keybindings.h"
#include "uri.h"
#include "userscripts.h"

#include <assert.h>
#include <glib/gi18n.h>   /* _() and other internationalization/localization helpers */
#include <libsoup/soup.h> /* soup* */
#include <locale.h>       /* LC_* */
#include <stdio.h>        /* perror(), fprintf(), snprintf() */
#include <stdlib.h>       /* malloc() */
#include <unistd.h>       /* access() */

const gchar *homepage = "https://hacktivis.me/projects/badwolf";
const gchar *version  = VERSION;

static gchar *web_extensions_directory;
static uint64_t context_id_counter = 0;
GtkTreeModel *bookmarks_completion_model;

static gboolean WebViewCb_close(WebKitWebView *webView, gpointer user_data);
static gboolean WebViewCb_web_process_terminated(WebKitWebView *webView,
                                                 WebKitWebProcessTerminationReason reason,
                                                 gpointer user_data);
static gboolean
WebViewCb_notify__uri(WebKitWebView *webView, GParamSpec *pspec, gpointer user_data);
static gboolean
WebViewCb_notify__title(WebKitWebView *webView, GParamSpec *pspec, gpointer user_data);
static gboolean
WebViewCb_notify__is__playing__audio(WebKitWebView *webView, GParamSpec *pspec, gpointer user_data);
static gboolean WebViewCb_notify__estimated_load_progress(WebKitWebView *webView,
                                                          GParamSpec *pspec,
                                                          gpointer user_data);
static gboolean WebViewCb_mouse_target_changed(WebKitWebView *webView,
                                               WebKitHitTestResult *hit,
                                               guint modifiers,
                                               gpointer user_data);
static WebKitWebView *WebViewCb_create(WebKitWebView *related_web_view,
                                       WebKitNavigationAction *navigation_action,
                                       gpointer user_data);
static gboolean WebViewCb_permission_request(WebKitWebView *web_view,
                                             WebKitPermissionRequest *request,
                                             gpointer user_data);
static gboolean WebViewCb_decide_policy(WebKitWebView *web_view,
                                        WebKitPolicyDecision *decision,
                                        WebKitPolicyDecisionType decision_type,
                                        gpointer user_data);
static void
WebViewCb_load_changed(WebKitWebView *webView, WebKitLoadEvent load_event, gpointer user_data);
static void web_contextCb_download_started(WebKitWebContext *web_context,
                                           WebKitDownload *download,
                                           gpointer user_data);
static gboolean locationCb_activate(GtkEntry *location, gpointer user_data);
static gboolean javascriptCb_toggled(GtkButton *javascript, gpointer user_data);
static gboolean auto_load_imagesCb_toggled(GtkButton *auto_load_images, gpointer user_data);
static void backCb_clicked(GtkButton *back, gpointer user_data);
static void forwardCb_clicked(GtkButton *forward, gpointer user_data);
static void printCb_clicked(GtkButton *forward, gpointer user_data);
static gboolean SearchEntryCb_next__match(GtkSearchEntry *search, gpointer user_data);
static gboolean SearchEntryCb_previous__match(GtkSearchEntry *search, gpointer user_data);
static gboolean SearchEntryCb_search__changed(GtkSearchEntry *search, gpointer user_data);
static gboolean SearchEntryCb_stop__search(GtkSearchEntry *search, gpointer user_data);
static void new_tabCb_clicked(GtkButton *new_tab, gpointer user_data);
static void closeCb_clicked(GtkButton *close, gpointer user_data);
static void
notebookCb_switch__page(GtkNotebook *notebook, GtkWidget *page, guint page_num, gpointer user_data);
void content_managerCb_ready(GObject *store, GAsyncResult *result, gpointer user_data);

static gboolean
WebViewCb_close(WebKitWebView *UNUSED(webView), gpointer user_data)
{
	struct Client *browser = (struct Client *)user_data;

	gtk_widget_destroy(browser->box);

	free(browser);

	return TRUE;
}

static gboolean
WebViewCb_web_process_terminated(WebKitWebView *UNUSED(webView),
                                 WebKitWebProcessTerminationReason reason,
                                 gpointer user_data)
{
	struct Client *browser = (struct Client *)user_data;

	switch(reason)
	{
	case WEBKIT_WEB_PROCESS_CRASHED:
		fprintf(stderr, "%s", _("the web process crashed.\n"));
		webView_tab_label_change(browser, _("Crashed"));
		break;
	case WEBKIT_WEB_PROCESS_EXCEEDED_MEMORY_LIMIT:
		fprintf(stderr, "%s", _("the web process exceeded the memory limit.\n"));
		webView_tab_label_change(browser, _("Out of Memory"));
		break;
	default:
		fprintf(stderr, "%s", _("the web process terminated for an unknown reason.\n"));
		webView_tab_label_change(browser, _("Unknown Crash"));
	}

	return FALSE;
}

static gboolean
WebViewCb_notify__uri(WebKitWebView *UNUSED(webView), GParamSpec *UNUSED(pspec), gpointer user_data)
{
	const gchar *location_uri;
	struct Client *browser = (struct Client *)user_data;

	location_uri = webkit_web_view_get_uri(browser->webView);
	printf("location_uri: <%s>\n", location_uri);

	// Don't set if location_uri is NULL / empty, latter happens on WebProcess termination
	if(location_uri == NULL || location_uri[0] == '\0')
	{
		return TRUE;
	}

	gtk_entry_set_text(GTK_ENTRY(browser->location), location_uri);

	if(webkit_uri_for_display(location_uri) != location_uri)
		gtk_widget_set_tooltip_text(browser->location, webkit_uri_for_display(location_uri));
	else
		gtk_widget_set_has_tooltip(browser->location, false);

	return TRUE;
}

GtkWidget *
badwolf_new_tab_box(const gchar *title, struct Client *browser)
{
	/* flawfinder: ignore. bound checks are done */
	char context_id_str[BADWOLF_CTX_SIZ] = {0, 0, 0, 0, 0, 0, 0};

	fmt_context_id(browser->context_id, context_id_str);

	GtkWidget *tab_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_name(tab_box, "browser__tabbox");

	GtkWidget *close =
	    gtk_button_new_from_icon_name("window-close-symbolic", GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_widget_set_name(close, "browser__tabbox__close");
	GtkWidget *label = gtk_label_new(title);
	gtk_widget_set_name(label, "browser__tabbox__label");
	GtkWidget *label_event_box = gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(label_event_box), label);
	GtkWidget *context_label = gtk_label_new(context_id_str);
	gtk_widget_set_name(context_label, "browser__tabbox__context_label");
	GtkWidget *context_label_event_box = gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(context_label_event_box), context_label);
	GtkWidget *playing =
	    gtk_image_new_from_icon_name("audio-volume-high-symbolic", GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_widget_set_name(playing, "browser__tabbox__playing");
	GtkWidget *playing_event_box = gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(playing_event_box), playing);

#ifdef BADWOLF_TAB_BOX_WIDTH
	gtk_widget_set_size_request(label, BADWOLF_TAB_BOX_WIDTH, -1);
#endif
#ifdef BADWOLF_TAB_LABEL_CHARWIDTH
	gtk_label_set_width_chars(GTK_LABEL(label), BADWOLF_TAB_LABEL_CHARWIDTH);
#endif
	gtk_widget_set_hexpand(tab_box, BADWOLF_TAB_HEXPAND);

	gtk_label_set_ellipsize(GTK_LABEL(label), BADWOLF_TAB_LABEL_ELLIPSIZE);
	gtk_label_set_single_line_mode(GTK_LABEL(label), TRUE);
	gtk_label_set_single_line_mode(GTK_LABEL(context_label), TRUE);

	gtk_box_pack_start(GTK_BOX(tab_box), context_label_event_box, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(tab_box), playing_event_box, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(tab_box), label_event_box, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(tab_box), close, FALSE, FALSE, 0);

	gtk_button_set_relief(GTK_BUTTON(close), GTK_RELIEF_NONE);

	g_signal_connect(close, "clicked", G_CALLBACK(closeCb_clicked), browser);

	gtk_widget_set_tooltip_text(tab_box, title);

	gtk_widget_show_all(tab_box);
	gtk_widget_set_visible(playing, webkit_web_view_is_playing_audio(browser->webView));
	gtk_widget_set_events(label, GDK_BUTTON_RELEASE_MASK);
	g_signal_connect(
	    tab_box, "button-release-event", G_CALLBACK(tab_boxCb_button_release_event), browser);

	return tab_box;
}

static gboolean
WebViewCb_notify__title(WebKitWebView *UNUSED(webView),
                        GParamSpec *UNUSED(pspec),
                        gpointer user_data)
{
	struct Client *browser = (struct Client *)user_data;

	// Don't set if title is NULL / empty, latter happens on WebProcess crash
	const char *title = webkit_web_view_get_title(browser->webView);
	printf("title: <%s>\n", title);
	if(title == NULL || title[0] == '\0')
	{
		return TRUE;
	}

	webView_tab_label_change(browser, NULL);

	return TRUE;
}

static gboolean
WebViewCb_notify__is__playing__audio(WebKitWebView *UNUSED(webView),
                                     GParamSpec *UNUSED(pspec),
                                     gpointer user_data)
{
	struct Client *browser = (struct Client *)user_data;

	webView_tab_label_change(browser, NULL);

	return TRUE;
}

void
webView_tab_label_change(struct Client *browser, const gchar *title)
{
	GtkWidget *notebook = browser->window->notebook;

#define title_IS_EMPTY title == NULL || title[0] == '\0'

	if(title_IS_EMPTY) title = webkit_web_view_get_title(browser->webView);
	if(title_IS_EMPTY) title = webkit_web_view_get_uri(browser->webView);
	if(title_IS_EMPTY) title = _("Empty Title");

	gtk_notebook_set_tab_label(
	    GTK_NOTEBOOK(notebook), browser->box, badwolf_new_tab_box(title, browser));
	gtk_notebook_set_menu_label_text(GTK_NOTEBOOK(notebook), browser->box, title);

	// Set the window title if the title change was on the current tab
	if(gtk_notebook_page_num(GTK_NOTEBOOK(notebook), browser->box) ==
	   gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)))
		gtk_window_set_title(GTK_WINDOW(browser->window->main_window), title);
}

static gboolean
WebViewCb_notify__estimated_load_progress(WebKitWebView *UNUSED(webView),
                                          GParamSpec *UNUSED(pspec),
                                          gpointer user_data)
{
	struct Client *browser = (struct Client *)user_data;
	gdouble progress;

	progress = webkit_web_view_get_estimated_load_progress(browser->webView);

	if(progress >= 1) progress = 0;

	gtk_entry_set_progress_fraction(GTK_ENTRY(browser->location), progress);

	return TRUE;
}

static gboolean
WebViewCb_mouse_target_changed(WebKitWebView *UNUSED(webView),
                               WebKitHitTestResult *hit,
                               guint UNUSED(modifiers),
                               gpointer user_data)
{
	struct Client *browser = (struct Client *)user_data;

	if(webkit_hit_test_result_context_is_link(hit))
	{
		const gchar *link_uri = webkit_hit_test_result_get_link_uri(hit);

		gtk_label_set_text(GTK_LABEL(browser->statuslabel), webkit_uri_for_display(link_uri));
	}
	else
		gtk_label_set_text(GTK_LABEL(browser->statuslabel), NULL);

	return FALSE;
}

static gboolean
WebViewCb_scroll_event(GtkWidget *UNUSED(widget), GdkEvent *event, gpointer data)
{
	struct Client *browser = (struct Client *)data;
	gdouble delta_x, delta_y;
	gdouble zoom;

	if(((GdkEventScroll *)event)->state & GDK_CONTROL_MASK)
	{
		gdk_event_get_scroll_deltas(event, &delta_x, &delta_y);
		zoom = webkit_web_view_get_zoom_level(WEBKIT_WEB_VIEW(browser->webView));
		zoom -= delta_y * 0.1;
		webkit_web_view_set_zoom_level(WEBKIT_WEB_VIEW(browser->webView), zoom);
		return TRUE;
	}

	return FALSE;
}

static WebKitWebView *
WebViewCb_create(WebKitWebView *related_web_view,
                 WebKitNavigationAction *UNUSED(navigation_action),
                 gpointer user_data)
{
	struct Client *old_browser = (struct Client *)user_data;
	struct Client *browser     = NULL;

	// shouldn't be needed but better be safe
	old_browser->webView = related_web_view;

	browser = new_browser(old_browser->window, NULL, old_browser);

	if(badwolf_new_tab(GTK_NOTEBOOK(old_browser->window->notebook), browser, FALSE) < 0)
		return NULL;
	else
		return browser->webView;
}

static gboolean
WebViewCb_permission_request(WebKitWebView *UNUSED(web_view),
                             WebKitPermissionRequest *request,
                             gpointer UNUSED(user_data))
{
	webkit_permission_request_deny(request);

	return TRUE; /* Stop other handlers */
}

static gboolean
WebViewCb_decide_policy(WebKitWebView *UNUSED(web_view),
                        WebKitPolicyDecision *decision,
                        WebKitPolicyDecisionType decision_type,
                        gpointer user_data)
{
	struct Client *old_browser = (struct Client *)user_data;
	WebKitResponsePolicyDecision *r;
	WebKitNavigationPolicyDecision *n;
	WebKitNavigationAction *navigation_action;

	switch(decision_type)
	{
	case WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION:
		n                 = WEBKIT_NAVIGATION_POLICY_DECISION(decision);
		navigation_action = webkit_navigation_policy_decision_get_navigation_action(n);

		if(GDK_CONTROL_MASK == webkit_navigation_action_get_modifiers(navigation_action) ||
		   2 == webkit_navigation_action_get_mouse_button(navigation_action))
		{
			WebKitURIRequest *uri   = webkit_navigation_action_get_request(navigation_action);
			const gchar *target_url = webkit_uri_request_get_uri(uri);
			struct Client *browser  = new_browser(old_browser->window, target_url, old_browser);

			badwolf_new_tab(GTK_NOTEBOOK(browser->window->notebook), browser, FALSE);
			webkit_web_view_load_uri(browser->webView, target_url);
			webkit_policy_decision_ignore(decision);
		}
		else
		{
			/* Use whatever default there is. */
			return FALSE;
		}
		break;
	case WEBKIT_POLICY_DECISION_TYPE_RESPONSE:
		r = WEBKIT_RESPONSE_POLICY_DECISION(decision);
		if(!webkit_response_policy_decision_is_mime_type_supported(r))
			webkit_policy_decision_download(decision);
		else
			webkit_policy_decision_use(decision);
		break;
	default:
		/* Use whatever default there is. */
		return FALSE;
	}

	return TRUE;
}

static void
WebViewCb_load_changed(WebKitWebView *UNUSED(webView),
                       WebKitLoadEvent UNUSED(load_event),
                       gpointer user_data)
{
	struct Client *browser = (struct Client *)user_data;

	gtk_widget_set_sensitive(browser->back, webkit_web_view_can_go_back(browser->webView));
	gtk_widget_set_sensitive(browser->forward, webkit_web_view_can_go_forward(browser->webView));
}

static char *
detail_tls_certificate_flags(GTlsCertificateFlags tls_errors)
{
	GString *errors = g_string_new(NULL);

	g_string_append_printf(errors,
	                       _("Couldn't verify the TLS certificate to ensure a better security of the "
	                         "connection. You might want to verify your machine and network.\n\n"));

	if(tls_errors & G_TLS_CERTIFICATE_UNKNOWN_CA)
		g_string_append_printf(errors, _("Error: The X509 Certificate Authority is unknown.\n"));

	if(tls_errors & G_TLS_CERTIFICATE_BAD_IDENTITY)
		g_string_append(errors, _("Error: The given identity doesn't match the expected one.\n"));

	if(tls_errors & G_TLS_CERTIFICATE_NOT_ACTIVATED)
		g_string_append(errors,
		                _("Error: The certificate isn't valid yet. Check your system's clock.\n"));

	if(tls_errors & G_TLS_CERTIFICATE_EXPIRED)
		g_string_append(errors, _("Error: The certificate has expired. Check your system's clock.\n"));

	if(tls_errors & G_TLS_CERTIFICATE_REVOKED)
		g_string_append(errors, _("Error: The certificate has been revoked.\n"));

	if(tls_errors & G_TLS_CERTIFICATE_INSECURE)
		g_string_append(errors, _("Error: The certificate is considered to be insecure.\n"));

	if(tls_errors & G_TLS_CERTIFICATE_GENERIC_ERROR)
		g_string_append(errors, _("Error: Some unknown error occurred validating the certificate.\n"));

	return g_string_free(errors, FALSE);
}

static gboolean
WebViewCb_load_failed_with_tls_errors(WebKitWebView *UNUSED(web_view),
                                      gchar *failing_text,
                                      GTlsCertificate *certificate,
                                      GTlsCertificateFlags errors,
                                      gpointer user_data)
{
	struct Client *browser = (struct Client *)user_data;
	gchar *error_details   = detail_tls_certificate_flags(errors);
	gint dialog_response;

#ifndef USE_LIBSOUP2
	GUri *failing_uri = g_uri_parse(failing_text, G_URI_FLAGS_NONE, NULL);
#else
	SoupURI *failing_uri = soup_uri_new(failing_text);
#endif

	GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(browser->window->main_window),
	                                           GTK_DIALOG_MODAL & GTK_DIALOG_DESTROY_WITH_PARENT,
	                                           GTK_MESSAGE_ERROR,
	                                           GTK_BUTTONS_NONE,
	                                           _("TLS Error for %s."),
	                                           failing_text);
	gtk_dialog_add_buttons(
	    GTK_DIALOG(dialog), _("Temporarily Add Exception"), 1, _("Continue"), 0, NULL);
	if(!failing_uri)
	{
		gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog), 1, FALSE);
	}
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), 0);
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s\n", error_details);

	dialog_response = gtk_dialog_run(GTK_DIALOG(dialog));

	if(dialog_response == 1)
	{
#ifndef USE_LIBSOUP2
		webkit_web_context_allow_tls_certificate_for_host(
		    webkit_web_view_get_context(browser->webView), certificate, g_uri_get_host(failing_uri));
#else
		webkit_web_context_allow_tls_certificate_for_host(
		    webkit_web_view_get_context(browser->webView), certificate, failing_uri->host);
#endif
		webkit_web_view_reload(browser->webView);
	}

#ifndef USE_LIBSOUP2
	/* Calling g_free(failing_uri) ought to be the correct way but this causes a segfault.
	 *
	 * - documentation describes it as something which should be free
	 * - implementation seems to make it a pointer that should be freed
	 * - epiphany doesn't seems to free/unref it but gnome code seems to frequently have memleaks
	 *
	 * Decided to at least continue to try with using g_uri_unref(failing_uri) instead.
	 * Related fediverse post: <https://queer.hacktivis.me/objects/cec7c4e8-6a58-4358-85bf-b66f9bb21a98>
	 */
	g_uri_unref(failing_uri);
#else
	soup_uri_free(failing_uri);
#endif
	g_free(error_details);
	gtk_widget_destroy(dialog);

	return FALSE; /* propagate the event further */
}

static void
web_contextCb_download_started(WebKitWebContext *UNUSED(web_context),
                               WebKitDownload *webkit_download,
                               gpointer user_data)
{
	struct Client *browser    = (struct Client *)user_data;
	struct Download *download = malloc(sizeof(struct Download));

	assert(webkit_download);

	if(download != NULL)
	{
		download->window = browser->window;

		download_new_entry(webkit_download, download);

		g_signal_connect(
		    G_OBJECT(webkit_download), "received-data", G_CALLBACK(downloadCb_received_data), download);
		g_signal_connect(G_OBJECT(webkit_download),
		                 "created-destination",
		                 G_CALLBACK(downloadCb_created_destination),
		                 download);
		g_signal_connect(G_OBJECT(webkit_download), "failed", G_CALLBACK(downloadCb_failed), download);
		g_signal_connect(
		    G_OBJECT(webkit_download), "finished", G_CALLBACK(downloadCb_finished), download);
	}

	g_signal_connect(G_OBJECT(webkit_download),
	                 "decide-destination",
	                 G_CALLBACK(downloadCb_decide_destination),
	                 user_data);
}

static gboolean
locationCb_activate(GtkEntry *location, gpointer user_data)
{
	struct Client *browser = (struct Client *)user_data;

	webkit_web_view_load_uri(browser->webView,
	                         badwolf_ensure_uri_scheme(gtk_entry_get_text(location), TRUE));

	return TRUE;
}

static gboolean
javascriptCb_toggled(GtkButton *javascript, gpointer user_data)
{
	struct Client *browser = (struct Client *)user_data;

	WebKitSettings *settings = webkit_web_view_get_settings(browser->webView);

	webkit_settings_set_enable_javascript_markup(
	    settings, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(javascript)));

	webkit_web_view_set_settings(browser->webView, settings);

	return TRUE;
}

static gboolean
auto_load_imagesCb_toggled(GtkButton *auto_load_images, gpointer user_data)
{
	struct Client *browser = (struct Client *)user_data;

	WebKitSettings *settings = webkit_web_view_get_settings(browser->webView);

	webkit_settings_set_auto_load_images(
	    settings, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(auto_load_images)));

	webkit_web_view_set_settings(browser->webView, settings);

	return TRUE;
}

static void
backCb_clicked(GtkButton *UNUSED(back), gpointer user_data)
{
	struct Client *browser = (struct Client *)user_data;

	webkit_web_view_go_back(browser->webView);
}

static void
forwardCb_clicked(GtkButton *UNUSED(forward), gpointer user_data)
{
	struct Client *browser = (struct Client *)user_data;

	webkit_web_view_go_forward(browser->webView);
}

static void
printCb_clicked(GtkButton *UNUSED(print), gpointer user_data)
{
	struct Client *browser = (struct Client *)user_data;

	WebKitPrintOperation *print_operation = webkit_print_operation_new(browser->webView);

	webkit_print_operation_run_dialog(print_operation, GTK_WINDOW(browser->window->main_window));
}

static gboolean
SearchEntryCb_next__match(GtkSearchEntry *UNUSED(search), gpointer user_data)
{
	struct Client *browser               = (struct Client *)user_data;
	WebKitFindController *findController = webkit_web_view_get_find_controller(browser->webView);

	webkit_find_controller_search_next(findController);

	return TRUE;
}

static gboolean
SearchEntryCb_previous__match(GtkSearchEntry *UNUSED(search), gpointer user_data)
{
	struct Client *browser               = (struct Client *)user_data;
	WebKitFindController *findController = webkit_web_view_get_find_controller(browser->webView);

	webkit_find_controller_search_previous(findController);

	return TRUE;
}

static gboolean
SearchEntryCb_search__changed(GtkSearchEntry *search, gpointer user_data)
{
	struct Client *browser               = (struct Client *)user_data;
	WebKitFindController *findController = webkit_web_view_get_find_controller(browser->webView);
	const gchar *search_text             = gtk_entry_get_text(GTK_ENTRY(search));

	webkit_find_controller_search(findController, search_text, 0, 0);

	return TRUE;
}

static gboolean
SearchEntryCb_stop__search(GtkSearchEntry *UNUSED(search), gpointer user_data)
{
	struct Client *browser               = (struct Client *)user_data;
	WebKitFindController *findController = webkit_web_view_get_find_controller(browser->webView);

	webkit_find_controller_search_finish(findController);

	return TRUE;
}

static gboolean
widgetCb_drop_button3_event(GtkWidget *UNUSED(widget), GdkEvent *event, gpointer UNUSED(user_data))
{
	// Button3 being right-click on right-handed mode, left-click on left-handed mode
	return ((GdkEventButton *)event)->button == 3;
}

struct Client *
new_browser(struct Window *window, const gchar *target_url, struct Client *old_browser)
{
	struct Client *browser = malloc(sizeof(struct Client));
	target_url             = badwolf_ensure_uri_scheme(target_url, (old_browser == NULL));
	char *badwolf_l10n     = NULL;

	WebKitWebContext *web_context = NULL;

	if(browser == NULL) return NULL;

	browser->window     = window;
	browser->context_id = old_browser == NULL ? context_id_counter++ : old_browser->context_id;
	browser->box        = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_name(browser->box, "browser__box");

	browser->toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_name(browser->toolbar, "browser__toolbar");

	browser->back =
	    gtk_button_new_from_icon_name("go-previous-symbolic", GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_widget_set_name(browser->back, "browser__back");

	browser->forward = gtk_button_new_from_icon_name("go-next-symbolic", GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_widget_set_name(browser->forward, "browser__forward");

	GtkWidget *toolbar_separator = gtk_separator_new(GTK_ORIENTATION_VERTICAL);

	browser->javascript = gtk_toggle_button_new_with_mnemonic(_("_JS"));
	gtk_widget_set_name(browser->javascript, "browser__javascript");
	gtk_widget_set_tooltip_text(browser->javascript, _("Toggle javascript"));
	gtk_button_set_relief(GTK_BUTTON(browser->javascript), GTK_RELIEF_NONE);

	browser->auto_load_images = gtk_toggle_button_new_with_mnemonic(_("_IMG"));
	gtk_widget_set_name(browser->auto_load_images, "browser__load_images");
	gtk_widget_set_tooltip_text(browser->auto_load_images, _("Toggle loading images automatically"));
	gtk_button_set_relief(GTK_BUTTON(browser->auto_load_images), GTK_RELIEF_NONE);

	browser->location = gtk_entry_new();
	gtk_widget_set_name(browser->location, "browser__location");

	GtkWidget *print =
	    gtk_button_new_from_icon_name("document-print-symbolic", GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_widget_set_name(browser->back, "browser__print");

	browser->statusbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_name(browser->statusbar, "browser__statusbar");
	browser->search = gtk_search_entry_new();
	gtk_widget_set_name(browser->search, "browser__search");
	browser->statuslabel = gtk_label_new(NULL);
	gtk_widget_set_name(browser->statuslabel, "browser__statuslabel");

	if(old_browser == NULL)
	{
		WebKitWebsiteDataManager *website_data_manager = webkit_website_data_manager_new_ephemeral();
		webkit_website_data_manager_set_itp_enabled(website_data_manager, TRUE);

		web_context = webkit_web_context_new_with_website_data_manager(website_data_manager);
		g_object_unref(website_data_manager);
		webkit_web_context_set_sandbox_enabled(web_context, TRUE);
		webkit_web_context_set_web_extensions_directory(web_context, web_extensions_directory);

		g_signal_connect(G_OBJECT(web_context),
		                 "download-started",
		                 G_CALLBACK(web_contextCb_download_started),
		                 browser);

		/* flawfinder: ignore. Consider that g_strsplit is safe enough */
		badwolf_l10n = getenv("BADWOLF_L10N");

		if(badwolf_l10n != NULL)
		{
			gchar **languages = g_strsplit(badwolf_l10n, ":", -1);
			webkit_web_context_set_spell_checking_languages(web_context, (const gchar *const *)languages);
			g_strfreev(languages);

			webkit_web_context_set_spell_checking_enabled(web_context, TRUE);
		}
	}

	WebKitSettings *settings = webkit_settings_new_with_settings(BADWOLF_WEBKIT_SETTINGS);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(browser->javascript),
	                             webkit_settings_get_enable_javascript_markup(settings));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(browser->auto_load_images),
	                             webkit_settings_get_auto_load_images(settings));

	browser->webView = WEBKIT_WEB_VIEW(g_object_new(WEBKIT_TYPE_WEB_VIEW,
	                                                "web-context",
	                                                web_context,
	                                                "related-view",
	                                                old_browser == NULL ? NULL : old_browser->webView,
	                                                "settings",
	                                                settings,
	                                                "user-content-manager",
	                                                window->content_manager,
	                                                NULL));

	gtk_widget_set_name(GTK_WIDGET(browser->webView), "browser__webView");
	if(old_browser == NULL)
	{
		g_object_unref(web_context);
	}
	g_object_unref(settings);

	gtk_box_pack_start(
	    GTK_BOX(browser->toolbar), GTK_WIDGET(browser->back), FALSE, FALSE, BADWOLF_TOOLBAR_PADDING);
	gtk_box_pack_start(GTK_BOX(browser->toolbar),
	                   GTK_WIDGET(browser->forward),
	                   FALSE,
	                   FALSE,
	                   BADWOLF_TOOLBAR_PADDING);
	gtk_box_pack_start(GTK_BOX(browser->toolbar),
	                   toolbar_separator,
	                   FALSE,
	                   FALSE,
	                   BADWOLF_TOOLBAR_SEPARATOR_PADDING);
	gtk_box_pack_start(GTK_BOX(browser->toolbar),
	                   GTK_WIDGET(browser->javascript),
	                   FALSE,
	                   FALSE,
	                   BADWOLF_TOOLBAR_PADDING);
	gtk_box_pack_start(GTK_BOX(browser->toolbar),
	                   GTK_WIDGET(browser->auto_load_images),
	                   FALSE,
	                   FALSE,
	                   BADWOLF_TOOLBAR_PADDING);
	gtk_box_pack_start(GTK_BOX(browser->toolbar),
	                   GTK_WIDGET(browser->location),
	                   TRUE,
	                   TRUE,
	                   BADWOLF_TOOLBAR_PADDING);
	gtk_box_pack_start(GTK_BOX(browser->toolbar), print, FALSE, FALSE, BADWOLF_TOOLBAR_PADDING);

	gtk_container_set_focus_child(GTK_CONTAINER(browser->box), browser->toolbar);
	gtk_container_set_focus_child(GTK_CONTAINER(browser->toolbar), browser->location);

	gtk_box_pack_start(
	    GTK_BOX(browser->box), GTK_WIDGET(browser->toolbar), FALSE, FALSE, BADWOLF_BOX_PADDING);
	gtk_box_pack_start(
	    GTK_BOX(browser->box), GTK_WIDGET(browser->webView), TRUE, TRUE, BADWOLF_BOX_PADDING);

	gtk_box_pack_start(
	    GTK_BOX(browser->box), GTK_WIDGET(browser->statusbar), FALSE, FALSE, BADWOLF_BOX_PADDING);

	gtk_box_pack_start(GTK_BOX(browser->statusbar),
	                   GTK_WIDGET(browser->search),
	                   FALSE,
	                   FALSE,
	                   BADWOLF_STATUSBAR_PADDING);
	gtk_box_pack_start(GTK_BOX(browser->statusbar),
	                   GTK_WIDGET(browser->statuslabel),
	                   FALSE,
	                   FALSE,
	                   BADWOLF_STATUSBAR_PADDING);

	gtk_widget_set_halign(browser->statusbar, GTK_ALIGN_START);

	gtk_label_set_single_line_mode(GTK_LABEL(browser->statuslabel), TRUE);
	gtk_label_set_ellipsize(GTK_LABEL(browser->statuslabel), BADWOLF_STATUSLABEL_ELLIPSIZE);

	gtk_entry_set_text(GTK_ENTRY(browser->location), target_url);
	gtk_entry_set_input_purpose(GTK_ENTRY(browser->location), GTK_INPUT_PURPOSE_URL);

	gtk_entry_set_placeholder_text(GTK_ENTRY(browser->search), _("search in current page"));

	/* signals for back/forward buttons */
	g_signal_connect(browser->back, "clicked", G_CALLBACK(backCb_clicked), browser);
	g_signal_connect(browser->forward, "clicked", G_CALLBACK(forwardCb_clicked), browser);
	/* prevents GtkNotebook from spawning it's context-menu */
	g_signal_connect(
	    browser->back, "button-press-event", G_CALLBACK(widgetCb_drop_button3_event), NULL);
	g_signal_connect(
	    browser->back, "button-release-event", G_CALLBACK(widgetCb_drop_button3_event), NULL);
	g_signal_connect(
	    browser->forward, "button-press-event", G_CALLBACK(widgetCb_drop_button3_event), NULL);
	g_signal_connect(
	    browser->forward, "button-release-event", G_CALLBACK(widgetCb_drop_button3_event), NULL);

	/* signals for javacript toggle widget */
	g_signal_connect(browser->javascript, "toggled", G_CALLBACK(javascriptCb_toggled), browser);
	/* prevents GtkNotebook from spawning it's context-menu */
	g_signal_connect(
	    browser->javascript, "button-press-event", G_CALLBACK(widgetCb_drop_button3_event), NULL);
	g_signal_connect(
	    browser->javascript, "button-release-event", G_CALLBACK(widgetCb_drop_button3_event), NULL);

	/* signals for auto_load_images toggle widget */
	g_signal_connect(
	    browser->auto_load_images, "toggled", G_CALLBACK(auto_load_imagesCb_toggled), browser);
	/* prevents GtkNotebook from spawning it's context-menu */
	g_signal_connect(browser->auto_load_images,
	                 "button-press-event",
	                 G_CALLBACK(widgetCb_drop_button3_event),
	                 NULL);
	g_signal_connect(browser->auto_load_images,
	                 "button-release-event",
	                 G_CALLBACK(widgetCb_drop_button3_event),
	                 NULL);

	/* signals for location entry widget */
	g_signal_connect(browser->location, "activate", G_CALLBACK(locationCb_activate), browser);

	/* signals for print button */
	g_signal_connect(print, "clicked", G_CALLBACK(printCb_clicked), browser);
	/* prevents GtkNotebook from spawning it's context-menu */
	g_signal_connect(print, "button-press-event", G_CALLBACK(widgetCb_drop_button3_event), NULL);
	g_signal_connect(print, "button-release-event", G_CALLBACK(widgetCb_drop_button3_event), NULL);

	/* signals for WebView widget */
	g_signal_connect(browser->webView,
	                 "web-process-terminated",
	                 G_CALLBACK(WebViewCb_web_process_terminated),
	                 browser);
	g_signal_connect(browser->webView, "notify::uri", G_CALLBACK(WebViewCb_notify__uri), browser);
	g_signal_connect(browser->webView, "notify::title", G_CALLBACK(WebViewCb_notify__title), browser);
	g_signal_connect(browser->webView,
	                 "notify::is-playing-audio",
	                 G_CALLBACK(WebViewCb_notify__is__playing__audio),
	                 browser);
	g_signal_connect(browser->webView,
	                 "mouse-target-changed",
	                 G_CALLBACK(WebViewCb_mouse_target_changed),
	                 browser);
	g_signal_connect(browser->webView,
	                 "notify::estimated-load-progress",
	                 G_CALLBACK(WebViewCb_notify__estimated_load_progress),
	                 browser);
	g_signal_connect(browser->webView, "create", G_CALLBACK(WebViewCb_create), browser);
	g_signal_connect(browser->webView, "close", G_CALLBACK(WebViewCb_close), browser);

	g_signal_connect(
	    browser->webView, "key-press-event", G_CALLBACK(WebViewCb_key_press_event), browser);
	g_signal_connect(browser->webView, "scroll-event", G_CALLBACK(WebViewCb_scroll_event), browser);
	g_signal_connect(
	    browser->webView, "permission-request", G_CALLBACK(WebViewCb_permission_request), NULL);
	g_signal_connect(browser->webView, "decide-policy", G_CALLBACK(WebViewCb_decide_policy), browser);
	g_signal_connect(browser->webView,
	                 "load-failed-with-tls-errors",
	                 G_CALLBACK(WebViewCb_load_failed_with_tls_errors),
	                 browser);
	g_signal_connect(browser->webView, "load-changed", G_CALLBACK(WebViewCb_load_changed), browser);

	/* signals for search widget */
	g_signal_connect(browser->search, "next-match", G_CALLBACK(SearchEntryCb_next__match), browser);
	g_signal_connect(
	    browser->search, "previous-match", G_CALLBACK(SearchEntryCb_previous__match), browser);
	g_signal_connect(
	    browser->search, "search-changed", G_CALLBACK(SearchEntryCb_search__changed), browser);
	g_signal_connect(browser->search, "stop-search", G_CALLBACK(SearchEntryCb_stop__search), browser);

	/* signals for box container */
	g_signal_connect(browser->box, "key-press-event", G_CALLBACK(boxCb_key_press_event), browser);

	if(old_browser == NULL) webkit_web_view_load_uri(browser->webView, target_url);

	return browser;
}

/* badwolf_new_tab: Inserts struct Client *browser in GtkNotebook *notebook 
 * and optionally switches selected tab to it.
 *
 * returns:
 * 0  : Ran successfully
 * -1 : Failed to insert a page for browser->box
 * -2 : browser is NULL
 */
int
badwolf_new_tab(GtkNotebook *notebook, struct Client *browser, bool auto_switch)
{
	gint current_page = gtk_notebook_get_current_page(notebook);
	gchar *title      = _("New tab");

	if(browser == NULL) return -2;

	gtk_widget_show_all(browser->box);

	if(gtk_notebook_insert_page(notebook, browser->box, NULL, (current_page + 1)) == -1) return -1;

	gtk_notebook_set_tab_reorderable(notebook, browser->box, TRUE);
	gtk_notebook_set_tab_label(notebook, browser->box, badwolf_new_tab_box(title, browser));
	gtk_notebook_set_menu_label_text(GTK_NOTEBOOK(notebook), browser->box, title);

	gtk_widget_queue_draw(GTK_WIDGET(notebook));

	if(auto_switch)
	{
		gtk_notebook_set_current_page(notebook, gtk_notebook_page_num(notebook, browser->box));
	}

	return 0;
}

static void
new_tabCb_clicked(GtkButton *UNUSED(new_tab), gpointer user_data)
{
	struct Window *window  = (struct Window *)user_data;
	struct Client *browser = new_browser(window, NULL, NULL);

	badwolf_new_tab(GTK_NOTEBOOK(window->notebook), browser, TRUE);
}

static void
closeCb_clicked(GtkButton *UNUSED(close), gpointer user_data)
{
	struct Client *browser = (struct Client *)user_data;

	webkit_web_view_try_close(browser->webView);
}

static void
notebookCb_switch__page(GtkNotebook *notebook,
                        GtkWidget *page,
                        guint UNUSED(page_num),
                        gpointer user_data)
{
	struct Window *window = (struct Window *)user_data;
	GtkWidget *label      = gtk_notebook_get_tab_label(notebook, page);

	// TODO: Maybe find a better way to store the title
	gtk_window_set_title(GTK_WINDOW(window->main_window), gtk_widget_get_tooltip_text(label));
}

void
content_managerCb_ready(GObject *UNUSED(store), GAsyncResult *result, gpointer user_data)
{
	struct Window *window = (struct Window *)user_data;
	GError *err           = NULL;

	WebKitUserContentFilter *filter =
	    webkit_user_content_filter_store_load_finish(window->content_store, result, &err);

	if(filter == NULL)
	{
		if(err == NULL)
		{
			fprintf(stderr, _("badwolf: failed to load content-filter, err: [%d] %s\n"), -1, "unknown");
		}
		else
		{
			fprintf(stderr,
			        _("badwolf: failed to load content-filter, err: [%d] %s\n"),
			        err->code,
			        err->message);
		}
	}
	else
	{
		fprintf(stderr, _("badwolf: content-filter loaded, adding to content-manager…\n"));
		webkit_user_content_manager_add_filter(window->content_manager, filter);
	}
}

static void
storeCb_finish(WebKitUserContentFilterStore *UNUSED(store),
               GAsyncResult *result,
               gpointer user_data)
{
	struct Window *window = (struct Window *)user_data;
	GError *err           = NULL;

	WebKitUserContentFilter *filter =
	    webkit_user_content_filter_store_save_finish(window->content_store, result, &err);

	if(filter == NULL)
	{
		if(err == NULL)
		{
			fprintf(stderr,
			        _("badwolf: failed to compile content-filters.json, err: [%d] %s\n"),
			        -1,
			        "unknown");
		}
		else
		{
			fprintf(stderr,
			        _("badwolf: failed to compile content-filters.json, err: [%d] %s\n"),
			        err->code,
			        err->message);
		}
	}
	else
	{
		webkit_user_content_filter_store_load(
		    window->content_store, "a", NULL, content_managerCb_ready, window);
	}
}

int
main(int argc, char *argv[])
{
	struct Window *window = &(struct Window){NULL, NULL, NULL, NULL, NULL, NULL};
	GApplication *application;
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, DATADIR "/locale");
	bind_textdomain_codeset(PACKAGE, "UTF-8");
	textdomain(PACKAGE);

	application = g_application_new("me.hacktivis.badwolf",
	                                G_APPLICATION_HANDLES_COMMAND_LINE |
	                                    G_APPLICATION_SEND_ENVIRONMENT | G_APPLICATION_NON_UNIQUE);
	g_application_register(application, NULL, NULL);
	//g_application_activate(application);

	gtk_init(&argc, &argv);

	fprintf(stderr, _("Running Badwolf version: %s\n"), version);
	fprintf(stderr,
	        _("Buildtime WebKit version: %d.%d.%d\n"),
	        WEBKIT_MAJOR_VERSION,
	        WEBKIT_MINOR_VERSION,
	        WEBKIT_MICRO_VERSION);
	fprintf(stderr,
	        _("Runtime WebKit version: %d.%d.%d\n"),
	        webkit_get_major_version(),
	        webkit_get_minor_version(),
	        webkit_get_micro_version());

	web_extensions_directory =
	    g_build_filename(g_get_user_data_dir(), "badwolf", "webkit-web-extension", NULL);
	fprintf(stderr, _("webkit-web-extension directory set to: %s\n"), web_extensions_directory);

	g_object_ref(bookmarks_completion_model);

	window->main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	window->notebook    = gtk_notebook_new();
	window->new_tab = gtk_button_new_from_icon_name("tab-new-symbolic", GTK_ICON_SIZE_SMALL_TOOLBAR);
	window->downloads_tab   = badwolf_downloads_tab_new();
	window->content_manager = webkit_user_content_manager_new();

	load_userscripts(window->content_manager);

	gchar *contentFilterPath =
	    g_build_filename(g_get_user_config_dir(), g_get_prgname(), "content-filters.json", NULL);
	GFile *contentFilterFile = g_file_new_for_path(contentFilterPath);
	fprintf(stderr, _("content-filters file set to: %s\n"), contentFilterPath);

	gchar *filtersPath = g_build_filename(g_get_user_cache_dir(), g_get_prgname(), "filters", NULL);
	window->content_store = webkit_user_content_filter_store_new(filtersPath);

	webkit_user_content_filter_store_save_from_file(window->content_store,
	                                                "a",
	                                                contentFilterFile,
	                                                NULL,
	                                                (GAsyncReadyCallback)storeCb_finish,
	                                                window);

	gtk_window_set_default_size(
	    GTK_WINDOW(window->main_window), BADWOLF_DEFAULT_WIDTH, BADWOLF_DEFAULT_HEIGHT);
	gtk_window_set_role(GTK_WINDOW(window->main_window), "browser");
	gtk_window_set_icon_name(GTK_WINDOW(window->main_window), "badwolf");

	gchar *provider_path_app = g_build_filename(DATADIR, "interface.css", NULL);
	/* flawfinder: ignore, just a presence check */
	if(access(provider_path_app, R_OK) == 0)
	{
		GtkCssProvider *css_provider_app = gtk_css_provider_new();
		gtk_css_provider_load_from_path(css_provider_app, provider_path_app, NULL);
		gtk_style_context_add_provider_for_screen(
		    gtk_widget_get_screen(GTK_WIDGET(window->main_window)),
		    GTK_STYLE_PROVIDER(css_provider_app),
		    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	}
	g_free(provider_path_app);

	gchar *provider_path_user =
	    g_build_filename(g_get_user_data_dir(), "badwolf", "interface.css", NULL);
	/* flawfinder: ignore, just a presence check */
	if(access(provider_path_user, R_OK) == 0)
	{
		GtkCssProvider *css_provider_user = gtk_css_provider_new();
		gtk_css_provider_load_from_path(css_provider_user, provider_path_user, NULL);
		gtk_style_context_add_provider_for_screen(
		    gtk_widget_get_screen(GTK_WIDGET(window->main_window)),
		    GTK_STYLE_PROVIDER(css_provider_user),
		    GTK_STYLE_PROVIDER_PRIORITY_USER);
	}
	g_free(provider_path_user);

	gtk_widget_set_tooltip_text(window->new_tab, _("Open new tab"));

	gtk_notebook_set_action_widget(GTK_NOTEBOOK(window->notebook), window->new_tab, GTK_PACK_END);
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(window->notebook), TRUE);
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(window->notebook), BADWOLF_TAB_POSITION);
	gtk_notebook_popup_enable(GTK_NOTEBOOK(window->notebook));

	gtk_container_add(GTK_CONTAINER(window->main_window), window->notebook);
	gtk_widget_queue_draw(window->notebook);

	badwolf_downloads_tab_attach(window);

	g_signal_connect(
	    window->main_window, "key-press-event", G_CALLBACK(main_windowCb_key_press_event), window);

	g_signal_connect(window->main_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
	g_signal_connect(window->new_tab, "clicked", G_CALLBACK(new_tabCb_clicked), window);
	g_signal_connect(window->notebook, "switch-page", G_CALLBACK(notebookCb_switch__page), window);

	gtk_widget_show(window->new_tab);
	gtk_widget_show_all(window->main_window);

	if(argc == 1)
		badwolf_new_tab(GTK_NOTEBOOK(window->notebook), new_browser(window, NULL, NULL), FALSE);
	else
		for(int i = 1; i < argc; ++i)
			badwolf_new_tab(GTK_NOTEBOOK(window->notebook), new_browser(window, argv[i], NULL), FALSE);

	gtk_notebook_set_current_page(GTK_NOTEBOOK(window->notebook), 1);

	gtk_main();

	g_object_unref(bookmarks_completion_model);

#if 0
	/* TRANSLATOR Ignore this entry. Done for forcing Unicode in xgettext. */
	_("ø");
#endif

	return 0;
}
