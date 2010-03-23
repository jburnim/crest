" Vim syntax file
" Language:	gnuplot
" Maintainer:	John Hoelzel <johnh51@earthlink.net>
" Last Change:	1999 Jun 14

" Remove any old syntax stuff hanging around
syn clear

" commands
syn keyword gnuplotStatement	cd call clear exit set plot splot help
syn keyword gnuplotStatement	load pause quit fit replot quit if
syn keyword gnuplotStatement	print pwd reread reset save show test !
syn keyword gnuplotConditional	if
" numbers fm c.vim
"	integer number, or floating point number without a dot and with "f".
syn case    ignore
syn match   gnuplotNumber	"\<[0-9]\+\(u\=l\=\|lu\|f\)\>"
"	floating point number, with dot, optional exponent
syn match   gnuplotFloat	"\<[0-9]\+\.[0-9]*\(e[-+]\=[0-9]\+\)\=[fl]\=\>"
"	floating point number, starting with a dot, optional exponent
syn match   gnuplotFloat	"\.[0-9]\+\(e[-+]\=[0-9]\+\)\=[fl]\=\>"
"	floating point number, without dot, with exponent
syn match   gnuplotFloat	"\<[0-9]\+e[-+]\=[0-9]\+[fl]\=\>"
"	hex number
syn match   gnuplotNumber	"\<0x[0-9a-f]\+\(u\=l\=\|lu\)\>"
syn case    match
"	flag an octal number with wrong digits by not hilighting
syn match   gnuplotOctalError	"\<0[0-7]*[89]"

" plot args
syn keyword gnuplotType		using title notit tit with wi steps fsteps fs
syn keyword gnuplotType		linespoints lines li via
" funcs
syn keyword gnuplotType		abs acos arg asin atan besj0 besj1 besy0 besy1
syn keyword gnuplotType		ceil column cos cosh erf erfc exp floor gamma
syn keyword gnuplotType		ibeta inverf igamma imag invnorm int lgamma
syn keyword gnuplotType		log log10 norm rand real sgn sin sinh sqrt tan
syn keyword gnuplotType		tanh valid
" set vars - comment out items you rarely use if they slow you up too much.
syn keyword gnuplotType		xdata timefmt grid noytics ytics notit tit fs
syn keyword gnuplotType		logscale time mxtics style
syn keyword gnuplotType		size origin multiplot xtics xrange yrange size
syn keyword gnuplotType		binary matrix index every thru using smooth
syn keyword gnuplotType		angles degrees arrow noarrow autoscale
syn keyword gnuplotType		noautoscale bar border noborder boxwidth
syn keyword gnuplotType		clabel noclabel clip noclip cntrparam contour
syn keyword gnuplotType		dgrid3d nodgrid3d dummy encoding format
syn keyword gnuplotType		function grid hidden3d isosamples key nokey
syn keyword gnuplotType		label nolabel logscale nologscale missing
syn keyword gnuplotType		mapping margin bmargin lmargin rmargin tmargin
syn keyword gnuplotType		multiplot nomultiplot mxtics nomxtics mytics
syn keyword gnuplotType		nomytics mztics nomztics mx2tics nomx2tics
syn keyword gnuplotType		my2tics nomy2tics offsets origin output
syn keyword gnuplotType		parametric noparametric pointsize polar nopolar
syn keyword gnuplotType		xrange yrange zrange x2range y2range rrange
syn keyword gnuplotType		trange urange vrange samples size terminal
syn keyword gnuplotType		bezier boxerrorbars boxes bargraph
syn keyword gnuplotType		boxxyerrorbars csplines dots fsteps impulses
syn keyword gnuplotType		lines linespoints points sbezier splines steps
syn keyword gnuplotType		vectors xerrorbars xyerrorbars yerrorbars
syn keyword gnuplotType		errorbars surface
syn keyword gnuplotType		tics ticslevel ticscale time timefmt title view
syn keyword gnuplotType		xdata xdtics noxdtics ydtics noydtics zdtics
syn keyword gnuplotType		nozdtics x2dtics nox2dtics y2dtics noy2dtics
syn keyword gnuplotType		xlabel ylabel zlabel x2label y2label xmtics
syn keyword gnuplotType		noxmtics ymtics noymtics zmtics nozmtics x2mtics
syn keyword gnuplotType		nox2mtics y2mtics noy2mtics xtics noxtics ytics
syn keyword gnuplotType		noytics ztics noztics x2tics nox2tics y2tics
syn keyword gnuplotType		noy2tics zero zeroaxis nozeroaxis xzeroaxis
syn keyword gnuplotType		noxzeroaxis yzeroaxis noyzeroaxis x2zeroaxis
syn keyword gnuplotType		nox2zeroaxis y2zeroaxis noy2zeroaxis angles

" strings
syn region gnuplotString	start=+"+ skip=+\\"+ end=+"+
syn region gnuplotString	start=+'+ skip=+\\'+ end=+'+

" comments
syn region gnuplotComment	start="^\s*#\s*" skip="\\$" end="$"

if !exists("did_gnuplot_syntax_inits")
  let did_gnuplot_syntax_inits = 1
  hi link gnuplotString		String
  hi link gnuplotStatement	Statement
  hi link gnuplotConditional	Conditional
  hi link gnuplotNumber		Number
  hi link gnuplotFloat		Float
  hi link gnuplotOctalError	Error
  hi link gnuplotType		Type
  hi link gnuplotComment	Comment
endif

let b:current_syntax = "gnuplot"

" vim: ts=8
