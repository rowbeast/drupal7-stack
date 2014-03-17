Description
-----------

This module uses METAR data to display current weather conditions from
anywhere in the world.

Features
--------

* Supports an unlimited number of blocks and an unlimited number
  of locations per block
* Provides a custom block which can be administrated by a user
* Users can search for a location and get a display of the
  weather conditions
* Integrates with Location module and shows the weather nearby
  a node's location
* Displays different images for daytime and nighttime
* Download of current METAR data is scheduled in a smart way
  to reduce unnecessary network traffic and keep your site
  responsive
* Provides three different methods to download the METAR data
  (fopen, curl, and wget) and multiple download locations
* Displayed units of the weather data are completely configurable
* Layout of displayed block is configurable with the template engine
* Fully translatable, currently available in English, German,
  Dutch, Czech, Norwegian, Russian, Hungarian, Danish, Italian,
  Spanish, French, Polish, Brazilian Portuguese, Turkish,
  Romanian, Greek, and Croatian.
* Works well with MySQL or PostgreSQL databases

Author
------
Tobias Quathamer <t.quathamer@gmx.net>

It was inspired by the Weather module which was written in 2004 by
Gerard Ryan <gerardryan@canada.com>.
