This is a small GUI application that helps to download and play Bungie's classic [Marathon games][def1].
Bungie has allowed to release the Marathon games gratis on Github but they're still under a proprietary
license, making it hard to release them through a distro's packaging system.

External binaries that are expected to be in PATH are [alephone][def2], wget, xdg-open and xterm.

A custom download script can be specified through command line.
See `marathon-game-launcher --help` for a full list of options.

Build dependencies are: `cmake xxd libpng zlib libx11 libxrender libxft libfontconfig`

On Debian-based systems: `apt install build-essential cmake xxd libpng-dev zlib1g-dev libx11-dev libxrender-dev libxft-dev libfontconfig-dev`

Be sure to download FLTK first with `./get-fltk.sh` or `git clone https://github.com/fltk/fltk`.
Then simply run `make`.

[def1]: https://github.com/Aleph-One-Marathon
[def2]: https://github.com/Aleph-One-Marathon/alephone

