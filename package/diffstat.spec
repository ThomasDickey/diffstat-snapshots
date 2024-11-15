Summary:  Make histogram from diff-output
%define AppProgram diffstat
%define AppVersion 1.67
# $XTermId: diffstat.spec,v 1.21 2024/11/11 16:08:57 tom Exp $
Name: %{AppProgram}
Version: %{AppVersion}
Release: 1
License: MIT
Group: Applications/Development
URL: https://invisible-island.net/archives/%{AppProgram}
Source0: %{AppProgram}-%{AppVersion}.tgz
Packager: Thomas Dickey <dickey@invisible-island.net>

%description
Diffstat is is useful for reviewing large, complex patch files.  It reads from
one or more input files which contain output from diff, producing a histogram
of the total lines changed for each file referenced.

%prep

%define debug_package %{nil}

%setup -q -n %{AppProgram}-%{AppVersion}

%build

INSTALL_PROGRAM='${INSTALL}' \
%configure \
  --target %{_target_platform} \
  --prefix=%{_prefix} \
  --bindir=%{_bindir} \
  --libdir=%{_libdir} \
  --mandir=%{_mandir}

make

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

make install                    DESTDIR=$RPM_BUILD_ROOT

strip $RPM_BUILD_ROOT%{_bindir}/%{AppProgram}

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{_prefix}/bin/%{AppProgram}
%{_mandir}/man1/%{AppProgram}.*

%changelog
# each patch should add its ChangeLog entries here

* Fri Jan 26 2024 Thomas Dickey
- update URLs

* Wed Aug 15 2018 Thomas Dickey
- use recommended compiler-flags

* Thu Jul 15 2010 Thomas Dickey
- initial version
