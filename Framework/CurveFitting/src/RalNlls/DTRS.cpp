#include "MantidCurveFitting/RalNlls/DTRS.h"
#include "MantidCurveFitting/FortranDefs.h"

#include <algorithm>
#include <limits>
#include <string>

namespace Mantid {
namespace CurveFitting {
namespace RalNlls {

namespace {

typedef double real;
typedef double REAL;
typedef int integer;
typedef int INTEGER;
typedef bool logical;
typedef bool LOGICAL;

double smallest(double a, double b, double c, double d) {
  return std::min( std::min(a, b), std::min(c, d));
}

double smallest(double a, double b, double c) {
  return std::min( std::min(a, b), c);
}

double biggest(double a, double b, double c, double d) {
  return std::max( std::max(a, b), std::max(c, d));
}

double biggest(double a, double b, double c) {
  return std::max( std::max(a, b), c);
}

double maxAbsVal(const DoubleFortranVector& v) {
  auto p = v.indicesOfMinMaxElements();
  return std::max(fabs(v.get(p.first)), fabs(v.get(p.second)));
}

double minAbsVal(const DoubleFortranVector& v) {
  auto p = v.indicesOfMinMaxElements();
  return std::min(fabs(v[p.first]), fabs(v[p.second]));
}

//double maxVal(const DoubleFortranVector& v) {
//  auto i = v.indicesOfMinMaxElements();
//  return std::max(fabs(v[p.first]), fabs(v[p.second]));
//}
//
//double minVal(const DoubleFortranVector& v) {
//  auto p = v.indicesOfMinMaxElements();
//  return std::min(fabs(v[p.first]), fabs(v[p.second]));
//}

const double HUGE = std::numeric_limits<double>::max();

const REAL zero = 0.0;
const REAL one = 1.0;
const REAL two = 2.0;
const REAL three = 3.0;
const REAL four = 4.0;
const REAL six = 6.0;
const REAL quarter = 0.25;
const REAL threequarters = 0.75;
const REAL onesixth = one / six;
const REAL onethird = one / three;
const REAL half = 0.5;
const REAL twothirds = two / three;
//!     REAL pi = four * ATAN( 1.0 );
const REAL pi = 3.1415926535897931;
//!     REAL magic = twothirds * pi
const REAL magic = 2.0943951023931953;  //!! 2 pi/3
const REAL epsmch = std::numeric_limits<double>::epsilon();
const REAL infinity = HUGE;
const REAL largest = HUGE;
const REAL lower_default = - half * largest;
const REAL upper_default = largest;

} // anonymous namespace

enum class ErrorCode {
      ral_nlls_ok                      = 0,
      ral_nlls_error_allocate          = - 1,
      ral_nlls_error_deallocate        = - 2,
      ral_nlls_error_restrictions      = - 3,
      ral_nlls_error_bad_bounds        = - 4,
      ral_nlls_error_primal_infeasible = - 5,
      ral_nlls_error_dual_infeasible   = - 6,
      ral_nlls_error_unbounded         = - 7,
      ral_nlls_error_no_center         = - 8,
      ral_nlls_error_analysis          = - 9,
      ral_nlls_error_factorization     = - 10,
      ral_nlls_error_solve             = - 11,
      ral_nlls_error_uls_analysis      = - 12,
      ral_nlls_error_uls_factorization = - 13,
      ral_nlls_error_uls_solve         = - 14,
      ral_nlls_error_preconditioner    = - 15,
      ral_nlls_error_ill_conditioned   = - 16,
      ral_nlls_error_tiny_step         = - 17,
      ral_nlls_error_max_iterations    = - 18,
      ral_nlls_error_time_limit        = - 19,
      ral_nlls_error_cpu_limit         =   ral_nlls_error_time_limit,
      ral_nlls_error_inertia           = - 20,
      ral_nlls_error_file              = - 21,
      ral_nlls_error_io                = - 22,
      ral_nlls_error_upper_entry       = - 23,
      ral_nlls_error_sort              = - 24,
      ral_nlls_error_input_status      = - 25,
      ral_nlls_error_unknown_solver    = - 26,
      ral_nlls_not_yet_implemented     = - 27,
      ral_nlls_error_qp_solve          = - 28,
      ral_nlls_unavailable_option      = - 29,
      ral_nlls_warning_on_boundary     = - 30,
      ral_nlls_error_call_order        = - 31,
      ral_nlls_error_integer_ws        = - 32,
      ral_nlls_error_real_ws           = - 33,
      ral_nlls_error_pardiso           = - 34,
      ral_nlls_error_wsmp              = - 35,
      ral_nlls_error_mc64              = - 36,
      ral_nlls_error_mc77              = - 37,
      ral_nlls_error_lapack            = - 38,
      ral_nlls_error_permutation       = - 39,
      ral_nlls_error_alter_diagonal    = - 40,
      ral_nlls_error_access_pivots     = - 41,
      ral_nlls_error_access_pert       = - 42,
      ral_nlls_error_direct_access     = - 43,
      ral_nlls_error_f_min             = - 44,
      ral_nlls_error_unknown_precond   = - 45,
      ral_nlls_error_schur_complement  = - 46,
      ral_nlls_error_technical         = - 50,
      ral_nlls_error_reformat          = - 52,
      ral_nlls_error_ah_unordered      = - 53,
      ral_nlls_error_y_unallocated     = - 54,
      ral_nlls_error_z_unallocated     = - 55,
      ral_nlls_error_scale             = - 61,
      ral_nlls_error_presolve          = - 62,
      ral_nlls_error_qpa               = - 63,
      ral_nlls_error_qpb               = - 64,
      ral_nlls_error_qpc               = - 65,
      ral_nlls_error_cqp               = - 66,
      ral_nlls_error_dqp               = - 67,
      ral_nlls_error_mc61              = - 69,
      ral_nlls_error_mc68              = - 70,
      ral_nlls_error_metis             = - 71,
      ral_nlls_error_spral             = - 72,
      ral_nlls_warning_repeated_entry  = - 73,
      ral_nlls_error_rif               = - 74,
      ral_nlls_error_ls28              = - 75,
      ral_nlls_error_ls29              = - 76,
      ral_nlls_error_cutest            = - 77,
      ral_nlls_error_evaluation        = - 78,
      ral_nlls_error_optional          = - 79,
      ral_nlls_error_mi35              = - 80,
      ral_nlls_error_spqr              = - 81,
      ral_nlls_error_alive             = - 82,
      ral_nlls_error_ccqp              = - 83
};


//!--------------------------
//!  Derived type definitions
//!--------------------------

//!  - - - - - - - - - - - - - - - - - - - - - - -
//!   control derived type with component defaults
//!  - - - - - - - - - - - - - - - - - - - - - - -
struct  dtrs_control_type {
//
//!  unit for error messages
// INTEGER  error = 6;

//!  unit for monitor output
// INTEGER  out = 6

//!  unit to write problem data into file problem_file
//
//        INTEGER  problem = 0
//
//!  controls level of diagnostic output
INTEGER  print_level = 0;

//!  maximum degree of Taylor approximant allowed
INTEGER  taylor_max_degree = 3;

//!  any entry of H that is smaller than h_min * MAXVAL( H ) we be treated as zero
REAL   h_min = epsmch;

//!  any entry of C that is smaller than c_min * MAXVAL( C ) we be treated as zero
REAL   c_min = epsmch;

//!  lower and upper bounds on the multiplier, if known
REAL   lower = lower_default;
REAL   upper = upper_default;

//!  stop when | ||x|| - radius | <=
//!     max( stop_normal * radius, stop_absolute_normal )
REAL   stop_normal = epsmch;
REAL   stop_absolute_normal = epsmch;

//!  is the solution is REQUIRED to lie on the boundary (i.e., is the constraint
//!  an equality)?
LOGICAL  equality_problem= false;

//!  name of file into which to write problem data
//
//        CHARACTER ( LEN = 30 )  problem_file =                               &
//         'trs_problem.data' // REPEAT( ' ', 14 )
//
//!  all output lines will be prefixed by
//!    prefix(2:LEN(TRIM(%prefix))-1)
//!  where prefix contains the required string enclosed in quotes,
//!  e.g. "string" or 'string'
//
//        CHARACTER ( LEN = 30 )  prefix  = '""                            '
//
};

//!  - - - - - - - - - - - - - - - - - - - - - - - -
//!   history derived type with component defaults
//!  - - - - - - - - - - - - - - - - - - - - - - - -
struct dtrs_history_type {
//
//!  value of lambda
REAL lambda = zero;

//!  corresponding value of ||x(lambda)||_M
REAL x_norm = zero;
};

//!  - - - - - - - - - - - - - - - - - - - - - - -
//!   inform derived type with component defaults
//!  - - - - - - - - - - - - - - - - - - - - - - -
//
struct dtrs_inform_type {
//
//!   reported return status:
//!      0 the solution has been found
//!     -3 n and/or Delta is not positive
//!    -16 ill-conditioning has prevented furthr progress
ErrorCode  status = ErrorCode::ral_nlls_ok;

//!  the number of (||x||_M,lambda) pairs in the history
INTEGER  len_history = 0;

//!  the value of the quadratic function
REAL obj = HUGE;

//!  the M-norm of x, ||x||_M
REAL   x_norm = zero;

//!  the Lagrange multiplier corresponding to the trust-region constraint
REAL   multiplier = zero;

//!  a lower bound max(0,-lambda_1), where lambda_1 is the left-most
//!  eigenvalue of (H,M)
REAL   pole = zero;

//!  has the hard case occurred?
LOGICAL  hard_case = false;

//!  history information
std::vector<dtrs_history_type> history;
};


void dtrs_solve_main( int n, double radius, double f, const DoubleFortranVector& c, 
  const DoubleFortranVector& h, DoubleFortranVector& x, const dtrs_control_type& control, dtrs_inform_type& inform );


//! Contains  ral_nlls_roots
//!             ral_dtrs
//! THIS VERSION: RAL_NLLS 1.1 - 07/03/2016 AT 09:45 GMT.
//
//!-*-*-*-*-*-*-*-*-*-  R A L _ N L L S _ R O O T S   M O D U L E  -*-*-*-*-*-*-*-
//
//!  Copyright reserved, Gould/Orban/Toint, for RAL_NLLS productions
//!  Principal author: Nick Gould
//
//!  History -
//!   extracted from GALAHAD package ROOTS, March 7th, 2016
//
//!  For full documentation, see
//!   http://galahad.rl.ac.uk/galahad-www/specs.html
//
//   MODULE RAL_NLLS_ROOTS_double
//
//!     --------------------------------------------------------------------
//!     |                                                                  |
//!     |  Find (all the) real roots of polynomials with real coefficients |
//!     |                                                                  |
//!     --------------------------------------------------------------------
//
//      USE RAL_NLLS_SYMBOLS
//
//      IMPLICIT NONE
//
//      PRIVATE
//      PUBLIC  ROOTS_quadratic, ROOTS_cubic, ROOTS_quartic
//
//!--------------------
//!   P r e c i s i o n
//!--------------------
//
//      INTEGER, PARAMETER  wp = KIND( 1.0D+0 )
//
//!----------------------
//!   P a r a m e t e r s
//!----------------------
//
//      INTEGER, PARAMETER  out = 6

//!  interface to LAPACK: eigenvalues of a Hessenberg matrix
//
//      INTERFACE HSEQR
//        SUBROUTINE SHSEQR( job, compz, n, ilo, ihi, H, ldh,  WR, WI, Z, ldz,   &
//                           WORK, lwork, info )
//        INTEGER, INTENT( IN )  ihi, ilo, ldh, ldz, lwork, n
//        INTEGER, INTENT( OUT )  info
//        CHARACTER ( LEN = 1 ), INTENT( IN )  compz, job
//        REAL, INTENT( INOUT )  H( ldh, * ), Z( ldz, * )
//        REAL, INTENT( OUT )  WI( * ), WR( * ), WORK( * )
//        END SUBROUTINE SHSEQR
//
//        SUBROUTINE DHSEQR( job, compz, n, ilo, ihi, H, ldh,  WR, WI, Z, ldz,   &
//                           WORK, lwork, info )
//        INTEGER, INTENT( IN )  ihi, ilo, ldh, ldz, lwork, n
//        INTEGER, INTENT( OUT )  info
//        CHARACTER ( LEN = 1 ), INTENT( IN )  compz, job
//        DOUBLE PRECISION, INTENT( INOUT )  H( ldh, * ), Z( ldz, * )
//        DOUBLE PRECISION, INTENT( OUT )  WI( * ), WR( * ), WORK( * )
//        END SUBROUTINE DHSEQR
//      END INTERFACE HSEQR
//
//   CONTAINS

///-*-*-*-*-*-   R O O T S _ q u a d r a t i c  S U B R O U T I N E   -*-*-*-*-*-
/// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
///
///  Find the number and values of real roots of the quadratic equation
///
///                   a2 * x**2 + a1 * x + a0 = 0
///
///  where a0, a1 and a2 are real
///
/// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void roots_quadratic(double a0, double a1, double a2, double tol, int &nroots, 
                     double& root1, double &root2, bool debug ) {

//!  Dummy arguments
//
//      INTEGER, INTENT( OUT )  nroots
//      REAL , INTENT( IN )  a2, a1, a0, tol
//      REAL , INTENT( OUT )  root1, root2
//      LOGICAL, INTENT( IN )  debug

  UNUSED_ARG(debug);
  auto rhs = tol * a1 * a1;
if (std::fabs(a0 * a2) > rhs) { // !  really is quadratic
  root2 = a1 * a1 - four * a2 * a0;
  if (abs(root2) <= pow(epsmch * a1, 2)) { // ! numerical double root
    nroots = 2;
    root1 = -half * a1 / a2;
    root2 = root1;
  } else if (root2 < zero) { // ! complex not real roots
    nroots = 0;
    root1 = zero;
    root2 = zero;
  } else { // ! distint real roots
    auto d = -half * (a1 + sign(sqrt(root2), a1));
    nroots = 2;
    root1 = d / a2;
    root2 = a0 / d;
    if (root1 > root2) {
      d = root1;
      root1 = root2;
      root2 = d;
    }
  }
} else if (a2 == zero) {
  if (a1 == zero) {
    if (a0 == zero) { //! the function is zero
      nroots = 1;
      root1 = zero;
      root2 = zero;
    } else { //! the function is constant
      nroots = 0;
      root1 = zero;
      root2 = zero;
    }
  } else { //! the function is linear
    nroots = 1;
    root1 = -a0 / a1;
    root2 = zero;
  }
} else { // ! very ill-conditioned quadratic
  nroots = 2;
  if (-a1 / a2 > zero) {
    root1 = zero;
    root2 = -a1 / a2;
  } else {
    root1 = -a1 / a2;
    root2 = zero;
  }
}

//!  perfom a Newton iteration to ensure that the roots are accurate

if (nroots >= 1) {
  auto p = (a2 * root1 + a1) * root1 + a0;
  auto pprime = two * a2 * root1 + a1;
  if (pprime != zero) {
    // if ( debug ) write( out, 2000 ) 1, root1, p, - p / pprime
    root1 = root1 - p / pprime;
    p = (a2 * root1 + a1) * root1 + a0;
  }
  // if ( debug ) write( out, 2010 ) 1, root1, p
  if (nroots == 2) {
    p = (a2 * root2 + a1) * root2 + a0;
    pprime = two * a2 * root2 + a1;
    if (pprime != zero) {
      // if ( debug ) write( out, 2000 ) 2, root2, p, - p / pprime
      root2 = root2 - p / pprime;
      p = (a2 * root2 + a1) * root2 + a0;
    }
    // if ( debug ) write( out, 2010 ) 2, root2, p
  }
}
//!  Non-executable statements
//
// 2000 FORMAT( ' root ', I1, ': value = ', ES12.4, ' quadratic = ', ES12.4, &
//              ' delta = ', ES12.4 )
// 2010 FORMAT( ' root ', I1, ': value = ', ES12.4, ' quadratic = ', ES12.4 )
//
//
} //!  End of subroutine ROOTS_quadratic

///-*-*-*-*-*-*-*-   R O O T S _ c u b i c  S U B R O U T I N E   -*-*-*-*-*-*-*-
/// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
///
///  Find the number and values of real roots of the cubic equation
///
///                a3 * x**3 + a2 * x**2 + a1 * x + a0 = 0
///
///  where a0, a1, a2 and a3 are real
///
/// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void roots_cubic(double a0, double a1, double a2, double a3, double tol,
                 int &nroots, double &root1, double &root2, double &root3,
                 bool debug) {
  //
  //!  Dummy arguments
  //
  //      INTEGER, INTENT( OUT )  nroots
  //      REAL , INTENT( IN )  a3, a2, a1, a0, tol
  //      REAL , INTENT( OUT )  root1, root2, root3
  //      LOGICAL, INTENT( IN )  debug
  UNUSED_ARG(debug);

  //!  define method used:
  //!    1 = Nonweiler, 2 = Littlewood, 3 = Viete, other = companion matrix
  //
  // const INTEGER method = 1;

  //!  Check to see if the cubic is actually a quadratic
  if (a3 == zero) {
    roots_quadratic(a0, a1, a2, tol, nroots, root1, root2, debug);
    root3 = infinity;
    return;
  }

  //!  Deflate the polnomial if the trailing coefficient is zero
  if (a0 == zero) {
    root1 = zero;
    roots_quadratic(a1, a2, a3, tol, nroots, root2, root3, debug);
    nroots = nroots + 1;
    return;
  }

  //!  1. Use Nonweiler's method (CACM 11:4, 1968, pp269)
  //      if ( method == 1 ) then

  double c0 = a0 / a3;
  double c1 = a1 / a3;
  double c2 = a2 / a3;

  double s = c2 / three;
  double t = s * c2;
  double b = 0.5 * (s * (twothirds * t - c1) + c0);
  t = (t - c1) / three;
  double c = t * t * t;
  double d = b * b - c;

  //! 1 real + 2 equal real or 2 complex roots
  if (d >= zero) {
    d = pow(sqrt(d) + fabs(b), onethird);
    if (d != zero) {
      if (b > zero) {
        b = -d;
      } else {
        b = d;
      }
      c = t / b;
    }
    d = sqrt(threequarters) * (b - c);
    b = b + c;
    c = -0.5 * b - s;
    root1 = b - s;
    if (d == zero) {
      nroots = 3;
      root2 = c;
      root3 = c;
    } else {
      nroots = 1;
    }
  } else { //! 3 real roots
    if (b == zero) {
      d = twothirds * atan(one);
    } else {
      d = atan(sqrt(-d) / fabs(b)) / three;
    }
    if (b < zero) {
      b = two * sqrt(t);
    } else {
      b = -two * sqrt(t);
    }
    c = cos(d) * b;
    t = -sqrt(threequarters) * sin(d) * b - half * c;
    d = -t - c - s;
    c = c - s;
    t = t - s;
    if (abs(c) > abs(t)) {
      root3 = c;
    } else {
      root3 = t;
      t = c;
    }
    if (abs(d) > abs(t)) {
      root2 = d;
    } else {
      root2 = t;
      t = d;
    }
    root1 = t;
    nroots = 3;
  }

  //!  reorder the roots

  //  900 CONTINUE
  if (nroots == 3) {
    if (root1 > root2) {
      double a = root2;
      root2 = root1;
      root1 = a;
    }
    if (root2 > root3) {
      double a = root3;
      if (root1 > root3) {
        a = root1;
        root1 = root3;
      }
      root3 = root2;
      root2 = a;
    }
    //        if ( debug ) write( out, "( ' 3 real roots ' )" )
    //      else if ( nroots == 2 ) then
    //        if ( debug ) write( out, "( ' 2 real roots ' )" )
    //      else
    //        if ( debug ) write( out, "( ' 1 real root ' )" )
  }

  //!  perfom a Newton iteration to ensure that the roots are accurate

  double p = ((a3 * root1 + a2) * root1 + a1) * root1 + a0;
  double pprime = (three * a3 * root1 + two * a2) * root1 + a1;
  if (pprime != zero) {
    //        if ( debug ) write( out, 2000 ) 1, root1, p, - p / pprime
    root1 = root1 - p / pprime;
    p = ((a3 * root1 + a2) * root1 + a1) * root1 + a0;
  }
  //      if ( debug ) write( out, 2010 ) 1, root1, p

  if (nroots == 3) {
    p = ((a3 * root2 + a2) * root2 + a1) * root2 + a0;
    pprime = (three * a3 * root2 + two * a2) * root2 + a1;
    if (pprime != zero) {
      // if ( debug ) write( out, 2000 ) 2, root2, p, - p / pprime
      root2 = root2 - p / pprime;
      p = ((a3 * root2 + a2) * root2 + a1) * root2 + a0;
    }
    //        if ( debug ) write( out, 2010 ) 2, root2, p

    p = ((a3 * root3 + a2) * root3 + a1) * root3 + a0;
    pprime = (three * a3 * root3 + two * a2) * root3 + a1;
    if (pprime != zero) {
      // if ( debug ) write( out, 2000 ) 3, root3, p, - p / pprime
      root3 = root3 - p / pprime;
      p = ((a3 * root3 + a2) * root3 + a1) * root3 + a0;
    }
    //        if ( debug ) write( out, 2010 ) 3, root3, p
  }

  //!  Non-executable statements
  //
  // 2000 FORMAT( ' root ', I1, ': value = ', ES12.4, ' cubic = ', ES12.4, &
  //              ' delta = ', ES12.4 )
  // 2010 FORMAT( ' root ', I1, ': value = ', ES12.4, ' cubic = ', ES12.4 )
  //
  //
  //!  End of subroutine ROOTS_cubic
  //
}

///-*-*-*-*-*-*-   R O O T S _ q u a r t i c   S U B R O U T I N E   -*-*-*-*-*-*-
///
/// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
///
///  Find the number and values of real roots of the quartic equation
///
///        a4 * x**4 + a3 * x**3 + a2 * x**2 + a1 * x + a0 = 0
///
///  where a0, a1, a2, a3 and a4 are real
///
/// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void roots_quartic(double a0, double a1, double a2, double a3, double a4, double tol, int &nroots, 
                     double& root1, double &root2, double &root3, double &root4, bool debug) {
//
//!  Dummy arguments
//
//      INTEGER, INTENT( OUT )  nroots
//      REAL , INTENT( IN )  a4, a3, a2, a1, a0, tol
//      REAL , INTENT( OUT )  root1, root2, root3, root4
//      LOGICAL, INTENT( IN )  debug

//!  Check to see if the quartic is actually a cubic

      if ( a4 == zero ) {
        roots_cubic( a0, a1, a2, a3, tol, nroots, root1, root2, root3, debug );
        root4 = infinity;
        return;
      }

//!  Use Ferrari's algorithm

//!  Initialize

      nroots = 0;
      double b1 = a3 / a4;
      double b2 = a2 / a4;
      double b3 = a1 / a4;
      double b4 = a0 / a4;
      double d3 = one;
      double d2 =  - b2;
      double d1 = b1 * b3 - four * b4;
      double d0 = b4 * ( four * b2 - b1 * b1 ) - b3 * b3;

//!  Compute the roots of the auxiliary cubic
      int nrootsc;
      double rootc1, rootc2, rootc3;
      roots_cubic( d0, d1, d2, d3, tol, nrootsc, rootc1, rootc2, rootc3, debug );
      if ( nrootsc > 1 ) {rootc1 = rootc3;}
      double x1 = b1 * b1 * quarter - b2 + rootc1;
      if ( x1 < zero ) {
        auto xmd = sqrt( - x1 );
        auto xnd = quarter * ( two * b3 - b1 * rootc1 ) / xmd;
        auto alpha = half * b1 * b1 - rootc1 - b2;
        auto beta = four * xnd - b1 * xmd;
        auto r = sqrt( alpha * alpha + beta * beta );
        auto gamma = sqrt( half * ( alpha + r ) );
        double delta = 0.0;
        if ( gamma == zero ) {
          delta = sqrt( - alpha );
        }else{
          delta = beta * half / gamma;
       }
        root1 = half * ( - half * b1 + gamma );
        root2 = half * ( xmd + delta );
        root3 = half * ( - half * b1 - gamma );
        root4 = half * ( xmd - delta );
        goto Label900;
      }
      double xm = 0.0;
      double xn = 0.0;
      if ( x1 != zero ) {
        xm = sqrt( x1 );
        xn = quarter * ( b1 * rootc1 - two * b3 ) / xm;
      }else{
        xm = zero;
        xn = sqrt( quarter * rootc1 * rootc1 - b4 );
      }
      auto alpha = half * b1 * b1 - rootc1 - b2;
      auto beta = four * xn - b1 * xm;
      auto gamma = alpha + beta;
      auto delta = alpha - beta;
      auto a = - half * b1;

//!  compute how many real roots there are

      int type_roots = 1;
      if ( gamma >= zero ) {
        nroots = nroots + 2;
        type_roots = 0;
        gamma = sqrt( gamma );
      }else{
        gamma = sqrt( - gamma );
      }
      if ( delta >= zero ) {
        nroots = nroots + 2;
        delta = sqrt( delta );
      }else{
        delta = sqrt( - delta );
      }
      type_roots = nroots + type_roots;

//!  two real roots

      if ( type_roots == 3 ) {
        root1 = half * ( a - xm - delta );
        root2 = half * ( a - xm + delta );
        root3 = half * ( a + xm );
        root4 = half * gamma;
        goto Label900;
      }else if ( type_roots != 4 ) {
        if ( type_roots == 2 ) {
          root1 = half * ( a + xm - gamma );
          root2 = half * ( a + xm + gamma );
        }else{
          //!  no real roots
          root1 = half * ( a + xm );
          root2 = half * gamma;
        }
        root3 = half * ( a - xm ) * half;
        root4 = half * delta;
        goto Label900;
      }

//!  four real roots

      auto b = half * ( a + xm + gamma );
      auto d = half * ( a - xm + delta );
      auto c = half * ( a - xm - delta );
      a = half * ( a + xm - gamma );

//!  sort the roots

      root1 = smallest( a, b, c, d );
      root4 = biggest( a, b, c, d );

      if ( a == root1 ) {
        root2 = smallest( b, c, d );
      }else if ( b == root1 ) {
        root2 = smallest( a, c, d );
      }else if ( c == root1 ) {
        root2 = smallest( a, b, d );
      }else{
        root2 = smallest( a, b, c );
      }

      if ( a == root4 ) {
        root3 = biggest( b, c, d );
      }else if ( b == root4 ) {
        root3 = biggest( a, c, d );
      }else if ( c == root4 ) {
        root3 = biggest( a, b, d );
      }else{
        root3 = biggest( a, b, c );
      }

Label900: // Oops, a label :(

//!  Perfom a Newton iteration to ensure that the roots are accurate
//
//      if ( debug ) {
//        if ( nroots == 0 ) {
//          write( out, "( ' no real roots ' )" )
//        }else if ( nroots == 2 ) {
//          write( out, "( ' 2 real roots ' )" )
//        }else if ( nroots == 4 ) {
//          write( out, "( ' 4 real roots ' )" )
//        }
//      }
      if ( nroots == 0 ) return;

      auto p = ( ( ( a4 * root1 + a3 ) * root1 + a2 ) * root1 + a1 ) * root1 + a0;
      auto pprime = ( ( four * a4 * root1 + three * a3 ) * root1 + two * a2 ) * root1 + a1;
      if ( pprime != zero ) {
//        if ( debug ) write( out, 2000 ) 1, root1, p, - p / pprime
        root1 = root1 - p / pprime;
        p = ( ( ( a4 * root1 + a3 ) * root1 + a2 ) * root1 + a1 ) * root1 + a0;
      }
//      if ( debug ) write( out, 2010 ) 1, root1, p

      p = ( ( ( a4 * root2 + a3 ) * root2 + a2 ) * root2 + a1 ) * root2 + a0;
      pprime = ( ( four * a4 * root2 + three * a3 ) * root2 + two * a2 ) * root2 + a1;
      if ( pprime != zero ) {
//        if ( debug ) write( out, 2000 ) 2, root2, p, - p / pprime
        root2 = root2 - p / pprime;
        p = ( ( ( a4 * root2 + a3 ) * root2 + a2 ) * root2 + a1 ) * root2 + a0;
      }
//      if ( debug ) write( out, 2010 ) 2, root2, p

      if ( nroots == 4 ) {
        p = ( ( ( a4 * root3 + a3 ) * root3 + a2 ) * root3 + a1 ) * root3 + a0;
        pprime = ( ( four * a4 * root3 + three * a3 ) * root3 + two * a2 ) * root3 + a1;
        if ( pprime != zero ) {
          //if ( debug ) write( out, 2000 ) 3, root3, p, - p / pprime
          root3 = root3 - p / pprime;
          p = ( ( ( a4 * root3 + a3 ) * root3 + a2 ) * root3 + a1 ) * root3 + a0;
        }
        //if ( debug ) write( out, 2010 ) 3, root3, p

        p = ( ( ( a4 * root4 + a3 ) * root4 + a2 ) * root4 + a1 ) * root4 + a0;
        pprime = ( ( four * a4 * root4 + three * a3 ) * root4 + two * a2 ) * root4 + a1;
        if ( pprime != zero ) {
          //if ( debug ) write( out, 2000 ) 4, root4, p, - p / pprime
          root4 = root4 - p / pprime;
          p = ( ( ( a4 * root4 + a3 ) * root4 + a2 ) * root4 + a1 ) * root4 + a0;
        }
        //if ( debug ) write( out, 2010 ) 4, root4, p
      }

//!  Non-executable statements
//
// 2000 FORMAT( ' root ', I1, ': value = ', ES12.4, ' quartic = ', ES12.4,       &
//              ' delta = ', ES12.4 )
// 2010 FORMAT( ' root ', I1, ': value = ', ES12.4, ' quartic = ', ES12.4 )
//
//!  End of subroutine ROOTS_quartic
//
}

