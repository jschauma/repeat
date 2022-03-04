Quick Summary
=============

`repeat` invokes the given command, over and over,
forever or a given number of times.

That is, it lets you repeat a simple or complex
command or pipeline without having to script a `while`
loop using `jot(1)` / `seq(1)`, for example.

For more details and examples, please see the
[manual page](https://github.com/jschauma/repeat/blob/master/doc/repeat.1.txt).

Installation
============

To install the command and manual page somewhere
convenient, run `make install`; the Makefile defaults
to '/usr/local' but you can change the PREFIX:

```
$ make PREFIX=~ install
```

Platforms
=========

repeat(1) was developed on a NetBSD 8.0 system.  It
was tested and confirmed to build on:

- NetBSD 8.0
- macOS 12.2.1
- RHEL 7.9

If you run into build/test issues, please [email
me](mailto:jschauma@netmeister.org).

---

```
NAME
     repeat - execute the given command repeatedly

SYNOPSIS
     repeat [-dhv] [-J replstr] [-n num] [-s sleep] command [argument ...]

DESCRIPTION
     repeat invokes the given command, over and over, forever or a given number
     of times.

OPTIONS
     The following options are supported by repeat:

     -J replstr	 When constructing the command to execute, replace the occurence
		 of replstr with the integer representing the number of
		 invocation performed.

     -d		 Don't wait for the command to complete before invoking it
		 again.	 Passing this flag also sets -s to zero.

     -h		 Display help and exit.

     -n num	 Only repeat the given command num times.

     -s sleep	 Sleep for sleep seconds in between invocations.

		 If not specified, repeat will repeat the given command as
		 quickly as possible.

		 Setting this option also disables -d.

     -v		 Verbose mode: prefix the output of each invocation of the given
		 command with a zero-padded number of the iteration ("<n>:
		 <output>").

EXAMPLES
     The following examples illustrate common usage of this tool.

     To run the date(1) command 30 times in succession in one-minute intervals:

	   repeat -n 30 -s 60 date

     In order to repeat complex commands including e.g., I/O redirection and
     pipelines, you can pass a sh(1) command to repeat.	 For example, to
     continuously count the number of files in the current directory:

	   repeat /bin/sh -c "ls | wc -l"

     repeat will repeatedly invoke the given utility with any subsequent
     arguments.	 If the -J flag is specified, then the given replstr in any of
     the arguments (including the utility itself) will be replaced with the
     number of the invocation.

     Since I/O redirection is processed by the invoking shell, you'd have to
     invoke a new shell to allow for redirection to e.g. a per-invocation output
     file.

     For example, to redirect input and output of 10 repeated invocations:

	   repeat -n 10 -J % /bin/sh -c "<%.in command >%.out"

EXIT STATUS
     The repeat command exits with the exit status of the last command it
     invoked.

SEE ALSO
     jot(1), seq(1), xargs(1), yes(1)

HISTORY
     repeat was originally written by Jan Schaumann <jschauma@netmeister.org> in
     March 2022.

BUGS
     Please file bugs and feature requests by emailing the author.
```
