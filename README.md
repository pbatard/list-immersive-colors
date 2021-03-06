list-immersive-colors: Lists Windows' immersive colors
======================================================

[![Build status](https://img.shields.io/appveyor/ci/pbatard/list-immersive-colors.svg?style=flat-square)](https://ci.appveyor.com/project/pbatard/list-immersive-colors)
[![Release](https://img.shields.io/github/release-pre/pbatard/list-immersive-colors.svg?style=flat-square)](https://github.com/pbatard/list-immersive-colors/releases)
[![Github stats](https://img.shields.io/github/downloads/pbatard/list-immersive-colors/total.svg?style=flat-square)](https://github.com/pbatard/list-immersive-colors/releases)
[![Licence](https://img.shields.io/badge/license-GPLv3-blue.svg?style=flat-square)](https://www.gnu.org/licenses/gpl-3.0.en.html)

This application is designed to list all the Immersive Colours that can be used with the
various __undocumented__ `GetImmersiveColorXXX()` APIs that can be found in `uxtheme.dll`.

Using the generated list should make it a lot easier to access and use these values in a
GUI application where you want to have your controls match the system's immersive theme.

![Screenshot](https://raw.githubusercontent.com/pbatard/list-immersive-colors/master/pics/screenshot.png)

The executable should run on any Windows 10 platform, and provided your console supports
24-bit colour output (Installing [Windows Terminal from the Microsoft Store](https://aka.ms/windowsterminal)
is __highly__ recommended), it should also display a sample for each individual colour
as per the screenshot above.

Alternatively, if you don't want to run the executable, you can display the immersive
colours by using the output that was generated by this application on a default Windows
10 Build 18363.752 platform by opening a console and typing:

```
type list-immersive-colors.txt
```
