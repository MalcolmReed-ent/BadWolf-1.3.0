// SPDX-FileCopyrightText: 2019-2022 Badwolf Authors <https://hacktivis.me/projects/badwolf>
// SPDX-License-Identifier: BSD-3-Clause

#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED
/* BADWOLF_TAB_POSITION: Position of the tab listing, can be one of:
 * - GTK_POS_TOP
 * - GTK_POS_BOTTOM
 * - GTK_POS_RIGHT
 * - GTK_POS_LEFT
 *
 * See https://docs.gtk.org/gtk3/enum.PositionType.html
 */
#define BADWOLF_TAB_POSITION GTK_POS_TOP

/* BADWOLF_TAB_LABEL_CHARWIDTH: Amount of characters the tab label text fits
 * Quite conflicts with BADWOLF_TAB_BOX_WIDTH, recommended to only define one
 */
#define BADWOLF_TAB_LABEL_CHARWIDTH 26

/* BADWOLF_TAB_BOX_WIDTH: Requested width (in pixels) for the whole tab
 * Quite conflicts with BADWOLF_TAB_LABEL_CHARWIDTH, recommended to only define one
 */
//#define BADWOLF_TAB_BOX_WIDTH 120

// BADWOLF_TAB_HEXPAND: Should the tab try to fill the available horizontal space?
#define BADWOLF_TAB_HEXPAND FALSE

/* BADWOLF_TAB_LABEL_ELLIPSIZE: pango ellipsize mode of the tab label text, can be one of:
 * - PANGO_ELLIPSIZE_NONE
 * - PANGO_ELLIPSIZE_START
 * - PANGO_ELLIPSIZE_MIDDLE
 * - PANGO_ELLIPSIZE_END
 *
 * See https://docs.gtk.org/Pango/enum.EllipsizeMode.html
 */
#define BADWOLF_TAB_LABEL_ELLIPSIZE PANGO_ELLIPSIZE_MIDDLE

// BADWOLF_BOX_PADDING: Amount of padding between browser’s box (tab child) elements
#define BADWOLF_BOX_PADDING 0

// BADWOLF_TOOLBAR_PADDING: Amount of padding between toolbar elements
#define BADWOLF_TOOLBAR_PADDING 0

// BADWOLF_TOOLBAR_PADDING: Amount of padding between toolbar elements
#define BADWOLF_TOOLBAR_SEPARATOR_PADDING 4

// BADWOLF_STATUSBAR_PADDING: Amount of padding between statusbar elements
#define BADWOLF_STATUSBAR_PADDING 0

// BADWOLF_DOWNLOAD_PADDING: Amount of padding between download list row-elements
#define BADWOLF_DOWNLOAD_PADDING 5

/* BADWOLF_DEFAULT_WIDTH / BADWOLF_DEFAULT_HEIGHT:
 * Used to define the default width/height of the window,
 * useful for floating Window Managers, probably useless in tiling ones
 *
 * See https://docs.gtk.org/gtk3/method.Window.set_default_size.html
 */
#define BADWOLF_DEFAULT_WIDTH 800
#define BADWOLF_DEFAULT_HEIGHT 600

/* BADWOLF_WEBKIT_SETTINGS:
 * Used when creating a new view with webkit_settings_new_with_settings the usage is:
 * setting-name, setting-value, setting-name, …, NULL
 *
 * See: https://webkitgtk.org/reference/webkit2gtk/stable/class.Settings.html#properties
 *
 * These Are Their Original Settings
 * ---------------------------------
 * enable-developer-extras | TRUE | This one adds developer options
 * enable-javascript-markup | FALSE | This one enable JS automatically from the toggle button
 */
// clang-format off
#define BADWOLF_WEBKIT_SETTINGS \
	"default-charset", "utf-8", \
	"enable-caret-browsing", FALSE, \
	"enable-developer-extras", FALSE, \
	"enable-dns-prefetching", FALSE, \
	"enable-hyperlink-auditing", FALSE, \
	"enable-java", FALSE, \
	"enable-javascript-markup", TRUE, \
	"enable-javascript", TRUE, \
	"enable-plugins", FALSE, \
	"javascript-can-access-clipboard", FALSE, \
	"javascript-can-open-windows-automatically", FALSE, \
	"media-playback-requires-user-gesture", TRUE, \
	"minimum-font-size", 8, \
	"allow-top-navigation-to-data-urls", FALSE, \
	NULL
// clang-format on

/* BADWOLF_STATUSLABEL_ELLIPSIZE: pango ellipsize mode of the status bar label text, can be one of:
 * - PANGO_ELLIPSIZE_NONE
 * - PANGO_ELLIPSIZE_START
 * - PANGO_ELLIPSIZE_MIDDLE
 * - PANGO_ELLIPSIZE_END
 *
 * See https://docs.gtk.org/Pango/enum.EllipsizeMode.html
 */
#define BADWOLF_STATUSLABEL_ELLIPSIZE PANGO_ELLIPSIZE_MIDDLE

/* BADWOLF_DOWNLOAD_FILE_PATH_ELLIPSIZE: pango ellipsize mode of the download destination path,
 * can be one of:
 * - PANGO_ELLIPSIZE_NONE
 * - PANGO_ELLIPSIZE_START
 * - PANGO_ELLIPSIZE_MIDDLE
 * - PANGO_ELLIPSIZE_END
 *
 * See https://docs.gtk.org/Pango/enum.EllipsizeMode.html
 */
#define BADWOLF_DOWNLOAD_FILE_PATH_ELLIPSIZE PANGO_ELLIPSIZE_MIDDLE

// BADWOLF_LOCATION_INLINE_SELECTION: show selected completion as a selection in location entry
#define BADWOLF_LOCATION_INLINE_SELECTION TRUE

#endif /* CONFIG_H_INCLUDED */
