#!/usr/bin/env zsh

alias sketchybar=bin/sketchybar

OPENWEATHER_API_KEY=$(cat ~/.keys/openweather.api)

IP=$(curl -s https://ipinfo.io/ip)
LOCATION_JSON=$(curl -s https://ipinfo.io/$IP/json)

LOCATION="$(echo $LOCATION_JSON | jq -r '.city')"
LOC="$(echo $LOCATION_JSON | jq -r '.loc')"
LAT="${LOC%%,*}"
LON="${LOC##*,}"

DELAY=1
MAX_DELAY=64
WEATHER_JSON=""
while [ -z "$WEATHER_JSON" ]; do
  WEATHER_JSON=$(curl -s "https://api.openweathermap.org/data/2.5/weather?lat=$LAT&lon=$LON&appid=$OPENWEATHER_API_KEY&units=imperial")
  if [ -z "$WEATHER_JSON" ]; then
    sleep $DELAY
    DELAY=$(( DELAY * 2 ))
    if (( DELAY > MAX_DELAY )); then
      DELAY=$MAX_DELAY
    fi
  fi
done

TEMPERATURE=$(echo $WEATHER_JSON | jq -r '.main.temp' | cut -d. -f1)
WEATHER_ID=$(echo $WEATHER_JSON | jq -r '.weather[0].id')
WEATHER_MAIN=$(echo $WEATHER_JSON | jq -r '.weather[0].main')

SUNRISE=$(echo $WEATHER_JSON | jq -r '.sys.sunrise')
SUNSET=$(echo $WEATHER_JSON | jq -r '.sys.sunset')
CURRENT=$(echo $WEATHER_JSON | jq -r '.dt')

IS_NIGHT=false
if (( CURRENT < SUNRISE || CURRENT > SUNSET )); then
  IS_NIGHT=true
fi

case $WEATHER_ID in
  800) ICON=$'\U000F0599' ;;                                       # clear → sunny
  801|802) ICON=$'\U000F0595' ;;                                   # few/scattered clouds → partly cloudy
  803|804) ICON=$'\U000F0590' ;;                                   # broken/overcast → cloudy
  701|711|721|741) ICON=$'\U000F0591' ;;                           # mist/smoke/haze/fog
  300|301|310|321|500) ICON=$'\U000F0F33' ;;                       # light drizzle/rain → partly rainy
  302|311|312|313|314|501|520|521) ICON=$'\U000F0597' ;;           # moderate drizzle/rain → rainy
  502|503|504|522|531) ICON=$'\U000F0596' ;;                       # heavy/extreme rain → pouring
  511) ICON=$'\U000F067F' ;;                                       # freezing rain → snowy rainy
  600|601|620|621) ICON=$'\U000F0598' ;;                           # snow → snowy
  602|622) ICON=$'\U000F0F36' ;;                                   # heavy snow → snowy heavy
  611|612|613|615|616) ICON=$'\U000F067F' ;;                       # sleet → snowy rainy
  200|201|202|230|231|232) ICON=$'\U000F067E' ;;                   # thunderstorm with rain → lightning rainy
  210|211|212|221) ICON=$'\U000F0593' ;;                           # thunderstorm → lightning
  731|751|761|762) ICON=$'\U000F0F30' ;;                           # dust/sand/ash → hazy
  771) ICON=$'\U000F059D' ;;                                       # squall → windy
  781) ICON=$'\U000F0F38' ;;                                       # tornado
  *) ICON=$'\U000F0590' ;;                                         # default: cloudy
esac

# Sunrise/sunset overrides (within 30 min, clear/partly cloudy only)
SUNRISE_DIFF=$(( CURRENT - SUNRISE ))
SUNSET_DIFF=$(( CURRENT - SUNSET ))
if (( SUNRISE_DIFF >= 0 && SUNRISE_DIFF <= 1800 )) && [[ $WEATHER_MAIN == "Clear" || $WEATHER_ID == 801 || $WEATHER_ID == 802 ]]; then
  ICON=$'\U000F059C'                                               # nf-md-weather_sunset_up
elif (( SUNSET_DIFF >= 0 && SUNSET_DIFF <= 1800 )) && [[ $WEATHER_MAIN == "Clear" || $WEATHER_ID == 801 || $WEATHER_ID == 802 ]]; then
  ICON=$'\U000F059B'                                               # nf-md-weather_sunset_down
elif $IS_NIGHT; then
  case $WEATHER_ID in
    800) ICON=$'\U000F0594' ;;                                     # nf-md-weather_night (moon)
    801|802) ICON=$'\uEEEF' ;;                                     # nf-fa-cloud_moon
  esac
fi

sketchybar --set $NAME icon="$ICON" label="$TEMPERATURE°"
