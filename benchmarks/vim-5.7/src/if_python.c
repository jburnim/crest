/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * Python extensions by Paul Moore.
 * Changes for Unix by David Leonard.
 *
 * This consists of four parts:
 * 1. Python interpreter main program
 * 2. Python output stream: writes output via [e]msg().
 * 3. Implementation of the Vim module for Python
 * 4. Utility functions for handling the interface between Vim and Python.
 */

#include <stdio.h>
#include <stdarg.h>
#include <limits.h>

#include <Python.h>
#ifdef macintosh
# include "macglue.h"
# include <CodeFragments.h>
#endif
#undef main /* Defined in python.h - aargh */
#undef HAVE_FCNTL_H /* Clash with os_win32.h */

/* Parser flags */
#define single_input	256
#define file_input	257
#define eval_input	258

#include "vim.h"

/******************************************************
 * Internal function prototypes.
 */

static int DoPythonCommand(EXARG *, const char *);
static int RangeStart;
static int RangeEnd;

static void PythonIO_Flush(void);
static int PythonIO_Init(void);
static int PythonMod_Init(void);

/* Utility functions for the vim/python interface
 * ----------------------------------------------
 */
static PyObject *GetBufferLine(BUF *, int);
static PyObject *GetBufferLineList(BUF *, int, int);

static int SetBufferLine(BUF *, int, PyObject *, int *);
static int SetBufferLineList(BUF *, int, int, PyObject *, int *);
static int InsertBufferLines(BUF *, int, PyObject *, int *);

static PyObject *LineToString(const char *);
static char *StringToLine(PyObject *);

static int VimErrorCheck(void);

#define PyErr_SetVim(str) PyErr_SetString(VimError, str)

/******************************************************
 * 1. Python interpreter main program.
 */

static int initialised = 0;

#if PYTHON_API_VERSION < 1007 /* Python 1.4 */
typedef PyObject PyThreadState;
#endif /* Python 1.4 */

static PyThreadState* saved_python_thread = NULL;

/* suspend a thread of the python interpreter
   - other threads are allowed to run */

static void Python_SaveThread() {
	saved_python_thread = PyEval_SaveThread();
}

/* restore a thread of the python interpreter
   - waits for other threads to block */

static void Python_RestoreThread() {
	PyEval_RestoreThread( saved_python_thread );
	saved_python_thread = NULL;
}

/* obtain a lock on the Vim data structures */

static void Python_Lock_Vim() {
}

/* release a lock on the Vim data structures */

static void Python_Release_Vim() {
}

    static int
Python_Init(void)
{
    if (!initialised)
    {
#ifndef macintosh
	Py_Initialize();
#else
	PyMac_Initialize();
#endif

	if (PythonIO_Init())
	    goto fail;

	if (PythonMod_Init())
	    goto fail;

	/* the first python thread is vim's */
	Python_SaveThread();

	initialised = 1;
    }

    return 0;

fail:
    /* We call PythonIO_Flush() here to print any Python errors.
     * This is OK, as it is possible to call this function even
     * if PythonIO_Init() has not completed successfully (it will
     * not do anything in this case).
     */
    PythonIO_Flush();
    return -1;
}

/* External interface
 */

    static int
DoPythonCommand(EXARG *eap, const char *cmd)
{
#ifdef macintosh
    GrafPtr oldPort;
    GetPort (&oldPort);
    /* Check if the Python library is available */
    if ( (Ptr) PyMac_Initialize == (Ptr) kUnresolvedCFragSymbolAddress)
	return FAIL;
#endif
    if (Python_Init())
	return FAIL;

    RangeStart = eap->line1;
    RangeEnd = eap->line2;
    Python_Release_Vim();	    /* leave vim */
    Python_RestoreThread();	    /* enter python */
    PyRun_SimpleString((char *)(cmd));
    Python_SaveThread();	    /* leave python */
    Python_Lock_Vim();		    /* enter vim */
    PythonIO_Flush();
#ifdef macintosh
    SetPort (oldPort);
#endif
    return OK;
}

    int
do_python(EXARG *eap)
{
    return DoPythonCommand(eap, (char *)eap->arg);
}

#define BUFFER_SIZE 1024

    int
do_pyfile(EXARG *eap)
{
    static char buffer[BUFFER_SIZE];
    const char *file = (char *)eap->arg;
    char *p;

    /* Have to do it like this. PyRun_SimpleFile requires you to pass a
     * stdio file pointer, but Vim and the Python DLL are compiled with
     * different options under Windows, meaning that stdio pointers aren't
     * compatible between the two. Yuk.
     *
     * Put the string "execfile('file')" into buffer. But, we need to
     * escape any backslashes or single quotes in the file name, so that
     * Python won't mangle the file name.
     */
    strcpy(buffer, "execfile('");
    p = buffer + 10; /* size of "execfile('" */

    while (*file && p < buffer + (BUFFER_SIZE - 3))
    {
	if (*file == '\\' || *file == '\'')
	    *p++ = '\\';
	*p++ = *file++;
    }

    /* If we didn't finish the file name, we hit a buffer overflow */
    if (*file != '\0')
	return FAIL;

    /* Put in the terminating "')" and a null */
    *p++ = '\'';
    *p++ = ')';
    *p++ = '\0';

    /* Execute the file */
    return DoPythonCommand(eap, buffer);
}

/******************************************************
 * 2. Python output stream: writes output via [e]msg().
 */

/* Implementation functions
 */

static PyObject *OutputGetattr(PyObject *, char *);
static int OutputSetattr(PyObject *, char *, PyObject *);

static PyObject *OutputWrite(PyObject *, PyObject *);
static PyObject *OutputWritelines(PyObject *, PyObject *);

typedef void (*writefn)(char_u *);
static void writer(writefn fn, char_u *str, int n);

/* Output object definition
 */

typedef struct
{
    PyObject_HEAD
    long softspace;
    long error;
} OutputObject;

static struct PyMethodDef OutputMethods[] = {
    /* name,	    function,		calling,    documentation */
    {"write",	    OutputWrite,	1,	    "" },
    {"writelines",  OutputWritelines,	1,	    "" },
    { NULL,	    NULL,		0,	    NULL }
};

static PyTypeObject OutputType = {
	PyObject_HEAD_INIT(0)
	0,
	"message",
	sizeof(OutputObject),
	0,

	(destructor) 0,
	(printfunc) 0,
	(getattrfunc) OutputGetattr,
	(setattrfunc) OutputSetattr,
	(cmpfunc) 0,
	(reprfunc) 0,

	0, /* as number */
	0, /* as sequence */
	0, /* as mapping */

	(hashfunc) 0,
	(ternaryfunc) 0,
	(reprfunc) 0
};

/*************/

    static PyObject *
OutputGetattr(PyObject *self, char *name)
{
    if (strcmp(name, "softspace") == 0)
	return PyInt_FromLong(((OutputObject *)(self))->softspace);

    return Py_FindMethod(OutputMethods, self, name);
}

    static int
OutputSetattr(PyObject *self, char *name, PyObject *val)
{
    if (val == NULL) {
	PyErr_SetString(PyExc_AttributeError, "can't delete OutputObject attributes");
	return -1;
    }

    if (strcmp(name, "softspace") == 0)
    {
	if (!PyInt_Check(val)) {
	    PyErr_SetString(PyExc_TypeError, "softspace must be an integer");
	    return -1;
	}

	((OutputObject *)(self))->softspace = PyInt_AsLong(val);
	return 0;
    }

    PyErr_SetString(PyExc_AttributeError, "invalid attribute");
    return -1;
}

/*************/

    static PyObject *
