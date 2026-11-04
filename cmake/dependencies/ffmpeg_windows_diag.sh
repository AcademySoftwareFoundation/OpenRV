#!/bin/sh
# TEMPORARY: diagnostics for the Windows FFmpeg awk "gsub(/\/" failure. Remove before merge.
# Runs in FFmpeg's source dir after configure, started the same way as configure (sh from MSBuild).
echo "=== RV FFMPEG DIAG: begin"
echo "--- PATH"; echo "$PATH" | tr ':' '\n'
echo "--- type -a"; for t in sh bash make awk gawk; do type -a "$t" 2>&1; done
echo "--- versions"
sh --version 2>&1 | head -1
make --version 2>&1 | head -1
awk --version 2>&1 | head -1
uname -a
echo "--- shell running this script: $0 BASH_VERSION=$BASH_VERSION MSYS=$MSYS MSYSTEM=$MSYSTEM"
echo "--- DEPCMD lines in ffbuild/config.mak (cat -A shows exact backslashes)"
grep -n 'including' ffbuild/config.mak | cat -A | cut -c1-400
echo "--- make + sh + awk backslash round trip"
# Kept in ffbuild/ so the CMake step can also run it with make started directly from MSBuild, like FFmpeg's build.
cat > ffbuild/rv_diag.mk <<'MK'
all:
	@echo 'make=$(MAKE) SHELL=$(SHELL) MAKE_VERSION=$(MAKE_VERSION)'
	-@echo 'single-quoted: gsub(/\\/, "/")'
	-@printf '%s\n' 'C:\a\b' | awk '{ gsub(/\\/, "/"); print "awk result: " $$0 }'
	-@/usr/bin/printf '%s\n' 'no shell, expect a\\b: a\\b'
MK
echo "(make started from sh)"
make -f ffbuild/rv_diag.mk 2>&1
echo "--- installed packages"
pacman -Q make bash dash msys2-runtime gawk coreutils 2>&1
echo "--- who owns and links make"
pacman -Qo /usr/bin/make /usr/bin/sh /usr/bin/awk 2>&1
type -P make
cygcheck "$(type -P make)" 2>&1 | head -20
grep -l 'usr/bin/make' /var/lib/pacman/local/*/files 2>&1 | head
ls -l /usr/bin/make* 2>&1
ls -l /usr/bin/sh /usr/bin/bash 2>&1
cmp /usr/bin/sh /usr/bin/bash >/dev/null 2>&1 && echo "sh is a copy of bash" || echo "sh differs from bash"
env | grep -iE '^(MAKE|MFLAGS|GNUMAKEFLAGS|SHELL|BASH_ENV|ENV|SHELLOPTS)' 2>&1
printf '%s\n' '--- sh without make (expect a\\b from both; echo can differ if sh has xpg_echo)'
# Built with printf so no shell quoting touches the two backslashes: the command is  echo 'a\\b'
sh_cmd=$(printf "echo '%s'" 'a\\b')
printf 'command passed to sh -c: %s\n' "$sh_cmd"
sh -c "$sh_cmd"
sh_cmd_ext=$(printf "/usr/bin/printf '%%s\\\\n' '%s'" 'a\\b')
printf 'command passed to sh -c: %s\n' "$sh_cmd_ext"
sh -c "$sh_cmd_ext"
echo "--- make --trace --debug=j"
make --trace --debug=j -f ffbuild/rv_diag.mk 2>&1
echo "--- make with SHELL=/usr/bin/bash"
make SHELL=/usr/bin/bash -f ffbuild/rv_diag.mk 2>&1
echo "=== RV FFMPEG DIAG: sh part end"
exit 0
