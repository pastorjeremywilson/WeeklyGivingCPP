#!/bin/bash
set -e
SOURCEDIR=/home/jeremy/CDist/WeeklyGiving/Release
DISTDIR=/home/jeremy/CDist/WeeklyGiving/Dist
PROGRAMNAME=WeeklyGiving
FRIENDLYNAME="Weekly Giving"
EXECNAME=WeeklyGiving
VERSION=0.1.4
ICONLOCATION=/home/jeremy/CDist/WeeklyGiving/Release/resources/icon.png
ICON=$PROGRAMNAME.png
CONTROLSECTION=utils
DEPENDENCIES=
DESCRIPTION="Weekly giving is a little program that tallies and stores for easy retrieval the giving at a worship service."
MENUSECTION=Applications/Office
TYPE=Application
CATEGORIES="Office"

if test -d "$DISTDIR/$PROGRAMNAME.$VERSION"; then
    echo Deleting Old Build Directory
    rm -rf "$DISTDIR/$PROGRAMNAME.$VERSION"
    rm -rf "$DISTDIR/$PROGRAMNAME"
fi

echo Creating Directories
mkdir -p "$DISTDIR/$PROGRAMNAME.$VERSION/DEBIAN"
mkdir -p "$DISTDIR/$PROGRAMNAME.$VERSION/usr/local/$PROGRAMNAME"
mkdir -p "$DISTDIR/$PROGRAMNAME.$VERSION/usr/share/applications"
mkdir -p "$DISTDIR/$PROGRAMNAME.$VERSION/usr/share/icons/hicolor/scalable/apps"

echo Creating control File
cat > "$DISTDIR/$PROGRAMNAME.$VERSION/DEBIAN/control" <<EOF
Package: $PROGRAMNAME
Version: $VERSION
Section: $CONTROLSECTION
Priority: optional
Architecture: all
Replaces: $PROGRAMNAME (<< $VERSION)
Depends: $DEPENDENCIES
Maintainer: Jeremy Wilson pastorjeremywilson@gmail.com
Homepage: https://pastorjeremywilson.github.io
Description: $DESCRIPTION

EOF

echo Creating menu File
cat > "$DISTDIR/$PROGRAMNAME.$VERSION/DEBIAN/$PROGRAMNAME.menu" <<EOF
Package($PROGRAMNAME): \
    Section="$MENUSECTION" \
    Title="$FRIENDLYNAME" \
    Command="/usr/local/$PROGRAMNAME/$EXECNAME" \
    Icon="/usr/share/icons/hicolor/scalable/apps/$ICON"
EOF

echo Creating desktop File
cat > "$DISTDIR/$PROGRAMNAME.$VERSION/usr/share/applications/$PROGRAMNAME.desktop" <<EOF
[Desktop Entry]
StartupWMClass=$FRIENDLYNAME
Version=1.0
Exec=/usr/local/$PROGRAMNAME/$EXECNAME
Comment=$DESCRIPTION
Terminal=false
PrefersNonDefaultGPU=false
Icon=/usr/share/icons/hicolor/scalable/apps/$ICON
Type=$TYPE
Name=$FRIENDLYNAME
Categories=$CATEGORIES;

EOF
chmod +x "$DISTDIR/$PROGRAMNAME.$VERSION/usr/share/applications/$PROGRAMNAME.desktop"

echo Creating postinst File
cat > "$DISTDIR/$PROGRAMNAME.$VERSION/DEBIAN/postinst" <<EOF
#!/bin/bash
set -e
if [ -x /usr/bin/update-icon-caches ]; then
    update-icon-caches /usr/share/icons/hicolor
fi
if [ -x /usr/bin/update-desktop-database ]; then
    update-desktop-database /usr/share/applications
fi
EOF
chmod 0555 "$DISTDIR/$PROGRAMNAME.$VERSION/DEBIAN/postinst"

echo Creating postrm File
cat > "$DISTDIR/$PROGRAMNAME.$VERSION/DEBIAN/postrm" <<EOF
#!/bin/sh
set -e
if [ -x /usr/bin/update-desktop-database ]; then
    update-desktop-database /usr/share/applications
fi
EOF
chmod 0555 "$DISTDIR/$PROGRAMNAME.$VERSION/DEBIAN/postrm"

echo Copying Program Data
cp "$SOURCEDIR/$EXECNAME" "$DISTDIR/$PROGRAMNAME.$VERSION/usr/local/$PROGRAMNAME"
cp -r "$SOURCEDIR" "$DISTDIR/$PROGRAMNAME.$VERSION/usr/local/$PROGRAMNAME"
cp "$ICONLOCATION" "$DISTDIR/$PROGRAMNAME.$VERSION/usr/share/icons/hicolor/scalable/apps/$ICON"
chmod +x "$DISTDIR/$PROGRAMNAME.$VERSION/usr/local/$PROGRAMNAME"

echo Building .deb File
sudo dpkg-deb --build "$DISTDIR/$PROGRAMNAME.$VERSION"

if [[ $# -eq 0 ]] || [[ $1 != "-k" && $1 != "--keep-build-dir" ]]; then
    echo Deleting Build Directory
    rm -rf "$DISTDIR/$PROGRAMNAME.$VERSION"
fi
