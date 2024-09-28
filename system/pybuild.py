# A Python only build system.
import os,subprocess
import hashlib


# returns True if the current platform is windows, False if it is not
def isWindows() -> bool:
    return os.name == 'nt'

# returns True if the current platform is POSIX (Linux / MacOS / BSD etc), False if it is not
def isPosix() -> bool:
    return os.name == 'posix'

# lists all contents of a directory, including other directories
def listDir(path: str) -> list[str]:
    return os.listdir(path)

# lists all files in a directory
def listFiles(path: str) -> list[str]:
    files = []
    for file in listDir(path):
        if os.path.isfile(os.path.join(path,file)):
            files.append(file)
    return files

# performs a replace(a,b) function on each path
def replacePaths(a: str,b: str,paths: list) -> list[str]:
    out = []
    for p in paths:
        out.append(p.replace(a,b))
    return out

# runs a command using subprocess.run() and get the stdout (not stderr)
def runCommand(command: list,shell = False, legacy=False) -> subprocess.CompletedProcess | bytes:
    print(listToString(command))
    res = subprocess.run(command, stdout=subprocess.PIPE,shell=shell)
    if legacy:
        return res.stdout
    return res

# runs a command using os.system() without getting the command output
def runCommand_os(command:str, echoCommand=False)->None:
    if echoCommand:
        print(command)
    os.system(command)

# returns True if path exists, False if it does not
def exists(full_path: str) -> bool:
    return os.path.exists(full_path)

# returns True if path is a file, False if it is not
def isFile(full_path: str) -> bool:
    return os.path.isfile(full_path)

# gets the file extension of a path, including the "." prefix
def getFileExtension(filename: str) -> str:
    return os.path.splitext(filename)[-1]

# gets the file content MD5 hash as a string
def getFileMd5(filename: str) -> str:
    with open(filename,"rb") as f:
        return hashlib.md5(f.read()).hexdigest()

# lists all files in the beginpath and all files in every subfolder
def getFilesRecursive(beginpath: str) -> list[str]:
    return [os.path.join(dp, f) for dp, dn, filenames in os.walk(beginpath) for f in filenames]

# lists all files in a directory with a specific extension (ext must have a "." prefix)
def listFilesWithExt(path: str, ext: str) -> list[str]:
    files = []
    for file in listFiles(path):
        if getFileExtension(file) == ext:
            files.append(os.path.join(path,file))
    return files

def listToString(listIn: list, sep=" ") -> str:
    return sep.join(listIn)

# deletes a file
def delFile(path):
    os.remove(path)

# deletes all files in a directory but not the directory
def clearDir(dirpath) -> None:
    to_remove = [os.path.join(dirpath,f) for f in listFiles(dirpath)]
    for f in to_remove:
        delFile(f)

# performs a function f on every item of the list l. f must return the result and have 1 argument, the current item being passed.
# the function f may return None, and the result will be ignored from the resulting list
def batchAction(l : list, f) -> list:
    out = []
    for item in l:
        r = f(item)
        if r != None:
            out.append(r)
    return out

# saves file hashes into hashpath. Used in hasFileChanged_md5
def saveFileChanges_md5(files: list, hashpath='hashes.pybuild') -> None:
    hashes = []
    for file in files:
        hashes.append(file+":"+getFileMd5(file))
    with open(hashpath,"w") as f:
        for h in hashes:
            f.write(h+"\n")

# checks if file has changed using hashes from hashpath
def hasFileChanged_md5(file: str, hashpath='hashes.pybuild') -> bool:
    if not exists(hashpath):
        print(f"*** Note ***: Hash file {hashpath} does not exist. Assuming this is a first run.")
        return True
    with open(hashpath,"r") as f:
        for line in f.read().split("\n"):
            fname,_hash = line.split(":")
            if fname == file:
                return not _hash == getFileMd5(file)

# gets the modification time (mtime) of a file as a UNIX timestamp        
def getFileModificationTime(file: str) -> float:
    return os.path.getmtime(file)

# identical to saveFileChanges_md5, but using mtime
def saveFileChanges_mtime(files: list, timepath='timestamps.pybuild') -> None:
    hashes = []
    for file in files:
        hashes.append(file+":"+str(getFileModificationTime(file)))
    with open(timepath,"w") as f:
        for h in hashes:
            f.write(h+"\n")

# identical to hasFileChanged_md5, but using mtime
def hasFileChanged_mtime(file: str, timepath='timestamps.pybuild') -> bool:
    if not exists(timepath):
        print(f"*** Note ***: Timestamp file {timepath} does not exist. Assuming this is a first run.")
        return True
    with open(timepath,"r") as f:
        for line in f.read().split("\n"):
            fname,_hash = line.split(":")
            if fname == file:
                return not float(_hash) == getFileModificationTime(file) 

# Used for creating targets that are executed when running pybuild (similar to phony functions in a Makefile)
class PyBuildArgMaker:
    def __init__(self) -> None:
        self.targets = {}
    
    # maps a target string to a function that runs when the user uses the supplied string as a target 
    def addTarget(self,name: str, fun) -> None:
        self.targets[name] = fun
    
    # runs the program. argv should be sys.argv however it can be changed as long as the element 1 (second element) is the target string
    def run(self,argv: list) -> None:
        if len(argv) < 2 and len(self.targets.keys()) > 1:
            print("*** Error ***: No target has been supplied and the target count is higher than one!")
            return None
        elif len(argv) < 2 and len(self.targets.keys()) == 1:
            print(f"*** Note ***: No target supplied. Defaulting to target {list(self.targets.keys())[0]}.")
            self.targets[list(self.targets.keys())[0]]()
            return None

        target = argv[1]
        if target not in self.targets.keys():
            print(f"*** Error ***: No target named '{target}' was found!")
            return None
        self.targets[target]()
