type specified = int
type maximum = int

exception BVIndexOutOfBounds of string * specified * maximum
exception BVZeroSized of string

let pzero = PL.Constant(PL.False)
let pone  = PL.Constant(PL.True)

type t = 
{
  name: string;
  bits: PL.prop array;
  real: bool;
}

let print t =
  let _ = Printf.printf "BitVector %s[%d] (anonymous %b):\n" t.name (Array.length(t.bits)) t.real in
  Array.iteri (fun i b -> Printf.printf "\t#%02d: %s\n" i (PL.string_of_prop b)) t.bits

let tzero = { name = "@zero"; bits = [|pzero|]; real = false; }
let tone  = { name = "@one";  bits = [|pone |]; real = false; }

let name_of_bv t = t.name
let size_of_bv t = Array.length t.bits
let bit t i = 
  let s = size_of_bv t in
  if i >= s then raise (BVIndexOutOfBounds(t.name,i,s))
  else t.bits.(i)
 
let set_bit t i v =
  let s = size_of_bv t in
  if i >= s then raise (BVIndexOutOfBounds(t.name,i,s))
  else t.bits.(i) <- v

(* breal: true if it corresponds to a user-declared bitvector, false if it is
   the result of evaluating an anonymous subexpression *)
let create_inner breal name n =
  let prefix = 
    if breal
    then name^"#"
    else "@"^name^"#"
  in
  { 
    name = name; 
    bits = Array.init n (fun i -> PL.Variable(prefix^string_of_int i));
    real = breal;
  }  

let create_fresh_named = create_inner true
  
let anon_ctr = ref 0

let make_anon_name () = 
  let i = !anon_ctr in
  incr anon_ctr;
  "a"^string_of_int i

let create_fresh_anon n = create_inner false (make_anon_name ()) n

let const_ctr = ref 0

let create_fresh_from_i64 i64 n =
  let i = !const_ctr in
  incr const_ctr;
  let name = "@c"^string_of_int i in
  let arr = Array.init n (fun i -> 
    if Int64.(logand i64 (shift_left 1L i)) <> 0L
    then pone
    else pzero)
  in
  { 
    name = name; 
    bits = arr;
    real = false;
  }

let blit a_write wpos a_read rpos len =
  Array.blit a_write wpos a_read rpos len;
  wpos+len

type bvsubconstructor =           
| Extract of t * int * int
| Repeat of bvsubconstructor * int
| Existing of t

let rec write_piece arr pos = function
| Extract(bv,h,l) -> blit arr pos bv.bits l (h-(l-1))
| Repeat(x,n) -> 
  let rec aux pos i = 
    if i = n
    then pos
    else aux (write_piece arr pos x) (i+1)
  in aux pos 0           
| Existing(bv) -> blit arr pos bv.bits 0 (size_of_bv bv)

let create_custom list = 
  let rec length_of_composed_bitvector = function
  | Extract(_,h,l) -> h-(l-1)
  | Repeat(x,n)    -> (length_of_composed_bitvector x)*n
  | Existing(bv)   -> size_of_bv bv
  in
  let len = List.fold_left (fun acc s -> acc + (length_of_composed_bitvector s)) 0 list in
  let arr = Array.make len pzero in
  let _ = List.fold_left (write_piece arr) 0 list in
  arr

let create_custom_named name list =
{ 
  name = name; 
  bits = create_custom list;
  real = true;
}

let create_custom_anon list = 
{ 
  name = make_anon_name ();
  bits = create_custom list;
  real = false;
}

(* Clean up later *)
let mapi_core f bv1 =
  let l1 = size_of_bv bv1 in
  let obv = Array.make l1 pzero in
  let rec aux i =
    if i = l1
    then obv
    else 
     (obv.(i) <- f i (bit bv1 i);
      aux (i+1))
  in aux 0

let map_core f bv1 = mapi_core (fun _ x -> f x) bv1

let map_anon f bv1 = 
{
  name = make_anon_name ();
  bits = map_core f bv1;
  real = false;
}
      
let map_named name f bv1 = 
{
  name = name;
  bits = map_core f bv1;
  real = true;
}

let mapi_anon f bv1 = 
{
  name = make_anon_name ();
  bits = mapi_core f bv1;
  real = false;
}
      
let map2_core f bv1 bv2 =
  let l1,l2 = size_of_bv bv1,size_of_bv bv2 in
  if l1<>l2 
  then invalid_arg (Printf.sprintf "BitVector.map2_core: size %d != %d" l1 l2)
  else
    let obv = Array.make l1 pzero in
    let rec aux i =
      if i = l1
      then obv
      else 
       (obv.(i) <- f (bit bv1 i) (bit bv2 i);
        aux (i+1))
    in aux 0

let map2_anon f bv1 bv2 = 
{
  name = make_anon_name ();
  bits = map2_core f bv1 bv2;
  real = false;
}
      
let map2_named name f bv1 bv2 = 
{
  name = name;
  bits = map2_core f bv1 bv2;
  real = true;
}

let iteri f bv =
  let len = size_of_bv bv in
  let rec aux i = 
    if i = len
    then ()
    else (f (bit bv i) i; aux (i+1))
  in aux 0
  
let fold_left f acc bv =
  let a = ref acc in
  iteri (fun b _ -> a := f (!a) b) bv;
  !a

let fold_left2 f acc bv1 bv2 =
  let l1,l2 = size_of_bv bv1,size_of_bv bv2 in
  if l1 != l2
  then invalid_arg (Printf.sprintf "BitVector.fold_left2: size %d != %d" l1 l2)
  else
    let rec aux acc i =
      if i = l1
      then acc
      else aux (f acc (bit bv1 i) (bit bv2 i)) (i+1)
    in aux acc 0
