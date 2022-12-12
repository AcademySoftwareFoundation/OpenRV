
documentation: {
io """
Basic I/O in the style of C++ iostreams.
"""
}

module: io {

documentation: {
path "The io.path module contains function to manipulate OS dependent paths."
}

module: path {

documentation: {
basename """Returns the name of the file without any preceding directories. For example,
+basename("/foo/bar.ext")+ will return +"bar.ext"+
"""
concat_paths """
Returns a OS dependent delimited string of paths created by joining the two arguments in order. For example,
+concat("a", "b")+ will return the +"a:b"+ on mac or linux and +"a;b"+ on windows.
"""
concat_separator "Returns the path list separator as a string"
dirname """Return the enclosing directory path of the file or the empty string "" if there is none.
For example, +dirname("/foo/bar.ext")+ returns +"/foo/"+
"""
exists "Returns true of the path to the file exists"
expand "Expands tilde characters into the full path to the user's home directory"
extension """
Returns the extension for the given filename. For example, +extension("/foo/bar.ext")+ will return +"ext"+
"""
join """
Returns a path created by joining the two arguments in order. For example,
+join("a", "b")+ will return the new path +"a/b"+ 
"""
path_separator "Return the path separator character as a string"
without_extension """Return the path name with its extension stripped. For example, +without_extension("/foo/bar.ext")+ 
returns +"/foo/bar"+
"""
}
}

documentation: {
stream "Base class for IO streams"
istream "Input stream base class for isstream and ifstream"
ostream "Output stream base class for osstream and ofstream"
ifstream "Input file stream -- a stream for reading from a file"
ofstream "Output file stream -- a stream for write to a file"
osstream "Output string stream -- an output stream to a string"
isstream "Input string stream -- an input stream from a string"
process "Provides a process control object which provides input and output streams to the extern process"
print (ostream; ostream, string) "Output a string as UTF8 to an output stream"
print (ostream; ostream, int) "Output an int to an output stream as UTF8"
print (ostream; ostream, float) "Output a float to an output stream as UTF8"
print (ostream; ostream, double) "Output a double to an output stream as UTF8"
print (ostream; ostream, byte) "Output a byte to an output stream as UTF8"
print (ostream; ostream, bool) "Output a bool to an output stream as UTF8"
print (ostream; ostream, char) "Output a char to an output stream as UTF8"
endl (ostream; ostream) "Output a text line terminator to an output stream. Causes a flush."
flush (ostream; ostream) "Flush any output stream buffers."
read_string (string; istream) "Read a single UTF8 encoded string from the input stream."
read_float (float; istream) "Read a UF8 encoded float from the input stream."
read_double (double; istream) "Read a UF8 encoded double from the input stream."
read_bool (bool; istream) "Read a bool from input stream."
read_byte (byte; istream) "Read a byte from input stream."
read_char (char; istream) "Read a UTF8 char from input stream."
read_line (string; istream) "Read a line of text from input stream."
read_all (string; istream) "Read the entire contents of the input stream into a string."
read_all_bytes (byte[]; istream) "Read the entire contents of an input stream into a byte[]."
out "The standard output stream."
in "The standard input stream."
error "The standard error stream."
directory """
Returns a list of (string,int) pairs representing the entries in a the given directory.
Each tuple in the list contains the name of the file and its type. The type is one of:

|===================================================
| UnknownFileType | The file type is some unknown type
| RegularFileType | A regular text or binary file
| DirFileType | A directory
| SymbolicLinkFileType | A symbolic link
| FIFOFileType | A named pipe
| CharDeviceFileType | A character device file
| BlockDeviceFileType | A block device file
| SocketFileType | A named socket
|===================================================
"""

dummy ""
}
}
