====================================================================
Elurair \ ē-'lür'er \
(c) 2012-2025 Ai4rei/AN

====================================================================

# About

Universal auto-patcher for all your updating needs combined with
a launcher, which is fully skin-able, highly customizable and easy
on resources. It is free of any cost and works on every 32-bit and
64-bit Microsoft Windows platform.

# Known Bugs & Issues

- [aavc] Is not safe for computers running Avast Anti-Virus
- [aavc] Is not safe for computers running Anti-Virus products
  incorporating Parametrically Averaging Inferences (PAI) as part of
  their detection routines.

# Requirements

## Operating System

- Windows 98/ME
- Windows 2000/XP/7/8/10/11

Other Windows versions or emulations thereof, and server editions
may work, but are not directly supported.

The Builder tool does not run on Windows 98/ME.

## Hardware

- x86 platform, either 32-bit or 64-bit
- Keyboard and/or a pointing device

# Usage

This software does not run out-of-the-box, but has to be configured
first. As part of the distribution package you shall find this
documentation, sample configuration (elurair.sample.ini) and
a sample skin (skin/). Most explanation is provided in the sample
configuration. At the very least, you have to configure following:

- Configuration ID `ConfigID` unique to your server, which ideally
  should include corresponding domain name
- All skin-related parameters, buttons and browsers that make up the
  entire look and feel of the patcher
- All patch-server-related parameters, unless patching functionality
  is not needed
- Mark the configuration as configured

To insert the configuration and skin files into the patcher binary,
use the included Builder tool.

# Command Line

Elurair accepts any options for purposes of argument forwarding
through the `ExternalArgs` setting. Following special arguments are
recognized and ignored during `ExternalArgs` processing:

## `-elurair:firstrun`

Initializes all missing `InfFile` files to zero, assuming a patch
instance without any history. See `doc/inffile-spec.txt` for the
consequences of using this function. Intended as post-install task
in installer applications.

## `-elurair:noui`

Does not start the UI and immediately exits. Intended for use with
other arguments for silent operation.

# Support

As this is a free product, support is limited and to 'benefit all',
when documentation is expanded, features added, changed or fixed.
Requests, that do not meet this description require a specific
sacrifice to be offered.

# Updating & Removal

For updating you have to check the website for newer versions.
Overwrite the old files with the new ones.

If you no longer want to use the patcher, delete elurair.exe,
associated docs and configuration files. Elurair does not store any
user data in the registry or anywhere else.

# Disclaimer

You use the application at YOUR OWN RISK. You are always encouraged
backup your data before using this release. I'm not liable for any
direct or indirect damages on software or hardware ( including, but
not limited to data loss, corruption, crashes ), that may arise from
using this piece of software. If you don't agree with this, delete
this application entirely.

# Contact

Website: http://ai4rei.net/p/skal
Support: http://ai4rei.net/p/bbs

# Legal Stuff & Used Components

Uses zlib 1.3.1 library for (de)compression Copyright (C) 1995-2024
Jean-loup Gailly and Mark Adler ( zlib.net ).
