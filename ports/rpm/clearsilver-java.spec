#
# spec file for ClearSilver JAVA Linux RPM
# We're making the sub-packages separate packages since I don't know how
# to make them optional
#
Summary: Neotonic ClearSilver JAVA Module
Name: clearsilver-java
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

%description
The clearsilver-java package provides a java jni interface to the
clearsilver templating system.

%files
%{__prefix}/lib/clearsilver.jar
%{__prefix}/lib/libclearsilver-jni.so

