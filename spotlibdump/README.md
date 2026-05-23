# spotlibdump
Dumps a user's Spotify library metadata

# How to set up

Most often you want to run this on a server. The server has to have redis
installed and running.

SPOTIFY AUTH: The first auth with Spotify should be done on a local computer
where the shell can open URLs with a browswer and listen to local connections
(eg.  your dev laptop)

Put the Spotify client ID and client secred into a file called
```spotify_creds.env``` (see ```spotify_creds.env.example```). The file should
have permission mode ```600```. Do not check this file into source control!

Then run ```./do_authenticate.sh```. This will open a browswer window where you
authenticate with Spotify and grant the app access.

Once this is done, back on the CLI this will print a current token and a
refresh token. These should be added to to redis on the server, just follow the
instructions printed in the terminal.

On the server, the token should be refreshed periodically, so add to crontab:

```*/30 * * * * source /path/to/spotify_creds.env && /path/to/refresh_access_token.sh```