OutputWrite(PyObject *self, PyObject *args)
{
    int len;
    char *str;
    int error = ((OutputObject *)(self))->error;

    if (!PyArg_ParseTuple(args, "s#", &str, &len))
	return NULL;

    Py_BEGIN_ALLOW_THREADS
    Python_Lock_Vim();
    writer((writefn)(error ? emsg : msg), (char_u *)str, len);
    Python_Release_Vim();
    Py_END_ALLOW_THREADS

    Py_INCREF(Py_None);
    return Py_None;
}

    static PyObject *
OutputWritelines(PyObject *self, PyObject *args)
{
    int n;
    int i;
    PyObject *list;
    int error = ((OutputObject *)(self))->error;

    if (!PyArg_ParseTuple(args, "O", &list))
	return NULL;
    Py_INCREF(list);

    if (!PyList_Check(list)) {
	PyErr_SetString(PyExc_TypeError, "writelines() requires list of strings");
	Py_DECREF(list);
	return NULL;
    }

    n = PyList_Size(list);

    for (i = 0; i < n; ++i)
    {
	PyObject *line = PyList_GetItem(list, i);
	char *str;
	int len;

	if (!PyArg_Parse(line, "s#", &str, &len)) {
	    PyErr_SetString(PyExc_TypeError, "writelines() requires list of strings");
	    Py_DECREF(list);
	    return NULL;
	}

	Py_BEGIN_ALLOW_THREADS
	Python_Lock_Vim();
	writer((writefn)(error ? emsg : msg), (char_u *)str, len);
	Python_Release_Vim();
	Py_END_ALLOW_THREADS
    }

    Py_DECREF(list);
    Py_INCREF(Py_None);
    return Py_None;
}

/* Output buffer management
 */

static char_u *buffer = NULL;
static int buffer_len = 0;
static int buffer_size = 0;

static writefn old_fn = NULL;

    static void
buffer_ensure(int n)
{
    int new_size;
    char_u *new_buffer;

    if (n < buffer_size)
	return;

    new_size = buffer_size;
    while (new_size < n)
	new_size += 80;

    if (new_size != buffer_size)
    {
	new_buffer = alloc((unsigned)new_size);

	if (new_buffer == NULL)
	{
	    EMSG("Out of memory!");
	    return;
	}

	if (buffer)
	{
	    memcpy(new_buffer, buffer, buffer_len);
	    vim_free(buffer);
	}

	buffer = new_buffer;
	buffer_size = new_size;
    }
}

    static void
PythonIO_Flush(void)
{
    if (old_fn && buffer_len)
    {
	buffer[buffer_len] = 0;
	old_fn(buffer);
    }

    buffer_len = 0;
}

    static void
writer(writefn fn, char_u *str, int n)
{
    char_u *ptr;

    if (fn != old_fn && old_fn != NULL)
	PythonIO_Flush();

    old_fn = fn;

    while (n > 0 && (ptr = memchr(str, '\n', n)) != NULL)
    {
	int len = ptr - str;

	buffer_ensure(buffer_len + len + 1);

	memcpy(buffer + buffer_len, str, len);
	buffer_len += len;
	buffer[buffer_len] = 0;
	fn(buffer);
	str = ptr + 1;
	n -= len + 1;
	buffer_len = 0;
    }

    /* Put the remaining text into the buffer for later printing */
    buffer_ensure(buffer_len + n + 1);
    memcpy(buffer + buffer_len, str, n);
    buffer_len += n;
}

/***************/

static OutputObject Output =
{
    PyObject_HEAD_INIT(&OutputType)
    0,
    0
};

static OutputObject Error =
{
    PyObject_HEAD_INIT(&OutputType)
    0,
    1
};

    static int
PythonIO_Init(void)
{
    /* Fixups... */
    OutputType.ob_type = &PyType_Type;

    PySys_SetObject("stdout", (PyObject *)(&Output));
    PySys_SetObject("stderr", (PyObject *)(&Error));

    if (PyErr_Occurred())
    {
	EMSG("Python: Error initialising I/O objects");
	return -1;
    }

    return 0;
}

/******************************************************
 * 3. Implementation of the Vim module for Python
 */

/* Vim module - Implementation functions
 * -------------------------------------
 */

static PyObject *VimError;

static PyObject *VimCommand(PyObject *, PyObject *);
static PyObject *VimEval(PyObject *, PyObject *);

/* Window type - Implementation functions
 * --------------------------------------
 */

typedef struct
{
    PyObject_HEAD
    WIN *win;
}
WindowObject;

#define INVALID_WINDOW_VALUE ((WIN*)(-1))

#define WindowType_Check(obj) ((obj)->ob_type == &WindowType)

static PyObject *WindowNew(WIN *);

static void WindowDestructor(PyObject *);
static PyObject *WindowGetattr(PyObject *, char *);
static int WindowSetattr(PyObject *, char *, PyObject *);
static PyObject *WindowRepr(PyObject *);

/* Buffer type - Implementation functions
 * --------------------------------------
 */

typedef struct
{
    PyObject_HEAD
    BUF *buf;
}
BufferObject;

#define INVALID_BUFFER_VALUE ((BUF*)(-1))

#define BufferType_Check(obj) ((obj)->ob_type == &BufferType)

static PyObject *BufferNew (BUF *);

static void BufferDestructor(PyObject *);
static PyObject *BufferGetattr(PyObject *, char *);
static PyObject *BufferRepr(PyObject *);

static int BufferLength(PyObject *);
static PyObject *BufferItem(PyObject *, int);
static PyObject *BufferSlice(PyObject *, int, int);
static int BufferAssItem(PyObject *, int, PyObject *);
static int BufferAssSlice(PyObject *, int, int, PyObject *);

static PyObject *BufferAppend(PyObject *, PyObject *);
static PyObject *BufferMark(PyObject *, PyObject *);
static PyObject *BufferRange(PyObject *, PyObject *);

/* Line range type - Implementation functions
 * --------------------------------------
 */

typedef struct
{
    PyObject_HEAD
    BufferObject *buf;
    int start;
    int end;
}
RangeObject;

#define RangeType_Check(obj) ((obj)->ob_type == &RangeType)

static PyObject *RangeNew(BUF *, int, int);

static void RangeDestructor(PyObject *);
static PyObject *RangeGetattr(PyObject *, char *);
static PyObject *RangeRepr(PyObject *);

static int RangeLength(PyObject *);
static PyObject *RangeItem(PyObject *, int);
static PyObject *RangeSlice(PyObject *, int, int);
static int RangeAssItem(PyObject *, int, PyObject *);
static int RangeAssSlice(PyObject *, int, int, PyObject *);

static PyObject *RangeAppend(PyObject *, PyObject *);

/* Window list type - Implementation functions
 * -------------------------------------------
 */

static int WinListLength(PyObject *);
static PyObject *WinListItem(PyObject *, int);

/* Buffer list type - Implementation functions
 * -------------------------------------------
 */

static int BufListLength(PyObject *);
static PyObject *BufListItem(PyObject *, int);

/* Current objects type - Implementation functions
 * -----------------------------------------------
 */

static PyObject *CurrentGetattr(PyObject *, char *);
static int CurrentSetattr(PyObject *, char *, PyObject *);

/* Vim module - Definitions
 */

static struct PyMethodDef VimMethods[] = {
    /* name,	     function,		calling,    documentation */
    {"command",	     VimCommand,	1,	    "" },
    {"eval",	     VimEval,		1,	    "" },
    { NULL,	     NULL,		0,	    NULL }
};

/* Vim module - Implementation
 */
/*ARGSUSED*/
    static PyObject *
