%{!?marker_version:%global marker_version 2026.10.0}

Name:           marker
Version:        %{marker_version}
Release:        1%{?dist}
Summary:        Powerful markdown editor for the GNOME desktop
License:        GPL-3.0-or-later
URL:            https://github.com/adrighem/Marker
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
BuildRequires:  pkgconfig(gtk4) >= 4.10.0
BuildRequires:  pkgconfig(libadwaita-1) >= 1.4.0
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
rm -f %{buildroot}%{_datadir}/com.github.adrighem.marker/icons/hicolor/generate.sh
%find_lang %{name} --with-gnome

%check
appstreamcli validate --no-net %{buildroot}%{_datadir}/metainfo/com.github.adrighem.marker.appdata.xml
desktop-file-validate %{buildroot}%{_datadir}/applications/com.github.adrighem.marker.desktop

%files -f %{name}.lang
%doc README.md
%license LICENSE.md
%{_bindir}/marker
%{_datadir}/applications/com.github.adrighem.marker.desktop
%{_datadir}/com.github.adrighem.marker/
%{_datadir}/glib-2.0/schemas/com.github.adrighem.marker.gschema.xml
%{_datadir}/icons/hicolor/*/apps/com.github.adrighem.marker*
%{_datadir}/metainfo/com.github.adrighem.marker.appdata.xml

%changelog
* Sat Oct 17 2026 Vincent van Adrighem <vincent@vanadrighem.eu> - %{version}-1
- Automated GitHub release packaging.
