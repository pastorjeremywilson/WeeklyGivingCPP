[![Status](https://img.shields.io/badge/status-active-brightgreen.svg?style=plastic)](https://github.com/pastorjeremywilson/WeeklyGiving/pulse/monthly)
[![License](https://img.shields.io/badge/license-GPL-blue.svg?style=plastic)](https://www.gnu.org/licenses/gpl-3.0.en.html)

<img src='https://github.com/pastorjeremywilson/public/blob/main/wgBanner.svg?raw=true' width='600px' />

# What Am I?

Weekly Giving is a little program to enter, store, print, retrieve, and graph weekly giving at your church.

# Why Weekly Giving?

Are you or your trustees counting the collection plate on a calculator or adding machine, filling out tally sheets 
by hand? Weekly Giving is a flexable program to simplify the process of counting the day's giving, printing an organized
and detailed report to give to your treasurer.

# Installation

Currently, Weekly Giving is available for Windows and Debian-based Linux operating
systems.
- <strong>Windows: </strong>Download the current Weekly Giving installer (Setup.Weekly.Giving.v.1.5.7.exe) and run it 
on your computer.</li>
- <strong>Linux: </strong>Download the current Weekly Giving package file (weekly-giving.1.5.5.deb). Double-click to 
install through your desktop's package manager or, from the command line, navigate to the file's directory and run the 
command, "sudo dpkg -i weekly-giving.1.5.5.deb".</li>

# Using Weekly Giving

*This information will get you started, but for more in-depth information*
*on using Weekly Giving, please click “Help” and “Help Topics” in the menu.*

### Screen Layout

When you first run Weekly Giving, you’ll be greeted with a screen consisting of five areas where you
can record the day's giving:

- Bills
- Coins
- Checks
- Special Designations
- Notes

In the Bills and Coins sections, you'll input the quantity of each denomination (i.e. if you have five ten-dollar
bills, you would enter '5' and if you have eight quarters, you would enter '8'). In the Checks section, you can
enter the value of each check. And in the Notes section, you can enter any notes pertaining to the giving. All of
these, including the notes, will be on the printed report.

### Settings

There are various changes that can be made to the Weekly Giving program. By choosing the "Settings" menu 
you will find that you can change such things as:

- Your Church's Name
- The Special Offering Designations
- Whether to include the Special Designations in the Total Deposit 
- The Maximum Number of Checks
- Where the Database is Stored

Changing the church name will change the name that appears at the upper-left of the screen as well as the name
printed on the report.

Special Offering Designations are those offerings that have been designated by the giver to go to a particular
ministry or fund. If you have special designations that are commonly given, you can change the default labels
to reflect those designations.

Most commonly, special designations are included in the total deposit your treasurer records for the week. However,
if your processes separate those special offerings out from the total deposit, you can choose to keep them out of the
total printed on the report.

The default number of check boxes available on the program's main screen is 30. If you commonly have more or less checks
than this, you can change the number of check boxes shown.

If you would like the database that stores all of your givings week-to-week to be stored in a different location, say,
a network drive, you can change that as well.

# Known Issues

# Technologies and Credits

<br>
<div>
    <img src='https://github.com/pastorjeremywilson/public/blob/main/python-logo-only.svg?raw=true' width=40px align='right' />
    Weekly Giving is written primarily in <a href="https://www.python.org" target="_blank">Python</a>, compiled through <a href="https://www.pyinstaller.org" target="_blank">PyInstaller</a>,
    and packaged into an installation executable with <a href="https://jrsoftware.org/isinfo.php" target="_blank">Inno Setup Compiler</a>.
</div>
<br>

<div>
    <img src='https://github.com/pastorjeremywilson/public/blob/main/sqlite370.jpg?raw=true' height=40px align='right' />
    Weekly Giving’s main database is <a href="https://www.sqlite.org" target="_blank">SQLite</a> and the remaining data files are
    stored in the <a href="https://www.json.org/json-en.html" target="_blank">JSON</a> format.
</div>
<br>

<div>
    <img src='https://github.com/pastorjeremywilson/public/blob/main/Qt-logo-neon.png?raw=true' height=40px align='right' />
    Weekly Giving uses <a href="https://www.qt.io/product/framework" target="_blank">Qt</a> (PyQt6) for the user interface.
</div>
<br>

<div>
    <img src='https://github.com/pastorjeremywilson/public/blob/main/ghostscript-plus-text.png?raw=true' height=40px align='right' />
    Printing made possible with the help of Artifex Software's <a href="https://www.ghostscript.com" target="_blank">Ghostscript</a>.
</div>
<br>
All trademarks (c) their respective owners.

# Licensing

<img src='https://github.com/pastorjeremywilson/public/blob/main/gnu-4.svg?raw=true' height=120px align='left' />
ProjectOn is licensed under the GNU General Public License (GNU GPL)
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version. See
<a href="http://www.gnu.org/licenses/" target="_blank">http://www.gnu.org/licenses/</a>.