//!  End of module ROOTS
//
//   END MODULE RAL_NLLS_ROOTS_double
//
//! THIS VERSION: RAL_NLLS 1.0 - 22/12/2015 AT 14:15 GMT.
//
//!-*-*-*-*-*-*-*-  R A L _ N L L S _ D T R S  double  M O D U L E  *-*-*-*-*-*-*-
//
//!  Copyright reserved, Gould/Orban/Toint, for GALAHAD productions
//!  Principal author: Nick Gould
//
//!  History -
//!   extracted from GALAHAD package TRS, December 22nd, 2015
//
//   MODULE RAL_NLLS_DTRS_double
//
//!       -----------------------------------------------
//!      |                                               |
//!      | Solve the trust-region subproblem             |
//!      |                                               |
//!      |    minimize     1/2 <x, H x> + <c, x> + f     |
//!      |    subject to      ||x||_2 <= radius          |
//!      |    or              ||x||_2  = radius          |
//!      |                                               |
//!      | where H is diagonal                           |
//!      |                                               |
//!       -----------------------------------------------
//
//      USE RAL_NLLS_SYMBOLS
//      USE RAL_NLLS_ROOTS_double, ONLY: ROOTS_cubic
//
//      IMPLICIT NONE
//
//      PRIVATE
//      PUBLIC  DTRS_initialize, DTRS_solve, DTRS_solve_main
//
//!--------------------
//!   P r e c i s i o n
//!--------------------
//
//      INTEGER, PARAMETER  wp = KIND( 1.0D+0 )
//
//!----------------------
//!   P a r a m e t e r s
//!----------------------
namespace {
//      INTEGER, PARAMETER  history_max = 100
//      INTEGER, PARAMETER  max_degree = 3
//      REAL , PARAMETER  zero = 0.0_wp
//      REAL , PARAMETER  half = 0.5_wp
//      REAL , PARAMETER  point4 = 0.4_wp
//      REAL , PARAMETER  one = 1.0_wp
//      REAL , PARAMETER  two = 2.0_wp
//      REAL , PARAMETER  three = 3.0_wp
//      REAL , PARAMETER  six = 6.0_wp
//      REAL , PARAMETER  sixth = one / six
//      REAL , PARAMETER  ten = 10.0_wp
//      REAL , PARAMETER  twentyfour = 24.0_wp
//      REAL , PARAMETER  epsmch = EPSILON( one )
//      REAL , PARAMETER  teneps = ten * epsmch
//      REAL , PARAMETER  roots_tol = teneps
//      LOGICAL  roots_debug = .FALSE.
const int ral_nlls_ok = 0;
} // anonymous namespace 
//!  interface to BLAS: two norm
//
//     INTERFACE NRM2
//
//       FUNCTION SNRM2( n, X, incx )
//       REAL  SNRM2
//       INTEGER, INTENT( IN )  n, incx
//       REAL, INTENT( IN ), DIMENSION( incx * ( n - 1 ) + 1 )  X
//       END FUNCTION SNRM2
//
//       FUNCTION DNRM2( n, X, incx )
//       DOUBLE PRECISION  DNRM2
//       INTEGER, INTENT( IN )  n, incx
//       DOUBLE PRECISION, INTENT( IN ), DIMENSION( incx * ( n - 1 ) + 1 )  X
//       END FUNCTION DNRM2
//
//     END INTERFACE
//
//    CONTAINS

