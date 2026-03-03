# Chapter 18 - RVPUSH

The RVPUSH command-line utility allows you to communicate with a running RV (you can designate a “target” RV with the -tag option). The help output from the command is as follows:

```
usage: rvpush [-tag <tag>] <command> <commandArgs>
       rvpush set <mediaArgs>
       rvpush merge <mediaArgs>
       rvpush mu-eval <mu>
       rvpush mu-eval-return <mu>
       rvpush py-eval <python>
       rvpush py-eval-return <python>
       rvpush py-exec <python>
       rvpush url rvlink://<rv-command-line>

       Examples:

       To set the media contents of the currently running RV:
           rvpush set [ foo.mov -in 101 -out 120 ]

       To add to the media contents of the currently running RV with tag "myrv":
           rvpush -tag myrv merge [ fooLeft.mov fooRight.mov ]

       To execute arbitrary Mu code in the currently running RV:
           rvpush mu-eval 'play()'

       To execute arbitrary Mu code in the currently running RV, and print the result:
           rvpush mu-eval-return 'frame()'

       To evaluate an arbitrary Python expression in the currently running RV:
           rvpush py-eval 'rv.commands.play()'

       To evaluate an arbitrary Python expression in the currently running RV, and print the result:
           rvpush py-eval-return 'rv.commands.frame()'

       To execute arbitrary Python statements in the currently running RV:
           rvpush py-exec 'from rv import commands; commands.play()'

       To process an rvlink url in the currently running RV, loading a movie into the current session:
           rvpush url 'rvlink:// -reuse 1 foo.mov'

       To process an rvlink url in the currently running RV, loading a movie into a new session:
           rvpush url 'rvlink:// -reuse 0 foo.mov'

       Set environment variable RVPUSH_RV_EXECUTABLE_PATH if you want rvpush to
       start something other than the default RV when it cannot find a running
       RV.  Set to 'none' if you want no RV to be started.

       Exit status:
           4: Connection to running RV failed
          11: Could not connect to running RV, and could not start new RV
          15: Cound not connect to running RV, started new one.
```

Any number of media sources and associated per-source options can be specified for the set and merge commands. For the mu-eval command, it's probably best to put all your Mu code in a single quoted string. In the next release we'll have a python-eval command as well.

If RVPUSH cannot find a running RV to talk to, it'll start one with the appropriate command-line options. Any later rvpush commands will use this RV until it exits. Note that the RV that RVPUSH starts will by default be the one in the same bin directory. If you'd rather start a different RV, or start a wrapper, etc, you can set the environment variable RVPUSH_RV_EXECUTABLE_PATH to point to the one you'd prefer. If you want RVPUSH to never start RV, you can set this environment variable to “none”.
