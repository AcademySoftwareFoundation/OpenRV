# Crash Reporting in Open RV

## Overview

Open RV can automatically capture a **crash report** (a minidump) when the
application crashes. A report records the state of the program at the moment of
the crash — the call stack, loaded modules, GPU information, and which Mu/Python
script was running — so the crash can be diagnosed and fixed without needing a
reproduction.

Crash reporting is **enabled by default**. Reports are written to a local folder
on your machine; nothing is sent anywhere automatically. You decide whether to
share a report (for example, by attaching it to a bug report or support request).

## Where crash reports are stored

When RV crashes, a `.dmp` file is written to a per-user location:

| Platform | Location |
| --- | --- |
| macOS | `~/Library/Logs/ASWF/Crashes/` |
| Linux | `~/.local/share/ASWF/OpenRV/Crashes/` |
| Windows | `%APPDATA%\ASWF\OpenRV\Crashes\` |

If log attachment is enabled (the default), the current session log is included
with the report to help correlate the crash with recent activity.

## Enabling and disabling crash reporting

Crash reporting is on by default. To **disable** it, set this environment
variable to `0` before launching RV:

```bash
RV_CRASH_DUMPS_ENABLED=0
```

Leaving it unset — or setting it to any value other than `0` — keeps crash
reporting enabled.

### Related environment variables

| Variable | Default | Description |
| --- | --- | --- |
| `RV_CRASH_DUMPS_ENABLED` | `1` | Set to `0` to disable crash reporting entirely. |
| `RV_CRASH_DUMPS_DIR` | per-platform default (above) | Override the directory where crash reports are written. |
| `RV_CRASH_DUMPS_MAX_COUNT` | `10` | Maximum number of crash reports to keep; older ones are pruned automatically. |
| `RV_CRASH_DUMPS_ATTACH_LOGS` | `1` | Set to `0` to omit the session log from the report. |

## Sharing a crash report

If you are asked to provide a crash report, locate the most recent `.dmp` file in
the directory above and attach it to your bug report or support request. A report
contains diagnostic information about the crash and, if log attachment is enabled,
your recent session log.
