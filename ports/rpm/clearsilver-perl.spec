#
# spec file for ClearSilver Perl Linux RPM
# We're making the sub-packages separate packages since I don't know how
# to make them optional
#
%define perl_sitearch %(eval "`perl -V:installsitearch`"; echo %$installsitearch)

Summary: Neotonic ClearSilver Perl Module
Name: clearsilver-perl
Version: 0.9.2
Release: 1
Copyright: Open Source - Neotonic ClearSilver License (Apache 1.1 based)
Group: Development/Libraries
Source: http://www.clearsilver.net/downloads/clearsilver-0.9.2.tar.gz
URL: http://www.clearsilver.net/
Vendor: Neotonic Software Corporation, Inc.
Packager: Brandon Long <blong@neotonic.com>
BuildRequires: zlib-devel
BuildRequires: perl >= 0:5.006
Requires: clearsilver = %PACKAGE_VERSION
Requires: perl >= 0:5.006
%requires_eq perl

%description
The clearsilver-perl package provides a perl interface to the
clearsilver templating system.

%build
./configure --prefix=${__prefix}
make

%install
make DESTDIR="$RPM_BUILD_ROOT" install

%files
%{perl_sitearch}/ClearSilver.pm
%{perl_sitearch}/auto/ClearSilver/ClearSilver.so
