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
MK
echo "(make started from sh)"
make -f ffbuild/rv_diag.mk 2>&1
echo "=== RV FFMPEG DIAG: sh part end"
exit 0
