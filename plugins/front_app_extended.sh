#!/bin/sh

# Some events send additional information specific to the event in the $INFO
# variable. E.g. the front_app_switched event sends the name of the newly
# focused application in the $INFO variable:
# https://felixkratz.github.io/SketchyBar/config/events#events-and-scripting

# Query yabai for window info
yabai_output=$(yabai -m query --windows --window)

BLUE=0xBA4DA6FF
RED=0xBAFF9580
GREEN=0xBA99FFBB
WINDOWSBLUE=0xBA0055E6
WINDOWSGREEN=0xCC1E941E
BLACK=0xFF000000
FIREFOX="0xEEE66000"
POWERPOINT="0xEEC13B1B"
ITERM2="0xEE111522"
THUNDERBIRD="0xEE001166"
OBSIDIAN="0xEE5E4190"
VSCODE="0xEE55CC88"
MESSAGES="0xEE18D845"
CHROME="0xEEF4B400"
SLACK="0xEEE01E5A"
ZOTERO="0xEEFF9580"
APP_DEFAULT="0xFFFF0000"

DEFAULT=$WINDOWSBLUE
FOREGROUND=0xFFFFFFFF

# parse yabai info; either uses python or a compiled binary

# this is the python script to perform the parse
PYTHON_PARSE_SCRIPT="import json
val=input().strip()
val=json.loads(val)
val=str(val['app']) + ': ' + str(val['title'])
print(val[0:100] + '...') if len(val) > 100 else print(val)"

# flag to choose if python used
USE_PYTHON=false

color=$APPDEFAULT


# if the output of the yabai query isn't empty, parse it
if [ -n "$yabai_output" ]; then
  if [ $USE_PYTHON = true ]; then
    yabai_name=$(echo $yabai_output | /usr/bin/python3 -c "$PYTHON_PARSE_SCRIPT")
  else
    # use the compiled binary implemented in front_app_parse.cpp
    # (to build: `g++ -std=c++11 front_app_parse.cpp -o front_app_parse`)
    yabai_name=$(./michael_config/plugins/front_app_parse $yabai_output)
  fi
# if the yabai parse output is empty, we are at a blank desktop; report finder
else
  yabai_name="Finder"
fi

# yabai_name is the full string we will report, containing the app name and title separated with a :
echo $yabai_name

# get the app name only to determine icon and color
app=$(echo $yabai_name | cut -f1 -d":")

# color map for each app
if [[ $app == "Firefox" ]]; then
  color=$FIREFOX
elif [[ $app == "Microsoft PowerPoint" ]]; then
  color=$POWERPOINT
elif [[ $app == "iTerm2" ]]; then
  color=$ITERM2
elif [[ $app == "Thunderbird" || $app == "Betterbird" ]]; then
  color=$THUNDERBIRD
elif [[ $app == "Obsidian" ]]; then
  color=$MAGENTA
elif [[ $app == "Code" || $app = "Cursor" ]]; then
  color=$VSCODE
elif [[ $app == "Messages" ]]; then
  color=$MESSAGES
elif [[ $app == "Google Chrome" || "Chromium" ]]; then
  color=$BRIGHTCYAN
elif [[ $app == "Slack" ]]; then
  color=$SLACK
elif [[ $app == "Zotero" ]]; then
  color=$RED
elif [[ $app == "Safari" ]]; then
  color=$BLUE
fi

# send everything to sketchybar
# (I should clean this up and put the static elements in the main instantiation)
sketchybar --set front_app label="$yabai_name" \
  icon.color=$color \
  icon="$($CONFIG_DIR/michael_config/plugins/icon_map_fn.sh "$app")" \
  icon.padding_left=10
