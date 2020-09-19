# sls

SLS, the Simple/Sane/? Library System

![](screenshot.jpg)

SLS integrates my various music libraries into a single database that's
automatically kept up to date, and offers a simple browsing experience.

Libraries currently supported:

* Spotify (via Spotify Web API)
* Vinyl (via Discogs Web API)

The main function of SLS is to display a random selection of albums in the
library in a clean & minimalist UI, something that is impossible to do in any
streaming music app I have ever tried. IMO Spotify in particuler does a very
poor job of enabling the user to browse and "re-discover" items in her own
library. Additionally, much has been written about whether the Spotify app's
random function is indeed random, to me it seems clear that it is not. (That
said, there are things Spotify excels at, such as discovery, and the streaming
experience itself.)

On the physical and digital media side, while many vinyl collectors use
Discogs to catalogue their collection; and while the Discogs app does offer a
"random album" function, it still misses the mark -- I like to see
20 or so randomly selected albums at once. Plus, it would be nice to see
all libraries combined into one UI... hence SLS.

SLS *does not* replace the use of the various apps themselves. I still use
Spotify for discovery, playback and library management itself. Similarly, I use
the Discogs app to manage my vinyl collection. But now, whenever deciding
what to listen to next, instead of being forced into the various apps' browsing
model, I start with SLS. Invariably within a few seconds, I will see something
that fits my mood or that I haven't heard in a long time and then can click through
to the album in the Spotify app (or go play the record).

SLS runs on one of my Raspi servers. The library refresh jobs are run every few
minutes via a cronjob and the libraries are stored in a Redis database (Ie. if
I add an album to my library in the Spotify app, it will show up in Redis
within a few minutes.)


