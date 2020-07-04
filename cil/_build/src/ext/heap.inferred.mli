type 'a t = {
  elements : (int * 'a option) array;
  mutable size : int;
  capacity : int;
}
val create : int -> 'a t
val clear : 'a t -> unit
val is_full : 'a t -> bool
val is_empty : 'a t -> bool
val insert : 'a t -> int -> 'a -> unit
val examine_max : 'a t -> int * 'a
val extract_max : 'a t -> int * 'a
