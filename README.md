bdecli
======

Copyright (C) 2010, 2014 Adrian Oboroc <https://github.com/oboroc/bdecli/>

Licensed to the Apache Software Foundation (ASF) under one or more
contributor license agreements.  See the NOTICE file distributed with
this work for additional information regarding copyright ownership.
The ASF licenses this file to You under the Apache License, Version 2.0
(the "License"); you may not use this file except in compliance with
the License.  You may obtain a copy of the License at

 <http://www.apache.org/licenses/LICENSE-2.0>

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.


About bdecli
------------

bdecli is a Borland Database command-line interface configuration tool. It can
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
executable has one serious bug:
<https://github.com/oboroc/bdecli/commit/75cb74338b90cc907a9d7b81a87f8b5bc95c55c6>.
If you used idapi32cfg program, it is best if you update to the latest 1.0.x
version of bdecli.

bdecli is compiled with Visual C++ 6.0 SP6. The advantage of using such an old
compiler is that any Windows version starting with 2000 and up includes full
shared run-time for Microsoft Visual C++ 6.0. This means that executable could
stay small, no need to link run-time statically. Also it means that shared
runtime is automatically updated with security fixes as part of Windows Update
or WSUS patching. Historically, idapi32cfg executable was compiled with
Visual C++ 2010 with staticly linked run-time and compressed with UPX.


Using bdecli
------------

Start with a generic blank idapi32.cfg file. You can create one by deleting
your current idapi32.cfg file, opening bdeadmin tool and saving default
settings in new idapi32.cfg. Close bdeadmin.

Export default settings to text file using the following command:

		bdecli -e idapi32.cfg default.txt

Now open bdeadmin program again and make necessary configuration changes for
your application. Save to idapi32.cfg and close bdeadmin.

Export newly updated idapi32.cfg file to another text file:

		bdecli -e idapi32.cfg updated.txt

Use your favorite text file comparison tool (windiff, winmerge, fc) to find all
lines in updated.txt that are not part of default.txt and put the difference in
new text file, lets call it cfgdiff.txt.

Add this new text file and bdecli executable to your MSI project or deployment
script. Use the following command to import application specific configuration
settings:

		bdecli -i idapi32.cfg cfgdiff.txt


Future development plans
------------------------

It is quite clear that applications that use BDE are on their last legs.
Nevertheless, I like working on this tool and plan to do major changes in future.
I want to add more command line settings and make bdecli work a lot like Windows
built-in reg.exe tool. This includes both batch and command-line querying,
adding, updating, removing and so on.

I will probably also add some test units for manual validation or CI integration.

I consider moving from built-in data structure handling code to uthash, but it is
a bit tough to learn how to use properly and currently not mainained for Windows
and Visual C++.

Finally, I will probably start providing a DLL version that could be called from
MSI custom action.
