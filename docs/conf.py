# Configuration file for the Sphinx documentation builder.

# -- Project information
import re

project = 'Open RV'
author = 'Open RV contributors'

version = '0.0'
release = '0.0.0'

# https://regex101.com/r/Sr8aQX/2
# This pattern assumes that the SET command follows the typical syntax of a CMake SET command, 
# with a variable name, a string value enclosed in quotes, and the CACHE STRING clause. 
# It captures the numeric part of the value assigned to these variables.
pattern = r'SET\(\s*(?:RV_MAJOR_VERSION|RV_MINOR_VERSION|RV_REVISION_NUMBER|RV_VERSION_YEAR)\s*"(\d+)"\s*CACHE\s*STRING\s*"[^"]+"\s*\)'
# Open the file and read its contents
with open('../CMakeLists.txt', 'r') as file:
    text = file.read()

# Find all matches of the pattern in the text
matches = re.findall(pattern, text)

# Extract the major, minor, and revision numbers from the matches
# https://www.sphinx-doc.org/en/master/usage/configuration.html
#
# version
# The major project version, used as the replacement for |version|. 
# For example, for the Python documentation, this may be something like 2.6.
#
# release
# The full project version, used as the replacement for |release| and e.g. in the HTML templates. 
# For example, for the Python documentation, this may be something like 2.6.0rc1.
#
# If you don’t need the separation provided between version and release, just set them both to the same value.

version = "%s.%s.%s" % (matches[0], matches[1], matches[2])
release = version
copyright = matches[3]

# -- General configuration

extensions = [
    'myst_parser',
    'sphinx_carousel.carousel',
    'sphinx_copybutton',
    'sphinx.ext.duration',
    'sphinx.ext.doctest',
    'sphinx.ext.autodoc',
    'sphinx.ext.autosummary',
    'sphinx.ext.intersphinx',
    'sphinx_tabs.tabs',
    'sphinx_rtd_theme'
]

intersphinx_mapping = {
    'python': ('https://docs.python.org/3/', None),
    'sphinx': ('https://www.sphinx-doc.org/en/master/', None),
}
intersphinx_disabled_domains = ['std']

templates_path = ['_templates']

# -- Options for HTML output

html_theme = 'sphinx_rtd_theme'

# -- Options for EPUB output
epub_show_urls = 'footnote'