///-*-*-*-*-*-*-  D T R S _ I N I T I A L I Z E   S U B R O U T I N E   -*-*-*-*-
///
/// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
///
///  .  Set initial values for the TRS control parameters  .
///
///  Arguments:
///  =========
///
///   control  a structure containing control information. See DTRS_control_type
///   inform   a structure containing information. See DRQS_inform_type
///
/// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void dtrs_initialize( dtrs_control_type &control, dtrs_inform_type &inform ) {
//
//!-----------------------------------------------
//!   D u m m y   A r g u m e n t
//!-----------------------------------------------
//
//      TYPE ( DTRS_CONTROL_TYPE ), INTENT( OUT )  control
//      TYPE ( DTRS_inform_type ), INTENT( OUT )  inform

inform.status = ErrorCode::ral_nlls_ok;

//!  Set initial control parameter values
control.stop_normal = pow(epsmch, 0.75);
control.stop_absolute_normal = pow(epsmch, 0.75);

}

///-*-*-*-*-*-*-*-*-  D T R S _ S O L V E   S U B R O U T I N E  -*-*-*-*-*-*-*-
///
/// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
///
///  Solve the trust-region subproblem
///
///      minimize     q(x) = 1/2 <x, H x> + <c, x> + f
///      subject to   ||x||_2 <= radius  or ||x||_2 = radius
///
///  where H is diagonal, using a secular iteration
///
/// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
///
///  Arguments:
///  =========
///
///   n - the number of unknowns
///
///   radius - the trust-region radius
///
///   f - the value of constant term for the quadratic function
///
///   C - a vector of values for the linear term c
///
///   H -  a vector of values for the diagonal matrix H
///
///   X - the required solution vector x
///
///   control - a structure containing control information. See DTRS_control_type
///
///   inform - a structure containing information. See DTRS_inform_type
///
/// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
void dtrs_solve( int n, double radius, double f, const DoubleFortranVector& c, const DoubleFortranVector& h, 
  DoubleFortranVector& x, const dtrs_control_type &control, dtrs_inform_type &inform ) {
//
//!-----------------------------------------------
//!   D u m m y   A r g u m e n t s
//!-----------------------------------------------
//
//      INTEGER, INTENT( IN )  n
//      REAL , INTENT( IN )  radius
//      REAL , INTENT( IN )  f
//      REAL , INTENT( IN ), DIMENSION( n )  C, H
//      REAL , INTENT( OUT ), DIMENSION( n )  X
//      TYPE ( DTRS_control_type ), INTENT( IN )  control
//      TYPE ( DTRS_inform_type ), INTENT( INOUT )  inform
//
//!  local variables
//
//      INTEGER  i
//      REAL   scale_h, scale_c, f_scale, radius_scale
//      REAL , DIMENSION( n )  C_scale, H_scale
//      TYPE ( DTRS_control_type )  control_scale
//
//!  scale the problem to solve instead
//
//!      minimize    q_s(x_s) = 1/2 <x_s, H_s x_s> + <c_s, x_s> + f_s
//!      subject to    ||x_s||_2 <= radius_s  or ||x_s||_2 = radius_s
//
//!  where H_s = H / s_h and c_s = c / s_c for scale factors s_h and s_c
//
//!  This corresponds to
//!    radius_s = ( s_h / s_c ) radius,
//!    f_s = ( s_h / s_c^2 ) f
//!  and the solution may be recovered as
//!    x = ( s_c / s_h ) x_s
//!    lambda = s_h lambda_s
//!    q(x) = ( s_c^2 / s_ h ) q_s(x_s)
//
//!write(6,"( A2, 5ES13.4E3 )" ) 'H', H
//!write(6,"( A2, 5ES13.4E3 )" ) 'C', C
//
//!  scale H by the largest H and remove relatively tiny H
//
      DoubleFortranVector h_scale(n);
      auto scale_h = maxAbsVal(h); // MAXVAL( ABS( H ) )
      if ( scale_h > zero ) {
        for(int i=1; i <= n; ++i) { // do i = 1, n
          if ( fabs( h( i ) ) >= control.h_min * scale_h ) {
            h_scale( i ) = h( i ) / scale_h;
          }else{
            h_scale( i ) = zero;
          }
        }
      }else{
        scale_h = one;
        h_scale = zero;
      }

//!  scale c by the largest c and remove relatively tiny c

      DoubleFortranVector c_scale(n);
      auto scale_c = maxAbsVal(c); // maxval( abs( c ) )
      if ( scale_c > zero ) {
        for(int i=1; i <= n; ++i) { // do i = 1, n
          if ( abs( c( i ) ) >= control.h_min * scale_c ) {
            c_scale( i ) = c( i ) / scale_c;
          }else{
            c_scale( i ) = zero;
          }
        }
      }else{
        scale_c = one;
        c_scale = zero;
      }

      double radius_scale = ( scale_h / scale_c ) * radius;
      double f_scale = ( scale_h / pow(scale_c, 2) ) * f;

      auto control_scale = control;
      if (control_scale.lower != lower_default) {
        control_scale.lower = control_scale.lower / scale_h;
      }
      if ( control_scale.upper != upper_default ) {
        control_scale.upper = control_scale.upper / scale_h;
      }

//!  solve the scaled problem

      dtrs_solve_main(n, radius_scale, f_scale, c_scale, h_scale, x,
                      control_scale, inform);

      //!  unscale the solution, function value, multiplier and related values

      //  x = ( scale_c / scale_h ) * x
      x *= scale_c / scale_h;
      inform.obj *= pow(scale_c, 2) / scale_h;
      inform.multiplier *= scale_h;
      inform.pole *= scale_h;
      for(size_t i = 0; i < inform.history.size(); ++i) { //      do i = 1, inform.len_history
        inform.history[i].lambda *= scale_h;
        inform.history[i].x_norm *= scale_c / scale_h;
      }
//
//!  End of subroutine DTRS_solve
//
}

