# to check current entitlements
#codesign -d --entitlements :- /Applications/Godot_v4.4.1-stable_macos.app/Contents/MacOS/Godot
# to overwrite
codesign --entitlements entitlements.plist -f -s - /Applications/Godot_v4.4.1-stable_macos.app/Contents/MacOS/Godot