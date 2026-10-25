# The version is NOT hardcoded in this spec: the release pipeline passes it
# from the git tag (rpmbuild --define "marker_version $VERSION", where
# $VERSION is the tag without the leading "v"). Building without the define
# fails immediately with a clear message instead of shipping a wrong version.
%{!?marker_version:%{error:marker_version is not defined - pass the git tag version, e.g. rpmbuild --define "marker_version 2026.10.7" -ba packaging/rpm/marker.spec}}

Name:           marker
Version:        %{marker_version}
Release:        1%{?dist}
Summary:        Powerful markdown editor for the GNOME desktop
License:        GPL-3.0-or-later
URL:            https://github.com/TheNathattify/Marker
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  appstream
BuildRequires:  desktop-file-utils
BuildRequires:  gcc
BuildRequires:  gettext
BuildRequires:  itstool
BuildRequires:  meson >= 0.62.0
BuildRequires:  ninja-build
BuildRequires:  pkgconfig(glib-2.0) >= 2.74.0
BuildRequires:  pkgconfig(gio-2.0) >= 2.74.0
BuildRequires:  pkgconfig(gtk4) >= 4.16.0
BuildRequires:  pkgconfig(libadwaita-1) >= 1.8.0
BuildRequires:  pkgconfig(gtksourceview-5) >= 5.6.0
BuildRequires:  pkgconfig(webkitgtk-6.0) >= 2.40.0

Recommends:     pandoc

%description
Marker is a fast and powerful markdown editor designed for the GNOME
desktop with real-time preview, syntax highlighting, math rendering,
and document export capabilities.

%prep
%autosetup

%build
%meson
%meson_build

%install
%meson_install
rm -f %{buildroot}%{_datadir}/com.github.thenathattify.marker/icons/hicolor/generate.sh
%find_lang %{name} --with-gnome

%check
appstreamcli validate --no-net %{buildroot}%{_datadir}/metainfo/com.github.thenathattify.marker.appdata.xml
desktop-file-validate %{buildroot}%{_datadir}/applications/com.github.thenathattify.marker.desktop

%files -f %{name}.lang
%doc README.md
%license LICENSE.md
%{_bindir}/marker
%{_datadir}/applications/com.github.thenathattify.marker.desktop
%{_datadir}/com.github.thenathattify.marker/
%{_datadir}/dbus-1/services/com.github.thenathattify.marker.service
%{_datadir}/glib-2.0/schemas/com.github.thenathattify.marker.gschema.xml
%{_datadir}/icons/hicolor/*/apps/com.github.thenathattify.marker*
%{_datadir}/metainfo/com.github.thenathattify.marker.appdata.xml

%changelog
* Sun Oct 25 2026 Nathan Obry <TheNathattify@users.noreply.github.com> - %{version}-1
- Automated GitHub release packaging.
