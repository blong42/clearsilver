#
# spec file for ClearSilver Linux RPM (based on RedHat installs)
#

%define python_sitepath	%(eval `python -c 'import sys; print "%%s/lib/python%%s/site-packages" %% (sys.exec_prefix, sys.version[:3])'`)

Summary: Neotonic ClearSilver
Name: clearsilver
Version: 0.9.2
Release: 1
Copyright: Open Source - Neotonic ClearSilver License (Apache 1.1 based)
Group: Development/Libraries
Source: http://www.clearsilver.net/downloads/clearsilver-0.9.2.tar.gz
URL: http://www.clearsilver.net/
Vendor: Neotonic Software Corporation, Inc.
Packager: Brandon Long <blong@neotonic.com>
BuildRequires: zlib-devel
BuildRequires: python-devel >= 1.5.2

%description
ClearSilver is a fast, powerful, and language-neutral HTML template system. 
In both static content sites and dynamic HTML applications, it provides a 
separation between presentation code and application logic which makes 
working with your project easier.

Because it's written as a C-library, and exported to scripting languages 
like Python and Perl via modules, it is extremely fast compared to template 
systems written in a script language. 

%package python
Summary: Neotonic ClearSilver Python Module
Group: Development/Libraries
Requires: clearsilver = %PACKAGE_VERSION
%requires_eq python

%description python
The clearsilver-python package provides a python interface to the
clearsilver CGI kit and templating system.

%prep
%setup 

%build
./configure --prefix=${__prefix} --disable-ruby --disable-perl --disable-java --disable-apache
make

%install
make DESTDIR="$RPM_BUILD_ROOT" install

%files 
%{__prefix}/include/ClearSilver.h
%{__prefix}/include/cs_config.h
%{__prefix}/include/cgi/cgi.h
%{__prefix}/include/cgi/cgiwrap.h
%{__prefix}/include/cgi/date.h
%{__prefix}/include/cgi/html.h
%{__prefix}/include/cs/cs.h
%{__prefix}/include/util/dict.h
%{__prefix}/include/util/filter.h
%{__prefix}/include/util/neo_date.h
%{__prefix}/include/util/neo_err.h
%{__prefix}/include/util/neo_files.h
%{__prefix}/include/util/neo_hash.h
%{__prefix}/include/util/neo_hdf.h
%{__prefix}/include/util/neo_misc.h*
%{__prefix}/include/util/neo_net.h
%{__prefix}/include/util/neo_rand.h
%{__prefix}/include/util/neo_server.h
%{__prefix}/include/util/neo_str.h
%{__prefix}/include/util/rcfs.h
%{__prefix}/include/util/skiplist.h
%{__prefix}/include/util/ulist.h
%{__prefix}/include/util/ulocks.h
%{__prefix}/include/util/wdb.h
%{__prefix}/include/util/wildmat.h
%{__prefix}/lib/libneo_cgi.a
%{__prefix}/lib/libneo_cs.a
%{__prefix}/lib/libneo_utl.a
%{__prefix}/bin/static.cgi
%{__prefix}/bin/cstest

%files python
${python_sitepath}/neo_cgi.so

