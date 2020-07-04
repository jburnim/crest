type mach = {
  version_major : int;
  version_minor : int;
  version : string;
  underscore_name : bool;
  sizeof_short : int;
  sizeof_int : int;
  sizeof_bool : int;
  sizeof_long : int;
  sizeof_longlong : int;
  sizeof_ptr : int;
  sizeof_float : int;
  sizeof_double : int;
  sizeof_longdouble : int;
  sizeof_void : int;
  sizeof_fun : int;
  size_t : string;
  wchar_t : string;
  alignof_short : int;
  alignof_int : int;
  alignof_bool : int;
  alignof_long : int;
  alignof_longlong : int;
  alignof_ptr : int;
  alignof_enum : int;
  alignof_float : int;
  alignof_double : int;
  alignof_longdouble : int;
  alignof_str : int;
  alignof_fun : int;
  alignof_aligned : int;
  char_is_unsigned : bool;
  const_string_literals : bool;
  little_endian : bool;
  __thread_is_keyword : bool;
  __builtin_va_list : bool;
}
val gcc : mach
val hasMSVC : bool
val msvc : mach
val theMachine : mach ref