VimCommand(PyObject *self, PyObject *args)
{
    char *cmd;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "s", &cmd))
	return NULL;

    PyErr_Clear();

    Py_BEGIN_ALLOW_THREADS
    Python_Lock_Vim();

    do_cmdline((char_u *)cmd, NULL, NULL, DOCMD_NOWAIT|DOCMD_VERBOSE);
    update_screen(NOT_VALID);

    Python_Release_Vim();
    Py_END_ALLOW_THREADS

    if (VimErrorCheck())
	result = NULL;
    else
	result = Py_None;

    Py_XINCREF(result);
    return result;
}

/*ARGSUSED*/
    static PyObject *
VimEval(PyObject *self, PyObject *args)
{
#ifdef WANT_EVAL
    char *expr;
    char *str;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "s", &expr))
	return NULL;

    Py_BEGIN_ALLOW_THREADS
    Python_Lock_Vim();
    str = (char *)eval_to_string((char_u *)expr, NULL);
    Python_Release_Vim();
    Py_END_ALLOW_THREADS

    if (str == NULL)
    {
	PyErr_SetVim("invalid expression");
	return NULL;
    }

    result = Py_BuildValue("s", str);

    Py_BEGIN_ALLOW_THREADS
    Python_Lock_Vim();
    vim_free(str);
    Python_Release_Vim();
    Py_END_ALLOW_THREADS

    return result;
#else
    PyErr_SetVim("expressions disabled at compile time");
    return NULL;
#endif
}

/* Common routines for buffers and line ranges
 * -------------------------------------------
 */
    static int
CheckBuffer(BufferObject *this)
{
    if (this->buf == INVALID_BUFFER_VALUE)
    {
	PyErr_SetVim("attempt to refer to deleted buffer");
	return -1;
    }

    return 0;
}

    static PyObject *
RBItem(BufferObject *self, int n, int start, int end)
{
    if (CheckBuffer(self))
	return NULL;

    if (n < 0 || n > end - start)
    {
	PyErr_SetString(PyExc_IndexError, "line number out of range");
	return NULL;
    }

    return GetBufferLine(self->buf, n+start);
}

    static PyObject *
RBSlice(BufferObject *self, int lo, int hi, int start, int end)
{
    int size;

    if (CheckBuffer(self))
	return NULL;

    size = end - start + 1;

    if (lo < 0)
	lo = 0;
    else if (lo > size)
	lo = size;
    if (hi < 0)
	hi = 0;
    if (hi < lo)
	hi = lo;
    else if (hi > size)
	hi = size;

    return GetBufferLineList(self->buf, lo+start, hi+start);
}

    static int
RBAssItem(BufferObject *self, int n, PyObject *val, int start, int end, int *new_end)
{
    int len_change;

    if (CheckBuffer(self))
	return -1;

    if (n < 0 || n > end - start)
    {
	PyErr_SetString(PyExc_IndexError, "line number out of range");
	return -1;
    }

    if (SetBufferLine(self->buf, n+start, val, &len_change) == FAIL)
	return -1;

    if (new_end)
	*new_end = end + len_change;

    return 0;
}

    static int
RBAssSlice(BufferObject *self, int lo, int hi, PyObject *val, int start, int end, int *new_end)
{
    int size;
    int len_change;

    /* Self must be a valid buffer */
    if (CheckBuffer(self))
	return -1;

    /* Sort out the slice range */
    size = end - start + 1;

    if (lo < 0)
	lo = 0;
    else if (lo > size)
	lo = size;
    if (hi < 0)
	hi = 0;
    if (hi < lo)
	hi = lo;
    else if (hi > size)
	hi = size;

    if (SetBufferLineList(self->buf, lo+start, hi+start, val, &len_change) == FAIL)
	return -1;

    if (new_end)
	*new_end = end + len_change;

    return 0;
}

    static PyObject *
RBAppend(BufferObject *self, PyObject *args, int start, int end, int *new_end)
{
    PyObject *lines;
    int len_change;
    int max;
    int n;

    if (CheckBuffer(self))
	return NULL;

    max = n = end - start + 1;

    if (!PyArg_ParseTuple(args, "O|i", &lines, &n))
	return NULL;

    if (n < 0 || n > max)
    {
	PyErr_SetString(PyExc_ValueError, "line number out of range");
	return NULL;
    }

    if (InsertBufferLines(self->buf, n + start - 1, lines, &len_change) == FAIL)
	return NULL;

    if (new_end)
	*new_end = end + len_change;

    Py_INCREF(Py_None);
    return Py_None;
}


/* Buffer object - Definitions
 */

static struct PyMethodDef BufferMethods[] = {
    /* name,	    function,		calling,    documentation */
    {"append",	    BufferAppend,	1,	    "" },
    {"mark",	    BufferMark,		1,	    "" },
    {"range",	    BufferRange,	1,	    "" },
    { NULL,	    NULL,		0,	    NULL }
};

static PySequenceMethods BufferAsSeq = {
    (inquiry)		BufferLength,	    /* sq_length,    len(x)   */
    (binaryfunc)	0, /* BufferConcat, */	     /* sq_concat,    x+y      */
    (intargfunc)	0, /* BufferRepeat, */	     /* sq_repeat,    x*n      */
    (intargfunc)	BufferItem,	    /* sq_item,      x[i]     */
    (intintargfunc)	BufferSlice,	    /* sq_slice,     x[i:j]   */
    (intobjargproc)	BufferAssItem,	    /* sq_ass_item,  x[i]=v   */
    (intintobjargproc)	BufferAssSlice,     /* sq_ass_slice, x[i:j]=v */
};

static PyTypeObject BufferType = {
    PyObject_HEAD_INIT(0)
    0,
    "buffer",
    sizeof(BufferObject),
    0,

    (destructor)    BufferDestructor,	/* tp_dealloc,	refcount==0  */
    (printfunc)     0,			/* tp_print,	print x      */
    (getattrfunc)   BufferGetattr,	/* tp_getattr,	x.attr	     */
    (setattrfunc)   0,			/* tp_setattr,	x.attr=v     */
    (cmpfunc)	    0,			/* tp_compare,	x>y	     */
    (reprfunc)	    BufferRepr,		/* tp_repr,	`x`, print x */

    0,		    /* as number */
    &BufferAsSeq,   /* as sequence */
    0,		    /* as mapping */

    (hashfunc) 0,			/* tp_hash, dict(x) */
    (ternaryfunc) 0,			/* tp_call, x()     */
    (reprfunc) 0,			/* tp_str,  str(x)  */
};

/* Buffer object - Implementation
 */

    static PyObject *
BufferNew(BUF *buf)
{
    /* We need to handle deletion of buffers underneath us.
     * If we add a "python_ref" field to the BUF structure,
     * then we can get at it in buf_freeall() in vim. We then
     * need to create only ONE Python object per buffer - if
     * we try to create a second, just INCREF the existing one
     * and return it. The (single) Python object referring to
     * the buffer is stored in "python_ref".
     * Question: what to do on a buf_freeall(). We'll probably
     * have to either delete the Python object (DECREF it to
     * zero - a bad idea, as it leaves dangling refs!) or
     * set the BUF* value to an invalid value (-1?), which
     * means we need checks in all access functions... Bah.
     */

    BufferObject *self;

    if (buf->python_ref)
	self = buf->python_ref;
    else
    {
	self = PyObject_NEW(BufferObject, &BufferType);
	if (self == NULL)
	    return NULL;
	self->buf = buf;
	buf->python_ref = self;
    }

    return (PyObject *)(self);
}

    static void
