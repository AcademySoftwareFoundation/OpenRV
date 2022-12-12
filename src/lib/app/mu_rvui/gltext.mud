
documentation: {
gltext """
A very basic GL TrueType (ttf) font rendering library.
"""
}

module: gltext {
documentation: {
ascenderHeight "Returns the ascender height of the current font specified with init()."
bounds """
Given a string returns a float[4] vector indicating the bounding box
of the string. The values can be pulled apart like so:

----
let b = gltext.bounds("foo");
let width = b[2] - b[0];
let height = b[3] - b[1];
----

Newlines in the string are *not* considered. The string is assumed to be a single line of text.
"""

boundsNL """
Given a string returns a float[4] vector indicating the bounding box
of the string. The values can be pulled apart like so:

----
let b = gltext.boundsNL("foo\\nbar");
let width = b[2] - b[0];
let height = b[3] - b[1];
----

New line characters in the string will be considered separate lines of
text to be rendered by writeNL(). Since writeNL() will left justify
rendered text for each line, boundsNL() will take into account the
entire bounding box of all the justified lines of text.
"""

color """
Set the current text color. This is independent of the GL color
states. All text rendered by the gltext module will use this
color. The arguments are red, green, blue, and alpha.
"""

descenderDepth "The descender distance of the current font."

height "The height of the passed in string. This could also be computed using bounds()."

heightNL "The height of the potentially multi-line string. The could also be computed using boundsNL()."

init """
Initialize the gltext module to use the given font. By default this
will be the built in helvetica-like font. A path to a .tff file can be
given to read and use an exact font.
"""

size "Set the current font size for rendering."

width "The width of the passed in string. This could also be computed using bounds()."

widthNL "The width of the potentially multi-line string. The could also be computed using boundsNL()."

writeAt """
Render the given string using the current size, color, and font and
the specified coordinates. There are two versions of the function --
one which takes the coordinate as a vector float[2] and the other which takes
the X and Y coordinats as separate arguments.

writeAt() does not accept newline characters.
"""

writeAtNL """
Render the given string using the current size, color, and font and
the specified coordinates. There are two versions of the function --
one which takes the coordinate as a vector float[2] and the other which takes
the X and Y coordinats as separate arguments.

writeAtNL() will break the string into sub-strings rendered left justified
at newline characters.
"""

} }
