# Nuke style file syntax in Open RV

You can use nuke's printf-esque %d syntax in RV and we've extended the existing shake-esque syntax so you can mix and match.

> **Note:** previous versions of RV required using -ns in order to use nuke-like syntax. This is no longer necessary.

Given some files called foo.0001.exr, foo.0002.exr, foo.0003.exr, .... foo.0100.exr:

From the shell you can refer to the entire sequence using any of the following:

```
shell> rv foo.#.exr  
shell> rv foo.@@@@.exr  
shell> rv foo.%04d.exr 
```

If you're current directory contains the sequence this will also work (in a unix shell):

```
shell> rv foo.*.exr 
```

To constrain the sequence to frames 1 through 50 any of the following will work:

```
shell> rv foo.1-50#.exr  
shell> rv foo.%04d.exr 1-50  
shell> rv foo.1-50%04d.exr  
shell> rv foo.#.exr 1-50  
```

Its also possible to use multiple ranges, but only the shake-like method will work for that currently:

```
shell> rv foo.1-10,20-50,60-100#.exr 
```

Any of these should also work inside brackets [ ].
