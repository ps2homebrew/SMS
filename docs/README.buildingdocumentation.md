This file is based on the equivalent file of the MPlayer documentation (hhtp://www.mplayerhq.hu). It is therefore covered by the GPL license version 2.

---

Tools required for building the documentation
---

* GNU make 3.80 or later
* DocBook 4.1.2 or later
* The DocBook XML DTD (also known as DocBk XML)
* DocBook XSL stylesheets -- version 1.50.0 or later is recommended.

I am not quite sure which tools work, but I used the following
ones successfully, so they are required:

* xmllint (part of libxml2) is used for validation.
* xsltproc (part of libxslt1) is used for transforming XML files into HTML
  files. Version 1.0.18 or later is recommended.

It `s also possible to use the Saxon XSLT Processor. The Russian translator
used it (version 6.4.4) for a while. If you have a suitable JavaVM and a
saxon.jar installed somewhere, configure will try to detect them. If
autodetection fails, try to tweak DOCS/xml/configure to get it working and
send us a patch :)

On Red Hat systems you need the following packages:
libxml2, libxslt, docbook-dtds, docbook-style-xsl

On Debian Sarge you will need these packages:
libxml2, libxml2-utils, docbook-xsl, libxslt1.1, docbook, docbook-utils

On Cygwin, you need to run setup.exe click keep, and add these packages:
devel->make, text->docbook-xml43, text->docbook-xsl, text->libxml2,
text->libxslt. You can do this while cygwin is running.

Installing the required tools from source


1) Download libxslt AND libxml2 packages from
   http://xmlsoft.org/XSLT/downloads.html

   Installing them should be straightforward, execute the usual "./configure"
   and "make" then "make install" commands.


2) Download the docbook-xml package from http://www.oasis-open.org/docbook/xml/
   Use the newest version. The URL will be something like this:

	http://www.oasis-open.org/docbook/xml/4.2/docbook-xml-4.2.zip

   Extract this package into a directory, enter it, and execute the following
   commands:

	```sh
   mkdir -p /usr/share/sgml/docbook/dtd/xml/4.2/
	cp -r * /usr/share/sgml/docbook/dtd/xml/4.2/
   ```


3) Download the docbook-xsl package from
   http://prdownloads.sourceforge.net/docbook/

   Use the newest version. The URL will be something like this:

	http://prdownloads.sourceforge.net/docbook/docbook-xsl-1.62.0.tar.gz

   Extract this package into a directory, enter it, and execute the following
   commands:

	```sh
   mkdir -p /usr/share/sgml/docbook/stylesheet/xsl/nwalsh
	cp -r VERSION common html lib \
		/usr/share/sgml/docbook/stylesheet/xsl/nwalsh
   ```


Building the documentation
---

Before trying to build the documentation, run

	make help

to see all available build targets and make your choice. If something goes
wrong, check the Configuration section of the Makefile and adjust the
variables.

The general procedure is:

	sh ./configure
	make html-single-en

This will build the `big html file` version of the english documentation.

`make help` describes more options.

Please look at the output of the configure script, it can help to figure
out what the problems are, if any.
