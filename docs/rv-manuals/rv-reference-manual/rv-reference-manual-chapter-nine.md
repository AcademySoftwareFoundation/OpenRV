# Chapter 9 - Package System

Use the package system described below to expand RV.

### 9.1 rvpkg Command Line Tool

The rvpkg command line tool makes it possible to manage packages from the shell. If you use rvpkg you do not need to use RV's preferences UI to install/uninstall add/remove packages from the file system. We recommend using this tool instead of manually editing files to prevent the necessity of keeping abreast of how all the state is stored in new versions.The rvpkg tool can perform a superset of the functions available in RV's packages preference user interface.

| | |
| --- | --- |
| -include directory | include directory as if part of RV_SUPPORT_PATH |
| -env | show RV_SUPPORT_PATH include app areas |
| -only directory | use directory as sole content of RV_SUPPORT_PATH |
| -add directory | add packages to specified support directory |
| -remove | remove packages (by name, rvpkg name, or full path to rvpkg) |
| -install | install packages (by name, rvpkg name, or full path to rvpkg) |
| -uninstall | uninstall packages (by name, rvpkg name, or full path to rvpkg) |
| -optin | opt-in (load) now on behalf of all users, so it will be as if they opted in |
| -list | list installed packages |
| -info | detailed info about packages (by name, rvpkg name, or full path to rvpkg) |
| -force | Assume answer is 'y' to any confirmations – don't be interactive |

Table 9.1: rvpkg Options

**Note** : many of the below commands, including install, uninstall, and remove will look for the designated packages in the paths in the RV_SUPPORT_PATH environment variable. If the package you want to operate on is not in a path listed there, that path can be added on the command line with the -include option.

#### 9.1.1 Getting a List of Available Packages

```
 shell> rvpkg -list 
```

Lists all packages that are available in the RV_SUPPORT_PATH directories. Typical output from rvpkg looks like this:

```
I L - 1.7 "Annotation" /SupportPath/Packages/annotate-1.7.rvpkg
I L - 1.1 "Documentation Browser" /SupportPath/Packages/doc_browser-1.1.rvpkg
I - O 1.1 "Export Cuts" /SupportPath/Packages/export_cuts-1.1.rvpkg
I - O 1.3 "Missing Frame Bling" /SupportPath/Packages/missing_frame_bling-1.3.rvpkg
I - O 1.4 "OS Dependent Path Conversion" /SupportPath/Packages/os_dependent_path_conversion_mode-1.4.rvpkg
I - O 1.1 "Nuke Integration" /SupportPath/Packages/rvnuke-1.1.rvpkg
I - O 1.2 "Sequence From File" /SupportPath/Packages/sequence_from_file-1.2.rvpkg
I L - 1.3 "Session Manager" /SupportPath/Packages/session_manager-1.3.rvpkg
I L - 2.2 "RV Color/Image Management" /SupportPath/Packages/source_setup-2.2.rvpkg
I L - 1.3 "Window Title" /SupportPath/Packages/window_title-1.3.rvpkg 
```

The first three columns indicate installation status (I), load status (L), and whether or not the package is optional (O).If you want to include a support path directory that is not in RV_SUPPORT_PATH, you can include it like this:

```
 shell> rvpkg -list -include /path/to/other/support/area 
```

To limit the list to a single support area:

```
 shell> rvpkg -list -only /path/to/area 
```

The -include and -only arguments may be applied to other options as well.

#### 9.1.2 Getting Information About the Environment

You can see the entire support path list with the command:

```
 shell> rvpkg -env 
```

This will show alternate version package areas constructed from the RV_SUPPORT_PATH environment variable to which packages maybe added, removed, installed and uninstalled. The list may differ based on the platform.

#### 9.1.3 Getting Information About a Package

```
 shell> rvpkg -info /path/to/file.rvpkg 
```

This will result in output like:

```
Name: Window Title
Version: 1.3
Installed: YES
Loadable: YES
Directory:
Author: Tweak Software
Organization: Tweak Software
Contact: an actual email address
URL: http://www.tweaksoftware.com
Requires:
RV-Version: 3.9.11
Hidden: YES
System: YES
Optional: NO
Writable: YES
Dir-Writable: YES
Modes: window_title
Files: window_title.mu 
```

