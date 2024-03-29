# SMS - Playstation 2 Simple Media System
## Information for Maintainers
---

General information
-------------------
 - First of all, many thanks for helping to maintain the documentation for SMS - your input is greatly appreciated!  We hope that by maintaining documentation in multiple languages, SMS will be more accessible to a wider audience.

 - The documentation format I've chosen to use is DocBook.  This is an xml format widely used in the open source / linux community.  The basic principle is that the source text is stored in a common format, and a series of automatic commands run by a makefile are used to create output documentation in a variety of formats.

 - The different translations of the text are stored all in the same files as the primary translation. The different translations are marked by the tag paramter lang="xx", (where xx is an ISO 639 language code)for example:

```html
<para lang="en">Here is my paragraph in english</para>
<para lang="de">Hier ist mein Punkt auf Deutsch</para>
```
   A process known as 'profiling' seperates the text for a particular language during processing. Text where the language is not specified will appear in all translations.

 - The prefered procedure for submitting translations or translation updates
   is:
   1) Download latest version of SMS source from here (git@github.com:ps2homebrew/SMS.git) This includes the documentation source.

   2) Modify your local copy

   3) If possible, revalidate and generate the output (requires linux or cygwin) to do this.  A good quick test is
```sh
$ make html-single-xx # (where xx is the language).
```
This will build the html single file version, but only for the language you  specified.
For more options go to the docs folder and type:
```sh
$ make help
```
   5) Create a patchfile of your changes and email it back to the maintainer:
      ps2@davet.org.

 - The ./configure script, css stylesheet and part of the makefile for this documentation were based on the MPlayer documentation (http://www.mplayerhq.hu). Many thanks to them for the info which helped to get this whole DocBook thing figured out in my head!  Technically, this means that the build scripts  are under the GPL license version 2 as well: (http://www.gnu.org/licenses/gpl.txt) for details.   Also, some ideals for the XML layout were based on the gimp documentation project.  Many thanks for their help too (http://docs.gimp.org/).


File organistaion/structure
---------------------------
 - The file `docs/src/SMS.xml` is the *base* or main document which referemces all other documents.  This defines the order fo the sections.

 - Each xml file in the `docs/src` folder represents the beginning (or whole of) a chapter, with no sub1 sections.

 - All other source files are in a subdirectory named after the chapter they are found in.  There is only one level of subdirectories.  Therefore all files are `docs/src/`chaptername.xml or `docs/src/chaptername/sectionname.xml` and no deeper.

 - Generated files are created in `docs/HTML/` and `docs/HTML-single/` please dont attempt to modify these html files, as your changes will be lost when the documentation is rebuilt.  All changes must be made in the xml files.  Generated translations are stored in subdirectories: `docs/HTML/xx/` and `docs/HTML-single/xx/`

 - The html stylesheet (`default.css`) is the same for all languages. It is copied from the stylesheet folder.

 - Image files must be stored in the main images directory (docs/images) or subdirectory, and referenced to this directory using relative paths in the xml (this is to make sure that the images all work when the images directory is copied for distributions.

 - Images are all in png format. Screenshots from the PS2 are all 400x300px.


Rules & notation
----------------
 - The primary language for the documentation is english (and will be english
   for the forseeable future).  Other translations into different languages
   will be translations of the english text into other languages, not the
   other way round.

 - The order of the languages in the source xml files is always:
   en (english) followed by all other ISO639 language codes in alphabetical
   order of language code, i.e.
   en English
   el Greek
   es Spanish
   pt Portuguese

 - Lines should always be kept 78 characters long or less, unless a url or
   particular language forbids line breaks.

 - Block indentation in the XML is two spaces and is preferably done with
   space characters, not hard tabs.

 - If you are ever commiting to SVN, please do not make changes to the docs
   section AND the main c code section of the repository at the same time.
   Usually changes will be submitted via a patch emailed to the maintaner,
   but if you are editing code too please submit those changes as seperate
   commits.   Equally, if you are committing changes to the c code, please
   don't (accidentally or otherwise) commit changes to the docs section.

 - Also, don't commit to SVN any documentation you know fails to
   compile.  If you are really having trouble, email what you've done as
   a patch and explain the problems.


Special syntax used for this documentation
------------------------------------------

#text surrounded by hashes#
This is text that has not yet been translated from the primary language.  It
will therefore appear in the final text as the original english text.  This
will be used to mark out sections that have not yet been translated into the
new language.  It is the job of the translators to convert these blocks of
text into the language they are tranlstating to.

~text surrounded by tildes~
This is text that has been translated in the past, but for whatever reason,
the english language text for the section has been changed.  The change is
significant, i.e. more than just a typo correction in the primary language,
so a ~tilded~ section needs to be reviewed.  The maintainer of the english
version will mark all other versions with this syntax when he/she changes
them.  This is effectively a signal to the other maintainers that they need
to re-word the corresponding sections.

If you spot a typo in your own language, you are weclcome to change it at
any point and submit a patch.  Changes such as typos or minor rewordings to
the english version do not require retranslation and will not be given the
~ ~ marks.


How to add a Translation
------------------------

Start off with a decent text editor, e.g. www.context.cx, or any editor that
does xml syntax highlighting.

Download the sms source from SVN, using an svn client (e.g. TortoiseSVN).
All of the documentation is in the docs directory.

If you are able, set up your computer to build the documentation (see
README.buildingdocumentation).  If you aren't able to build the documentation
don't worry.  Your contributions are still wanted!  It just means the changes
will need to be validated and compiled by someone else.

Open up one of the xml files (e.g. docs/src/usage.xml), and take a look at
the content - you will notice XML tags, many of which start with

`<para lang="en">.....`

These are the original english phrases that need to be translated.  In order
to create a translation, copy the whole section (from

`<para lang="en"> to </para>):`

And paste it all directly below the first english paragraph. Now change
the 'en' to the ISO639 language code you are tranlsating to. You should now
have two copies of the english text, one starting with `lang="en"`, the other
starting with `lang="de"` (for example - German).

Modify the second english copy and translate it into your chosen language.

Now repeat for all other `<para>` and `<phrase>` tags in the xml document.

If this is the first time you've done this, i suggest you submit the first
page back to me (use svn client to create a patch file from your local copy
and email it to ps2@davet.org).  I can then check that what you've done is
ok before you attempt the rest of the xml files.  (This should reduce the
ammount of time you might waste!)

The process needs to ultimately be repeated for all the .xml files in
the `docs/src` directory & subdirectories.
Once the bulk of the translation is complete, the xml patch files should be
submitted back to me (preferably after validating them), then
i'll put it back into SVN.  Remember, only valid xml should go back into
SVN, so if you can't run the validation tool, you shouldn't commit to svn,
instead, email a patch.  Please also advise how you would like to be
credited on the website etc. (e.g. name or nickname etc).

Wherever you see a tag mid-way through a para section (e.g.
`<emphasis role="bold">blue</emphasis>`  ) , this is to mark up
the text in some way.  You will need to preserve these tags in your
translated version of each paragraph.   Do not change the tags or the
properties themselves, only the content.  (i.e. change the word 'blue'
to your tranlsation of the colour 'blue'.

And finally...

MANY THANKS FOR HELPING TO SUPPORT THE SMS MANUAL!