BufferDestructor(PyObject *self)
{
    BufferObject *this = (BufferObject *)(self);

    if (this->buf && this->buf != INVALID_BUFFER_VALUE)
	this->buf->python_ref = NULL;

    PyMem_DEL(self);
}

    static PyObject *
BufferGetattr(PyObject *self, char *name)
{
    BufferObject *this = (BufferObject *)(self);

    if (CheckBuffer(this))
	return NULL;

    if (strcmp(name, "name") == 0)
	return Py_BuildValue("s",this->buf->b_ffname);
    else if (strcmp(name,"__members__") == 0)
	return Py_BuildValue("[s]", "name");
    else
	return Py_FindMethod(BufferMethods, self, name);
}

    static PyObject *
BufferRepr(PyObject *self)
{
    static char repr[50];
    BufferObject *this = (BufferObject *)(self);

    if (this->buf == INVALID_BUFFER_VALUE)
    {
	sprintf(repr, "<buffer object (deleted) at %8lX>", (long)(self));
	return PyString_FromString(repr);
    }
    else
    {
	char *name = (char *)this->buf->b_fname;
	int len;

	if (name == NULL)
	    name = "";
	len = strlen(name);

	if (len > 35)
	    name = name + (35 - len);

	sprintf(repr, "<buffer %s%s>", len > 35 ? "..." : "", name);

	return PyString_FromString(repr);
    }
}

/******************/

    static int
BufferLength(PyObject *self)
{
    /* HOW DO WE SIGNAL AN ERROR FROM THIS FUNCTION? */
    if (CheckBuffer((BufferObject *)(self)))
	return -1; /* ??? */

    return (((BufferObject *)(self))->buf->b_ml.ml_line_count);
}

    static PyObject *
BufferItem(PyObject *self, int n)
{
    return RBItem((BufferObject *)(self), n, 1,
		  (int)((BufferObject *)(self))->buf->b_ml.ml_line_count);
}

    static PyObject *
BufferSlice(PyObject *self, int lo, int hi)
{
    return RBSlice((BufferObject *)(self), lo, hi, 1,
		   (int)((BufferObject *)(self))->buf->b_ml.ml_line_count);
}

    static int
BufferAssItem(PyObject *self, int n, PyObject *val)
{
    return RBAssItem((BufferObject *)(self), n, val, 1,
		     (int)((BufferObject *)(self))->buf->b_ml.ml_line_count,
		     NULL);
}

    static int
BufferAssSlice(PyObject *self, int lo, int hi, PyObject *val)
{
    return RBAssSlice((BufferObject *)(self), lo, hi, val, 1,
		      (int)((BufferObject *)(self))->buf->b_ml.ml_line_count,
		      NULL);
}

    static PyObject *
BufferAppend(PyObject *self, PyObject *args)
{
    return RBAppend((BufferObject *)(self), args, 1,
		    (int)((BufferObject *)(self))->buf->b_ml.ml_line_count,
		    NULL);
}

    static PyObject *
BufferMark(PyObject *self, PyObject *args)
{
    FPOS    *posp;
    char    mark;
    BUF	    *curbuf_save;

    if (CheckBuffer((BufferObject *)(self)))
	return NULL;

    if (!PyArg_ParseTuple(args, "c", &mark))
	return NULL;

    curbuf_save = curbuf;
    curbuf = ((BufferObject *)(self))->buf;
    posp = getmark(mark, FALSE);
    curbuf = curbuf_save;

    if (posp == NULL)
    {
	PyErr_SetVim("invalid mark name");
	return NULL;
    }

    /* Ckeck for keyboard interrupt */
    if (VimErrorCheck())
	return NULL;

    if (posp->lnum == 0)
    {
	/* Or raise an error? */
	Py_INCREF(Py_None);
	return Py_None;
    }

    return Py_BuildValue("(ll)", (long)(posp->lnum), (long)(posp->col));
}

    static PyObject *
BufferRange(PyObject *self, PyObject *args)
{
    int start;
    int end;

    if (CheckBuffer((BufferObject *)(self)))
	return NULL;

    if (!PyArg_ParseTuple(args, "ii", &start, &end))
	return NULL;

    return RangeNew(((BufferObject *)(self))->buf, start, end);
}

/* Line range object - Definitions
 */

static struct PyMethodDef RangeMethods[] = {
    /* name,	    function,		calling,    documentation */
    {"append",	    RangeAppend,	1,	    "" },
    { NULL,	    NULL,		0,	    NULL }
};

static PySequenceMethods RangeAsSeq = {
    (inquiry)		RangeLength,	    /* sq_length,    len(x)   */
    (binaryfunc)	0, /* RangeConcat, */	     /* sq_concat,    x+y      */
    (intargfunc)	0, /* RangeRepeat, */	     /* sq_repeat,    x*n      */
    (intargfunc)	RangeItem,	    /* sq_item,      x[i]     */
    (intintargfunc)	RangeSlice,	    /* sq_slice,     x[i:j]   */
    (intobjargproc)	RangeAssItem,	    /* sq_ass_item,  x[i]=v   */
    (intintobjargproc)	RangeAssSlice,	    /* sq_ass_slice, x[i:j]=v */
};

static PyTypeObject RangeType = {
    PyObject_HEAD_INIT(0)
    0,
    "range",
    sizeof(RangeObject),
    0,

    (destructor)    RangeDestructor,	/* tp_dealloc,	refcount==0  */
    (printfunc)     0,			/* tp_print,	print x      */
    (getattrfunc)   RangeGetattr,	/* tp_getattr,	x.attr	     */
    (setattrfunc)   0,			/* tp_setattr,	x.attr=v     */
    (cmpfunc)	    0,			/* tp_compare,	x>y	     */
    (reprfunc)	    RangeRepr,		/* tp_repr,	`x`, print x */

    0,		    /* as number */
    &RangeAsSeq,    /* as sequence */
    0,		    /* as mapping */

    (hashfunc) 0,			/* tp_hash, dict(x) */
    (ternaryfunc) 0,			/* tp_call, x()     */
    (reprfunc) 0,			/* tp_str,  str(x)  */
};

/* Line range object - Implementation
 */

    static PyObject *
RangeNew(BUF *buf, int start, int end)
{
    BufferObject *bufr;
    RangeObject *self;
    self = PyObject_NEW(RangeObject, &RangeType);
    if (self == NULL)
	return NULL;

    bufr = (BufferObject *)BufferNew(buf);
    if (bufr == NULL)
    {
	PyMem_DEL(self);
	return NULL;
    }
    Py_INCREF(bufr);

    self->buf = bufr;
    self->start = start;
    self->end = end;

    return (PyObject *)(self);
}

    static void
RangeDestructor(PyObject *self)
{
    Py_DECREF(((RangeObject *)(self))->buf);
    PyMem_DEL(self);
}

    static PyObject *
RangeGetattr(PyObject *self, char *name)
{
    return Py_FindMethod(RangeMethods, self, name);
}

    static PyObject *
RangeRepr(PyObject *self)
{
    static char repr[75];
    RangeObject *this = (RangeObject *)(self);

    if (this->buf->buf == INVALID_BUFFER_VALUE)
    {
	sprintf(repr, "<range object (for deleted buffer) at %8lX>",
								(long)(self));
	return PyString_FromString(repr);
    }
    else
    {
	char *name = (char *)this->buf->buf->b_fname;
	int len;

	if (name == NULL)
	    name = "";
	len = strlen(name);

	if (len > 45)
	    name = name + (45 - len);

	sprintf(repr, "<range %s%s (%d:%d)>",
		len > 45 ? "..." : "", name,
		this->start, this->end);

	return PyString_FromString(repr);
    }
}

/****************/

    static int
