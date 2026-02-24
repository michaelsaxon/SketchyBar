#!/bin/bash

# skhd -k "cmd - space"


sketchybar --set "$NAME" background.gradient.color_start=0xFF68EF53 \
    background.gradient.color_end=0xFF358929
sleep 0.02
open /System/Applications/Launchpad.app
sleep 0.1
sketchybar --set "$NAME" background.gradient.color_start=0xFF68CD53 \
    background.gradient.color_end=0xFF356729 \
