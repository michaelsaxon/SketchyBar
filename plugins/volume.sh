#!/bin/sh

# The volume_change event supplies a $INFO variable in which the current volume
# percentage is passed to the script.

alias sketchybar=bin/sketchybar

WHITE=0xFFFFFFFF
DEFAULT=0xff75B4EE

device=$(SwitchAudioSource -c)

if [ "$SENDER" = "volume_change" ]; then
  VOLUME="$INFO"

  case "$VOLUME" in
  [6-9][0-9] | 100)
    ICON="󰕾"
    ;;
  [3-5][0-9])
    ICON="󰖀"
    ;;
  [1-9] | [1-2][0-9])
    ICON="󰕿"
    ;;
  *) ICON="󰖁" ;;
  esac

  if [ "$device" = "DELL G3223Q" ]; then
    ICON="󰽟"
  fi

  sketchybar --set "$NAME" background.color="0x00000000" icon.color=$WHITE icon="$ICON"
  #sketchybar --set "$NAME" icon="$ICON" label="$VOLUME%"
elif [ "$SENDER" = "mouse.clicked" ]; then
  if [ "$device" = "DELL G3223Q" ]; then
    echo 'Cannot mute monitor'
  else
    osascript -e 'set volume output muted not (output muted of (get volume settings))'
    sketchybar --set "$NAME" background.color=$WHITE icon.color=$DEFAULT background.drawing="on" icon="󰖁"
    sleep 0.15
    sketchybar --set "$NAME" background.color="0x00000000" icon.color=$WHITE
  fi
fi

sketchybar --set "$NAME" label="$device"
