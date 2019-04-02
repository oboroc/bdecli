bdecli
======

Copyright (c) 2010-2019 Adrian Oboroc <https://github.com/oboroc/bdecli/>

About bdecli
------------

bdecli is a Borland Database Engine command-line configuration tool. It can
be included in deployment scripts or MSI custom actions to add or overwrite
idapi32.cfg configuration settings in unattended mode.

One way to handle idapi32.cfg file updates is to keep a master copy,
modify it in bdeadmin tool from Borland when necessary and then push updated
file to all workstation. If this is how you handle it, you dont need bdecli.

If, however, you want to have multiple configurations in the field and you
don't want to have an MSI overwrite idapi32.cfg file and delete all previous
custom settings, you should give bdecli a try. It will allow you to import or
update just the settings for the app you deploy and keep all other settings
intact.

bdecli is a bugfixed and renamed version of idapi32cfg 1.0.0 project. It was
moved from CodePlex to GitHub. Functionally they are identical, but idapi32cfg
executable has two serious bugs:
[75cb743](https://github.com/oboroc/bdecli/commit/75cb743) and
[4a48981](https://github.com/oboroc/bdecli/commit/4a48981).

If you use idapi32cfg program, it is best to update to the latest 1.0.x
version of bdecli.

bdecli is compiled with Visual C++ 6.0 SP6. The advantage of using such an old
compiler is that Windows 2000 and up includes full shared run-time. This means
that executable could stay small, no need to link run-time statically. Also it
means that shared runtime is automatically updated with security fixes as part
of Windows Update or WSUS patching. idapi32cfg executable from April 2010 was
compiled using Visual C++ 2010 with statically linked run-time and compressed
with UPX.


Using bdecli
------------

Start with a generic blank idapi32.cfg file. You can create one by deleting
your current idapi32.cfg file, opening bdeadmin tool and saving default
settings in new idapi32.cfg.

Export default settings to a text file using the following command:

		bdecli -e idapi32.cfg default.txt

Now open bdeadmin program again and make necessary configuration changes for
your application. Save to idapi32.cfg.

Export updated idapi32.cfg file to another text file:

		bdecli -e idapi32.cfg updated.txt

Use your favorite text file comparison tool (fc, windiff, winmerge etc.) to
find all lines in updated.txt that are not part of default.txt and put the
difference in a new text file. Lets call it cfgdiff.txt.

Add this new text file and bdecli executable to your MSI project or deployment
script. Use the following command to import application specific configuration
settings:

		bdecli -i idapi32.cfg cfgdiff.txt


Future development plans
------------------------

It is quite clear that applications that use BDE are on their last legs.
Nevertheless, I like working on this tool and plan to do major changes in
future. I'd like to:

* Add more command line settings and make bdecli work a lot like built-in
Windows reg.exe tool. This includes both batch and command-line querying,
adding, updating, removing and so on.

* Add some test units for manual validation or CI integration.

* Move from built-in data structure handling to uthash. It is a bit tough to
learn how to use it properly and it is currently not passing its own test
suite on Windows and Visual C++.

* Start providing a DLL version that could be called from MSI custom action.

* Add icon to bdecli executable. Use rc.exe or <http://code.google.com/p/rescle/>
for this at build time.

* Create better documentation.

* Add new release link to all my old forums posts about previous CodePlex release:
    - <http://www.dbforums.com/corel-paradox/1199036-modifying-idapi32-cfg.html>
    - <http://www.xtremevbtalk.com/showthread.php?t=209454>
    - <http://www.itninja.com/question/idapi32-cfg-file-for-bde-help>

* Consider using established command-line parameter parsing library like [argtable](http://argtable.sourceforge.net/).