RangeLength(PyObject *self)
{
    /* HOW DO WE SIGNAL AN ERROR FROM THIS FUNCTION? */
    if (CheckBuffer(((RangeObject *)(self))->buf))
	return -1; /* ??? */

    return (((RangeObject *)(self))->end - ((RangeObject *)(self))->start + 1);
}

    static PyObject *
RangeItem(PyObject *self, int n)
{
    return RBItem(((RangeObject *)(self))->buf, n,
		  ((RangeObject *)(self))->start,
		  ((RangeObject *)(self))->end);
}

    static PyObject *
RangeSlice(PyObject *self, int lo, int hi)
{
    return RBSlice(((RangeObject *)(self))->buf, lo, hi,
		   ((RangeObject *)(self))->start,
		   ((RangeObject *)(self))->end);
}

    static int
RangeAssItem(PyObject *self, int n, PyObject *val)
{
    return RBAssItem(((RangeObject *)(self))->buf, n, val,
		     ((RangeObject *)(self))->start,
		     ((RangeObject *)(self))->end,
		     &((RangeObject *)(self))->end);
}

    static int
RangeAssSlice(PyObject *self, int lo, int hi, PyObject *val)
{
    return RBAssSlice(((RangeObject *)(self))->buf, lo, hi, val,
		      ((RangeObject *)(self))->start,
		      ((RangeObject *)(self))->end,
		      &((RangeObject *)(self))->end);
}

    static PyObject *
RangeAppend(PyObject *self, PyObject *args)
{
    return RBAppend(((RangeObject *)(self))->buf, args,
		    ((RangeObject *)(self))->start,
		    ((RangeObject *)(self))->end,
		    &((RangeObject *)(self))->end);
}

/* Buffer list object - Definitions
 */

typedef struct
{
    PyObject_HEAD
}
BufListObject;

static PySequenceMethods BufListAsSeq = {
    (inquiry)		BufListLength,	    /* sq_length,    len(x)   */
    (binaryfunc)	0,		    /* sq_concat,    x+y      */
    (intargfunc)	0,		    /* sq_repeat,    x*n      */
    (intargfunc)	BufListItem,	    /* sq_item,      x[i]     */
    (intintargfunc)	0,		    /* sq_slice,     x[i:j]   */
    (intobjargproc)	0,		    /* sq_ass_item,  x[i]=v   */
    (intintobjargproc)	0,		    /* sq_ass_slice, x[i:j]=v */
};

static PyTypeObject BufListType = {
    PyObject_HEAD_INIT(0)
    0,
    "buffer list",
    sizeof(BufListObject),
    0,

    (destructor)    0,			/* tp_dealloc,	refcount==0  */
    (printfunc)     0,			/* tp_print,	print x      */
    (getattrfunc)   0,			/* tp_getattr,	x.attr	     */
    (setattrfunc)   0,			/* tp_setattr,	x.attr=v     */
    (cmpfunc)	    0,			/* tp_compare,	x>y	     */
    (reprfunc)	    0,			/* tp_repr,	`x`, print x */

    0,		    /* as number */
    &BufListAsSeq,  /* as sequence */
    0,		    /* as mapping */

    (hashfunc) 0,			/* tp_hash, dict(x) */
    (ternaryfunc) 0,			/* tp_call, x()     */
    (reprfunc) 0,			/* tp_str,  str(x)  */
};

/* Buffer list object - Implementation
 */

/*ARGSUSED*/
    static int
BufListLength(PyObject *self)
{
    BUF *b = firstbuf;
    int n = 0;

    while (b)
    {
	++n;
	b = b->b_next;
    }

    return n;
}

/*ARGSUSED*/
    static PyObject *
BufListItem(PyObject *self, int n)
{
    BUF *b;

    for (b = firstbuf; b; b = b->b_next, --n)
    {
	if (n == 0)
	    return BufferNew(b);
    }

    PyErr_SetString(PyExc_IndexError, "no such buffer");
    return NULL;
}

/* Window object - Definitions
 */

static struct PyMethodDef WindowMethods[] = {
    /* name,	    function,		calling,    documentation */
    { NULL,	    NULL,		0,	    NULL }
};

static PyTypeObject WindowType = {
    PyObject_HEAD_INIT(0)
    0,
    "window",
    sizeof(WindowObject),
    0,

    (destructor)    WindowDestructor,	/* tp_dealloc,	refcount==0  */
    (printfunc)     0,			/* tp_print,	print x      */
    (getattrfunc)   WindowGetattr,	/* tp_getattr,	x.attr	     */
    (setattrfunc)   WindowSetattr,	/* tp_setattr,	x.attr=v     */
    (cmpfunc)	    0,			/* tp_compare,	x>y	     */
    (reprfunc)	    WindowRepr,		/* tp_repr,	`x`, print x */

    0,		    /* as number */
    0,		    /* as sequence */
    0,		    /* as mapping */

    (hashfunc) 0,			/* tp_hash, dict(x) */
    (ternaryfunc) 0,			/* tp_call, x()     */
    (reprfunc) 0,			/* tp_str,  str(x)  */
};

/* Window object - Implementation
 */

    static PyObject *
WindowNew(WIN *win)
{
    /* We need to handle deletion of windows underneath us.
     * If we add a "python_ref" field to the WIN structure,
     * then we can get at it in win_free() in vim. We then
     * need to create only ONE Python object per window - if
     * we try to create a second, just INCREF the existing one
     * and return it. The (single) Python object referring to
     * the window is stored in "python_ref".
     * On a win_free() we set the Python object's WIN* field
     * to an invalid value. We trap all uses of a window
     * object, and reject them if the WIN* field is invalid.
     */

    WindowObject *self;

    if (win->python_ref)
	self = win->python_ref;
    else
    {
	self = PyObject_NEW(WindowObject, &WindowType);
	if (self == NULL)
	    return NULL;
	self->win = win;
	win->python_ref = self;
    }

    return (PyObject *)(self);
}

    static void
WindowDestructor(PyObject *self)
{
    WindowObject *this = (WindowObject *)(self);

    if (this->win && this->win != INVALID_WINDOW_VALUE)
	this->win->python_ref = NULL;

    PyMem_DEL(self);
}

    static int
CheckWindow(WindowObject *this)
{
    if (this->win == INVALID_WINDOW_VALUE)
    {
	PyErr_SetVim("attempt to refer to deleted window");
	return -1;
    }

    return 0;
}

    static PyObject *
WindowGetattr(PyObject *self, char *name)
{
    WindowObject *this = (WindowObject *)(self);

    if (CheckWindow(this))
	return NULL;

    if (strcmp(name, "buffer") == 0)
	return (PyObject *)BufferNew(this->win->w_buffer);
    else if (strcmp(name, "cursor") == 0)
    {
	FPOS *pos = &this->win->w_cursor;
	return Py_BuildValue("(ll)", (long)(pos->lnum), (long)(pos->col));
    }
    else if (strcmp(name, "height") == 0)
	return Py_BuildValue("l", (long)(this->win->w_height));
    else if (strcmp(name,"__members__") == 0)
	return Py_BuildValue("[sss]", "buffer", "cursor", "height");
    else
	return Py_FindMethod(WindowMethods, self, name);
}

    static int
