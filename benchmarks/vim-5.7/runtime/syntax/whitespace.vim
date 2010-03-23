" Simplistic way to make spaces and Tabs visible

" This can be added to an already active syntax.
"syn clear

syn match Space " "
syn match Tab "\t"
if &background == "dark"
  hi Space ctermbg=darkred guibg=#500000
  hi Tab ctermbg=darkgreen guibg=#003000
else
  hi Space ctermbg=lightred guibg=#ffd0d0
  hi Tab ctermbg=lightgreen guibg=#d0ffd0
endif
