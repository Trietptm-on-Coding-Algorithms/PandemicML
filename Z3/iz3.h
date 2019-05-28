/* Copyright 2011 Microsoft Research. */

#ifndef IZ3_H
#define IZ3_H

#ifdef __cplusplus
extern "C" {
#endif
#include "z3.h"
#ifdef __cplusplus
}
#endif

#ifdef WIN32
#define IZ3_EXPORT __declspec(dllexport)
#else
#define IZ3_EXPORT __attribute__ ((visibility ("default")))
#endif


/*! \defgroup iz3 iZ3 C and C++ API
    \brief Programming interface for iZ3

    There functions provide an interface to iZ3, allowing the
    generation of interpolants from proofs generated by Z3.

    \{

 */

#ifdef __cplusplus
extern "C" {
#endif

/** This function generates a Z3 context suitable for generation of
   interpolants. Formulas can be generated as abstract syntx trees in
   this context using the Z3 C API.

   Interpolants are also generated as AST's in this context.

   If cfg is non-null, it will be used as the base configuration
   for the Z3 context. This makes it possible to set Z3 options
   to be used during interpolation. This feature should be used
   with some caution however, as it may be that certain Z3 options
   are incompatible with interpolation. 
 */

  IZ3_EXPORT Z3_context Z3_mk_interpolation_context(Z3_config cfg);


  struct Z3_interpolation_options_struct;

  /** An object containing options for interpolation. */
  typedef Z3_interpolation_options_struct *Z3_interpolation_options;

  /** Create an empty interpolation options object. */

  IZ3_EXPORT 
  Z3_interpolation_options
  Z3_mk_interpolation_options();

  /** Dispose of an interpolation options object.

      \param opts The object to dispose of 
  */
  IZ3_EXPORT
  void
  Z3_del_interpolation_options(Z3_interpolation_options opts);

  /** Set an option in an interpolation options object.

      \param opts The interpolation options object  
      \param name The name of the option to set
      \param value The value of the option

      Currently available options:

      "weak": set to "1" to compute weak interpolants, default is strong

  */
  IZ3_EXPORT
  void
  Z3_set_interpolation_option(Z3_interpolation_options opts, 
			      Z3_string name,
			      Z3_string value);


/** Constant reprepresenting a root of a formula tree for tree interpolation */
#define IZ3_ROOT SHRT_MAX

/** This function uses Z3 to determine satisfiability of a set of
    constraints. If UNSAT, an interpolant is returned, based on the
    refutation generated by Z3. If SAT, a model is returned.

    If "parents" is non-null, computes a tree interpolant. The tree is
    defined by the array "parents".  This array maps each formula in
    the tree to its parent, where formulas are indicated by their
    integer index in "cnsts". The parent of formula n must have index
    greater than n. The last formula is the root of the tree. Its
    parent entry should be the constant IZ3_ROOT.

    If "parents" is null, computes a sequence interpolant. 

    \param ctx The Z3 context. Must be generated by iz3_mk_context
    \param num The number of constraints in the sequence
    \param cnsts Array of constraints (AST's in context ctx)
    \param parents The parents vector defining the tree structure
    \param options Interpolation options (may be NULL)
    \param interps Array to return interpolants (size at least num-1, may be NULL)
    \param model Returns a Z3 model if constraints SAT (may be NULL)
    \param labels Returns relevant labels if SAT (may be NULL)
    \param incremental 

    VERY IMPORTANT: All the Z3 formulas in cnsts must be in Z3
    context ctx. The model and interpolants returned are also
    in this context. 

    The return code is as in Z3_check_assumptions, that is,

    Z3_L_FALSE = constraints UNSAT (interpolants returned)
    Z3_L_TRUE = constraints SAT (model returned)
    Z3_L_UNDEF = Z3 produced no result, or interpolation not possible

    Currently, this function supports integer and boolean variables,
    as well as arrays over these types, with linear arithmetic,
    uninterpreted functions and quantifiers over integers (that is
    AUFLIA). Interpolants are produced in AUFLIA. However, some
    uses of array operations may cause quantifiers to appear in the
    interpolants even when there are no quantifiers in the input formulas.
    Although quantifiers may appear in the input formulas, Z3 may give up in
    this case, returning Z3_L_UNDEF.

    If "incremental" is true, cnsts must contain exactly the set of
    formulas that are currently asserted in the context. If false,
    there must be no formulas currently asserted in the context.
    Setting "incremental" to true makes it posisble to incrementally
    add and remove constraints from the context until the context
    becomes UNSAT, at which point an interpolant is computed. Caution
    must be used, however. Before popping the context, if you wish to
    keep the interolant formulas, you *must* preserve them by using
    Z3_persist_ast. Also, if you want to simplify the interpolant
    formulas using Z3_simplify, you must first pop all of the
    assertions in the context (or use a different context). Otherwise,
    the formulas will be simplified *relative* to these constraints,
    which is almost certainly not what you want.
    

    Current limitations on tree interpolants. In a tree interpolation
    problem, each constant (0-ary function symbol) must occur only
    along one path from root to leaf. Function symbols (of arity > 0)
    are considered to have global scope (i.e., may appear in any
    interpolant formula). 

*/ 


  IZ3_EXPORT Z3_lbool Z3_interpolate(Z3_context ctx, 
				      int num,
				      Z3_ast *cnsts,
				      int *parents,
				      Z3_interpolation_options options,
				      Z3_ast *interps,
				      Z3_model *model = 0,
				      Z3_literals *labels = 0,
				      bool incremental = false);

  /** Return a string summarizing cumulative time used for
      interpolation.  This string is purely for entertainment purposes
      and has no semantics.
      
      \param ctx The context (currently ignored)
  */

  IZ3_EXPORT Z3_string Z3_interpolation_profile(Z3_context ctx);
  
  /** Check the correctness of an interpolant. The Z3 context must
      have no constraints asserted when this call is made. That means
      that after interpolating, you must first fully pop the Z3
      context before calling this. See Z3_interpolate for meaning of parameters.

      \param ctx The Z3 context. Must be generated by Z3_mk_interpolation_context
      \param num The number of constraints in the sequence
      \param cnsts Array of constraints (AST's in context ctx)
      \param parents The parents vector (or NULL for sequence)
      \param interps The interpolant to check
      \param error Returns an error message if interpolant incorrect (do not free the string)

      Return value is Z3_L_TRUE if interpolant is verified, Z3_L_FALSE if
      incorrect, and Z3_L_UNDEF if unknown.

  */
  
  IZ3_EXPORT bool Z3_check_interpolant(Z3_context ctx, int num, Z3_ast *cnsts, int *parents, Z3_ast *interps, const char **error);


  /** Write an interpolation problem to file suitable for reading with
      Z3_read_interpolation_problem.

      \param ctx The Z3 context. Must be generated by z3_mk_interpolation_context
      \param num The number of constraints in the sequence
      \param cnsts Array of constraints
      \param parents The parents vector (or NULL for sequence)
      \param filename The file name to write

      See Z3_read_interpolation_problem for a discussion of file
      formats.

  */

  IZ3_EXPORT
  void
  Z3_write_interpolation_problem(Z3_context ctx,
				 int num, 
				 Z3_ast *cnsts, 
				 int *parents,
				 const char *filename);


  /**
     Read an interpolation problem from file.

     \param ctx The Z3 context. This resets the error handler of ctx.
     \param filename The file name to read.
     \param num Returns length of sequence.
     \param cnsts Returns sequence of formulas (do not free)
     \param parents Returns the parents vector (or NULL for sequence)
     \param error Returns an error message in case of failure (do not free the string)

     Returns true on success. 

     File formats: Currently two formats are supposrted, based on
     SMTLIB 1.2. For sequence interpolants, the sequence of
     constraints is represented by a sequence of "assumption" and
     "formula" entries in the file. The assumptions must precede the
     formulas in the file.

     For tree interpolants, one symbol of type bool is associated to
     each vertex of the tree. For each vertex v there is an assumption or
     formula of the form:

     (implies (and c1 ... cn f) v)

     where c1 .. cn are the children of v (which must precede v in the file)
     and f is the formula assiciated to node v. The last formula in the
     file is the root vertex, and is represented by the predicate "false".

     A solution to a tree interpolation problem can be thought of as a
     valuation of the vertices that makes all the implications true
     where each value is represented using the common symbols between
     the formulas in the subtree and the remainder of the formulas.


  */
  

  IZ3_EXPORT
  bool
  Z3_read_interpolation_problem(Z3_context ctx,
				int *num, 
				Z3_ast **cnsts, 
				int **parents,
				const char *filename,
				const char **error);


#ifdef __cplusplus
}
#endif

/*! \} */

#endif
