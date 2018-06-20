# Installing the LEMON library	{#LemonInstall}

This page explains how to install the LEMON library.
You may either install it in your filesystem or include it locally in ``mathtrader++``.

## Download

The latest stable release of the LEMON library may be found [here](http://lemon.cs.elte.hu/trac/lemon/wiki/Downloads).
It is recommended to download version ``1.3.1``
([tar.gz](http://lemon.cs.elte.hu/pub/sources/lemon-1.3.1.tar.gz),
[zip](http://lemon.cs.elte.hu/pub/sources/lemon-1.3.1.zip)).

## Including the library

This section describes how to install the LEMON library.
The official installation guide may be found [here](http://lemon.cs.elte.hu/trac/lemon/wiki/InstallGuide).

Please note that:

  * Packages ``GLPK``, ``ILOG``, ``COIN`` and ``SOPLEX`` may be reported
  as missing. It is not necessary to install them to build LEMON.
  * On a Windows IDE, you might have to appropriately configure your IDE to enable C++14 support.

### Build and install from source

To install from source, please refer to the official [Linux](http://lemon.cs.elte.hu/trac/lemon/wiki/InstallLinux)
or [Windows](http://lemon.cs.elte.hu/trac/lemon/wiki/InstallCmake) guides.

### Include as ``mathtrader`` subproject

If you cannot install the library or do not want to, you may alternatively
include the LEMON library within the ``mathtrader`` project.
To do that:

1. Decompress (unzip) the ``lemon-x.y.z.tar.gz`` or ``.zip`` file **in the top ``mathtrader/`` folder**.
2. Rename it to ``lemon`` or move it under ``deps/lemon``.

The ``cmake`` script of ``mathtrader`` will build and include the library together with the ``mathtrader++`` executable.
