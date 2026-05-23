#!/bin/bash

# Based on:
# https://gist.github.com/hughrawlinson/1d24595d3648d53440552436dc215d90#
#
# Usage: authorize.sh <client_id> <client_secret>
#
# Generates an access and a refresh token. Run this on the machine
# where SLS is hosted. You should be able to listen for connections from
# the outside world.
#
# ncat (nc) must be installed on the host.
#
# On your Spotify dev dashboard, your app must have
# "http://<your_server_address>:8082/' # as its redirect_uri.
#

if [[ -z "$1" || -z "$2" ]]; then
	echo "Invalid arguments"
	exit -1
fi	

REDIS_CLI=redis6_CLI
CLIENT_ID=$1
CLIENT_SECRET=$2
PORT=8082
REDIRECT_URI="http%3A%2F%2Flocalhost%3A$PORT%2F"
SCOPES="playlist-read-private user-library-read user-modify-playback-state"
AUTH_URL="https://accounts.spotify.com/authorize/?response_type=code&client_id=$CLIENT_ID&redirect_uri=$REDIRECT_URI"
REDIS_KEY_CREDS="sls:spotify:credentials"
REDIS_KEY_ACCESSTOK="sls:spotify:access_token"
REDIS_CLI=redis6_cli

if [[ ! -z $SCOPES ]]; then
	ENCODED_SCOPES=$(echo $SCOPES| tr ' ' '%' | sed s/%/%20/g)
	AUTH_URL="$AUTH_URL&scope=$ENCODED_SCOPES"
fi

# Start user authentication
# Can't get Safari to work with nc reliably!
echo "Please open this URL in Chrome: $AUTH_URL"


# Serve up a response once the redirect happens.
RESPONSE=$(echo -e "HTTP/1.1 200 OK\nAccess-Control-Allow-Origin:*\nCache-Control: no-cache, no-store, must-revalidate\nContent-Length:77\n\n<html><body>Authorization successful, please close this page.</body></html>\n" | nc -l -c $PORT)

#echo $RESPONSE

CODE=$(echo "$RESPONSE" | grep "code=" | sed -e 's/^.*code=//' | sed -e 's/ .*$//')

RESPONSE=$(curl -s https://accounts.spotify.com/api/token \
  -H "Content-Type:application/x-www-form-urlencoded" \
  -H "Authorization: Basic $(echo -n "$CLIENT_ID:$CLIENT_SECRET" | base64)" \
  -d "grant_type=authorization_code&code=$CODE&redirect_uri=http%3A%2F%2Flocalhost%3A$PORT%2F")

echo $RESPONSE
echo "Expires:"
echo $RESPONSE | jq -r '.expires_in'

echo
echo "Access token:"
echo $RESPONSE | jq -r '.access_token'
echo
echo "Refresh token:"
echo $RESPONSE | jq -r '.refresh_token'


OUT="{"$'\n'
OUT+="   \"client_id\" : \"$CLIENT_ID\","$'\n'
OUT+="   \"client_secret\" : \"$CLIENT_SECRET\","$'\n'
OUT+="   \"refresh_token\": \""
OUT+=`echo $RESPONSE | jq -j '.refresh_token'`
OUT+="\""$'\n'
OUT+="}"$'\n'


$REDIS_CLI set "$REDIS_KEY_CREDS" "\"$OUT\""

ACCESS_TOKEN=`echo $RESPONSE | jq -r '.access_token'`

$REDIS_CLI set "$REDIS_KEY_ACCESSTOK" "$ACCESS_TOKEN"

