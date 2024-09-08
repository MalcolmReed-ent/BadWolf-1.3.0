# BadWolf
## Recent Changes

### Makefile Modifications
- Removed the use of ./configure and meson/ninja for a Makefile
- Removed gettext dependency and installation of localization files (*.mo)
- Removed libxml dependency, which disables bookmark functionality
- Eliminated 'inkscape' flag and related functionality
- Integrated configure.h definitions directly into the Makefile
- Updated installation process to reflect removed components
- Modified compilation process to build directly from source files without creating intermediate .o files

### Source Code and Documentation
- Removed KnowledgeBase.md and interface.md from source folder and Makefile
- Deleted fonts.sh and version.sh scripts from source folder
- Removed decisions.md documentation file
- Deleted badwolf.inkscape.svg file
- Removed bookmarks.c bookmarks.h bookmarks_test.c from source folder and bookmarks.h from badwolf.c
- Added a sloc.png to view the Significant Lines of Code
- Implemented a keybinding in keybindings.c for Ctrl+W to close the current tab.

### Documentation Cleanup
- Retained only the essential manual (badwolf.1)
- Removed all other guides and manuals

### Interface CSS Changes
- Updated interface.css to set the main font to monospace
- Retained original specifications for browser__location, browser__statuslabel, and browser__tabbox__context_label
- Added commented-out sections for additional customizations, allowing easy modification of:
  - Browser window background
  - Tab bar appearance
  - Location bar styling
  - Button designs
  - Status bar formatting
  - Context menu label styling
- Provided flexibility to revert to original settings or apply new styles by uncommenting specific sections

## Required Dependencies
### To successfully build and run BadWolf, you'll need the following libraries:
# Browser Engine
	- libwebkit2gtk-4.1-dev

## Optional Dependencies, this build you dont need the two, but default Badwolf needs these and some extras listed below
# Bookmark Management
	- libxml2-dev

# Localization Support
	- gettext (for handling 'locale/fr/LC_MESSAGES/Badwolf/*.mo' files)

## Optional Dependencies, Cont.d
	- meson/ninja compiling method
	- ed required when compiling with ninja, probably only needed on debian
	
# One line Debian Trixie/SID install command
	sudo apt install libwebkit2gtk-4.1-dev libxml2-dev gettext

```
SPDX-FileCopyrightText: 2019-2022 Badwolf Authors <https://hacktivis.me/projects/badwolf>
SPDX-License-Identifier: BSD-3-Clause
```

Minimalist and privacy-oriented WebKitGTK+ browser.

Homepage: <https://hacktivis.me/projects/badwolf>

The name is a reference to BBC's Doctor Who TV series, I took it simply because I wanted to have a specie in the name, like some other web browsers do, but doesn't go into the “gentle” zone.

## Differences
Comparing from other small WebKit browsers for unixes found in the wild:

- Independent of environment, should just work if GTK and WebKitGTK does
- Storing data should be:
  - explicit and optional (i.e. Applying preferences doesn't imply Saving to disk)
  - not queryabe by WebKit (so the web can't use it)
  - done in a standard format (like XBEL for bookmarks)
- Static UI, no element should be added at runtime, this is to avoid potential tracking via viewport changes
- Small codebase, should be possible to read and understand it completely over an afternoon.
- Does not use modal editing (from vi) as that was designed for editing, not browsing
- UTF-8 encoding by default

Motivation from other clients <https://hacktivis.me/articles/www-client%20are%20broken>

## Contributing
### Translations
You need to have gettext installed. If you want a GUI, poedit exists and Weblate is a good web platform that I might consider hosting at some point.

- Syncing POT file with the source code: ``ninja po/messages.pot``
- Syncing PO file with the POT file: ``ninja po/de.po``
- Initialising a new PO file (example for German, `de_DE`): ``msginit -l de_DE -i po/messages.pot -o po/de.po``

## Contacts / Discussions
- IRC: `#badwolf-browser` on [GeekNode](https://www.geeknode.org/)
- Matrix (bridge): [#badwolf-browser:matrix.fdn.fr](https://matrix.to/#/#badwolf-browser:matrix.fdn.fr)

## Repositories
### git
- Main: <https://hacktivis.me/git/badwolf/>, <git://hacktivis.me/git/badwolf.git>
- Mirror: <https://gitlab.com/lanodan/badWolf.git>, this one can also be used if you prefer tickets/PRs over emails

### release assets
- Main: <https://hacktivis.me/releases/>
- Mirror: <https://gitlab.com/lanodan/badWolf/tags>

- `*.tar.*` files are tarballs archives to be extracted with a program like `tar(1)`, GNU tar and LibArchive bsdtar are known to work.
- `*.sign` files are minisign (OpenBSD `signify(1)` compatible) signatures, they key used for it can be found at <https://hacktivis.me/releases/signify/> as well as other places (feel free to ping me to get it)

## Manual Installation
### Dependencies
- POSIX-compatible Shell (i.e. mrsh, dash, lksh)
- [make](https://www.gnu.org/software/make/)
- C11 Compiler (such as clang or gcc)
- [WebKitGTK](https://webkitgtk.org/), only the latest stable(2.32.0+) is supported
- (optional, bookmarking)[libxml-2.0](http://www.xmlsoft.org/), no known version limitation
- (optional, translating) [po4a](https://po4a.org/) to modify manpage translations
- (optional, translating) gettext implementation (such as GNU Gettext)
- (optional, translating) ed(1), the standard editor
- (optional, lint) [mandoc](https://mdocml.bsd.lv/) (the command) for linting the manpage
- (optional, lint) [shellcheck](https://www.shellcheck.net/) for linting the `./configure` script
- (optional, lint) [flawfinder](https://www.dwheeler.com/flawfinder/) for examining C source code for flaws
- (optional, lint) [reuse](https://reuse.software/) for checking REUSE compliance

Note: Packagers can safely ignore the lint dependencies.

### Compiling
```
make
```

### Installing
```
sudo make install && sudo make clean install
```

Debian users will probably want to use `checkinstall` instead.

An example AppArmor profile is provided at `usr.bin.badwolf`, please do some long runtime checks before shipping it or a modified version, help can be provided but with no support.

If you modify the icons, you'll need `inkscape` (command line only), to regenerate the bitmap versions. It isn't needed for a normal installation as the bitmap icons are bundled.

## Notes
Most of the privacy/security stuff will be done with patches against WebKit as quite a lot isn't into [WebKitSettings](https://webkitgtk.org/reference/webkit2gtk/stable/WebKitSettings.html) and with generic WebKit extensions that should be reusable.