WindowSetattr(PyObject *self, char *name, PyObject *val)
{
    WindowObject *this = (WindowObject *)(self);

    if (CheckWindow(this))
	return -1;

    if (strcmp(name, "buffer") == 0)
    {
	PyErr_SetString(PyExc_TypeError, "readonly attribute");
	return -1;
    }
    else if (strcmp(name, "cursor") == 0)
    {
	long lnum;
	long col;

	if (!PyArg_Parse(val, "(ll)", &lnum, &col))
	    return -1;

	if (lnum <= 0 || lnum > this->win->w_buffer->b_ml.ml_line_count)
	{
	    PyErr_SetVim("cursor position outside buffer");
	    return -1;
	}

	/* Check for keyboard interrupts */
	if (VimErrorCheck())
	    return -1;

	/* NO CHECK ON COLUMN - SEEMS NOT TO MATTER */

	this->win->w_cursor.lnum = lnum;
	this->win->w_cursor.col = col;
	update_screen(NOT_VALID);

	return 0;
    }
    else if (strcmp(name, "height") == 0)
    {
	int height;
	WIN *savewin;

	if (!PyArg_Parse(val, "i", &height))
	    return -1;

#ifdef USE_GUI
	need_mouse_correct = TRUE;
#endif
	savewin = curwin;
	curwin = this->win;
	win_setheight(height);
	curwin = savewin;

	/* Check for keyboard interrupts */
	if (VimErrorCheck())
	    return -1;

	return 0;
    }
    else
    {
	PyErr_SetString(PyExc_AttributeError, name);
	return -1;
    }
}

    static PyObject *
WindowRepr(PyObject *self)
{
    static char repr[50];
    WindowObject *this = (WindowObject *)(self);

    if (this->win == INVALID_WINDOW_VALUE)
    {
	sprintf(repr, "<window object (deleted) at %.8lX>", (long)(self));
	return PyString_FromString(repr);
    }
    else
    {
	int i = 0;
	WIN *w;

	for (w = firstwin; w != NULL && w != this->win; w = w->w_next)
	    ++i;

	if (w == NULL)
	    sprintf(repr, "<window object (unknown) at %.8lX>", (long)(self));
	else
	    sprintf(repr, "<window %d>", i);

	return PyString_FromString(repr);
    }
}

/* Window list object - Definitions
 */

typedef struct
{
    PyObject_HEAD
}
WinListObject;

static PySequenceMethods WinListAsSeq = {
    (inquiry)		WinListLength,	    /* sq_length,    len(x)   */
    (binaryfunc)	0,		    /* sq_concat,    x+y      */
    (intargfunc)	0,		    /* sq_repeat,    x*n      */
    (intargfunc)	WinListItem,	    /* sq_item,      x[i]     */
    (intintargfunc)	0,		    /* sq_slice,     x[i:j]   */
    (intobjargproc)	0,		    /* sq_ass_item,  x[i]=v   */
    (intintobjargproc)	0,		    /* sq_ass_slice, x[i:j]=v */
};

static PyTypeObject WinListType = {
    PyObject_HEAD_INIT(0)
    0,
    "window list",
    sizeof(WinListObject),
    0,

    (destructor)    0,			/* tp_dealloc,	refcount==0  */
    (printfunc)     0,			/* tp_print,	print x      */
    (getattrfunc)   0,			/* tp_getattr,	x.attr	     */
    (setattrfunc)   0,			/* tp_setattr,	x.attr=v     */
    (cmpfunc)	    0,			/* tp_compare,	x>y	     */
    (reprfunc)	    0,			/* tp_repr,	`x`, print x */

    0,		    /* as number */
    &WinListAsSeq,  /* as sequence */
    0,		    /* as mapping */

    (hashfunc) 0,			/* tp_hash, dict(x) */
    (ternaryfunc) 0,			/* tp_call, x()     */
    (reprfunc) 0,			/* tp_str,  str(x)  */
};

/* Window list object - Implementation
 */
/*ARGSUSED*/
    static int
WinListLength(PyObject *self)
{
    WIN *w = firstwin;
    int n = 0;

    while (w)
    {
	++n;
	w = w->w_next;
    }

    return n;
}

/*ARGSUSED*/
    static PyObject *
WinListItem(PyObject *self, int n)
{
    WIN *w;

    for (w = firstwin; w; w = w->w_next, --n)
    {
	if (n == 0)
	    return WindowNew(w);
    }

    PyErr_SetString(PyExc_IndexError, "no such window");
    return NULL;
}

/* Current items object - Definitions
 */

typedef struct
{
    PyObject_HEAD
}
CurrentObject;

static PyTypeObject CurrentType = {
    PyObject_HEAD_INIT(0)
    0,
    "current data",
    sizeof(CurrentObject),
    0,

    (destructor)    0,			/* tp_dealloc,	refcount==0  */
    (printfunc)     0,			/* tp_print,	print x      */
    (getattrfunc)   CurrentGetattr,	/* tp_getattr,	x.attr	     */
    (setattrfunc)   CurrentSetattr,	/* tp_setattr,	x.attr=v     */
    (cmpfunc)	    0,			/* tp_compare,	x>y	     */
    (reprfunc)	    0,			/* tp_repr,	`x`, print x */

    0,		    /* as number */
    0,		    /* as sequence */
    0,		    /* as mapping */

    (hashfunc) 0,			/* tp_hash, dict(x) */
    (ternaryfunc) 0,			/* tp_call, x()     */
    (reprfunc) 0,			/* tp_str,  str(x)  */
};

/* Current items object - Implementation
 */
/*ARGSUSED*/
    static PyObject *
CurrentGetattr(PyObject *self, char *name)
{
    if (strcmp(name, "buffer") == 0)
	return (PyObject *)BufferNew(curbuf);
    else if (strcmp(name, "window") == 0)
	return (PyObject *)WindowNew(curwin);
    else if (strcmp(name, "line") == 0)
	return GetBufferLine(curbuf, (int)curwin->w_cursor.lnum);
    else if (strcmp(name, "range") == 0)
	return RangeNew(curbuf, RangeStart, RangeEnd);
    else if (strcmp(name,"__members__") == 0)
	return Py_BuildValue("[ssss]", "buffer", "window", "line", "range");
    else
    {
	PyErr_SetString(PyExc_AttributeError, name);
	return NULL;
    }
}

/*ARGSUSED*/
    static int
CurrentSetattr(PyObject *self, char *name, PyObject *value)
{
    if (strcmp(name, "line") == 0)
    {
	if (SetBufferLine(curbuf, (int)curwin->w_cursor.lnum, value, NULL) == FAIL)
	    return -1;

	return 0;
    }
    else
    {
	PyErr_SetString(PyExc_AttributeError, name);
	return -1;
    }
}

/* External interface
 */

    void
python_buffer_free(BUF *buf)
{
    if (buf->python_ref)
    {
	BufferObject *bp = buf->python_ref;
	bp->buf = INVALID_BUFFER_VALUE;
	buf->python_ref = NULL;
    }
}

    void
python_window_free(WIN *win)
{
    if (win->python_ref)
    {
	WindowObject *wp = win->python_ref;
	wp->win = INVALID_WINDOW_VALUE;
	win->python_ref = NULL;
    }
}

static BufListObject TheBufferList =
{
    PyObject_HEAD_INIT(&BufListType)
};

static WinListObject TheWindowList =
{
    PyObject_HEAD_INIT(&WinListType)
};

static CurrentObject TheCurrent =
{
    PyObject_HEAD_INIT(&CurrentType)
};

    static int
PythonMod_Init(void)
{
    PyObject *mod;
    PyObject *dict;

    /* Fixups... */
    BufferType.ob_type = &PyType_Type;
    RangeType.ob_type = &PyType_Type;
    WindowType.ob_type = &PyType_Type;
    BufListType.ob_type = &PyType_Type;
    WinListType.ob_type = &PyType_Type;
    CurrentType.ob_type = &PyType_Type;

    mod = Py_InitModule("vim", VimMethods);
    dict = PyModule_GetDict(mod);

    VimError = Py_BuildValue("s", "vim.error");

    PyDict_SetItemString(dict, "error", VimError);
    PyDict_SetItemString(dict, "buffers", (PyObject *)(&TheBufferList));
    PyDict_SetItemString(dict, "current", (PyObject *)(&TheCurrent));
    PyDict_SetItemString(dict, "windows", (PyObject *)(&TheWindowList));

    if (PyErr_Occurred())
	return -1;

    return 0;
}

