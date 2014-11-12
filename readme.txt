The Core Pocket Media Player

1) Homepage
-----------

http://www.tcpmp.com

2) Authors
----------

- Player engine
  Copyright (C) 2004-2006 Gabor Kovacs <picard@demoscene.hu>

- Ogg Vorbis Tremor
  Copyright (c) 2002, Xiph.org Foundation

- libmad - MPEG audio decoder library
  Copyright (C) 2000-2004 Underbit Technologies, Inc.

- liba52
  Copyright (C) 2000-2003 Michel Lespinasse <walken@zoy.org>
  Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>

- libfaad - FAAD2 - Freeware Advanced Audio (AAC) Decoder including SBR decoding
  Copyright (C) 2003-2004 M. Bakker, Ahead Software AG, http://www.nero.com
  
- MatroskaParser
  Copyright (c) 2004-2005 Mike Matsnev

- zlib data compression 
  Copyright (C) 1995-2003 Jean-loup Gailly and Mark Adler

- Portable Musepack decoder (as separate plugin)
  http://www.musepack.net

- FFmpeg library
  Copyright (c) 2000-2005 The FFmpeg Project.

- Intel 2700G SDK headers
  Copyright (c) 2004, Intel Corporation

- Mobile Stream Rotation extension headers
  Copyright (c) 2005 Mobile Stream (http://www.mobile-stream.com)

- MikMod sound library
  Copyright (c) 1998, 1999, 2002 Miodrag Vallat and others - see file AUTHORS for complete list.

- CoreTheque
  Copyright (c) 2005 Steve Lhomme

- libFLAC - Free Lossless Audio Codec library
  Copyright (C) 2001,2002,2003,2004,2005  Josh Coalson

- TTA Hardware Players Library
  Copyright (c) 2004 True Audio Software. All rights reserved.

- WAVPACK
  Copyright (c) 1998-2004 Conifer Software. All rights reserved.

- CoreASP
  Copyright (C) 2004-2006 Gabor Kovacs <picard@demoscene.hu>

- CoreAVC
  Copyright (C) 2004-2006 Gabor Kovacs <picard@demoscene.hu>

- CoreMP3
  Copyright (C) 2004-2006 Gabor Kovacs <picard@demoscene.hu>


Special thanks for these project - used as reference:

- PocketMVP
- FFmpeg
- XviD
- The Magic Lantern

3) Licensing
------------

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

4) Latest version
-----------------

You can export or checkout the latest version with 
SubVersion (http://subversion.tigris.org) from the repository:

http://svn.corecodec.org/tcpmp/trunk

4) WinCE Compiling Info
-----------------
What you need for compiling:
* eMbedded Visual C++ 4.0 (+SP4) (http://download.microsoft.com)

additionaly for AMR decoder:
* AMR reference code from: 
  http://www.3gpp.org/ftp/Specs/archive/26_series/26.104/26104-610.zip
  http://www.3gpp.org/ftp/Specs/archive/26_series/26.204/26204-600.zip

additionaly for x86 or emulator build:
* nasm - The Netwide Assembler (http://nasm.sourceforge.net)

additionaly for batch building (build.bat) you need:
* eMbedded Visual Tools 3.0 - 2002 Edition (EVC3)
* GCC ARM cross compiler (see crossgcc.txt)
* Smartphone 2002 SDK (included in EVC3)
* EZSETUP (http://www.spbsoftwarehouse.com/products/ezsetup/?en)
* UPX (http://upx.sourceforge.net/)
* tar,zip

For the release version I use a special mixture enviroment
to get the best code. I use EVC3 with armasm.exe from EVC4 (wce420\bin)
and GCC ARM cross compiler for some codec parts. 

EVC compiling settings (if project files are recreated)
* warning level 1
* for release: optimizations = maximize speed (except MIPS common.dll which should be default!)
* for dlls define: <NAME>_EXPORTS
* check 'changelog' in library directoies for other defines
* insert '../' before output file name for all player projects
* linker: /stack:0x20000

5) Palm Compiling info
----------------------

For the non plugin version (Makefile) you will need the 
official prc-tools, pilrc, make, gcc packages (gcc needed to compile some
tools helping compilation on desktop)

Additionally for the official plugin version (Makeplugin) 
you need a modified gcc. Check cross-compile subdirectory 
for details and binutils,gcc patches.

Additionally required tools:

Palm OS SDK from PalmSource
http://www.palmos.com/dev/tools/core.html
(minimum version 5r4)

palmOne SDK from palmOne (have to sign in to PluggedIn member area)
(minimum version 4.3)

Sony CLIE SDK
http://www.palm.projekt-base.de/wiki/cgi-bin/wiki.cgi?ClieDeveloper/Sony_SDK5
(minimum version 50e)

AMR reference code from: 
http://www.3gpp.org/ftp/Specs/archive/26_series/26.104/26104-610.zip
http://www.3gpp.org/ftp/Specs/archive/26_series/26.204/26204-600.zip

