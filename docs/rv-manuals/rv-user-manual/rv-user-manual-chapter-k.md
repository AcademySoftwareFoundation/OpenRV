# K - Localizing media paths with RV_OS_PATH or RV_PATHSWAP

## RV_OS_PATH Variables

**RV_OS_PATH** variables work similarly to **RV_PATHSWAP** variables, but they can be used to adjust paths of media files (and LUTs etc) that are more arbitrary, and are specifically targeted at supporting multi-OS environments.

For example, if your site supports Linux/Windows/OSX machines, and the same basic file system structure appears on all OSs but at different roots, setting a group of three environment variables on all machines will allow some interoperability. Suppose you have the following three "roots" under which your directory structure is identical on all OSs:

| OS | Root |
| --- | --- |
| OSX | /shows |
| LINUX | /net/shows |
| WINDOWS | c:/shows |

Then if you can ensure that all users have the following environment variables set, paths will automatically be converted on input to RV:

| Variable Name | Variable Value |
| --- | --- |
| RV_OS_PATH_OSX | /shows |
| RV_OS_PATH_LINUX | /net/shows |
| RV_OS_PATH_WINDOWS | c:/shows |

For example if RV is running on Windows and receives a path like "/net/shows/sw4/trailer.mov" it will convert it to "c:/shows/sw4/tralier.mov" before use. ( **Note:** UNC paths are also supported).

If some of your production data appears in a separate hierachy, you can add additional variables to support exceptions. For example, suppose that all show data is stored as above, but reference data is stored separately under these "roots":

| OS | Root |
| --- | --- |
| OSX | /ref |
| LINUX | /net/reference |
| WINDOWS | c:/global/reference |

In that case, you can set another triplet of environment variables to allow for that exception:

| Variable Name | Variable Value |
| --- | --- |
| RV_OS_PATH_OSX_REF | /ref |
| RV_OS_PATH_LINUX_REF | /net/reference |
| RV_OS_PATH_WINDOWS_REF | c:/global/reference |

Some additional details about RV_OS_PATH variables:

*   If you only use two different OSs, you only need to specify the corresponding pairs of environment variables.
*   As above, additional sets of environment variables are considered to refer to the same "root" if the portion following the OS name matches ("REF" in the above example).
*   You can set any number of sets of environment variables.
*   RV_OS_PATH variables affect all incoming filenames **except** those which contain RV_PATHSWAP variables.
*   RV_OS_PATH variables do not affect outgoing filenames.
*   If more than one match is found, the variable that matches the largest number of characters in the incoming path will be used.

> **Note:** If you need more dynamic control over your path remapping, you can author an RV package to handle transforming your paths with the ' [incoming-source-path](../rv-manuals/rv-reference-manual/rv-reference-manual-chapter-five.md) '.

> **Note:** Due to how environments propagate, it is highly recommended to restart your computer after defining an environment variable on your system.

## Open RV PATHSWAP Variables


If you're comfortable with environment variables, **RV_PATHSWAP** variables can provide a way to share session files across platforms and/or studio locations.

> **Note:** Due to the difficult nature involving changing file paths, we do not recommend RV_PATHSWAP variables unless absolutely necessary.

Suppose that you're working on a project called 'myshow' in two locations, but with shared or mirrored data. Location 'Win' is windows-based and at that location all the media lives in paths that have names that start with `\\projects\myshow`. The other location, 'Lin', is linux-based and at that site all the media for myshow appears in paths that start with '/shows/myshow'.  
  
To localize the media you can define a site-wide environment variable in each location called RV_PATHSWAP_MYSHOW, but with the corresponding value for that site:  
  
At location 'Lin',

```
RV_PATHSWAP_MYSHOW = "/shows/myshow"
```

And at location 'Win',

```
RV_PATHSWAP_MYSHOW = "//projects/myshow"
```

(Note the forward slashes in the above windows path. The PATHSWAP variables operate on internal RV paths, and at this point backward slashes have been converted to forward slashes.)

You can have any number of these variables (they just all need to start with "RV_PATHSWAP_" so you could have one per show, for example. But note that if the above path pattern holds for all your projects, you can localize them all at once with variables like this one:

```
RV_PATHSWAP_ROOT = "/shows"
```

Once RV is running in an environment with these variables, it'll look for them in incoming paths (in session file, on the command line, in rvlink URLs, etc), and add them to paths it writes into session files (and network packets between synchronized RVs).  
  
The up-shot is that a session file written at either site can be read at either site, and a sync session between sites can refer to the same media with the appropriate path for that site.  
  
And of course these same benefits apply to using RV at a single site, but on several different platforms.

## Hand-written Session Files or RVLINK URLs

If you're writing your own session files to feed to RV "by hand", note that the format RV expects is something like this:

```
string movie = "${RV_PATHSWAP_MYSHOW}/myseq/myshot/mymov.mov"
```

Similarly, a URL to play that meda could look like:

```
rvlink://${RV_PATHSWAP_MYSHOW}/myseq/myshot/mymov.mov
```

## Remote Sync

RV automatically swaps the values of appropriate PATHSWAP variables in and out of the names of media transmitted across a sync connection, so once you have them set up, these variables can also make Remote Sync seamless across sites or platforms.
