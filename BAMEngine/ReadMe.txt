Tips to compile it with Visual Studio 2017 Community

- Missing "rpc.h":
Detais:
https://developercommunity.visualstudio.com/content/problem/24354/cant-compile-vs2015-project-in-vs2017-missing-c-in.html
Run VS 2017 intaller and select "Windows 8.1 SDK + UCRT SDK" under the "Desktop development with C++".

- When you get error: 1>LINK : fatal error LNK1104: cannot open file 'gdi32.lib'
Run VS 2017 intaller and select "Windows 10 SDK (10.0.15063.0) for Desktop C++ x86 and x64" under the "Desktop development with C++".
Details: 
https://social.msdn.microsoft.com/Forums/vstudio/en-US/a114380e-29f2-473f-9d26-b780fb16d856/vs2017-community-new-project-link-fatal-error-lnk1104-cannot-open-file-gdi32lib?forum=vcgeneral
https://stackoverflow.com/questions/33599723/fatal-error-lnk1104-cannot-open-file-gdi32-lib


Any way.... how reads "readme.txt"?
========================================================================
    DYNAMIC LINK LIBRARY : BAMEngine Project Overview
========================================================================

AppWizard has created this BAMEngine DLL for you.

This file contains a summary of what you will find in each of the files that
make up your BAMEngine application.


BAMEngine.vcxproj
    This is the main project file for VC++ projects generated using an Application Wizard.
    It contains information about the version of Visual C++ that generated the file, and
    information about the platforms, configurations, and project features selected with the
    Application Wizard.

BAMEngine.vcxproj.filters
    This is the filters file for VC++ projects generated using an Application Wizard. 
    It contains information about the association between the files in your project 
    and the filters. This association is used in the IDE to show grouping of files with
    similar extensions under a specific node (for e.g. ".cpp" files are associated with the
    "Source Files" filter).

BAMEngine.cpp
    This is the main DLL source file.

	When created, this DLL does not export any symbols. As a result, it
	will not produce a .lib file when it is built. If you wish this project
	to be a project dependency of some other project, you will either need to
	add code to export some symbols from the DLL so that an export library
	will be produced, or you can set the Ignore Input Library property to Yes
	on the General propert page of the Linker folder in the project's Property
	Pages dialog box.

/////////////////////////////////////////////////////////////////////////////
Other standard files:

StdAfx.h, StdAfx.cpp
    These files are used to build a precompiled header (PCH) file
    named BAMEngine.pch and a precompiled types file named StdAfx.obj.

/////////////////////////////////////////////////////////////////////////////
Other notes:

AppWizard uses "TODO:" comments to indicate parts of the source code you
should add to or customize.

/////////////////////////////////////////////////////////////////////////////