#### 9.1.4 Adding a Package to a Support Area

```
 shell> rvpkg -add /path/to/area /path/to/file1.rvpkg /path/to/file2.rvpkg 
```

You can add multiple packages at the same time. Remember that adding a package makes it become available for installation, it does not install it.

#### 9.1.5 Removing a Package from a Support Area

```
 shell> rvpkg -remove /path/to/area/Packages/file1.rvpkg 
```

Unlike adding, the package in this case is the one in the support area's Packages directory. You can remove multiple packages at the same time.If the package is installed rvpkg will interactively ask for confirmation to uninstall it first. You can override that by using -force as the first argument:

```
 shell> rvpkg -force -remove /path/to/area/Packages/file1.rvpkg 
```

#### 9.1.6 Installing and Uninstalling Available Packages

```
 shell> rvpkg -install /path/to/area/Packages/file1.rvpkg
shell> rvpkg -uninstall /path/to/area/Packages/file1.rvpkg 
```

If files are missing when uninstalling rvpkg may complain. This can happen if multiple versions where somehow installed into the same area.

#### 9.1.7 Combining Add and Install for Automated Installation

If you're using rvpkg from an automated installation script you will want to use the -force option to prevent the need for interaction. rvpkg will assume the answer to any questions it might ask is “yes”. This will probably be the most common usage:

```
 shell> rvpkg -force -install -add /path/to/area /path/to/some/file1.rvpkg 
```

Multiple packages can be specified with this command. All of the packages are installed into /path/to/area.To force uninstall followed by removal:

```
 shell> rvpkg -force -remove /path/to/area/Packages/file1.rvpkg 
```

The -uninstall option is unnecessary in this case.

#### 9.1.8 Overrideing Default Optional Package Load Behavior

If you want optional packages to be loaded by default for all users, you can do the following:

```
 shell> rvpkg -optin /path/to/area/Packages/file1.rvpkg 
```

In this case, rvkpg will rewrite the rvload2 file associated with the support area to indicate the package is no longer optional. The user can still unload the package if they want, but it will be loaded by default after running the command.

### 9.2 Package File Contents

A package file is zip file with at least one special file called PACKAGE along with .mu, .so, .dylib, and support files (plain text, images, icons, etc) which implement the actual package.Creating a package requires the zip binary. The zip binary is usually part of the default install on each of the OSes that RV runs on.The contents of the package should NOT be put in a parent directory before being zipped up. The PACKAGE manifest as well as any other files should be at the root level of the zip file.When a package is installed, RV will place all of its contents into subdirectories in one of the RV_SUPPORT_PATH locations. If the RV_SUPPORT_PATH is not defined in the environment, it is assumed to have the value of RV_HOME/plugins followed by the home directory support area (which varies with each OS: see the user manual for more info). Files contained in one zip file will all be under the same support path directory; they will not be installed distributed over more than one support path location.The install locations of files in the zip file is described in a filed called PACKAGE which must be present in the zip file. The minimum package file contains two files: PACKAGE and one other file that will be installed. A package zip file must reside in the subdirectory called Packages in one of the support path locations in order to be installed. When the user adds a package in the RV package manager, this is where the file is copied to.

### 9.3 PACKAGE Format

