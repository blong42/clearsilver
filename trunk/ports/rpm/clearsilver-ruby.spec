#
# spec file for ClearSilver Ruby Linux RPM
# We're making the sub-packages separate packages since I don't know how
# to make them optional
#
Summary: Neotonic ClearSilver Ruby Module
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
Requires: clearsilver = %PACKAGE_VERSION
Requires: ruby >= 1.4.5

%description
The clearsilver-ruby package provides a ruby interface to the
clearsilver templating system.

%files
%{ruby_sitepath}/%(ruby_version}/neo.rb
%{ruby_sitepath}/%(ruby_version}/$(ruby_arch}/hdf.so