/*************************************************************************
 * 4. Utility functions for handling the interface between Vim and Python.
 */

/* Get a line from the specified buffer. The line number is
 * in Vim format (1-based). The line is returned as a Python
 * string object.
 */
    static PyObject *
GetBufferLine(BUF *buf, int n)
{
    return LineToString((char *)ml_get_buf(buf, (linenr_t)n, FALSE));
}

/* Get a list of lines from the specified buffer. The line numbers
 * are in Vim format (1-based). The range is from lo up to, but not
 * including, hi. The list is returned as a Python list of string objects.
 */
    static PyObject *
GetBufferLineList(BUF *buf, int lo, int hi)
{
    int i;
    int n = hi - lo;
    PyObject *list = PyList_New(n);

    if (list == NULL)
	return NULL;

    for (i = 0; i < n; ++i)
    {
	PyObject *str = LineToString((char *)ml_get_buf(buf, (linenr_t)(lo+i), FALSE));

	/* Error check - was the Python string creation OK? */
	if (str == NULL)
	{
	    Py_DECREF(list);
	    return NULL;
	}

	/* Set the list item */
	if (PyList_SetItem(list, i, str))
	{
	    Py_DECREF(str);
	    Py_DECREF(list);
	    return NULL;
	}
    }

    /* The ownership of the Python list is passed to the caller (ie,
     * the caller should Py_DECREF() the object when it is finished
     * with it).
     */

    return list;
}

/* Replace a line in the specified buffer. The line number is
 * in Vim format (1-based). The replacement line is given as
 * a Python string object. The object is checked for validity
 * and correct format. Errors are returned as a value of FAIL.
 * The return value is OK on success.
 * If OK is returned and len_change is not NULL, *len_change
 * is set to the change in the buffer length.
 */
    static int
SetBufferLine(BUF *buf, int n, PyObject *line, int *len_change)
{
    /* First of all, we check the thpe of the supplied Python object.
     * There are three cases:
     *	  1. NULL, or None - this is a deletion.
     *	  2. A string	   - this is a replacement.
     *	  3. Anything else - this is an error.
     */
    if (line == Py_None || line == NULL)
    {
	BUF *savebuf = curbuf;

	PyErr_Clear();
	curbuf = buf;

	if (u_savedel((linenr_t)n, 1L) == FAIL)
	    PyErr_SetVim("cannot save undo information");
	else if (ml_delete((linenr_t)n, FALSE) == FAIL)
	    PyErr_SetVim("cannot delete line");
	else
	{
	    mark_adjust((linenr_t)n, (linenr_t)n, (long)MAXLNUM, -1L);
	    changed();
	}

	curbuf = savebuf;
	update_screen(NOT_VALID);

	if (PyErr_Occurred() || VimErrorCheck())
	    return FAIL;

	if (len_change)
	    *len_change = -1;

	return OK;
    }
    else if (PyString_Check(line))
    {
	char *save = StringToLine(line);
	BUF *savebuf = curbuf;

	if (save == NULL)
	    return FAIL;

	/* We do not need to free save, as we pass responsibility for
	 * it to vim, via the final parameter of ml_replace().
	 */
	PyErr_Clear();
	curbuf = buf;

	if (u_savesub((linenr_t)n) == FAIL)
	    PyErr_SetVim("cannot save undo information");
	else if (ml_replace((linenr_t)n, (char_u *)save, TRUE) == FAIL)
	    PyErr_SetVim("cannot replace line");
	else
	{
	    changed();
#ifdef SYNTAX_HL
	    syn_changed((linenr_t)n); /* recompute syntax hl. for this line */
#endif
	}

	curbuf = savebuf;
	update_screen(NOT_VALID);

	if (PyErr_Occurred() || VimErrorCheck())
	    return FAIL;

	if (len_change)
	    *len_change = 0;

	return OK;
    }
    else
    {
	PyErr_BadArgument();
	return FAIL;
    }
}

/* Replace a range of lines in the specified buffer. The line numbers are in
 * Vim format (1-based). The range is from lo up to, but not including, hi.
 * The replacement lines are given as a Python list of string objects. The
 * list is checked for validity and correct format. Errors are returned as a
 * value of FAIL.  The return value is OK on success.
 * If OK is returned and len_change is not NULL, *len_change
 * is set to the change in the buffer length.
 */
    static int
SetBufferLineList(BUF *buf, int lo, int hi, PyObject *list, int *len_change)
{
    /* First of all, we check the thpe of the supplied Python object.
     * There are three cases:
     *	  1. NULL, or None - this is a deletion.
     *	  2. A list	   - this is a replacement.
     *	  3. Anything else - this is an error.
     */
    if (list == Py_None || list == NULL)
    {
	int i;
	int ok = 0;
	int n = hi - lo;
	BUF *savebuf = curbuf;

	PyErr_Clear();
	curbuf = buf;

	if (u_savedel((linenr_t)lo, (long)n) == FAIL)
	    PyErr_SetVim("cannot save undo information");
	else
	{
	    ok = 1;

	    for (i = 0; i < n; ++i)
	    {
		if (ml_delete((linenr_t)lo, FALSE) == FAIL)
		{
		    PyErr_SetVim("cannot delete line");
		    ok = 0;
		    break;
		}
		changed();
	    }
	}

	if (ok)
	    mark_adjust((linenr_t)lo, (linenr_t)(hi-1), (long)MAXLNUM,
								    (long)-n);

	curbuf = savebuf;
	update_screen(NOT_VALID);

	if (PyErr_Occurred() || VimErrorCheck())
	    return FAIL;

	if (len_change)
	    *len_change = -n;

	return OK;
    }
    else if (PyList_Check(list))
    {
	int i;
	int n = PyList_Size(list);
	int lines = hi - lo;
	char **array;
	BUF *savebuf;

	array = (char **)alloc((unsigned)(n * sizeof(char *)));
	if (array == NULL)
	{
	    PyErr_NoMemory();
	    return FAIL;
	}

	for (i = 0; i < n; ++i)
	{
	    PyObject *line = PyList_GetItem(list, i);
	    array[i] = StringToLine(line);
	    if (array[i] == NULL)
	    {
		while (i)
		    vim_free(array[--i]);
		vim_free(array);
		return FAIL;
	    }
	}

	savebuf = curbuf;

	PyErr_Clear();
	curbuf = buf;

	if (u_save((linenr_t)(lo-1), (linenr_t)hi) == FAIL)
	    PyErr_SetVim("cannot save undo information");

	/* If the size of the range is reducing (ie, n < lines) we
	 * need to delete some lines. We do this at the start, by
	 * repeatedly deleting line "lo".
	 */
	if (!PyErr_Occurred())
	{
	    for (i = 0; i < lines - n; ++i)
	    {
		if (ml_delete((linenr_t)lo, FALSE) == FAIL)
		{
		    PyErr_SetVim("cannot delete line");
		    break;
		}
		changed();
	    }
	}

	/* For as long as possible, replace the existing lines with the
	 * new lines. This is a more efficient operation, as it requires
	 * less memory allocation and freeing.
	 */
	if (!PyErr_Occurred())
	{
	    for (i = 0; i < lines && i < n; ++i)
	    {
		if (ml_replace((linenr_t)(lo+i), (char_u *)array[i], TRUE)
								      == FAIL)
		{
		    PyErr_SetVim("cannot replace line");
		    break;
		}
		changed();
#ifdef SYNTAX_HL
		/* recompute syntax hl. for this line */
		syn_changed((linenr_t)(lo+i));
#endif
	    }
	}

	/* Now we may need to insert the remaining new lines. If we do, we
	 * must free the strings as we finish with them (we can't pass the
	 * responsibility to vim in this case).
	 */
	if (!PyErr_Occurred())
	{
	    while (i < n)
	    {
		if (ml_append((linenr_t)(lo+i-1), (char_u *)array[i], 0, FALSE) == FAIL)
		{
		    PyErr_SetVim("cannot insert line");
		    break;
		}
		changed();
		vim_free(array[i]);
		++i;
	    }
	}

	/* Free any left-over lines, as a result of an error */
	while (i < n)
	{
	    vim_free(array[i]);
	    ++i;
	}

	/* Adjust marks. Invalidate any which lie in the
	 * changed range, and move any in the remainder of the buffer.
	 */
	if (!PyErr_Occurred())
	    mark_adjust((linenr_t)lo, (linenr_t)(hi-1), (long)MAXLNUM, (long)(n - lines));

	/* Free the array of lines. All of its contents have now
	 * been dealt with (either freed, or the responsibility passed
	 * to vim.
	 */
	vim_free(array);

	curbuf = savebuf;
	update_screen(NOT_VALID);

	if (PyErr_Occurred() || VimErrorCheck())
	    return FAIL;

	if (len_change)
	    *len_change = n - lines;

	return OK;
    }
    else
    {
	PyErr_BadArgument();
	return FAIL;
    }
}

