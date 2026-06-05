#!/bin/bash

REDIS_CLI="redis6-cli"
REDIS_KEY_CREDS="sls:spotify:credentials"
REDIS_KEY_ACCESSTOK="sls:spotify:access_token"

KEY_EXISTS=`$REDIS_CLI --raw exists "$REDIS_KEY_CREDS"`

if [[ "$KEY_EXISTS" -ne "1" ]]; then
	echo "No credentials found in redis, run do_authorize.sh first"
	exit -1
fi

CREDS=`$REDIS_CLI --raw get "$REDIS_KEY_CREDS"`

CLIENT_ID=$(echo "$CREDS" | jq -r '.client_id')
CLIENT_SECRET=$(echo "$CREDS" | jq -r '.client_secret')
REFRESH_TOKEN=$(echo "$CREDS" | jq -r '.refresh_token')

#echo "Client ID: $CLIENT_ID"
#echo "Client Secret: $CLIENT_SECRET"
#echo "Refresh token: $REFRESH_TOKEN"

#echo Refreshing

URL="https://accounts.spotify.com/api/token"

RESPONSE=$(curl -s "$URL" -H "Content-Type:application/x-www-form-urlencoded" \
  -H "Authorization: Basic $(echo -n "$CLIENT_ID:$CLIENT_SECRET" | base64 -w 0)" \
  -d "grant_type=refresh_token&refresh_token=$REFRESH_TOKEN")

#echo "$RESPONSE"

ACCESS_TOKEN=`echo $RESPONSE | jq -r '.access_token'`

$REDIS_CLI set "$REDIS_KEY_ACCESSTOK" "$ACCESS_TOKEN" >/dev/null

#echo Done
