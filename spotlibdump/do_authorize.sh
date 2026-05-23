#!/bin/bash

set +x

source spotify_creds.env

./authorize.sh

echo "yo: $SPOTIFY_CLIENT_ID"
