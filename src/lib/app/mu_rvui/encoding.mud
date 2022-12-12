
documentation: {
encoding """
This module provides functions to convert to/from strings and byte arrays. 
"""
}

module: encoding {
documentation: {
from_base64 "Converts a byte array of ASCII base 64 characters into a byte array of values in the range [0,255]"
string_to_utf8 "Converts the given string into a UTF8 encoded byte array"
to_base64 "Converts a byte array of binary data into a byte array of ASCII base 64 representation"
utf8_to_string "Converts a UTF8 encoded byte array into a string"
}
}
