#
# spec file for ClearSilver Apache Linux RPM
# We're making the sub-packages separate packages since I don't know how
# to make them optional
#

%define apache_libexec %(eval `/httpd/bin/apxs -q LIBEXECDIR`)

Summary: Neotonic ClearSilver Apache Module
Name: clearsilver-apache
Version: 0.9.2
Release: 1
Copyright: Open Source - Neotonic ClearSilver License (Apache 1.1 based)
Group: Development/Libraries
Source: http://www.clearsilver.net/downloads/clearsilver-0.9.2.tar.gz
URL: http://www.clearsilver.net/
Vendor: Neotonic Software Corporation, Inc.
Packager: Brandon Long <blong@neotonic.com>
BuildRequires: zlib-devel
Requires: clearsilver = %PACKAGE_VERSION
Requires: apache >= 1.3.0
Requires: apache < 1.4

%description
The clearsilver-apache package provides an Apache 1.3.x module for
loading ClearSilver CGI's as shared libraries.

%files
%{apache_libexec}/mod_ecs.so