///-*-*-*-*-*-*-*-*-  D T R S _ S O L V E   S U B R O U T I N E  -*-*-*-*-*-*-*-
///
/// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
///
///  Solve the trust-region subproblem
///
///      minimize     1/2 <x, H x> + <c, x> + f
///      subject to    ||x||_2 <= radius  or ||x||_2 = radius
///
///  where H is diagonal, using a secular iteration
///
/// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
///
///  Arguments:
///  =========
///
///   n - the number of unknowns
///
///   radius - the trust-region radius
///
///   f - the value of constant term for the quadratic function
///
///   C - a vector of values for the linear term c
///
///   H -  a vector of values for the diagonal matrix H
///
///   X - the required solution vector x
///
///   control - a structure containing control information. See DTRS_control_type
///
///   inform - a structure containing information. See DTRS_inform_type
///
/// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
void dtrs_solve_main( int n, double radius, double f, const DoubleFortranVector& c, 
  const DoubleFortranVector& h, DoubleFortranVector& x, const dtrs_control_type& control, dtrs_inform_type& inform ) {
//
//!-----------------------------------------------
//!   D u m m y   A r g u m e n t s
//!-----------------------------------------------
//
//      INTEGER, INTENT( IN )  n
//      REAL , INTENT( IN )  radius
//      REAL , INTENT( IN )  f
//      REAL , INTENT( IN ), DIMENSION( n )  C, H
//      REAL , INTENT( OUT ), DIMENSION( n )  X
//      TYPE ( DTRS_control_type ), INTENT( IN )  control
//      TYPE ( DTRS_inform_type ), INTENT( INOUT )  inform
//
//!-----------------------------------------------
//!   L o c a l   V a r i a b l e s
//!-----------------------------------------------
//
//      INTEGER  i, it, out, nroots, print_level
//      INTEGER  max_order, n_lambda, i_hard
//      REAL   lambda, lambda_l, lambda_u, delta_lambda
//      REAL   alpha, utx, distx
//      REAL   c_norm, c_norm_over_radius, v_norm2, w_norm2
//      REAL   beta, z_norm2, root1, root2, root3
//      REAL   lambda_min, lambda_max, lambda_plus, c2
//      REAL   a_0, a_1, a_2, a_3, a_max
//      REAL , DIMENSION( 3 )  lambda_new
//      REAL , DIMENSION( 0 : max_degree )  x_norm2, pi_beta
//      LOGICAL  printi, printt, printd, problem_file_exists
//      CHARACTER ( LEN = 1 )  region
//
//!  prefix for all output
//
//      CHARACTER ( LEN = LEN( TRIM( control%prefix ) ) - 2 )  prefix
//      IF ( LEN( TRIM( control%prefix ) ) > 2 )                                 &
//        prefix = control%prefix( 2 : LEN( TRIM( control%prefix ) ) - 1 )
//
//!write(6,"( A2, 5ES13.4E3 )" ) 'H', H
//!write(6,"( A2, 5ES13.4E3 )" ) 'C', C
//!write(6,"( A, ES13.4E3 )" ) 'radius', radius
//
//!  output problem data
//
//      IF ( control%problem > 0 ) THEN
//        INQUIRE( FILE = control%problem_file, EXIST = problem_file_exists )
//        IF ( problem_file_exists ) THEN
//          OPEN( control%problem, FILE = control%problem_file,                  &
//                FORM = 'FORMATTED', STATUS = 'OLD' )
//          REWIND control%problem
//        ELSE
//          OPEN( control%problem, FILE = control%problem_file,                  &
//                FORM = 'FORMATTED', STATUS = 'NEW' )
//        END IF
//        WRITE( control%problem, * ) n, COUNT( C( : n ) != zero ),              &
//          COUNT( H( : n ) != zero )
//        WRITE( control%problem, * ) radius, f
//        DO i = 1, n
//          IF ( C( i ) != zero ) WRITE( control%problem, * ) i, C( i )
//        END DO
//        DO i = 1, n
//          IF ( H( i ) != zero ) WRITE( control%problem, * ) i, i, H( i )
//        END DO
//        CLOSE( control%problem )
//      END IF
//
//!  set initial values

      x = zero;
      inform.x_norm = zero;
      inform.obj = f;
      inform.hard_case = false;
      double delta_lambda = zero;

//!  record desired output level
//
//      out = control%out
//      print_level = control%print_level
//      printi = out > 0 .AND. print_level > 0
//      printt = out > 0 .AND. print_level > 1
//      printd = out > 0 .AND. print_level > 2

//!  check for n < 0 or delta < 0
      if ( n < 0 || radius < 0 ) {
         inform.status = ErrorCode::ral_nlls_error_restrictions;
         return;
      }

//!  compute the two-norm of c and the extreme eigenvalues of H

      double c_norm = c.norm2(); // two_norm( c )
      double lambda_min = minAbsVal(h); //  minval( h( : n ) )
      double lambda_max = maxAbsVal(h); // maxval( h( : n ) )
//
//      IF ( printt ) WRITE( out, "( A, ' ||c|| = ', ES10.4, ', ||H|| = ',       &
//     &                             ES10.4, ', lambda_min = ', ES11.4 )" )      &
//          prefix, c_norm, MAXVAL( ABS( H( : n ) ) ), lambda_min
//
//      region = 'L'
//      IF ( printt )                                                            &
//        WRITE( out, "( A, 4X, 28( '-' ), ' phase two ', 28( '-' ) )" ) prefix
//      IF ( printi ) WRITE( out, 2030 ) prefix
//
//!  check for the trivial case
//
//      IF ( c_norm == zero .AND. lambda_min >= zero ) THEN
//        IF (  control%equality_problem ) THEN
//          DO i = 1, n
//            IF ( H( i ) == lambda_min ) THEN
//              i_hard = i
//              EXIT
//            END IF
//          END DO
//          X( i_hard ) = one / radius
//          inform%x_norm = radius
//          inform%obj = f + lambda_min * radius ** 2
//          lambda = - lambda_min
//        ELSE
//          lambda = zero
//        END IF
//        IF ( printi ) THEN
//          WRITE( out, "( A, A2, I4, 3ES22.15 )" )  prefix, region,             &
//          0, ABS( inform%x_norm - radius ), lambda, ABS( delta_lambda )
//          WRITE( out, "( A,                                                    &
//       &    ' Normal stopping criteria satisfied' )" ) prefix
//        END IF
//        inform%status = RAL_NLLS_ok
//        GO TO 900
//      END IF
//
//!  construct values lambda_l and lambda_u for which lambda_l <= lambda_optimal
//!   <= lambda_u, and ensure that all iterates satisfy lambda_l <= lambda
//!   <= lambda_u
//
//      c_norm_over_radius = c_norm / radius
//      IF ( control%equality_problem ) THEN
//        lambda_l = MAX( control%lower,  - lambda_min,                          &
//                        c_norm_over_radius - lambda_max )
//        lambda_u = MIN( control%upper,                                         &
//                        c_norm_over_radius - lambda_min )
//      ELSE
//        lambda_l = MAX( control%lower, zero, - lambda_min,                     &
//                        c_norm_over_radius - lambda_max )
//        lambda_u = MIN( control%upper,                                         &
//                        MAX( zero, c_norm_over_radius - lambda_min ) )
//      END IF
//      lambda = lambda_l
//
//!  check for the "hard case"
//
//      IF ( lambda == - lambda_min ) THEN
//        c2 = zero
//        inform%hard_case = .TRUE.
//        DO i = 1, n
//          IF ( H( i ) == lambda_min ) THEN
//!           IF ( ABS( C( i ) ) > epsmch ) THEN
//            IF ( ABS( C( i ) ) > epsmch * c_norm ) THEN
//              inform%hard_case = .FALSE.
//              c2 = c2 + C( i ) ** 2
//            ELSE
//              i_hard = i
//            END IF
//          END IF
//        END DO
//
//!  the hard case may occur
//
//        IF ( inform%hard_case ) THEN
//          DO i = 1, n
//            IF ( H( i ) != lambda_min ) THEN
//              X( i )  = - C( i ) / ( H( i ) + lambda )
//            ELSE
//              X( i ) = zero
//            END IF
//          END DO
//          inform%x_norm = TWO_NORM( X )
//
//!  the hard case does occur
//
//          IF ( inform%x_norm <= radius ) THEN
//            IF ( inform%x_norm < radius ) THEN
//
//!  compute the step alpha so that X + alpha E_i_hard lies on the trust-region
//!  boundary and gives the smaller value of q
//
//              utx = X( i_hard ) / radius
//              distx = ( radius - inform%x_norm ) *                             &
//                ( ( radius + inform%x_norm ) / radius )
//              alpha = sign( distx / ( abs( utx ) +                             &
//                            sqrt( utx ** 2 + distx / radius ) ), utx )
//
//!  record the optimal values
//
//              X( i_hard ) = X( i_hard ) + alpha
//            END IF
//            inform%x_norm = TWO_NORM( X )
//            inform%obj =                                                       &
//                f + half * ( DOT_PRODUCT( C, X ) - lambda * radius ** 2 )
//            IF ( printi ) THEN
//              WRITE( out, "( A, A2, I4, 3ES22.15 )" )  prefix, region,         &
//              0, ABS( inform%x_norm - radius ), lambda, ABS( delta_lambda )
//              WRITE( out, "( A,                                                &
//           &    ' Hard-case stopping criteria satisfied' )" ) prefix
//            END IF
//            inform%status = RAL_NLLS_ok
//            GO TO 900
//
//!  the hard case didn't occur after all
//
//          ELSE
//            inform%hard_case = .FALSE.
//
//!  compute the first derivative of ||x|(lambda)||^2 - radius^2
//
//            w_norm2 = zero
//            DO i = 1, n
//              IF ( H( i ) != lambda_min )                                      &
//                w_norm2 = w_norm2 + C( i ) ** 2 / ( H( i ) + lambda ) ** 3
//            END DO
//            x_norm2( 1 ) = - two * w_norm2
//
//!  compute the Newton correction
//
//            lambda =                                                           &
//              lambda + ( inform%x_norm ** 2 - radius ** 2 ) / x_norm2( 1 )
//            lambda_l = MAX( lambda_l, lambda )
//          END IF
//
//!  there is a singularity at lambda. Compute the point for which the
//!  sum of squares of the singular terms is equal to radius^2
//
//        ELSE
//!         lambda = lambda + SQRT( c2 ) / radius
//          lambda = lambda + MAX( SQRT( c2 ) / radius, lambda * epsmch )
//          lambda_l = MAX( lambda_l, lambda )
//        END IF
//      END IF
//
//!  the iterates will all be in the L region. Prepare for the main loop
//
//      it = 0
//      max_order = MAX( 1, MIN( max_degree, control%taylor_max_degree ) )
//
//!  start the main loop
//
//      DO
//        it = it + 1
//
//!  if H(lambda) is positive definite, solve  H(lambda) x = - c
//
//        DO i = 1, n
//          X( i )  = - C( i ) / ( H( i ) + lambda )
//        END DO
//
//!  compute the two-norm of x
//
//        inform%x_norm = TWO_NORM( X )
//        x_norm2( 0 ) = inform%x_norm ** 2
//
//!  if the Newton step lies within the trust region, exit
//
//        IF ( lambda == zero .AND. inform%x_norm <= radius ) THEN
//          inform%obj = f + half * DOT_PRODUCT( C, X )
//          inform%status = RAL_NLLS_ok
//          region = 'L'
//          IF ( printi ) THEN
//            WRITE( out, "( A, A2, I4, 2ES22.15 )" ) prefix,                    &
//              region, it, inform%x_norm - radius, lambda
//            WRITE( out, "( A, ' Interior stopping criteria satisfied')" ) prefix
//          END IF
//          GO TO 900
//        END IF
//
//!  the current estimate gives a good approximation to the required root
//
//        IF ( ABS( inform%x_norm - radius ) <=                                  &
//               MAX( control%stop_normal * radius,                              &
//                    control%stop_absolute_normal ) ) THEN
//          IF ( inform%x_norm > radius ) THEN
//            lambda_l = MAX( lambda_l, lambda )
//          ELSE
//            region = 'G'
//            lambda_u = MIN( lambda_u, lambda )
//          END IF
//          IF ( printt .AND. it > 1 ) WRITE( out, 2030 ) prefix
//          IF ( printi ) THEN
//            WRITE( out, "( A, A2, I4, 3ES22.15 )" )  prefix, region,           &
//            it, ABS( inform%x_norm - radius ), lambda, ABS( delta_lambda )
//            WRITE( out, "( A,                                                  &
//         &    ' Normal stopping criteria satisfied' )" ) prefix
//          END IF
//          inform%status = RAL_NLLS_ok
//          EXIT
//        END IF
//
//        lambda_l = MAX( lambda_l, lambda )
//
//!  debug printing
//
//        IF ( printd ) THEN
//          WRITE( out, "( A, 8X, 'lambda', 13X, 'x_norm', 15X, 'radius' )" )    &
//            prefix
//          WRITE( out, "( A, 3ES20.12 )") prefix, lambda, inform%x_norm, radius
//        ELSE IF ( printi ) THEN
//          IF ( printt .AND. it > 1 ) WRITE( out, 2030 ) prefix
//          WRITE( out, "( A, A2, I4, 3ES22.15 )" ) prefix, region, it,          &
//            ABS( inform%x_norm - radius ), lambda, ABS( delta_lambda )
//        END IF
//
//!  record, for the future, values of lambda which give small ||x||
//
//        IF ( inform%len_history < history_max ) THEN
//          inform%len_history = inform%len_history + 1
//          inform%history( inform%len_history )%lambda = lambda
//          inform%history( inform%len_history )%x_norm = inform%x_norm
//        END IF
//
//!  a lambda in L has been found. It is now simply a matter of applying
//!  a variety of Taylor-series-based methods starting from this lambda
//
//!  precaution against rounding producing lambda outside L
//
//        IF ( lambda > lambda_u ) THEN
//          inform%status = RAL_NLLS_error_ill_conditioned
//          EXIT
//        END IF
//
//!  compute first derivatives of x^T M x
//
//!  form ||w||^2 = x^T H^-1(lambda) x
//
//        w_norm2 = zero
//        DO i = 1, n
//          w_norm2 = w_norm2 + C( i ) ** 2 / ( H( i ) + lambda ) ** 3
//        END DO
//
//!  compute the first derivative of x_norm2 = x^T M x
//
//        x_norm2( 1 ) = - two * w_norm2
//
//!  compute pi_beta = ||x||^beta and its first derivative when beta = - 1
//
//        beta = - one
//        CALL DTRS_pi_derivs( 1, beta, x_norm2( : 1 ), pi_beta( : 1 ) )
//
//!  compute the Newton correction (for beta = - 1)
//
//        delta_lambda = - ( pi_beta( 0 ) - ( radius ) ** beta ) / pi_beta( 1 )
//
//        n_lambda = 1
//        lambda_new( n_lambda ) = lambda + delta_lambda
//
//        IF ( max_order >= 3 ) THEN
//
//!  compute the second derivative of x^T x
//
//          z_norm2 = zero
//          DO i = 1, n
//            z_norm2 = z_norm2 + C( i ) ** 2 / ( H( i ) + lambda ) ** 4
//          END DO
//          x_norm2( 2 ) = six * z_norm2
//
//!  compute the third derivatives of x^T x
//
//          v_norm2 = zero
//          DO i = 1, n
//            v_norm2 = v_norm2 + C( i ) ** 2 / ( H( i ) + lambda ) ** 5
//          END DO
//          x_norm2( 3 ) = - twentyfour * v_norm2
//
//!  compute pi_beta = ||x||^beta and its derivatives when beta = 2
//
//          beta = two
//          CALL DTRS_pi_derivs( max_order, beta, x_norm2( : max_order ),        &
//                               pi_beta( : max_order ) )
//
//!  compute the "cubic Taylor approximaton" step (beta = 2)
//
//          a_0 = pi_beta( 0 ) - ( radius ) ** beta
//          a_1 = pi_beta( 1 )
//          a_2 = half * pi_beta( 2 )
//          a_3 = sixth * pi_beta( 3 )
//          a_max = MAX( ABS( a_0 ), ABS( a_1 ), ABS( a_2 ), ABS( a_3 ) )
//          IF ( a_max > zero ) THEN
//            a_0 = a_0 / a_max ; a_1 = a_1 / a_max
//            a_2 = a_2 / a_max ; a_3 = a_3 / a_max
//          END IF
//          CALL ROOTS_cubic( a_0, a_1, a_2, a_3, roots_tol, nroots,             &
//                            root1, root2, root3, roots_debug )
//          n_lambda = n_lambda + 1
//          IF ( nroots == 3 ) THEN
//            lambda_new( n_lambda ) = lambda + root3
//          ELSE
//            lambda_new( n_lambda ) = lambda + root1
//          END IF
//
//!  compute pi_beta = ||x||^beta and its derivatives when beta = - 0.4
//
//          beta = - point4
//          CALL DTRS_pi_derivs( max_order, beta, x_norm2( : max_order ),        &
//                               pi_beta( : max_order ) )
//
//!  compute the "cubic Taylor approximaton" step (beta = - 0.4)
//
//          a_0 = pi_beta( 0 ) - ( radius ) ** beta
//          a_1 = pi_beta( 1 )
//          a_2 = half * pi_beta( 2 )
//          a_3 = sixth * pi_beta( 3 )
//          a_max = MAX( ABS( a_0 ), ABS( a_1 ), ABS( a_2 ), ABS( a_3 ) )
//          IF ( a_max > zero ) THEN
//            a_0 = a_0 / a_max ; a_1 = a_1 / a_max
//            a_2 = a_2 / a_max ; a_3 = a_3 / a_max
//          END IF
//          CALL ROOTS_cubic( a_0, a_1, a_2, a_3, roots_tol, nroots,             &
//                            root1, root2, root3, roots_debug )
//          n_lambda = n_lambda + 1
//          IF ( nroots == 3 ) THEN
//            lambda_new( n_lambda ) = lambda + root3
//          ELSE
//            lambda_new( n_lambda ) = lambda + root1
//          END IF
//        END IF
//
//!  record all of the estimates of the optimal lambda
//
//        IF ( printd ) THEN
//          WRITE( out, "( A, ' lambda_t (', I1, ')', 3ES20.13 )" )              &
//            prefix, MAXLOC( lambda_new( : n_lambda ) ),                        &
//            lambda_new( : MIN( 3, n_lambda ) )
//          IF ( n_lambda > 3 ) WRITE( out, "( A, 13X, 3ES20.13 )" )             &
//            prefix, lambda_new( 4 : MIN( 6, n_lambda ) )
//        END IF
//
//!  compute the best Taylor improvement
//
//        lambda_plus = MAXVAL( lambda_new( : n_lambda ) )
//        delta_lambda = lambda_plus - lambda
//        lambda = lambda_plus
//
//!  improve the lower bound if possible
//
//        lambda_l = MAX( lambda_l, lambda_plus )
//
//!  check that the best Taylor improvement is significant
//
//        IF ( ABS( delta_lambda ) < epsmch * MAX( one, ABS( lambda ) ) ) THEN
//          IF ( printi ) WRITE( out, "( A, ' normal exit with no ',             &
//         &                     'significant Taylor improvement' )" ) prefix
//          inform%status = RAL_NLLS_ok
//          EXIT
//        END IF
//
//!  End of main iteration loop
//
//      END DO
//
//!  Record the optimal obective value
//
//      inform%obj = f + half * ( DOT_PRODUCT( C, X ) - lambda * x_norm2( 0 ) )
//
//!  ----
//!  Exit
//!  ----
//
// 900  CONTINUE
//      inform%multiplier = lambda
//      inform%pole = MAX( zero, - lambda_min )
//      RETURN
//
//! Non-executable statements
//
// 2030 FORMAT( A, '    it     ||x||-radius             lambda ',                &
//                 '              d_lambda' )
//
//!  End of subroutine DTRS_solve_main
//
} //      END SUBROUTINE DTRS_solve_main

