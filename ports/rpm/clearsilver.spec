#
# spec file for ClearSilver Linux RPM (based on RedHat installs)
#

%define __prefix        /usr/local
%define __python        /neo/opt/bin/python
%define	with_python_subpackage	1 %{nil}
%define with_perl_subpackage	0

# These packages aren't tested at all and probably won't build
%define with_apache_subpackage	0
%define with_java_subpackage	0
%define with_ruby_subpackage	0
%define with_csharp_subpackage	0

%define python_sitepath %(%{__python} -c "import site; print site.sitedirs[0]")
%define perl_sitearch %(eval "`perl -V:installsitearch`"; echo %$installsitearch)
%define ruby_sitepath %(echo "i dunno")
%define ruby_version %(echo "i dunno")
%define ruby_arch %(echo "i dunno")
%define apache_libexec %(eval `/httpd/bin/apxs -q LIBEXECDIR`)

Summary: Neotonic ClearSilver
Name: clearsilver
Version: 0.9.3
Release: 1
Copyright: Open Source - Neotonic ClearSilver License (Apache 1.1 based)
Group: Development/Libraries
Source: http://www.clearsilver.net/downloads/clearsilver-0.9.3.tar.gz
URL: http://www.clearsilver.net/
Vendor: Neotonic Software Corporation, Inc.
Packager: Brandon Long <blong@neotonic.com>
BuildRequires: zlib-devel
%if %{with_python_subpackage}
BuildRequires: python-devel >= 1.5.2
%endif
%if %{with_perl_subpackage}
BuildRequires: perl >= 0:5.006
%endif
%if %{with_ruby_subpackage}
BuildRequires: ruby >= 1.4.5
%endif

BuildRoot: %{_tmppath}/%{name}-root

%description
ClearSilver is a fast, powerful, and language-neutral HTML template system. 
In both static content sites and dynamic HTML applications, it provides a 
separation between presentation code and application logic which makes 
working with your project easier.

Because it's written as a C-library, and exported to scripting languages 
like Python and Perl via modules, it is extremely fast compared to template 
systems written in a script language. 

%if %{with_python_subpackage}
%package python
Summary: Neotonic ClearSilver Python Module
Group: Development/Libraries
Requires: clearsilver = %PACKAGE_VERSION
%requires_eq python

%description python
The clearsilver-python package provides a python interface to the
clearsilver CGI kit and templating system.
%endif

%if %{with_perl_subpackage}
%package perl
Summary: Neotonic ClearSilver Perl Module
Group: Development/Libraries
Requires: clearsilver = %PACKAGE_VERSION
Requires: perl >= 0:5.006
%requires_eq perl

%description perl
The clearsilver-perl package provides a perl interface to the
clearsilver templating system.
%endif

%if %{with_ruby_subpackage}
%package ruby
Summary: Neotonic ClearSilver Ruby Module
Group: Development/Libraries
Requires: clearsilver = %PACKAGE_VERSION
Requires: ruby >= 1.4.5

%description ruby
The clearsilver-ruby package provides a ruby interface to the
clearsilver templating system.
%endif

%if %{with_apache_subpackage}
%package apache
Summary: Neotonic ClearSilver Apache Module
Group: Development/Libraries
Requires: clearsilver = %PACKAGE_VERSION
Requires: apache >= 1.3.0
Requires: apache < 1.4

%description apache
The clearsilver-apache package provides an Apache 1.3.x module for
loading ClearSilver CGI's as shared libraries.
%endif

%if %{with_java_subpackage}
%package java
Group: Development/Libraries
Requires: clearsilver = %PACKAGE_VERSION

%description java
The clearsilver-java package provides a java jni interface to the
clearsilver templating system.
%endif

%prep
%setup 

%build
./configure --prefix=%{__prefix} --disable-ruby --disable-perl --disable-java --disable-apache
make

%install
make DESTDIR="$RPM_BUILD_ROOT" install

%files 
%{__prefix}/include/ClearSilver/ClearSilver.h
%{__prefix}/include/ClearSilver/cs_config.h
%{__prefix}/include/ClearSilver/cgi/cgi.h
%{__prefix}/include/ClearSilver/cgi/cgiwrap.h
%{__prefix}/include/ClearSilver/cgi/date.h
%{__prefix}/include/ClearSilver/cgi/html.h
%{__prefix}/include/ClearSilver/cs/cs.h
%{__prefix}/include/ClearSilver/util/dict.h
%{__prefix}/include/ClearSilver/util/filter.h
%{__prefix}/include/ClearSilver/util/neo_date.h
%{__prefix}/include/ClearSilver/util/neo_err.h
%{__prefix}/include/ClearSilver/util/neo_files.h
%{__prefix}/include/ClearSilver/util/neo_hash.h
%{__prefix}/include/ClearSilver/util/neo_hdf.h
%{__prefix}/include/ClearSilver/util/neo_misc.h
%{__prefix}/include/ClearSilver/util/neo_net.h
%{__prefix}/include/ClearSilver/util/neo_rand.h
%{__prefix}/include/ClearSilver/util/neo_server.h
%{__prefix}/include/ClearSilver/util/neo_str.h
%{__prefix}/include/ClearSilver/util/rcfs.h
%{__prefix}/include/ClearSilver/util/skiplist.h
%{__prefix}/include/ClearSilver/util/ulist.h
%{__prefix}/include/ClearSilver/util/ulocks.h
%{__prefix}/include/ClearSilver/util/wdb.h
%{__prefix}/include/ClearSilver/util/wildmat.h
%{__prefix}/lib/libneo_cgi.a
%{__prefix}/lib/libneo_cs.a
%{__prefix}/lib/libneo_utl.a
%{__prefix}/bin/static.cgi
%{__prefix}/bin/cstest

%if %{with_python_subpackage}
%files python
%{python_sitepath}/neo_cgi.so
%endif

%if %{with_perl_subpackage}
%files perl
%{perl_sitearch}/ClearSilver.pm
%{perl_sitearch}/auto/ClearSilver/ClearSilver.so
%endif

%if %{with_ruby_subpackage}
%files ruby
%{ruby_sitepath}/%(ruby_version}/neo.rb
%{ruby_sitepath}/%(ruby_version}/$(ruby_arch}/hdf.so
%endif

%if %{with_apache_subpackage}
%files apache
%{apache_libexec}/mod_ecs.so
%endif

%if %{with_java_subpackage}
%files java
%{__prefix}/lib/clearsilver.jar
%{__prefix}/lib/libclearsilver-jni.so
%endif
