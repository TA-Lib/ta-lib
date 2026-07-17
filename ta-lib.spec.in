# List of contributors:
#
#   Initial  Name/description
#   -------------------------------------------------------------------
#   MW       Michael Williamson
#   JS       John Sheehy <jes@e-techservices.com>
#   MF       Mario Fortier
#   AA       Avi Alkalay (Fedora RPM spec, SourceForge patch 5, 2021)
#   CC       Claude (AI assistant)

# Change history:
#  MMDDYY BY     Description
#  -------------------------------------------------------------------
#  082104 MW     Initial Version
#  012908 JS     Enhancement + add x86_64 support
#  011024 MF     Change source to Github, add URL
#  071626 MF,CC  Modernize for current Fedora/RHEL: configure/make_build/
#                make_install macros, _libdir (fixes x86_64 lib64
#                packaging), build from shipped configure, drop Buildroot
#                and clean sections, keep debuginfo, split -devel subpackage.

%define ta_ver 0.8.1

Summary: Technical Analysis Library
Name: ta-lib
Version: %{ta_ver}
Release: 1%{?dist}
License: BSD-3-Clause
URL: https://github.com/ta-lib/ta-lib
Source: https://github.com/ta-lib/ta-lib/releases/download/v%{ta_ver}/ta-lib-%{ta_ver}-src.tar.gz

BuildRequires: gcc
BuildRequires: gcc-c++
BuildRequires: make

%description
TA-Lib provides common functions for the technical analysis of
stock/future/commodity market data.

TA-Lib is intended for software developers looking to add technical
analysis functionality to their application. It is a library of more than
150 functions that can be integrated in your application. It is not an
application by itself.

%package devel
Summary: Development files for TA-Lib
Requires: %{name}%{?_isa} = %{version}-%{release}

%description devel
This package contains the header files, the shared-object development
link, the static library and the pkg-config file needed to build
applications against TA-Lib.

%prep
%autosetup -n %{name}-%{ta_ver}

%build
%configure
%make_build

%install
%make_install
# Drop the libtool archive; nothing packaged needs it.
find %{buildroot} -name '*.la' -delete

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%license LICENSE
%doc README.md
%{_libdir}/libta-lib.so.0
%{_libdir}/libta-lib.so.0.*

%files devel
%{_includedir}/ta-lib/
%{_libdir}/libta-lib.so
%{_libdir}/libta-lib.a
%{_libdir}/pkgconfig/ta-lib.pc

%changelog
* Thu Jul 16 2026 Mario Fortier <tictactec@gmail.com> - 0.8.1-1
- Modernize the RPM spec for current Fedora/RHEL (issue #123): use
  %%configure/%%make_build/%%make_install, package %%{_libdir} so x86_64
  lib64 content is included, build from the shipped configure, drop the
  deprecated Buildroot/%%clean sections, keep debuginfo, and split a
  -devel subpackage.