//!-*-*-*-*-*-*-  D T R S _ P I _ D E R I V S   S U B R O U T I N E   -*-*-*-*-*-
//
//      SUBROUTINE DTRS_pi_derivs( max_order, beta, x_norm2, pi_beta )
//
//! =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//!
//!  Compute pi_beta = ||x||^beta and its derivatives
//!
//!  Arguments:
//!  =========
//!
//!  Input -
//!   max_order - maximum order of derivative
//!   beta - power
//!   x_norm2 - (0) value of ||x||^2,
//!             (i) ith derivative of ||x||^2, i = 1, max_order
//!  Output -
//!   pi_beta - (0) value of ||x||^beta,
//!             (i) ith derivative of ||x||^beta, i = 1, max_order
//!
//!  Extracted wholesale from module RAL_NLLS_RQS
//!
//! =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
//!-----------------------------------------------
//!   D u m m y   A r g u m e n t s
//!-----------------------------------------------
//
//      INTEGER, INTENT( IN )  max_order
//      REAL , INTENT( IN )  beta, x_norm2( 0 : max_order )
//      REAL , INTENT( OUT )  pi_beta( 0 : max_order )
//
//!-----------------------------------------------
//!   L o c a l   V a r i a b l e
//!-----------------------------------------------
//
//      REAL   hbeta
//
//      hbeta = half * beta
//      pi_beta( 0 ) = x_norm2( 0 ) ** hbeta
//      pi_beta( 1 ) = hbeta * ( x_norm2( 0 ) ** ( hbeta - one ) ) * x_norm2( 1 )
//      IF ( max_order == 1 ) RETURN
//      pi_beta( 2 ) = hbeta * ( x_norm2( 0 ) ** ( hbeta - two ) ) *             &
//        ( ( hbeta - one ) * x_norm2( 1 ) ** 2 + x_norm2( 0 ) * x_norm2( 2 ) )
//      IF ( max_order == 2 ) RETURN
//      pi_beta( 3 ) = hbeta * ( x_norm2( 0 ) ** ( hbeta - three ) ) *           &
//        ( x_norm2( 3 ) * x_norm2( 0 ) ** 2 + ( hbeta - one ) *                 &
//          ( three * x_norm2( 0 ) * x_norm2( 1 ) * x_norm2( 2 ) +               &
//            ( hbeta - two ) * x_norm2( 1 ) ** 3 ) )
//
//      RETURN
//
//!  End of subroutine DTRS_pi_derivs
//
//      END SUBROUTINE DTRS_pi_derivs
//
//!-*-*-*-*-*  D T R S _ T H E T A _ D E R I V S   S U B R O U T I N E   *-*-*-*-
//
//      SUBROUTINE DTRS_theta_derivs( max_order, beta, lambda, sigma,            &
//                                     theta_beta )
//
//! =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//!
//!  Compute theta_beta = (lambda/sigma)^beta and its derivatives
//!
//!  Arguments:
//!  =========
//!
//!  Input -
//!   max_order - maximum order of derivative
//!   beta - power
//!   lambda, sigma - lambda and sigma
//!  Output -
//!   theta_beta - (0) value of (lambda/sigma)^beta,
//!             (i) ith derivative of (lambda/sigma)^beta, i = 1, max_order
//!
//!  Extracted wholesale from module RAL_NLLS_RQS
//!
//! =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
//!-----------------------------------------------
//!   D u m m y   A r g u m e n t s
//!-----------------------------------------------
//
//      INTEGER, INTENT( IN )  max_order
//      REAL , INTENT( IN )  beta, lambda, sigma
//      REAL , INTENT( OUT )  theta_beta( 0 : max_order )
//
//!-----------------------------------------------
//!   L o c a l   V a r i a b l e
//!-----------------------------------------------
//
//      REAL   los, oos
//
//      los = lambda / sigma
//      oos = one / sigma
//
//      theta_beta( 0 ) = los ** beta
//      theta_beta( 1 ) = beta * ( los ** ( beta - one ) ) * oos
//      IF ( max_order == 1 ) RETURN
//      theta_beta( 2 ) = beta * ( los ** ( beta - two ) ) *                    &
//                        ( beta - one ) * oos ** 2
//      IF ( max_order == 2 ) RETURN
//      theta_beta( 3 ) = beta * ( los ** ( beta - three ) ) *                  &
//                        ( beta - one ) * ( beta - two ) * oos ** 3
//
//      RETURN
//
//!  End of subroutine DTRS_theta_derivs
//
//      END SUBROUTINE DTRS_theta_derivs
//
//!-*-*-*-*-  G A L A H A D   T W O  _ N O R M   F U N C T I O N   -*-*-*-*-
//
//       FUNCTION TWO_NORM( X )
//
//!  Compute the l_2 norm of the vector X
//
//!  Dummy arguments
//
//       REAL   TWO_NORM
//       REAL , INTENT( IN ), DIMENSION( : )  X
//
//!  Local variable
//
//       INTEGER  n
//       n = SIZE( X )
//
//       IF ( n > 0 ) THEN
//         TWO_NORM = NRM2( n, X, 1 )
//       ELSE
//         TWO_NORM = zero
//       END IF
//       RETURN
//
//!  End of function TWO_NORM
//
//       END FUNCTION TWO_NORM
//
//!-*-*-*-*-*-  End of R A L _ N L L S _ D T R S  double  M O D U L E  *-*-*-*-*-
//
//   END MODULE RAL_NLLS_DTRS_double
//
//



} // RalNlls
} // CurveFitting
} // Mantid

