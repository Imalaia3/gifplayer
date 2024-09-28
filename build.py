from system.pybuild import *
from sys import argv
from shutil import copy as copyFile

maker = PyBuildArgMaker()

def toSubproccessList(command: str):
    return command.split(' ')

def target_all():
    cppFiles = listFilesWithExt('src/','.cpp')
    if not exists("bin/"):
        os.mkdir("bin/")

    if isWindows():
        print("Windows Operating System detected.")
        sdl2IncludePath = input("Enter SDL2 Include Path> ")
        if not exists(sdl2IncludePath):
            print(f"**Error** {sdl2IncludePath} does not exist!")
            exit(1)
        sdl2LibPath = input("Enter SDL2 Lib Path> ")
        if not exists(sdl2LibPath):
            print(f"**Error** {sdl2LibPath} does not exist!")
            exit(1)

        ret = runCommand(toSubproccessList(f"g++ {listToString(cppFiles)} -Wall -Wextra -Wpedantic -I{sdl2IncludePath} -L{sdl2LibPath} -lSDL2 -lSDL2main -o bin/reader.exe"), True)
        if ret.returncode != 0:
            print("g++ failed!")
            exit(1)

        if not exists("bin/SDL2.dll"):
            print("SDL2.dll does not exist in bin/. Trying to copy from SDL2 library directory.")
            dllPath = os.path.join(sdl2LibPath,"SDL2.dll")
            if exists(dllPath):
                copyFile(dllPath, "bin/SDL2.dll")
            else:
                print(f"{dllPath} does not exist. Program may not run.")

    elif isPosix():
        print("Linux / Linux-like Operating System detected.")
        ret = runCommand(toSubproccessList(f"g++ {listToString(cppFiles)} -Wall -Wextra -Wpedantic -lSDL2 -o bin/reader.exe"), True)
        if ret.returncode != 0:
            print("g++ failed!")
            exit(1)

maker.addTarget("all",target_all)


if __name__ == "__main__":
    maker.run(argv)