/* Insert a number of lines into the specified buffer after the specifed line.
 * The line number is in Vim format (1-based). The lines to be inserted are
 * given as a Python list of string objects or as a single string. The lines
 * to be added are checked for validity and correct format. Errors are
 * returned as a value of FAIL.  The return value is OK on success.
 * If OK is returned and len_change is not NULL, *len_change
 * is set to the change in the buffer length.
 */
    static int
InsertBufferLines(BUF *buf, int n, PyObject *lines, int *len_change)
{
    /* First of all, we check the type of the supplied Python object.
     * It must be a string or a list, or the call is in error.
     */
    if (PyString_Check(lines))
    {
	char *str = StringToLine(lines);
	BUF *savebuf;

	if (str == NULL)
	    return FAIL;

	savebuf = curbuf;

	PyErr_Clear();
	curbuf = buf;

	if (u_save((linenr_t)n, (linenr_t)(n+1)) == FAIL)
	    PyErr_SetVim("cannot save undo information");
	else if (ml_append((linenr_t)n, (char_u *)str, 0, FALSE) == FAIL)
	    PyErr_SetVim("cannot insert line");
	else
	{
	    mark_adjust((linenr_t)(n+1), (linenr_t)MAXLNUM, 1L, 0L);
	    changed();
	}

	vim_free(str);
	curbuf = savebuf;
	update_screen(NOT_VALID);

	if (PyErr_Occurred() || VimErrorCheck())
	    return FAIL;

	if (len_change)
	    *len_change = 1;

	return OK;
    }
    else if (PyList_Check(lines))
    {
	int i;
	int ok = 0;
	int size = PyList_Size(lines);
	char **array;
	BUF *savebuf;

	array = (char **)alloc((unsigned)(size * sizeof(char *)));
	if (array == NULL)
	{
	    PyErr_NoMemory();
	    return FAIL;
	}

	for (i = 0; i < size; ++i)
	{
	    PyObject *line = PyList_GetItem(lines, i);
	    array[i] = StringToLine(line);

	    if (array[i] == NULL)
	    {
		while (i)
		    vim_free(array[--i]);
		vim_free(array);
		return FAIL;
	    }
	}

	savebuf = curbuf;

	PyErr_Clear();
	curbuf = buf;

	if (u_save((linenr_t)n, (linenr_t)(n+1)) == FAIL)
	    PyErr_SetVim("cannot save undo information");
	else
	{
	    ok = 1;
	    for (i = 0; i < size; ++i)
	    {
		if (ml_append((linenr_t)(n+i), (char_u *)array[i], 0, FALSE) == FAIL)
		{
		    PyErr_SetVim("cannot insert line");
		    ok = 0;

		    /* Free the rest of the lines */
		    while (i < size)
			vim_free(array[i++]);

		    break;
		}
		changed();
		vim_free(array[i]);
	    }
	}

	if (ok)
	    mark_adjust((linenr_t)(n+1), (linenr_t)MAXLNUM, (long)size, 0L);

	/* Free the array of lines. All of its contents have now
	 * been freed.
	 */
	vim_free(array);

	curbuf = savebuf;
	update_screen(NOT_VALID);

	if (PyErr_Occurred() || VimErrorCheck())
	    return FAIL;

	if (len_change)
	    *len_change = size;

	return OK;
    }
    else
    {
	PyErr_BadArgument();
	return FAIL;
    }
}

/* Convert a Vim line into a Python string.
 * All internal newlines are replaced by null characters.
 *
 * On errors, the Python exception data is set, and NULL is returned.
 */
    static PyObject *
LineToString(const char *str)
{
    PyObject *result;
    int len = strlen(str);
    char *p;

    /* Allocate an Python string object, with uninitialised contents. We
     * must do it this way, so that we can modify the string in place
     * later. See the Python source, Objects/stringobject.c for details.
     */
    result = PyString_FromStringAndSize(NULL, len);
    if (result == NULL)
	return NULL;

    p = PyString_AsString(result);

    while (*str)
    {
	if (*str == '\n')
	    *p = '\0';
	else
	    *p = *str;

	++p;
	++str;
    }

    return result;
}

/* Convert a Python string into a Vim line.
 *
 * The result is in allocated memory. All internal nulls are replaced by
 * newline characters. It is an error for the string to contain newline
 * characters.
 *
 * On errors, the Python exception data is set, and NULL is returned.
 */
    static char *
StringToLine(PyObject *obj)
{
    const char *str;
    char *save;
    int len;
    int i;

    if (obj == NULL || !PyString_Check(obj))
    {
	PyErr_BadArgument();
	return NULL;
    }

    str = PyString_AsString(obj);
    len = PyString_Size(obj);

    /* Error checking: String must not contain newlines, as we
     * are replacing a single line, and we must replace it with
     * a single line.
     */
    if (memchr(str, '\n', len))
    {
	PyErr_SetVim("string cannot contain newlines");
	return NULL;
    }

    /* Create a copy of the string, with internal nulls replaced by
     * newline characters, as is the vim convention.
     */
    save = (char *)alloc((unsigned)(len+1));
    if (save == NULL)
    {
	PyErr_NoMemory();
	return NULL;
    }

    for (i = 0; i < len; ++i)
    {
	if (str[i] == '\0')
	    save[i] = '\n';
	else
	    save[i] = str[i];
    }

    save[i] = '\0';

    return save;
}

/* Check to see whether a Vim error has been reported, or a keyboard
 * interrupt has been detected.
 */
    static int
VimErrorCheck(void)
{
    if (got_int)
    {
	PyErr_SetNone(PyExc_KeyboardInterrupt);
	return 1;
    }
    else if (did_emsg && !PyErr_Occurred())
    {
	PyErr_SetNone(VimError);
	return 1;
    }

    return 0;
}


#if PYTHON_API_VERSION < 1007 /* Python 1.4 */

    char *
Py_GetProgramName(void)
{
    return "vim";
}
#endif /* Python 1.4 */