The PACKAGE file is a [YAML](http://www.yaml.org/) file providing information about how the package is used and installed as well as user documentation. Every package must have a PACKAGE file with an accurate description of its contents.The top level of the file may contain the following fields:

| Field | Value Type | Required | Description |
| --- | --- | --- | --- |
| package | string | • | The name of the package in human readable form |
| author | string | | The name of the author/creator of the package |
| organization | string | | The name of the organization (company) the author created the package for |
| contact | email address | | The email contact of the author/support person |
| version | version number | • | The package version |
| url | URL | | Web location for the package where updates, additional documentation resides |
| rv | version number | • | The minimum version of commercial RV which this package is compatible with |
| openrv | version number | • | The minimum version of Open RV which this package is compatible with |
| requires | zip file name list | | Any other packages (as zip file names) which are required in order to install/load this package |
| icon | PNG file name | | The name of an file with an icon for this package |
| imageio | file list | | List of files in package which implement Image I/O |
| movieio | file list | | List of files in package which implement Movie I/O |
| hidden | boolean | | Either “true” or “false” indicating whether package should be visible by default in the package manager |
| system | boolean | | Either “true” or “false” indicating whether the package was pre-installed with RV and cannot be removed/uninstalled |
| optional | boolean | | Either “true” or “false” indicating whether the package should appear loaded by default. If true the package is not loaded by default after it is installed. Typically this is used only for packages that are pre-installed. (Added in 3.10.9) |
| modes | YAML list | | List of modes implemented in the package |
| files | YAML list | | List non-mode file handling information |
| description | HTML 1.0 string | • | HTML documentation of the package for user viewing in the package manager |

Table 9.2:Top level fields of PACKAGE file.Each element of the modes list describes one Mu module which is implemented as either a .mu file or a .so file. Files implementing modes are assumed to be Mu module files and will be placed in the Mu subdirectory of the support path location. The other fields are used to optionally create a menu item and/or a short cut key either of which will toggle the mode on/off. The load field indicates when the mode should be loaded: if the value is “delay” the mode will be loaded the first time it is activated, if the value is “immediate” the mode will be loaded on start up.

| Field | Value Type | Required | Description |
| --- | --- | --- | --- |
| file | string | • | The name of the file which implements the mode |
| menu | string | | If defined, the string which will appear in a menu item to indicate the status (on/off) of the mode |
| shortcut | string | | If defined and menu is defined the shortcut for the menu item |
| event | string | | Optional event name used to toggle mode on/off |
| load | string | • | Either immediate or delay indicating when the mode should be loaded |
| icon | PNG image file | | Icon representing the mode |
| requires | mode file name list | | Names of other mode files required to be active for this mode to be active |

Table 9.3:Mode Fields

As an example, the package window_title-1.0.rvpkg has a relatively simple PACKAGE file shown here:
<a id="example-PACKAGE"></a>

```
package: Window Title
author: Tweak Software
organization: Tweak Software
contact: some email address of the usual form
version: 1.0
url: http://www.tweaksoftware.com
rv: 3.6
requires: ''

modes:
  - file: window_title
    load: immediate

description: |

  <p> This package sets the window title to something that indicates the
  currently viewed media.
  </p>

  <h2>How It Works</h2>

  <p> The events play-start, play-stop, and frame-changed, are bound to
  functions which call setWindowTitle(). </p> 
```

When the package zip file contains additional support files (which are not specified as modes) the package manager will try to install them in locations according to the file type. However, you can also directly specify where the additional files go relative to the support path root directory.

| | | | |
| --- | --- | --- | --- |
| Field | Value Type | Required | Description |
| file | string | • | The name of the file in the package zip file |
| location | string | • | Location to install file in relative to the support path root. This can contain the variable $PACKAGE to specify special package directories. E.g. SupportFiles/$PACKAGE is the support directory for the package. |

Table 9.4:File FieldsFor example if you package contains icon files for user interface, they can be forced into the support files area of the package like this:

```
 files:
  - file: myicon.tif
    location: SupportFiles/$PACKAGE 
```

### 9.4 Package Management Configuration Files

There are two files which the package manager creates and uses: rvload2 (previous releases had a file called rvload) in the Mu subdirectory and rvinstall in the Packages subdirectory. rvload2 is used on start up to load package modes and create stubs in menus or events for toggling the modes on/off if they are lazy loaded. rvinstall lists the currently known package zip files with a possible an asterisk in front of each file that is installed. The rvinstall file in used only by the package manager in the preferences to keep track of which packages are which.The rvload2 file has a one line entry for each mode that it knows about. This file is automatically generated by the package manager when the user installs a package with modes in it. The first line of the file indicates the version number of the rvload2 file itself (so we can change it in the future) followed by the one line descriptions.For example, this is the contents of rvload2 after installing the window title package:

```
3
window_title,window_title.zip,nil,nil,nil,true,true,false 
```

The fields are:

1. The mode name (as it appears in a require statement in Mu)
2. The name of the package zip file the mode originally comes from
3. An optional menu item name
4. An optional menu shortcut/accelerator if the menu item exists
5. An optional event to bind mode toggling to
6. A boolean indicating whether the mode should be loaded immediately or not
7. A boolean indicating whether the mode should be activated immediately
8. A boolean indicating whether the mode is optional so it should not be loaded by default unless the user opts-in.

Each field is separated by a comma and there should be no extra whitespace on the line. The rvinstall file is much simpler: it contains a single zip file name on each line and an asterisk next to any file which is current known to be installed. For example:

```
crop.zip
layer_select.zip
metadata_info.zip
sequence_from_file.zip
*window_title.zip 
```

In this case, five modes would appear in the package manager UI, but only the window title package is actually installed. The zip files should exist in the same directory that rvinstall lives in.

### 9.5 Developing a New Package

In order to start a new package there is a chicken and egg problem which needs to be overcome: the package system wants to have a package file to install.The best way to start is to create a source directory somewhere (like your source code repository) where you can build the zip file form its contents. Create a file called PACKAGE in that directory by copying and pasting from either this manual (listing [9.3](#example-PACKAGE) ) or from another package you know works and edit the file to reflect what you will be doing (i.e. give it a name, etc).If you are writing a Mu module implementing a mode or widget (which is also a mode) then create the .mu file in that directory also.You can at that point use zip to create the package like so:

```
 shell> zip new_package-0.0.rvpkg PACKAGE the_new_mode.mu 
```

This will create the new_package-0.0.rvpkg file. At this point you're ready to install your package that doesn't do anything. Open RV's preferences and in the package manager UI add the zip file and install it (preferably in your home directory so it's visible only to you while you implement it).Once you've done this, the rvload2 and rvinstall files will have been either created or updated automatically. You can then start hacking on the installed version of your Mu file (not the one in the directory you created the zip file in). Once you have it working the way you want copy it back to your source directory and create the final zip file for distribution and delete the one that was added by RV into the Packages directory.

#### 9.5.2 Using the Mode Manager While Developing

It's possible to delay making an actual package file when starting development on individual modes. You can force RV to load your mode (assuming it's in the MU_MODULE_PATH someplace) like so:

```
 shell> rv -flags ModeManagerLoad=my_new_mode 
```

where my_new_mode is the name of the .mu file with the mode in it (without the extension).You can get verbose information on what's being loaded and why (or why not by setting the verbose flag):

```
 shell> rv -flags ModeManagerVerbose 
```

The flags can be combined on the command line.

```
 shell> rv -flags ModeManagerVerbose ModeManagerLoad=my_new_mode 
```

If your package is installed already and you want to force it to be loaded (this overrides the user preferences) then:

```
 shell> rv -flags ModeManagerPreload=my_already_installed_mode 
```

similarly, if you want to force a mode not to be loaded:

```
 shell> rv -flags ModeManagerReject=my_already_installed_mode 
```

#### 9.5.3 Using -debug mu

Normally, RV will compile Mu files to conserve space in memory. Unfortunately, that means loosing a lot of information like source locations when exceptions are thrown. You can tell RV to allow debugging information by adding -debug mu to the end of the RV command line. This will consume more memory but report source file information when displaying a stack trace.

#### 9.5.4 The Mu API Documentation Browser

The Mu modules are documented dynamically by the documentation browser. This is available under RV's help menu “Mu API Documentation Browser”.

### 9.6 Loading Versus Installing and User Override

The package manager allows each user to individually install and uninstall packages in support directories that they have permission in. For directories that the user does not have permission in the package manager maintains a separate list of packages which can be excluded by the user.For example, there may be a package installed facility wide owned by an administrator. The support directory with facility wide packages only allows read permission for normal users. Packages that were installed and loaded by the administrator will be automatically loaded by all users.In order to allow a user to override the loading of system packages, the package manager keeps a list of packages not to load. This is kept in the user's preferences file (see user manual for location details). In the package manager UI the “load” column indicates the user status for loading each package in his/her path.

#### 9.6.1 Optional Packages

The load status of optional packages are also kept in the user's preferences, however these packages use a different preference variable to determine whether or not they should be loaded. By default optional packages are not loaded when installed. A package is made optional by setting the \`\`optional'' value in the PACKAGE file to true.
