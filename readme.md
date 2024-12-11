# Simple console mOTP tool

[![Build-macOS-latest Actions Status](https://github.com/rozhuk-im/motp/workflows/build-macos-latest/badge.svg)](https://github.com/rozhuk-im/motp/actions)
[![Build-Ubuntu-latest Actions Status](https://github.com/rozhuk-im/motp/workflows/build-ubuntu-latest/badge.svg)](https://github.com/rozhuk-im/motp/actions)


Rozhuk Ivan <rozhuk.im@gmail.com> 2024-2025

mOTP is C based software for generate Mobile-OTP codes.



## Licence
BSD licence.


## Donate
Support the author
* **GitHub Sponsors:** [!["GitHub Sponsors"](https://camo.githubusercontent.com/220b7d46014daa72a2ab6b0fcf4b8bf5c4be7289ad4b02f355d5aa8407eb952c/68747470733a2f2f696d672e736869656c64732e696f2f62616467652f2d53706f6e736f722d6661666266633f6c6f676f3d47697448756225323053706f6e736f7273)](https://github.com/sponsors/rozhuk-im) <br/>
* **Buy Me A Coffee:** [!["Buy Me A Coffee"](https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png)](https://www.buymeacoffee.com/rojuc) <br/>
* **PayPal:** [![PayPal](https://srv-cdn.himpfen.io/badges/paypal/paypal-flat.svg)](https://paypal.me/rojuc) <br/>
* **Bitcoin (BTC):** `1AxYyMWek5vhoWWRTWKQpWUqKxyfLarCuz` <br/>



## Features
* Allow set timezone while code genereation
* Allow set time while code genereation



## What is this for?

If you need to generate [![one-time passwords](//en.wikipedia.org/wiki/One-time_password)] using the [![Mobile-OTP / mOTP algorithm](http://motp.sourceforge.net/)], from the command-line, without futzing around with your phone.

This is a self-proclaimed “standard” algorithm for one-time passwords. It is confusing, seemingly almost obsolete, and appears to mainly be used in German-speaking countries. **You don't want to use this unless you are forced to interact with an existing system that uses such codes for 2FA.**


## How does it work?

The principle of the algorithm is [![explained badly](http://motp.sourceforge.net/#1.1)]
and [![implemented confusingly](http://motp.sourceforge.net/bash/otpverify.sh)] but it's very simple: you have a token secret
(an ASCII string, usually hexadecimal or alphanumeric) and a PIN (an ASCII string, usually 4 digits), and this is converted to a 6-digit code using:

```
counter = ASCII((UNIX epoch time) / 10s)
digest = MD5HEX(counter || secret || pin)
code = LEFT(digest, 6 characters)
```


## Compilation

### Linux
```
sudo apt-get install build-essential git fakeroot
git clone --recursive https://github.com/rozhuk-im/motp.git
cd motp/src
cc motp.c -O2 -I../3rdparty/liblcb/include/crypto/hash -lm -o motp
```

### FreeBSD / DragonFlyBSD
```
sudo pkg install git
git clone --recursive https://github.com/rozhuk-im/motp.git
cd motp/src
cc motp.c -O2 -I../3rdparty/liblcb/include/crypto/hash -lm -o motp
```


## Usage

### help
``` shell
motp     Simple console mOTP tool
Usage: motp [options]
options:
	-help, -? 		Show help
	-secret, -s <string>	Shared secret
	-pin, -p <string>	PIN
	-duration, -P seconds>	Code duration interval. Default: 10
	-length, -d <number>	Result code length. Default: 6
	-time, -t <string>	Time string, in one of formats: HTTP date / RFC 822, RFC 850, ANSI C, YYYY-MM-DD HH:MM:SS, Number of seconds since the Epoch (UTC)
	-tz, -T <string>	The timezone time zone offset from UTC. Will override time zone from 'time' string is set. Ex: +0100, -0500.
```

### example
``` shell
motp -s 'secret' -p '1111'
```
