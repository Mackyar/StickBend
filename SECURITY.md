# Security Policy

## Supported versions

Security fixes go into the latest release of StickBend. Please update before reporting.

## Reporting a vulnerability

Email **macky.business725@gmail.com** with the subject "StickBend security". Please include:

- the StickBend version and format (VST3 or CLAP), your DAW and its version
- what happens, and the steps or project file that cause it

Please report privately first and give a reasonable time for a fix (aim: a reply within 7 days, a fix within 30) before posting details publicly. Reporters are credited in the changelog unless they prefer not to be.

## What StickBend does and doesn't do

These are the boundaries the plugin is designed to keep. Anything that breaks one of them is a vulnerability:

- It makes **no network connections**. The only links are the fixed addresses in the About panel, opened in your browser only when you click them.
- It **reads and writes no files**. Settings live in your DAW project, handled by the DAW.
- It **reads only the game controller** (through Windows XInput) and the MIDI your DAW sends it.
- Saved settings from project files are treated as untrusted: unknown data is ignored, and every value is range-checked before it is used.

## How releases are hardened

- Dependencies are pinned to exact commits, not tags.
- Windows builds use Control Flow Guard, CET shadow stack compatibility, ASLR, DEP and stack cookies.
- Every build runs a hostile-input test (`tests/statetest.cpp`) that feeds random and malicious project data to the plugin.
- Only download StickBend from the official sources listed on [mackyar.com](https://www.mackyar.com). Check the SHA-256 checksum published with each release.
