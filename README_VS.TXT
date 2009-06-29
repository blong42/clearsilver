
ClearSilver-0.10.5 compilation under Micro$oft Visual Studio 2005 and 2008
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

1. Download and extract clearsilver-0.10.5.tar.gz

1. Download and extract GnuWin32 "patch" utility:
http://gnuwin32.sourceforge.net/downlinks/patch-bin-zip.php

3. Copy the clearsilver-0.10.5-vs.patch file in clearsilver-0.10.5 directory.

2. Open "DOS" console, and change directory to clearsilver-0.10.5 directory.

3. Apply the patch:
[patch_utily_dir\]patch -p1 <clearsilver-0.10.5-vs.patch

2. Load the "vs/clearsilver.sln" solution in MSVC.

3. Enjoy!




Modifications & patches:
T. Szolnoki (szolnoki@freemail.hu)