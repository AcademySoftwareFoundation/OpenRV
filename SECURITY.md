# Security and UTV

The UTV maintainers take security very seriously.
We strive to design secure software, and utilize continuous
integration and code analysis tools to help identify potential
vulnerabilities.

UTV is heavily dependent on third party dependencies like openexr, ffmpeg, openimageio, and others that are installed as part of the UTV installation.  This is a weakness in that those dependencies could be compromised.  It is also a strength in that those dependencies are motivated to secure their own software and rapidly repond to CVE's in a way that a no single project can do on their own.

Users should exercise caution when working with untrusted data (UTV
session files, external UTV packages, external UTV movie and image
plugins, etc.). UTV takes every precaution to read only valid data, but it
would be naive to say our code is immune to every exploit.

## Reporting Vulnerabilities

Quickly resolving security related issues is a priority.
To report a security issue, please use the GitHub Security Advisory ["Report a Vulnerability"](https://github.com/AcademySoftwareFoundation/UTV/security/advisories/new) tab.

Include detailed steps to reproduce the issue, and any other information that
could aid an investigation.

## Outstanding Security Issues

None

## Addressed Security Issues

None
