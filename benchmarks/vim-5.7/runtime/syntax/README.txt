This directory contains vim scripts for specific languages.

syntax.vim	Used for the ":syntax on" command.  Uses synload.vim.

manual.vim	Used for the ":syntax manual" command.  Uses synload.vim.

synload.vim	Contains autocommands to load a language file when a certain
		file name (extension) is used.  And sets up the Syntax menu
		for the GUI.

nosyntax.vim	Used for the ":syntax off" command.  Undo the loading of
		synload.vim.


A few special files:

2html.vim	Converts any highlighted file to HTML (GUI only).
colortest.vim	Check for color names and actual color on screen.
hitest.vim	View the current highlight settings.
whitespace.vim  View Tabs and Spaces.


If you want to add your own syntax file for your personal use, read the docs
at ":help mysyntaxfile".

If you make a new syntax file which would be useful for others, please send it
to Bram@vim.org.  Include instructions for detecting the file type for this
language, by file name extension or by checking a few lines in the file.
And please stick to the rules below.


Rules for making a syntax file:
- Use the same layout as the other syntax files.
- The name of the file must be the same as the head of the group names in the
  file.  This avoids using the same group name as another syntax file.  Use
  the same name for the string that b:current_syntax is set to.  Always use
  lower case.
- Start with a "syntax clear".
- Do not include anything that is a user preference.
- Do not include mappings or abbreviations.  Only include setting 'iskeyword'
  if it is really necessary for recognizing keywords.
- Avoid using specific colors.  Use the standard highlight groups whenever
  possible.  Don't forget that some people use a different background color,
  or have only eight colors available.

If you have remarks about an existing file, send them to the maintainer of
that file.  Only when you get no response send a message to Bram@vim.org.

If you are the maintainer of a syntax file and make improvements, send the new
version to Bram@vim.org.


For further info see ":help syntax" in Vim.
