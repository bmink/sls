#!/bin/bash

# Based on:
# https://gist.github.com/hughrawlinson/1d24595d3648d53440552436dc215d90#
#
# Usage: authorize.sh <client_id> <client_secret>
#
# Generates an access and a refresh token.
#
# Run this on your local machine where you can open a browser and can listen
# for local connections
#
# On your Spotify dev dashboard, your app must have "http://127.0.0.1:8082/"
# as its redirect_uri.
#

if [ -z "$SPOTIFY_CLIENT_ID" ] || [ -z "$SPOTIFY_CLIENT_SECRET" ]; then
	echo "Spotify client creds not set"
	exit -1
fi	

REDIS_CLI=redis6_CLI
PORT=8082
REDIRECT_URI="http%3A%2F%2F127%2E0%2E0%2E1%3A$PORT%2Fcallback"
SCOPES="playlist-read-private user-library-read user-modify-playback-state"
AUTH_URL="https://accounts.spotify.com/authorize/?response_type=code&client_id=$SPOTIFY_CLIENT_ID&redirect_uri=$REDIRECT_URI" # &show_dialog=true"
REDIS_KEY_CREDS="sls:spotify:credentials"
REDIS_KEY_ACCESSTOK="sls:spotify:access_token"
REDIS_CLI=redis6_cli

if [[ ! -z $SCOPES ]]; then
	ENCODED_SCOPES=$(echo $SCOPES| tr ' ' '%' | sed s/%/%20/g)
	AUTH_URL="$AUTH_URL&scope=$ENCODED_SCOPES"
fi

# Start user authentication
# Can't get Safari to work with nc reliably!
echo "Opening $AUTH_URL..."

open "$AUTH_URL"

# Serve up a response once the redirect happens.
RESPONSE=$(echo -e "HTTP/1.1 200 OK\nAccess-Control-Allow-Origin:*\nCache-Control: no-cache, no-store, must-revalidate\nContent-Length:77\n\n<html><body>Authorization successful, please close this page.</body></html>\n" | nc -l -c $PORT)

#echo $RESPONSE

CODE=$(echo "$RESPONSE" | grep "code=" | sed -e 's/^.*code=//' | sed -e 's/ .*$//')

RESPONSE=$(curl -s https://accounts.spotify.com/api/token \
  -H "Content-Type:application/x-www-form-urlencoded" \
  -H "Authorization: Basic $(echo -n "$SPOTIFY_CLIENT_ID:$SPOTIFY_CLIENT_SECRET" | base64)" \
  -d "grant_type=authorization_code&code=$CODE&redirect_uri=$REDIRECT_URI")

#echo $RESPONSE
echo 
echo -n "Expires:"
echo $RESPONSE | jq -r '.expires_in'
echo
echo -n "Access token:"
echo $RESPONSE | jq -r '.access_token'
echo
echo -n "Refresh token:"
echo $RESPONSE | jq -r '.refresh_token'


OUT="{ "
OUT+="\\\"client_id\\\" : \\\"$SPOTIFY_CLIENT_ID\\\", "
OUT+="\\\"client_secret\\\" : \\\"$SPOTIFY_CLIENT_SECRET\\\", "
OUT+="\\\"refresh_token\\\": \\\" "
OUT+=`echo $RESPONSE | jq -j '.refresh_token'`
OUT+="\\\""
OUT+=" }"

echo
echo "On the server hosting SLS, please execute:"
echo

echo "redis-cli set \"$REDIS_KEY_CREDS\" \"$OUT\""

ACCESS_TOKEN=`echo $RESPONSE | jq -r '.access_token'`

echo
echo "and"
echo
echo "redis-cli set \"$REDIS_KEY_ACCESSTOK\" \"$ACCESS_TOKEN\""

