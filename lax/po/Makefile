

include ../../Makefile-toinclude

compile:
	@echo "Compiling po files..."
	$(foreach NAME, $(basename $(shell ls *.po)), $(shell msgfmt $(NAME).po -o $(NAME).mo))
	@echo "Done!"

install:
	@echo Installing language files...
	$(foreach NAME, $(basename $(wildcard *.mo)), \
		$(shell	install -D -m755 $(NAME).mo $(PO) $(PREFIX)/share/locale/$(NAME)/LC_MESSAGES/laxkit.mo))
	@echo Done installing language files.

installhere:
	@echo Installing language files...
	$(foreach NAME, $(basename $(wildcard *.mo)), \
		$(shell	install -D -m755 $(NAME).mo $(PO) temp-install/locale/$(NAME)/LC_MESSAGES/laxkit.mo))
	@echo Done installing language files.

update: freshdump
	msgmerge -U laxkit.pot laxkit-freshdump.pot

 #generate POTFILES
potfiles:
	find .. -name '*.cc' > POTFILES

freshdump:
	xgettext -C -d laxkit  --files-from POTFILES -o laxkit-freshdump.pot --keyword=_ --flag=_:1:pass-c-format

clean:
	rm -f *.mo
	rm -f laxkit-freshdump.pot


.PHONY: potfiles rawdump update compile clean all install installhere
