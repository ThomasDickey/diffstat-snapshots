Summary:  Make histogram from diff-output
# $XTermId: diffstat.spec,v 1.25 2026/04/15 00:31:21 tom Exp $
Name: diffstat
Version: 1.69
Release: 1
License: MIT
Group: Applications/Development
URL: https://invisible-island.net/archives/%{name}
Source0: %{name}-%{version}.tgz
Packager: Thomas Dickey <dickey@invisible-island.net>

%description
Diffstat is is useful for reviewing large, complex patch files.  It reads from
one or more input files which contain output from diff, producing a histogram
of the total lines changed for each file referenced.

%prep

%define debug_package %{nil}

%setup -q -n %{name}-%{version}

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

strip $RPM_BUILD_ROOT%{_bindir}/%{name}

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{_prefix}/bin/%{name}
%{_mandir}/man1/%{name}.*

%changelog
# each patch should add its ChangeLog entries here

* Tue Apr 14 2026 Thomas E. Dickey
- testing diffstat 1.69-1

* Fri Jan 26 2024 Thomas Dickey
- update URLs

* Wed Aug 15 2018 Thomas Dickey
- use recommended compiler-flags

* Thu Jul 15 2010 Thomas Dickey
- initial version
