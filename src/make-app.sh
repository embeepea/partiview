#! /bin/sh

TARGET="$1"
FOURCC="$2"

case "$TARGET" in
   ""|-*)
	echo "Usage: $0  somefile
Given existing MacOS executable \"somefile\",
create \"somefile.app\", containing the right stuff
to be a MacOS GUI application.  Thanks to Erco's FLTK Cheat Page
for showing how to do this." >&2
	exit 1
	;;

   *.app)
	TARGET="`expr 'x$TARGET' : 'x\(.*\)\.app$'`"
	;;
esac


if [ ! -f "$TARGET" ] || [ ! -x "$TARGET" ]; then
    echo "$0: Won't create ${TARGET}.app -- need ${TARGET} to already exist and be an executable." >&2
    exit 1
fi

case "$FOURCC" in
    "")
	FOURCC="`expr "/$TARGET    " : '.*/\([^/][^/][^/][^/]\)$'`" ;;
esac

CONTENTS="${TARGET}.app/Contents"
mkdir -p "${CONTENTS}/MacOS" "${CONTENTS}/Resources"

ln -f "${TARGET}"  "${CONTENTS}/MacOS/"

echo -n "APPL$FOURCC" > "${CONTENTS}/PkgInfo"

BASE=`basename "$TARGET"`

# If we had an icon, we'd reprocess it with
# "/Developer/Applications/Utilities/Icon Composer.app"
# put the result into ${TARGET}.app/Contents/Resources/icon.icns

cat >"${CONTENTS}/Info.plist" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist SYSTEM "file://localhost/System/Library/DTDs/PropertyList.dtd">
<plist version="0.9">
<dict>
	<key>CFBundleName</key>
	<string>$BASE</string>
	<key>CFBundlePackageType</key>
	<string>APPL</string>
	<key>CFBundleVersion</key>
	<string>59</string>
	<key>CFBundleShortVersionString</key>
	<string>1.1</string>
	<key>CFBundleSignature</key>
	<string>none</string>
</dict>
</plist>
EOF
