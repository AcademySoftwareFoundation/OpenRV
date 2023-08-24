import argparse, shutil, pathlib, re, sys


def patch_execute(filename):
    did_find_match = False
    backup_cmakelist_path = f"{filename}.bk"
    shutil.copyfile(filename, backup_cmakelist_path)
    with open(backup_cmakelist_path, "r") as backup_cmakelist:
        with open(filename, "w") as cmakelist:
            for line in backup_cmakelist:
                if re.match("^\s*set.*_PublicLibs", line):
                    new_line = line.replace(
                        "set(_PublicLibs ${Python_LIBRARIES})",
                        "set(_PublicLibs ${RV_Python_LIBRARIES})",
                    )
                    cmakelist.write(new_line)
                    did_find_match = True
                else:
                    cmakelist.write(line)
    if not did_find_match:
        raise Exception(f"File {filename} doesn't contain _PublicLibs. Cannot patch")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    parser.add_argument("path", type=pathlib.Path)

    args = parser.parse_args()

    CMAKE_FILE = args.path
    if not CMAKE_FILE.exists():
        raise Exception(f"File not found! ({CMAKE_FILE})")
    if CMAKE_FILE.suffix != ".txt" or CMAKE_FILE.suffix != ".txt":
        raise Exception(f"File is not a CMake file: ({CMAKE_FILE.name})")

    patch_execute(str(CMAKE_FILE))
