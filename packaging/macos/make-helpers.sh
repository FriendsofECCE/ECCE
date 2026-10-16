#!/bin/bash
# make-helpers.sh <ECCE.app>: every wx program of ECCE becomes a small
# application bundle in ECCE.app/Contents/Helpers, so macOS gives it the
# ECCE icon and its own name in the Dock, Cmd+Tab and the menu bar.
# The program moves into the bundle; ecce/bin/<name> stays as a link to it.
# Run after the files are in place and before bundle_libs.py, which
# then sets each program's rpath for its new place.
set -euo pipefail
APP=$(cd "$1" && pwd)
RES=$APP/Contents/Resources
HELP=$APP/Contents/Helpers
VERSION=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' "$APP/Contents/Info.plist")
mkdir -p "$HELP"

# Programs with no window of their own: no Dock icon, no menu bar.
accessory=" gateway vizthumbnail eccejobstore eccejobmaster ecmd load_tgbs "

title_of() {
  case "$1" in
    organizer)    echo "ECCE Organizer" ;;
    builder)      echo "ECCE Builder" ;;
    calced)       echo "ECCE Calculation Editor" ;;
    basistool)    echo "ECCE Basis Set Tool" ;;
    machbrowser)  echo "ECCE Machine Browser" ;;
    machregister) echo "ECCE Machine Register" ;;
    launcher)     echo "ECCE Job Launcher" ;;
    pertable)     echo "ECCE Periodic Table" ;;
    solvate)      echo "ECCE Solvate" ;;
    mdprepare)    echo "ECCE MD Prepare" ;;
    mddynamics)   echo "ECCE MD Dynamics" ;;
    mdenergy)     echo "ECCE MD Energy" ;;
    mdoptimize)   echo "ECCE MD Optimize" ;;
    metadyn)      echo "ECCE Metadynamics" ;;
    polyed)       echo "ECCE Polyrate Editor" ;;
    dirdyed)      echo "ECCE Dirdy Editor" ;;
    *)            echo "ECCE" ;;   # gateway, dialogs and other small helpers
  esac
}

made=()
for exe in "$RES/ecce/bin"/*; do
  [ -f "$exe" ] && [ ! -L "$exe" ] || continue
  case "$(head -c4 "$exe" | xxd -p)" in cffaedfe|cafebabe) ;; *) continue ;; esac
  libs=$(otool -L "$exe")
  case "$libs" in *libwx*) ;; *) continue ;; esac
  name=$(basename "$exe")
  title=$(title_of "$name")
  # Programs sharing the plain name "ECCE" get one bundle each.
  dir=$title
  [ "$title" = "ECCE" ] && dir="ECCE $name"
  b=$HELP/$dir.app
  mkdir -p "$b/Contents/MacOS" "$b/Contents/Resources"
  mv "$exe" "$b/Contents/MacOS/$name"
  ln -s "../../../Helpers/$dir.app/Contents/MacOS/$name" "$exe"
  cp "$RES/ecce.icns" "$b/Contents/Resources/ecce.icns"
  ui=""
  case "$accessory" in *" $name "*) ui="<key>LSUIElement</key><true/>" ;; esac
  # The folder descriptions are here too: the prompt names the program
  # that reads the file, and each tool is its own program.
  cat > "$b/Contents/Info.plist" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleName</key><string>$title</string>
  <key>CFBundleDisplayName</key><string>$title</string>
  <key>CFBundleIdentifier</key><string>org.friendsofecce.ecce.$name</string>
  <key>CFBundleExecutable</key><string>$name</string>
  <key>CFBundleIconFile</key><string>ecce</string>
  <key>CFBundlePackageType</key><string>APPL</string>
  <key>CFBundleShortVersionString</key><string>$VERSION</string>
  <key>CFBundleVersion</key><string>$VERSION</string>
  <key>LSMinimumSystemVersion</key><string>12.0</string>
  <key>NSHighResolutionCapable</key><true/>
  $ui
  <key>NSLocalNetworkUsageDescription</key><string>ECCE connects to the central ECCE server (data server and message broker) and to compute hosts on your network.</string>
  <key>NSDownloadsFolderUsageDescription</key><string>ECCE reads the structure and input files you pick in your Downloads folder.</string>
  <key>NSDesktopFolderUsageDescription</key><string>ECCE reads the structure and input files you pick on your Desktop.</string>
  <key>NSDocumentsFolderUsageDescription</key><string>ECCE reads the structure and input files you pick in your Documents folder.</string>
</dict>
</plist>
PLIST
  plutil -lint "$b/Contents/Info.plist" >/dev/null
  made+=("$name:$dir")
done

# The scripts that start a program by $ECCE_HOME/bin/<name> start it by
# the real path instead: macOS takes the bundle (name, icon, no Dock icon
# for the gateway) only from a process started that way, not through the link.
# Bundle names have spaces; the replacement stays inside the existing quotes,
# and the path is normalised: macOS finds the bundle from the path as given.
for pair in ${made[@]+"${made[@]}"}; do
  name=${pair%%:*}
  dir=${pair#*:}
  for f in "$RES/bin"/* "$RES/ecce/bin"/*; do
    [ -f "$f" ] && [ ! -L "$f" ] || continue
    grep -Iq . "$f" 2>/dev/null || continue
    NAME=$name DIR=$dir perl -pi -e 's{"\$ECCE_HOME/bin/\Q$ENV{NAME}\E"}{"\$(cd "\$ECCE_HOME/../.." && pwd -P)/Helpers/$ENV{DIR}.app/Contents/MacOS/$ENV{NAME}"}g' "$f"
  done
done
echo "make-helpers: ${#made[@]} helper bundles: ${made[*]-}"
