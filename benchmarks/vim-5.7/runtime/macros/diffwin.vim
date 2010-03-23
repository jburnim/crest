" diffwin.vim: a simple way to use diff to compare two source files
"		and to synchronize the three windows to the current,
"		next, and previous difference blocks.
"
"	Author : Charles E. Campbell, Jr.   (Charles.E.Campbell.1@gsfc.nasa.gov)
"	Date   : 10/20/1999
"
" To enable: put this file into your <.vimrc> or source it from there.
"		You may wish to modify the maps' temporary directory;
"		its easiest to use vms's version: copy it, then :s/tmp:/newpath/g
"
" To use: start Vim as shown below, use \df to generate differences,
"         and then hit the <F8> key:
"
"		vim -o newfile oldfile
"       \df
"		<F8>
"
" The resulting three windows will look like this:
"
"		+----+		Diff Block Format:
"		|diff|		Old code:	*** oldfilename date
"		+----+					new->old changes
"		|new |		New code:	--- newfilename date
"		+----+					old->new changes
"		|old |
"		+----+
"
" You can synchronize the files in the new&old windows to the current
" difference-block being considered: just move the cursor in the diff
" window to the difference of interest and hit the F8 key.  Use \dn
" and \dp to navigate to the next/previous difference block, respectively.
"
" Maps:
"  \df : opens a third window on top with the diff file.
"  \dc : synchronize windows to current  diff
"  \dn : synchronize windows to next     diff
"  \dp : synchronize windows to previous diff
"  <F8>: same as \dc

if has("unix")
  map \df <C-W>k<C-W>j:!diff -c <C-R>% <C-R># > /tmp/vimtmp.dif<C-M><C-W>k<C-W>s:e! /tmp/vimtmp.dif<C-M>:!/bin/rm -f /tmp/vimtmp.dif<C-M><C-M>
elseif has("win32")
  map \df   <C-W>k<C-W>j:!diff -c <C-R>% <C-R># > vimtmp.dif<C-M><C-W>k<C-W>s:e vimtmp.dif<C-M>:!erase vimtmp.dif<C-M><C-M>
elseif has("vms")
  map \df <C-W>k<C-W>j:!diff -c <C-R>% <C-R># > tmp:vimtmp.dif<C-M><C-W>k<C-W>s:e! tmp:vimtmp.dif<C-M>:!/bin/rm -f tmp:vimtmp.dif<C-M><C-M>
endif
map \dc	<C-W>k<C-W>k?^\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*$<CR>jYpdwf,DAGz<C-V><CR><Esc>"aYdd<C-W>j<C-W>j@a<C-W>k<C-W>k?^\*\*\*\*<CR>/^--- <CR>Ypdwf,DAGz<C-V><CR><Esc>"aYdd<C-W>j@a<C-W>kuuz<CR>
map \dn <C-W>k<C-W>k/^\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*$<CR>jYpdwf,DAGz<C-V><CR><Esc>"aYdd<C-W>j<C-W>j@a<C-W>k<C-W>k?^\*\*\*\*<CR>/^--- <CR>Ypdwf,DAGz<C-V><CR><Esc>"aYdd<C-W>j@a<C-W>kuuz<CR>
map \dp	<C-W>k<C-W>k?^\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*$<CR>?<CR>jYpdwf,DAGz<C-V><CR><Esc>"aYdd<C-W>j<C-W>j@a<C-W>k<C-W>k?^\*\*\*\*<CR>/^--- <CR>Ypdwf,DAGz<C-V><CR><Esc>"aYdd<C-W>j@a<C-W>kuuz<CR>
map <F8> \dc
" vim:ts=4
