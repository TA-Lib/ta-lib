# List of contributors:
# 
#   Initial  Name/description
#   -------------------------------------------------------------------
#   MW       Michael Williamson
#   JS       John Sheehy <jes@e-techservices.com>

# Change history:
#  MMDDYY BY     Description
#  -------------------------------------------------------------------
#  082104 MW     Initial Version
#  012908 JS     Enhancement + add x86_64 support 


%define ta_ver 0.5.0

Summary: Technical Analysis Library
Name: ta-lib
Version: %{ta_ver}
Release: 1
License: BSD
Group: Development/Libraries
Source: http://prdownloads.sourceforge.net/%{name}/%{name}-%{ta_ver}-src.tar.gz
Prefix: /usr
Buildroot: %{_tmppath}/%{name}-%{version}

%description

  TA-Lib provides common functions for the technical analysis of
  stock/future/commodity market data. 

  TA-Lib is intended for software developers looking to add technical
  analysis functionality to their application. It is a library of more than
  150 functions that can be integrated in your application. It is not an
  application by itself.




%prep
%setup -n %{name}

%build
  ./autogen.sh
%ifarch x86_64
  CFLAGS="-g0 -O2 -pipe" ./configure --prefix=%{prefix} --libdir=%{prefix}/lib64
%else
  CFLAGS="-g0 -O2 -pipe" ./configure --prefix=%{prefix}
%endif
  make

%install
  [ "${RPM_BUILD_ROOT}" != "/" ] && rm -rf ${RPM_BUILD_ROOT}
  make DESTDIR=${RPM_BUILD_ROOT} install

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%clean
  [ "${RPM_BUILD_ROOT}" != "/" ] && rm -rf ${RPM_BUILD_ROOT}

%files
  %defattr(-,root,root)
  /usr

