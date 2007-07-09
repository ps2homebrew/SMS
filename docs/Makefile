# Makefile for generating the HTML documentation

# List of languages to be processed (new langs added here - space between)
SUBDIRS = en pt ru

# Generated chunked HTML files go here.
HTML_CHUNKED = HTML

# Generated single HTML files go here.
HTML_SINGLE = HTML-single

# Zipfiles go here
ZIP = zip

# Generated PDF files go here
PDF = pdf

# The CSS stylesheet will be used for all the HTML outputs
HTML_STYLESHEET ?= stylesheets/default.css

# The 'profiling' XSL stylesheet which masks all but the desired language.
PROFILING_XSL = stylesheets/profile.xsl

# The stylesheet which puts all the content together in one big html file.
HTML_SINGLE_XSL := stylesheets/html-single.xsl
XSL_DEPS := $(HTML_SINGLE_XSL) stylesheets/html-common.xsl

# The stylesheet which breaks up the html into section chunks
HTML_CHUNK_XSL := stylesheets/html-chunk.xsl
CHUNK_XSL_DEPS := $(HTML_CHUNK_XSL) stylesheets/html-common.xsl

# Temporary intermediate xml files go here
TEMP_DIR = temp

.PHONY: all
all: html-chunked html-single

.PHONY: help
help:
	@echo "Targets:"
	@echo "********"
	@echo "all                   : Build everything (default)."
	@echo "html-single           : Build HTML documentation (single file)."
	@echo "html-single-LANG      : As above, but only one language."
	@echo "html-chunked          : Build HTML documentation (multiple files)."
	@echo "html-chunked-LANG     : As above, but only one language."
	@echo "zip-html-single       : Put HTML documentation into zip files."
	@echo "zip-html-single-LANG  : As above, but only one language."
	@echo "zip-html-chunked      : Put HTML documentation into zip files."
	@echo "zip-html-chunked-LANG : As above, but only one language."
	@echo "xmllint               : Check syntax of all xml files."
	@echo "clean-html-single     : Purge the 'HTML-single' directory."
	@echo "clean-html-chunked    : Purge the 'HTML' directory."
	@echo "clean                 : Remove all generated files."
	@echo "web                   : Make everything needed to update the website."
	@echo "Substitute LANG for one of $(SUBDIRS)"

.PHONY: html-chunked chunked-dir chunked-dir-$(1)
html-chunked: xsltproc.sh xmllint.sh chunked-dir $(addprefix html-chunked-,$(SUBDIRS))
chunked-dir:
	test -d $(HTML_CHUNKED) || mkdir $(HTML_CHUNKED)

temp-dir:
	test -d $(TEMP_DIR) || mkdir $(TEMP_DIR)

.PHONY: html-single single-dir
html-single: xsltproc.sh xmllint.sh single-dir $(addprefix html-single-,$(SUBDIRS))

web: html-single html-chunked $(addprefix zip-html-single-,$(SUBDIRS)) \
  $(addprefix zip-html-chunked-,$(SUBDIRS))

single-dir:
	test -d $(HTML_SINGLE) || mkdir $(HTML_SINGLE)

zip-dir:
	test -d $(ZIP) || mkdir $(ZIP)

pdf-dir:
	test -d $(PDF) || mkdir $(PDF)

.PHONY: xmllint
xmllint: xmllint.sh

.PHONY: clean-temp clean-html-chunked clean-html-single clean-zip
clean-temp:
	-rm -rf $(TEMP_DIR)

clean-html-chunked:
	-rm -rf $(HTML_CHUNKED)

clean-html-single:
	-rm -rf $(HTML_SINGLE)

clean-zip:
	-rm -rf $(ZIP)

.PHONY: releaseclean distclean clean
releaseclean: clean-temp $(addprefix releaseclean-,$(SUBDIRS))
	-rm -f html-chunk.xsl html-single.xsl xsltproc.sh xmllint.sh

distclean: clean releaseclean

xsltproc.sh xmllint.sh:
	sh configure

clean: clean-temp clean-html-chunked clean-html-single clean-zip

.PHONY: images-single images-chunked
images-single: single-dir
	test -d $(HTML_SINGLE)/images || mkdir $(HTML_SINGLE)/images
	cp images/*.png $(HTML_SINGLE)/images/

images-chunked: chunked-dir
	test -d $(HTML_CHUNKED)/images || mkdir $(HTML_CHUNKED)/images
	cp images/*.png $(HTML_CHUNKED)/images/

define lang-def
.PHONY: html-chunked-$(1) html-single-$(1) xmllint-$(1) \
  distclean-$(1) releaseclean-$(1) clean-$(1) single-dir-$(1) chunked-dir-$(1)

chunked-dir-$(1): chunked-dir
	test -d $(HTML_CHUNKED)/$(1) || mkdir $(HTML_CHUNKED)/$(1)

single-dir-$(1): single-dir
	test -d $(HTML_SINGLE)/$(1) || mkdir $(HTML_SINGLE)/$(1)

pdf-dir-$(1): pdf-dir
	test -d $(PDF)/$(1) || mkdir $(PDF)/$(1)

$(TEMP_DIR)/sms-$(1).xml: temp-dir xmllint.sh
	@echo validating xml source file...
	./xmllint.sh src/SMS.xml
	@echo profiling for language '$(1)'...
	./xsltproc.sh $(TEMP_DIR)/sms-$(1).xml $(PROFILING_XSL) src/SMS.xml $(1)
	@echo validating xml intermediate file...
	./xmllint.sh $(TEMP_DIR)/sms-$(1).xml

html-chunked-$(1): $(XSL_DEPS) xsltproc.sh xmllint.sh chunked-dir \
    chunked-dir-$(1) images-chunked $(TEMP_DIR)/sms-$(1).xml
	-rm -f $(HTML_CHUNKED)/$(1)/*
	@echo creating chunked html output...
	cp -f $(HTML_STYLESHEET) $(HTML_CHUNKED)/$(1)/
	./xsltproc.sh $(HTML_CHUNKED)/$(1)/ $(HTML_CHUNK_XSL) $(TEMP_DIR)/sms-$(1).xml $(1)

html-single-$(1): $(XSL_DEPS) xsltproc.sh xmllint.sh single-dir \
    single-dir-$(1) images-single $(TEMP_DIR)/sms-$(1).xml
	-rm -f $(HTML_SINGLE)/$(1)/SMS.html
	cp -f $(HTML_STYLESHEET) $(HTML_SINGLE)/$(1)
	@echo creating single html output...
	./xsltproc.sh $(HTML_SINGLE)/$(1)/SMS.html $(HTML_SINGLE_XSL) $(TEMP_DIR)/sms-$(1).xml $(1)

pdf-$(1): $(XSL_DEPS) xsltproc.sh xmllint.sh pdf-dir \
    pdf-dir-$(1) $(TEMP_DIR)/sms-$(1).xml
	@echo pdf is not yet supported by the makfile (sorry).
	#./xsltproc

zip-html-chunked-$(1): zip-dir html-chunked-$(1)
	zip -r $(ZIP)/sms-html-chunked-$(1).zip $(HTML_CHUNKED)/$(1) \
    $(HTML_CHUNKED)/images

zip-html-single-$(1): zip-dir html-single-$(1)
	zip -r $(ZIP)/sms-html-single-$(1).zip $(HTML_SINGLE)/$(1) \
    $(HTML_SINGLE)/images

endef

$(foreach lang, $(SUBDIRS),$(eval $(call lang-def,$(lang))))


stylesheets/html-chunk.xsl stylesheets/html-single.xsl:
	sh configure
