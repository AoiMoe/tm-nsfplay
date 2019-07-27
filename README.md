# tm-nsfplay

This is a CUI based NSF player for Linux, written by T.MITSUYOSHI <micchan@geocities.co.jp> in 2000's.


## Background

In 2000's, there were two individual NSF players called 'nsfplay':

- One was for Windows, distributed by [Digital Sound Antiques](http://dsa.sakura.ne.jp/)
- Another was for Linux, written by T.MITSUYOSHI <micchan@geocities.co.jp>.

This repository contains the source files of the latter one, with modifications to work with the recent Linux platforms.


## Requirement

This program is confirmed to work on Ubuntu-16.04.  To compile this on Ubuntu, the following packages will be needed:

- build-essential
- linux-headers
- alsa-oss
- zlib-dev


## Licenses

- kbhit : GPLv2
- nezplug([external source](https://github.com/AoiMoe/nezplug)) : PDS, although there are some exceptions
- nsfplay.c : GPLv2

For the details, see the text files in each portions.
