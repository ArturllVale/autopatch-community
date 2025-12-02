# Frequently Asked Questions

## Where can I find documentation or guide on how to configure Elurair?

Check the sample configuration `elurair.default.ini` in the 'Free' release. It contains both documentation for each setting, as well as examples.

## Are encrypted GRFs supported?

Yes. Most common encryption schemes are supported as long they only encrypt the item data, but otherwise do not violate the GRF file format.

Note: When using the encryption feature of "GRF Editor" by "Tokei", do not encrypt patches used for the `delete` modifier. Doing so will destroy the hidden `__encryption.info` item, which is needed for the encryption feature to work. To restore it, add an encrypted patch without delete modifier.

## Why isn't the *patch progress file* (aka `patch.inf`) created automagically, why do I have to provide it?

This is a safety precaution to avoid

1. running given patcher in a folder it wasn't meant for (ex. attempting to update *kRO Renewal* in a *kRO Zero* installation),
2. accidentally pseudo-bootstrapping a non-RO folder (i.e. downloading the entire patch history, which may amount to several GiB of data, OR failing to download at all, because the initial patch server is already dead),
3. further corrupting a corrupt install (to raise awareness, that the install is indeed corrupt).

It is a fact, that the *original patcher* DOES auto-create this file, but past real-world experience has shown, that it is a bad idea. If you have trouble creating this file, you may use the `http://ai4rei.net/make/patch.inf/<desired numeric id>` tool to create it.

## Why isn't the *patch progress file* included with the patcher?

The contents of it depend on the setup. So while most custom GRF archives may start at 0, some setups may install the patcher as drop-in replacement (where meta files remain the same) or an official GRF archive may be included, which already has its own progress file. Blindly dumping a progress file into the patcher folder without understanding the implications would be the same, as creating it automagically (see previous).

## Why does my ZIP archive fail to open/extract?

ZIP archives must comply with ISO IEC 21320-1. Features, that are not part of the standard are not supported.

## Why does my PNG skin file fail to load or not load correctly?

PNG interlacing is not supported in any form and will cause the image to fail to load. Both transparent color and alpha channel are currently ignored.

## How do I enable the developer menu?

Enable developer mode (`DevMode`) and right-click an unobscured part of the Elurair UI.
