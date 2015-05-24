#! /bin/sh

aclocal --install -I m4 && automake --gnu --add-missing \
	&& autoconf && ./configure $@
