# New ports collection makefile for:   clearsilver
# Date created:        13 April 2003
# Whom:                Brandon Long <blong@fiction.net>
#
# $FreeBSD$
#

PORTNAME=      clearsilver
PORTVERSION=   0.9.7
CATEGORIES=    www python perl ruby
DISTDIR=       /home/blong/ports/distfiles
MASTER_SITES=  http://www.clearsilver.net/downloads/

MAINTAINER=    blong@fiction.net
COMMENT=       ClearSilver is a fast, powerful, and language-neutral HTML template system.

.if defined(PACKAGE_BUILDING)
WITH_PERL=	yes
WITH_PYTHON=	yes
.endif

.if defined(WITH_PYTHON)
USE_PYTHON=	yes
.endif

.if defined(WITH_PERL)
USE_PERL5=	yes
.endif

.if defined(WITH_RUBY)
USE_RUBY=	yes
.endif

GNU_CONFIGURE= yes
USE_GMAKE=     yes

.include <bsd.port.mk>
