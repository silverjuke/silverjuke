Localization
================================================================================

Silverjuke uses the "gettext" system for localisation of the strings appearing
in the user interface. This means, you can use all common tools to edit the
files belonging to a localization project. However, for people who do not know
"gettext", here's a brief overview:


**A brief overview**

"gettext" uses two file types: So called "Portable Objects" with the extension
*.po and so called "Machine objects" with the extension *.mo -- the first file
type will be edited during the translation process, and the latter is then
created from it. Only *.mo files are usable from within Silverjuke.

So the steps at a glance are: Download a *.po object (eg. from Github), edit it
and then create a *.mo file which may then be used by Silverjuke.  The edited
*.po file may be shared with other users by uploading eg. to Github.


**What's needed**

To edit *.po files, you only need a text-editor. However, we won't describe
the syntax of *.po files here. An easier way is to use a special program for
this purpose: poEdit is a good choice. So first of all download and install
poEdit from the following site:

http://www.poedit.net

You may decide to use another editor, however, the following steps are described
using poEdit.


**Let's start!**

To start a localisation, please use one of the existing "Portable Object"
(*.po) files. Eg. the file "de.po" contains the english strings as sources and
German strings as the current translation (which will be replaced
by your translation).

After the download, rename the file to the two-letter ISO 639.1
(see http://www.loc.gov/standards/iso639-2/englangn.html ) abbreviation of the
language to create, use eg. "fr.po" for "French" or "es.po" for "Spanish".

Then, just open the file in poEdit and define the new language and the new
country using "Catalog/Settings/Project info", you can also add your e-mail
address there. Moreover, make sure, the "Machine objects" (*.mo) files are
written automatically, see "File/Preferences/Editor/Automatically compile
.mo files on save".

Now you can begin with the translation: poEdit shows the original strings aleft
and the translated strings (still German) aright.

For a first try, change some strings and save your project. Then make sure,
the *.mo file is at the right place and start Silverjuke. In Silverjuke you may
need to switch to the new language by entering the canonical name to
"Advanved Settings/Languages"

If all this works, you can translate all other strings.


**Placeholders and special strings**

Many strings contain placeholders as "%s" or "%i" which will be replaced by
other strings by Silverjuke. You have to leave all placeholders
_in the same order_ in your translation. Also, if possible, leave other special
characters as trailing columns (":"), included pipes ("|"), three points ("...")
or surrounding brackets. The meaning of most placeholders and special characters
is explained in the comment window in poEdit (make sure "View/Show comment
window" is enabled).

Some strings are not really translatable strings but settings for the locale.
The original strings of these strings begin and end with two underscores, eg.
"__DATE_SUNDAY_FIRST__". Again, the meaning and the possible values are
explained in the comment window inside poEdit.


**Publishing your work**

We would be very happy if you publish your work eg. on Github by a
"Pull Request".  The language changes will come along with the next version of
Silverjuke then.

---

Copyright (c) Silverjuke contributors

