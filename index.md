Introduction
============

This tool converts a tab separated table to a visually improved plain text format, which conforms to the recommendations at [markdownguide.org][1].

Description
===========

The functionality is best explained by an example. The three lines below are separated by tabs. They can be created directly in a text editor or by copy & paste from a spreadsheet.

    :ID:	:Long header:	Value	Left
    1	abc	5	Hello, world
    125	foo bar	898.786384	Lorem ipsum

In a plain text editor, the columns are not aligned and it is hard to see, which value belongs to which column.
This tool converts such a table to the form below:

    | ID  | Long header |      Value | Left         |
    |:---:|:-----------:|-----------:|--------------|
    |  1  |     abc     |          5 | Hello, world |
    | 125 |   foo bar   | 898.786384 | Lorem ipsum  |

You can now clearly see the table structure directly, especially when using a monospaced font. By default all columns are aligned to the left. E.g. the last column labeled `Left` in the example above.

However, tsv checks for numbers. If _all_ cells of a column are numbers (or empty), the default alignment becomes to the right. E.g. the `Value` column.

You can override the default alignment by adding colons around column names as follows:

- A colon on the left side of the header means left alignment
- A colon on the right side of the header means right alignment
- A colon on both sides of the header means centered

The `ID` column has only numbers, so it would be aligned to the left. But it is centered, because the input explicitly specifies the alignment. See `:ID:`

Some markdown tools can turn this output to various formats. From HTML to PDF there are many possibilities. E.g. [markdown-it][2] has an online version for immediate viewing as a web page. 

Installing
==========

1. At the top level, build the release configuration:

    ./build_release.sh

2. Install at the default location:

    cd build_release
    
    sudo make install

Usage
=====
1. Normally, just type tsv and the name of the input file.

    tsv INPUT_FILE

The output goes to the standard output. If you wish to save the output to a file, use redirection. E.g. `tsv in.txt > out.md`

2. Just print the version

    tsv --version

3. Just print a help message

    tsv -h

4. Software developers can print the AST or even a parser trace.

    tsv INPUT_FILE [--ast] [--trace]

Development environment
=======================

- Ubuntu 20.04.2 LTS
- cmake version 3.16.3
- gcc version 9.3
- Visual Studio Code 1.53.2 (Optional)
- Valgrind (optional)

Also successfully tested on Mac OS Version 11.1 Terminal

Developing
==========

Using the terminal
------------------

Use the bash scripts at the top level. E.g typing `./build_debug.sh` creates the directory `build_debug` if it doesn't exist, configures the build using cmake and starts the build using make.

The script `./run_debug.sh` builds the executable and runs an example in the `test` directory

The script `./build_release.sh` builds the executable for release. Note that the script creates a C Header File, which includes the PEG and is included into the source code. Since the script recreates this file each time it is called, there will be some compiling effort even if there were no changes to the PEG.

Using Visual Studio Code
------------------------

The software comes with predefined tasks. Chose one to build a debugging version, a release version, run an example (debug version) or start the unit testing.

There is also a launch configuration for debugging. In case there were any modifications, the tool automatically starts the build process first.

Implementation details
======================

This tool uses a Parsing Expression Grammar (PEG) for tab separated tables and generates an AST. Then it can create a table in various formats. The current implementation supports only (the default) markdown table. Other formats such as JSON can be added as an option.

The SW contains unit testing, which is currently only at the very beginning, such that all the cmake and visual studio code setup is working with a hello world type of initial unit test.

There are bash scripts at the top level, which can be used to build, run, etc. as their names suggest.

Since the whole project is rather simple, it can serve as an example C++ project template for cmake and visual studio code for building and running an executable, which includes libraries. Unit testing and debugging is configured.

The [PEG Library][3] and the [unit testing framework][4] are copied from github. Both of them have a MIT Licence.

The main C++ file uses a mechanism to include the PEG into the source code for a release build. A debug build reads the PEG from a file in runtime. The latter accelerates the development when the PEG is under construction. This way we don't need to recompile the PEG Library (peglib.h), which takes many seconds again and again.

Again, this can serve as an example for including arbitrary files into the source code.

TODO
====

- Add an option to create a JSON Version
- Create a working development environment for Windows 10
- Allow input from a pipe such as `cat input.txt | tsv`
- Create a docker image, preferably based on alpine linux

[1]: https://www.markdownguide.org/extended-syntax/
[2]: https://markdown-it.github.io/
[3]: https://github.com/yhirose/cpp-peglib
[4]: https://github.com/drleq/CppUnitTestFramework
