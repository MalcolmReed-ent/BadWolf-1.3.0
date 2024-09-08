# BadWolf-1.3.0
My deblobbed version of the BadWolf WebKitGTK browser.

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
- Added a desktop.png to view the browser in action
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
  libwebkit2gtk-4.1-dev

## Optional Dependencies, this build you dont need the two, but default Badwolf needs these and some extras listed below
# Bookmark Management
  libxml2-dev

# Localization Support
  gettext (for handling 'locale/fr/LC_MESSAGES/Badwolf/*.mo' files)

## Optional Dependencies, Cont.d
  meson/ninja compiling method
  ed required when compiling with ninja
	
# One line Debian Trixie/SID install command
	sudo apt install libwebkit2gtk-4.1-dev
