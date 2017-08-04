///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: UFFParams.C
//
//
// DESIGN:
//     Class UFFParams provides parameters and scalar force and energy
//     functions for the so-called "Universal ForceField" (UFF) of
//     Rappe' and co-workers (J. Am. Chem. Soc. 1992, v114, p10024-10035)
//     For now, only molecular systems are supported; R&G have published 
//     extensions, including one for metals, that could be added later.  
//     (See also UFFParams.H)
//
//     The number of Torsion terms that are included around a bond 
//     and Inversions around an atom
//     varies among forcefields (some include one; most, like the UFF,
//     include all), so the loops that generate these are in this class,
//     as well.
//
//     The parameters that are used by the UFF are actually defined in a 
//     static structure below.
//
//    WHEN AND HOW TO ADD TO THE PARAMS
//      The UFFParam1 structure below is very incomplete.  Refer to 
//      the paper for details.  This is especially true for metals which
//      are of interest for cleanup issues.  So it is likely additions
//      will be necessary as user requests/problems show up.
//
//      The steps are:
//         1. Add the appropriate line to the UFFParams1 structure.  It
//            may take some investigation to get the right numbers or 
//            you might get lucky.  Get help from a chemist.  Don't forget
//            to update all the indexes in the comments of the structure.
//         2. Update the offsets array.
//         3. Add special logic to initUFFIndex
//         4. Change code that sets the nub index since it gets pushed 
//            to the bottom.
//         5. Change line of code that sets numAtomTypes -
//              i.e. search for "static int numAtomTypes" 
//
//    TO DO:
//      In some list generation, need to use NeighborLists that ignore
//      metal-pi bonds.
//
///////////////////////////////////////////////////////////////////////////////

// system includes
#include <math.h>
#include <string.h>
// library includes
// application includes
#include "util/ErrMsg.H"
#include "tdat/TAtm.H"
#include "tdat/ShapeData.H"
#include "tdat/TBond.H"
#include "tdat/TPerTab.H"

#include "tdat/UFFParams.H"
//#include "tdat/NeighborCursor.H"

#define  PI  3.14159265358979323846
#define  DEG * PI / 180.0

#ifndef MIN
#define MIN(x,y) (((x) < (y)) ? (x) : (y))
#endif /* MIN */


//////////////////////////////////////////////////////////////////////////////
//
//  Description:  static data table
//
//   Nub is defined in TPerTab
//
//   Below, trailing asterisks denote fitted estimates of unpublished GMP
//   electronegativies extrapolated from the published GMP values and
//   the Allred-Rowchow values.  
//
////////////////////////////////////////////////////////////////////////////// 


static struct UFFParam1 {
     char atomType[6];
     double covRadius;
     double bondAngle;
     double vdWRadius;
     double vdWEnergy;
     double vdWScale;
     double effCharge;
     double electroneg;
} UFFParam1[] = {
  {  "H_   ", 0.354, 180.0,   2.886,  0.044,  12.0,   0.712, 104.34830 },       //  0
  {  "H_b  ", 0.460,  83.5,   2.886,  0.044,  12.0,   0.712, 104.34830  },      //  1
  {  "He4+4", 0.849,  90.0,   2.362,  0.056,  15.24,  0.098, 252.29 },          //  2*
  {  "Li   ", 1.336, 180.0,   2.451,  0.025,  12.0,   1.026,  69.27363 },       //  3
  {  "Be3+2", 1.074, 109.47,  2.745,  0.085,  12.0,   1.565,  90.77 },          //  4*
  {  "B_3  ", 0.838, 109.47,  4.083,  0.180,  12.052, 1.755, 122.72 },          //  5*
  {  "B_2  ", 0.828, 120.0,   4.083,  0.180,  12.052, 1.755, 122.72 },          //  6*
  {  "C_3  ", 0.757, 109.47,  3.851,  0.105,  12.73,  1.912, 123.13007 },       //  7
  {  "C_R  ", 0.729, 120.0,   3.851,  0.105,  12.73,  1.912, 123.13007 },       //  8
  {  "C_2  ", 0.732, 120.0,   3.851,  0.105,  12.73,  1.912, 123.13007 },       //  9
  {  "C_1  ", 0.706, 180.0,   3.851,  0.105,  12.73,  1.912, 123.13007 },       // 10
  {  "N_3  ", 0.700, 106.7,   3.660,  0.069,  13.407, 2.544, 158.98828 },       // 11
  {  "N_R  ", 0.699, 120.0,   3.660,  0.069,  13.407, 2.544, 158.98828 },       // 12
  {  "N_2  ", 0.685, 111.2,   3.660,  0.069,  13.407, 2.544, 158.98828 },       // 13
  {  "N_1  ", 0.656, 180.0,   3.660,  0.069,  13.407, 2.544, 158.98828 },       // 14
  {  "O_3  ", 0.658, 104.51,  3.500,  0.060,  14.085, 2.300, 201.43739 },       // 15
  {  "O_R  ", 0.680, 110.0,   3.500,  0.060,  14.085, 2.300, 201.43739 },       // 16
  {  "O_2  ", 0.634, 120.0,   3.500,  0.060,  14.085, 2.300, 201.43739 },       // 17
  {  "O_1  ", 0.639, 180.0,   3.500,  0.060,  14.085, 2.300, 201.43739 },       // 18
  {  "O_3_z", 0.528, 146.0,   3.500,  0.060,  14.085, 2.300, 201.43739 },       // 19
  {  "F_   ", 0.668, 180.0,   3.364,  0.050,  14.762, 1.735, 250.59263 },       // 20
  {  "Ne4+4", 0.920,  90.0,   3.243,  0.042,  15.440, 0.194, 293.71 },          // 21*
  {  "Na   ", 1.539, 180.0,   2.983,  0.030,  12.0,   1.081,  65.51727 },       // 22
  {  "Mg3+2", 1.421, 109.47,  3.021,  0.111,  12.0,   1.787,  76.57 },          // 23*
  {  "Al3  ", 1.244, 109.47,  4.499,  0.505,  11.278, 1.792,  90.77 },          // 24*
  {  "Si3  ", 1.117, 109.47,  4.295,  0.402,  12.175, 2.323,  96.05206 },       // 25
  {  "P_3+5", 1.056, 109.47,  4.147,  0.305,  13.072, 2.863, 125.89549 },       // 26
  {  "P_3+q", 1.056, 109.47,  4.147,  0.305,  13.072, 2.863, 125.89549 },       // 27
  {  "P_3+3", 1.101,  93.8,   4.147,  0.305,  13.072, 2.863, 125.89549 },       // 28
  {  "S_3+2", 1.064,  92.1,   4.035,  0.274,  13.969, 2.703, 159.65659 },       // 29
  {  "S_3+4", 1.049, 103.20,  4.035,  0.274,  13.969, 2.703, 159.65659 },       // 30
  {  "S_3+6", 1.027, 109.47,  4.035,  0.274,  13.969, 2.703, 159.65659 },       // 31
  {  "S_R  ", 1.077,  92.2,   4.035,  0.274,  13.969, 2.703, 159.65659 },       // 32
  {  "S_2  ", 0.854, 120.0,   4.035,  0.274,  13.969, 2.703, 159.65659 },       // 33
  {  "Cl   ", 1.044, 180.0,   3.947,  0.227,  14.866, 2.348, 197.35841 },       // 34
  {  "Ar4+4", 1.032,  90.0,   3.868,  0.185,  15.763, 0.300, 195.49 },          // 35*
  {  "K_   ", 1.953, 180.0,   3.812,  0.035,  12.0,   1.165,  55.79223 },       // 36
  {  "Ca6+2", 1.761,  90.0,   3.399,  0.238,  12.0,   2.141,  65.33 },          // 37*
  {  "Sc3+3", 1.513, 109.47,  3.295,  0.019,  12.0,   2.592, 74.80 },           // 38*
  {  "Ti3+4", 1.412, 109.47,  3.175,  0.017,  12.0,   2.659, 81.90 },           // 39*
  {  "Ti6+4", 1.412,  90.0,   3.175,  0.017,  12.0,   2.659, 81.90 },           // 40*
  {  "V_3+5", 1.402, 109.47,  3.144,  0.016,  12.0,   2.679, 89.59 },           // 41*
  {  "Cr6+3", 1.345,  90.0,   3.023,  0.015,  12.0,   2.463, 96.10 },           // 42*
  {  "Mn6+2", 1.382,  90.0,   2.961,  0.013,  12.0,   2.43,  98.46 },           // 43*
  {  "Fe3+2", 1.270, 109.47,  2.912,  0.013,  12.0,   2.43,  100.83 },          // 44*
  {  "Fe6+2", 1.335,  90.0,   2.912,  0.013,  12.0,   2.43,  100.83 },          // 45*
  {  "Co6+3", 1.241,  90.0,   2.872,  0.014,  12.0,   2.43,  104.38 },          // 46*
  {  "Ni4+2", 1.164,  90.0,   2.834,  0.015,  12.0,   2.43,  107.34 },          // 47*
  {  "Cu3+1", 1.302, 109.47,  3.495,  0.005,  12.0,   1.756, 107.34 },          // 48*
  {  "Zn3+2", 1.193, 109.47,  2.763,  0.124,  12.0,   1.308, 102.01 },          // 49*
  {  "Ga3+3", 1.260, 109.47,  4.383,  0.415,  11.0,   1.821, 111.48 },          // 50 *
  {  "Ge3  ", 1.197, 109.47,  4.280,  0.379,  12.0,   2.789, 123.31 },          // 51 *
  {  "As3+3", 1.211,  92.1,   4.230,  0.309,  13.0,   2.864, 133.96 },          // 52 *
  {  "Se3+2", 1.190,  90.6,   4.205,  0.291,  14.0,   2.764, 150.53 },          // 53 *
  {  "Br   ", 1.192, 180.0,   4.189,  0.251,  15.0,   2.519, 179.52148 },       // 54 
  {  "Kr4+4", 1.147,  90.0,   4.141,  0.220,  16.0,   0.452, 175.38 },          // 55 *
  {  "Rb   ", 2.260, 180.0,   4.114,  0.04,   12.0,   1.592,  53.71817 },       // 56 
  {  "Sr6+2", 2.052,  90.0,   3.641,  0.235,  12.0,   2.449, 62.37 },           // 57 *
  {  "Y_3+3", 1.698, 109.47,  3.345,  0.072,  12.0,   3.257, 69.47 },           // 58 *
  {  "Zr3+4", 1.564, 109.47,  3.124,  0.069,  12.0,   3.667, 75.98 },           // 59 *
  {  "Nb3+5", 1.473, 109.47,  3.165,  0.059,  12.0,   3.618, 76.57 },           // 60 *
  {  "Mo6+6", 1.467,  90.0,   3.052,  0.056,  12.0,   3.40, 80.71 },            // 61 *
  {  "Mo3+6", 1.484, 109.47,  3.052,  0.056,  12.0,   3.40, 80.71 },            // 62 *
  {  "Tc6+5", 1.322,  90.0,   2.998,  0.048,  12.0,   3.40, 84.26 },            // 63 *
  {  "Ru6+2", 1.478,  90.0,   2.963,  0.056,  12.0,   3.40, 87.81 },            // 64 *
  {  "Rh6+3", 1.332,  90.0,   2.929,  0.053,  12.0,   3.508, 89.59 },           // 65 *
  {  "Pd4+2", 1.338,  90.0,   2.899,  0.048,  12.0,   3.21, 83.67 },            // 66 *
  {  "Ag1+1", 1.386, 180.0,   3.148,  0.036,  12.0,   1.956, 87.81 },           // 67 *
  {  "Cd3+2", 1.403, 109.47,  2.848,  0.228,  12.0,   1.65, 90.18 },            // 68 *
  {  "In3+3", 1.459, 109.47,  4.463,  0.599,  11.0,   2.07, 91.96 },            // 69 *
  {  "Sn3  ", 1.398, 109.47,  4.392,  0.567,  12.0,   2.961, 105.56 },          // 70 *
  {  "Sb3+3", 1.407,  91.6,   4.420,  0.449,  13.0,   2.704, 111.48 },          // 71 *
  {  "Te3+2", 1.386, 90.25,   4.470,  0.398,  14.0,   2.882, 122.72 },          // 72 *
  {  "I_   ", 1.382, 180.0,   4.50,   0.339,  15.0,   2.65,   157.21381 },      // 73 
  {  "Xe4+4", 1.267,  90.0,   4.404,  0.332,  12.0,   0.556, 157.63 },          // 74 *
  {  "Cs   ", 2.570, 180.0,   4.517,  0.045,  12.0,   1.573,   50.30749 },      // 75 
  {  "Ba6+2", 2.277,  90.0,   3.703,  0.364,  12.0,   2.727, 61.19 },           // 76 *
  {  "La3+3", 1.943, 109.47,  3.522,  0.017,  12.0,   3.30, 67.70 },            // 77 *
  {  "Ce6+3", 1.841,  90.0,   3.556,  0.013,  12.0,   3.30, 66.52 },            // 78 *
  {  "Pr6+3", 1.823,  90.0,   3.606,  0.010,  12.0,   3.30, 67.11 },            // 79 *
  {  "Nd6+3", 1.816,  90.0,   3.575,  0.010,  12.0,   3.30, 67.11 },            // 80 *
  {  "Pm6+3", 1.801,  90.0,   3.547,  0.009,  12.0,   3.30, 67.11 },            // 81 *
  {  "Sm6+3", 1.780,  90.0,   3.520,  0.008,  12.0,   3.30, 67.11 },            // 82 *
  {  "Eu6+3", 1.771,  90.0,   3.493,  0.008,  12.0,   3.30, 63.56 },            // 83 *
  {  "Gd6+3", 1.735,  90.0,   3.368,  0.009,  12.0,   3.30, 69.47 },            // 84 *
  {  "Tb6+3", 1.732,  90.0,   3.451,  0.007,  12.0,   3.30, 68.88 },            // 85 *
  {  "Dy6+3", 1.710,  90.0,   3.428,  0.007,  12.0,   3.30, 68.88 },            // 86 *
  {  "Ho6+3", 1.696,  90.0,   3.409,  0.007,  12.0,   3.416, 68.88 },           // 87 *
  {  "Er6+3", 1.673,  90.0,   3.391,  0.007,  12.0,   3.30, 68.88 },            // 88 *
  {  "Tm6+3", 1.660,  90.0,   3.374,  0.006,  12.0,   3.30, 69.47 },            // 89 *
  {  "Yb6+3", 1.637,  90.0,   3.355,  0.228,  12.0,   2.618, 66.52 },           // 90 *
  {  "Lu6+3", 1.671,  90.0,   3.640,  0.041,  12.0,   3.271, 71.25 },           // 91 *
  {  "Hf3+4", 1.611, 109.47,  3.141,  0.072,  12.0,   3.921, 76.57 },           // 92 *
  {  "Ta3+5", 1.511, 109.47,  3.170,  0.081,  12.0,   4.075, 82.49 },           // 93 *
  {  "W_6+6", 1.392,  90.0,   3.069,  0.067,  12.0,   3.70, 86.63 },            // 94 *
  {  "W_3+4", 1.526, 109.47,  3.069,  0.067,  12.0,   3.70, 86.63 },            // 95 *
  {  "W_3+6", 1.380, 109.47,  3.069,  0.067,  12.0,   3.70, 86.63 },            // 96 *
  {  "Re6+5", 1.372,  90.0,   2.954,  0.066,  12.0,   3.70, 90.18 },            // 97 *
  {  "Re3+7", 1.314, 109.47,  2.954,  0.066,  12.0,   3.70, 90.18 },            // 98 *
  {  "Os6+6", 1.372,  90.0,   3.120,  0.037,  12.0,   3.70, 93.73 },            // 99 *
  {  "Ir6+3", 1.371,  90.0,   2.840,  0.073,  12.0,   3.731, 95.51 },           // 100*
  {  "Pt4+2", 1.364,  90.0,   2.754,  0.080,  12.0,   3.382, 89.00 },           // 101*
  {  "Au4+3", 1.262,  90.0,   3.293,  0.039,  12.0,   2.625, 87.81 },           // 102*
  {  "Hg1+2", 1.340, 180.0,   2.705,  0.385,  12.0,   1.75,  89.00 },           // 103*
  {  "Tl3+3", 1.518, 120.0,   4.347,  0.680,  11.0,   2.068,  89.00 },          // 104*
  {  "Pb3  ", 1.459, 109.47,  4.297,  0.663,  12.0,   2.846,  95.51 },          // 105*
  {  "Bi3+3", 1.512,  90.0,   4.370,  0.518,  13.0,   2.470,  102.61 },         // 106*
  {  "Po3+2", 1.50,   90.0,   4.709,  0.325,  14.0,   2.33, 107.93 },           // 107*
  {  "At   ", 1.545, 180.0,   4.750,  0.284,  15.0,   2.24, 119.76 },           // 108*
  {  "Rn4+4", 1.420,  90.0,   4.765,  0.248,  16.0,   0.583,122.13 },           // 109*
  {  "Fr   ", 2.880, 180.0,   4.90,   0.050,  12.0,   1.847, 54.68 },           // 110*
  {  "Ra6+2", 2.512,  90.0,   3.677,  0.404,  12.0,   2.92,  61.19 },           // 111*
  {  "Ac6+3", 1.983,  90.0,   3.478,  0.033,  12.0,   3.90,  62.97 },           // 112*
  {  "Th   ", 1.721,  180.0,   3.396,  0.026,  12.0,   4.202, 69.47 },          // 113*
  {  "Th6+4", 1.721,  90.0,   3.396,  0.026,  12.0,   4.202, 69.47 },           // 114*
  {  "Pa6+4", 1.711,  90.0,   3.424,  0.022,  12.0,   3.90,  71.25 },           // 115*
  {  "U_6+4", 1.684,  90.0,   3.395,  0.022,  12.0,   3.90,  75.98 },           // 116*
  {  "Np6+4", 1.666,  90.0,   3.424,  0.019,  12.0,   3.90,  75.98 },           // 117*
  {  "Pu6+4", 1.657,  90.0,   3.424,  0.016,  12.0,   3.90,  75.98 },           // 118*
  {  "Am6+4", 1.660,  90.0,   3.381,  0.014,  12.0,   3.90,  75.98 },           // 119*
  {  "Cm6+3", 1.801,  90.0,   3.326,  0.013,  12.0,   3.90,  75.98 },           // 120*
  {  "Bk6+3", 1.761,  90.0,   3.339,  0.013,  12.0,   3.90,  75.98 },           // 121*
  {  "Cf6+3", 1.750,  90.0,   3.313,  0.013,  12.0,   3.90,  75.98 },           // 122*
  {  "Es6+3", 1.724,  90.0,   3.299,  0.012,  12.0,   3.90,  75.98 },           // 123*
  {  "Fm6+3", 1.712,  90.0,   3.286,  0.012,  12.0,   3.90,  75.98 },           // 124*
  {  "Md6+3", 1.689,  90.0,   3.274,  0.011,  12.0,   3.90,  75.98 },           // 125*
  {  "No6+3", 1.679,  90.0,   3.248,  0.011,  12.0,   3.90,  75.98 },           // 126*
  {  "Lw6+3", 1.698,  90.0,   3.236,  0.011,  12.0,   3.90,  75.98 },           // 127*
  {  "Rf6+3", 1.698,  90.0,   3.236,  0.011,  12.0,   3.90,  75.98 },           // 128*
  {  "Ha6+3", 1.698,  90.0,   3.236,  0.011,  12.0,   3.90,  75.98 },           // 129*
  {  "Uh6+3", 1.698,  90.0,   3.236,  0.011,  12.0,   3.90,  75.98 },           // 130*
  {  "Us6+3", 1.698,  90.0,   3.236,  0.011,  12.0,   3.90,  75.98 },           // 131*
  {  "Uo+3",  1.698,  90.0,   3.236,  0.011,  12.0,   3.90,  75.98 },           // 132*
  {  "Nub  ", 0.17,  180.0,   0.10,  0.001,  12.0,   0.71, 104.34830 },         // 133
};

static int numAtomTypes = 134;

////////////////////////////////////////////////////////////////////////
// offset[] is indexed by (atomicNumber-1), the value is the index of the
// first instance of the element in the UFFParam1 structure
////////////////////////////////////////////////////////////////////////
static int offset[] =
{
  0, 2,  //He
  3, 4, 5, 7, 11, 15, 20, 21,  //Ne
  22, 23, 24, 25, 26, 29, 34, 35, // Ar
  36, 37, 38, 39, 41, 42, 43, 44, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, //Kr
  56, 57, 58, 59, 60, 61, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, //Xe
  75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 
  93,94,97,99,100,101,102,103,104,105,
  106,107,108,109,110,111,112,113,115,116,117,118,119,120,121,122,123,124,
  125,126,127,128,129,130,131,132,133,
  134 // Nub
};

// -----------------------
// Public Member Functions
// -----------------------

// ---------- Constructors ------------
//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description:
//       UFFParams constructor
//
////////////////////////////////////////////////////////////////////////////// 
UFFParams::UFFParams( const vector<TAtm*>* atmPtrs) 
  : p_atmPtrs(atmPtrs)
{
#if (!defined(INSTALL) && defined(DEBUG))
  //  if ( !staticsOK() ) {
    dumpStatics(cout);
    //  }
#endif 
  initAtomTypeList();
  initPartialCharges();
  initDisplayRadii();
}

// ---------- Virtual Destructor ------------
UFFParams::~UFFParams(void)
{
  delete[] p_atomTypeList;
  p_atomTypeList = (int*)0;
  delete[] p_partialCharge;
  p_partialCharge = (double*)0;
  delete[] p_displayRadii;
  p_displayRadii = (double*)0;
}

// ------------ Accessors ------------

double UFFParams::partialCharge( int iatom ) const
{ return p_partialCharge[iatom]; }

double UFFParams::displayRadii( int iatom ) const
{ return p_displayRadii[iatom]; }

//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description:
//      True if the atomic number is that of an element in the Nitrogen column
//      of the periodic table (group 5).
//
////////////////////////////////////////////////////////////////////////////// 
bool UFFParams::isGroup5( int atomicNumber ) const
{
  return ( atomicNumber == 7  || atomicNumber == 15 || atomicNumber == 33 ||
           atomicNumber == 51 || atomicNumber == 83 );
}
//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description:
//      True if the atomic number is that of an element in the Oxygen column
//      of the periodic table (group 6).
//
////////////////////////////////////////////////////////////////////////////// 
bool UFFParams::isGroup6( int atomicNumber ) const
{
  return ( atomicNumber == 8  || atomicNumber == 16 || atomicNumber == 34 ||
           atomicNumber == 52 || atomicNumber == 84 );
}
//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description:
//      True if atomicNumber is that of a metal.
//     
////////////////////////////////////////////////////////////////////////////// 
bool UFFParams::isMetal( int atomicNumber ) const
{
  return ( ! ( ( atomicNumber <= 2 ) || 
               ( atomicNumber >=  5 && atomicNumber <=10 ) || 
               ( atomicNumber >= 14 && atomicNumber <=18 ) || 
               ( atomicNumber >= 32 && atomicNumber <=36 ) || 
               ( atomicNumber >= 51 && atomicNumber <=54 ) || 
               ( atomicNumber == 86 ) ) );
}
//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description:
//      True if an atom is "resonant", meaning that one if its bonds is
//      of non-integral order greater than 1.0.  (Usually 1.5).
//
////////////////////////////////////////////////////////////////////////////// 
bool UFFParams::isResonant( int iatom ) const
{
  bool isResonant = false;
  double tempOrder;
  int ibond;
  const vector<TBond*>& nghbrs = (*p_atmPtrs)[iatom]->bondList();
  for (ibond=0; ibond < nghbrs.size(); ibond++) {
    tempOrder = nghbrs[ibond]->order();
    if ( tempOrder > 1.0 && (tempOrder-(double)(int)(tempOrder) > 0.01) ) {
      isResonant = true;  break;
    }
  }
  return isResonant;
}

//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description:
//      True if an atom has a double bond.
//
////////////////////////////////////////////////////////////////////////////// 
bool UFFParams::hasDouble( int iatom ) const
{
  bool hasDouble = false;
  int ibond;
  const vector<TBond*>& nghbrs = (*p_atmPtrs)[iatom]->bondList();
  for (ibond=0; ibond < nghbrs.size(); ibond++) {
    if (fabs(nghbrs[ibond]->order()-2.0) < 0.000001) {
      hasDouble = true;  break;
    }
  }
  return hasDouble;
}

//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description:
//      True if an atom has an sp2-hybridized (trigonal planar) neighbor
//      Must be called from within a transaction.
//
////////////////////////////////////////////////////////////////////////////// 
bool UFFParams::hasSp2Neighbor( int iatom ) const
{
  bool hasSp2Neighbor = false;
  vector<TAtm*>* nghbrs = (*p_atmPtrs)[iatom]->connectedAtoms();
  int iat;
  for (iat=0; iat < nghbrs->size(); iat++) {
    hasSp2Neighbor = (*nghbrs)[iat]->shape() == ShapeData::trigonalPlanar;
    if (hasSp2Neighbor) break;
  }
  return hasSp2Neighbor;
}

//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description:
//      Returns the period or row of the periodic table for an atomic number,
//      counting H and He as row 0.
//
////////////////////////////////////////////////////////////////////////////// 
int UFFParams::period( int atomicNumber ) const
{
  int period = 0;

  if ( atomicNumber <= 2 ) {
    period = 0;
  } else if ( atomicNumber <= 10 ) {
    period = 1;
  } else if ( atomicNumber <= 18 ) {
    period = 2;
  } else if ( atomicNumber <= 36 ) {
    period = 3;
  } else if ( atomicNumber <= 54 ) {
    period = 4;
  } else if ( atomicNumber <= 86 ) {
    period = 5;
  } else {
    period = 6;
  }
  return period;
}
//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description:
//      Returns the value of the Torsion barrier height V_{sp3} from Table 3
//      of Rappe'.  Units are kcal/mol
//
////////////////////////////////////////////////////////////////////////////// 
double UFFParams::vsp3( int atomicNumber ) const
{
  double vsp3 = 0;

  switch (atomicNumber) {

  case 6:  { vsp3 = 2.119; break; }  // C
  case 7:  { vsp3 = 0.450; break; }  // N
  case 8:  { vsp3 = 0.018; break; }  // O

  case 14: { vsp3 = 1.225; break; }  // Si
  case 15: { vsp3 = 2.400; break; }  // P
  case 16: { vsp3 = 0.484; break; }  // S

  case 32: { vsp3 = 0.701; break; }  // Ge
  case 33: { vsp3 = 1.500; break; }  // As
  case 34: { vsp3 = 0.335; break; }  // Se

  case 50: { vsp3 = 0.199; break; }  // Sn
  case 51: { vsp3 = 1.100; break; }  // Sb
  case 52: { vsp3 = 0.300; break; }  // Te

  case 82: { vsp3 = 0.100; break; }  // Pb
  case 83: { vsp3 = 1.000; break; }  // Bi
  case 84: { vsp3 = 0.300; break; }  // Po

  default: { ErrMsg().message("EE_BAD_VALUE", EE_FATAL, WHERE,
            "UFFParams::vsp3 illegal atomic number" );  break; }
  }
  return vsp3;
}
//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description:
//      Return the Universal Force Field "atom type" label corresponding 
//      to the integer type index
//
////////////////////////////////////////////////////////////////////////////// 
char* UFFParams::atomType( int uffTypeIndex ) const
{ return ::UFFParam1[uffTypeIndex].atomType; }

//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description:
//      Returns the covalent radius from the static data structure UFFParam1
//      indexed by the UFF atom type.
//
////////////////////////////////////////////////////////////////////////////// 
double UFFParams::covRadius( int uffTypeIndex ) const
{ return ::UFFParam1[uffTypeIndex].covRadius; }

//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description:
//      Returns the equilibrium bond angle from the static data 
//      structure UFFParam1, indexed by the UFF atom type.
//
////////////////////////////////////////////////////////////////////////////// 
double UFFParams::bondAngle( int uffTypeIndex ) const
{ return ::UFFParam1[uffTypeIndex].bondAngle; }

//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description:
//      Returns the van der Waals radius from the static data 
//      structure UFFParam1, indexed by the UFF atom type.
//
////////////////////////////////////////////////////////////////////////////// 
double UFFParams::vdWRadius( int uffTypeIndex ) const
{ return ::UFFParam1[uffTypeIndex].vdWRadius; }

//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description:
//      Returns the van der Waals energy parameter Di from the static data 
//      structure UFFParam1, indexed by the UFF atom type.
//
////////////////////////////////////////////////////////////////////////////// 
double UFFParams::vdWEnergy( int uffTypeIndex ) const
{ return ::UFFParam1[uffTypeIndex].vdWEnergy; }

//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description:
//      Returns the van der Waals scale factor from the static data 
//      structure UFFParam1, indexed by the UFF atom type.
//
////////////////////////////////////////////////////////////////////////////// 
double UFFParams::vdWScale( int uffTypeIndex ) const
{ return ::UFFParam1[uffTypeIndex].vdWScale; }

//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description:
//      Returns the "effective charge" parameter for the Universal Force
//      Field  from the static data structure UFFParam1, indexed by the 
//      UFF atom type.
//
////////////////////////////////////////////////////////////////////////////// 
double UFFParams::effCharge( int uffTypeIndex ) const
{ return ::UFFParam1[uffTypeIndex].effCharge; }

//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description:
//     Returns the "Generalized Mulliken-Pauling" (GMP) electronegativity,
//     (or an interpolated estimate, for unpublished values), from the static 
//     data structure UFFParam1, indexed by the UFF atom type.
//
////////////////////////////////////////////////////////////////////////////// 
double UFFParams::electroneg( int uffTypeIndex ) const
{ return ::UFFParam1[uffTypeIndex].electroneg; }

//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description:
//      Debug function; checks for reasonable values in static data
//      
////////////////////////////////////////////////////////////////////////////// 
bool UFFParams::staticsOK(void) const
{
  bool isOK = true;
  for (int i=0; i<numAtomTypes; i++) {
    if (    covRadius(i) < 0.0  || covRadius(i) > 1.e6
         || vdWRadius(i) < 0.0  || vdWRadius(i) > 1.e6
         || vdWEnergy(i) < 0.0  || vdWEnergy(i) > 1.e6
         || vdWScale(i)  < 0.0  || vdWScale(i)  > 1.e6
         || effCharge(i) < 0.0  || effCharge(i) > 1.e6
         || electroneg(i) < 0.0 || electroneg(i) > 1.e6 ) {
      isOK = false;
      break;
    }
  }
  return isOK;
}
//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description:
//      Write out static parameters (for debug)
//      
////////////////////////////////////////////////////////////////////////////// 
ostream& UFFParams::dumpStatics(ostream& os) const
{
  os << "     -----UFFParams static data-----     " << endl;
  os << " parameters: " << endl;
  int i;
  for (i=0; i<numAtomTypes; i++) {
    os << i << ": "
       << atomType(i) << " "
       << covRadius(i) << " "
       << vdWRadius(i) << " " 
       << vdWEnergy(i) << " "
       << vdWScale(i)  << " "
       << effCharge(i) << " "
       << electroneg(i) << " " 
       << endl;
  }
  os << "offsets: " << endl;
  for (i=0; i<108; i++) {
    os << i << ": " << offset[i] << "   ";
    if (i%10 == 9) os << endl;
  }
  os << endl;
  return os;
}
//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description:
//       Simple accessor:  returns cached UFF atom type of atom number iatom
//  Implementation:
//
////////////////////////////////////////////////////////////////////////////// 
int UFFParams::uffIndex( int iatom) const
{ return p_atomTypeList[iatom]; }

//////////////////////////////////////////////////////////////////////////////
//
//  Description:  
//      Return UFF atom type index for a TAtm.  This is the logical guts
//      of the UFF ForceField.
//
//  Implementation: offset array points to first entry for a given atomic
//        for elements with multiple entries (for different geometries and/or
//        oxidation states) a further small offset may be added for non-default
//        atom types.
//
////////////////////////////////////////////////////////////////////////////// 
int UFFParams::initUFFIndex( int iatom ) const
{
  static int nubAtom = TPerTab().nubAtom();
  const TAtm* atom = (*p_atmPtrs)[iatom];
  int result;
  int atomicNumber = 0;

  atomicNumber = (int)atom->atomicNumber();
  ShapeData::Shape shape = atom->shape();

  if (atomicNumber == 0) {
    result = numAtomTypes-1; // for ghost atoms and nubs
  } else if (atomicNumber == nubAtom) {
    result = numAtomTypes-1;
  } else {
    result = offset[atomicNumber-1];
  }

  ////////////////////////////////////////////////////////////////////////
  // for atoms with more than one parameterization in the list, choose one
  // based on Shape and number and type of attached bonds.
  // See list of available parameters in struct UFFParam1[]
  ////////////////////////////////////////////////////////////////////////
  switch (atomicNumber) {
  case 1: { //  H
    if ( shape == ShapeData::bent ) result++; // bridging H
    break; }
  case 5: { //  B
    if ( shape == ShapeData::trigonalPlanar ) result++; // trigonal (else sp3)
    break; }
  case 6: { // C default is sp3
    if ( shape == ShapeData::trigonalPlanar ) {
      if ( isResonant(iatom) ) { 
        result ++; // C_R
      } else { 
        result += 2; // sp2 (C_2)
      }
    } else if ( shape == ShapeData::linear ) {
      result += 3;   // sp
    }
    break; }
  case 7: { // N default is N_3 tetrahedral angles for sp3 pyramidal
    if ( shape == ShapeData::trigonalPlanar ) { // resonant sp2
      result ++;  // N_R
    } else if ( shape == ShapeData::pyramidal && hasDouble(iatom) ) {
      result += 2;  // N_2 sp2, not quite planar
    } else if ( shape == ShapeData::linear ) { // sp linear
      result += 3;
    }
    break; }
  case 8: { // O default is sp3  (zeolite parameters not used)
    if ( shape == ShapeData::bent && isResonant(iatom)) { // resonant; slightly bent
      result++;
    } else if ( shape == ShapeData::trigonalPlanar ) { // sp2 (O_2)
      result += 2;
    } else if ( shape == ShapeData::terminal ) { // sp
      result += 3;
    }
    break; }

  ////////////////////////////////////////////////////////////////////////
  // several tetrahedral vs. octahedral cases... 
  ////////////////////////////////////////////////////////////////////////
  case 22:  //  Ti
  case 26:  //  Fe
    if ( shape == ShapeData::octahedral ) result++; // octahedral
    break;
  case 42:  //  Mo
  case 74:  //  W (tungsten)
  case 75:  //  Re
    if ( shape == ShapeData::tetrahedral ) result++; // tetrahedral
    break;
  case 90:  //  Th
    if ( shape == ShapeData::cubic ) result++; 
    break;
  default: // just use the default index value set at the top
    break;
  }
  return result;
}

//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description:
//      Return the equilibrium bond length parameter for the two bonded
//      atoms, based on atom types and bond order.
//
////////////////////////////////////////////////////////////////////////////// 
double UFFParams::r12( int uffTypeIndex1,
                       int uffTypeIndex2,
                       double bondOrder ) const
{
  double r1 = UFFParam1[uffTypeIndex1].covRadius;
  double r2 = UFFParam1[uffTypeIndex2].covRadius;
  double chi1 = UFFParam1[uffTypeIndex1].electroneg;
  double chi2 = UFFParam1[uffTypeIndex2].electroneg;

  ////////////////////////////////////////////////////////////////////////
  // sign choice of electronegativity correction rEN opposite to Rappe' 
  // et al.; theirs makes the bond _longer_ for ionic character
  ////////////////////////////////////////////////////////////////////////
  static const double lambda = -0.1332;
  double tmp =  ( sqrt(chi1) - sqrt(chi2) );
  double rEN = r1 * r2 * tmp * tmp / (chi1*r1 + chi2*r2); 
  double rBO = lambda * (r1 + r2) * log(bondOrder); // Angstroms assumed
  double result = r1 + r2 + rBO - rEN;

  return result;
}

// ---------- General Methods  ----------},


// --------------------------
// Protected Member Functions
// --------------------------

//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description:
//      Initializes the cached list of UFF atom types for the Chemical System.
//      These are accessed by the function uffIndex(atm)
//
////////////////////////////////////////////////////////////////////////////// 
void UFFParams::initAtomTypeList(void)
{
  int numAtoms = p_atmPtrs->size();
  p_atomTypeList = new int[numAtoms];
  for (int iatom=0; iatom<numAtoms; iatom++) {
    p_atomTypeList[iatom] = initUFFIndex(iatom);
  }
}

//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description:
//      Initializes the cached list of atomic partial charges.
//      These are accessed by partialCharge(iatom).
//
////////////////////////////////////////////////////////////////////////////// 
void UFFParams::initPartialCharges(void)
{
  int numAtoms = p_atmPtrs->size();
  p_partialCharge = new double[numAtoms];
  for (int iatom=0; iatom<numAtoms; iatom++) {
    p_partialCharge[iatom] = (*p_atmPtrs)[iatom]->partialCharge();
  }
}

//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description:
//      Initializes the cached list of display radii.  These are
//      used as equilibrium bond lengths from atoms to their "bonding
//      nubs", so that the nubs appear half-submerged in their host
//      atoms in the builder.
//
////////////////////////////////////////////////////////////////////////////// 
void UFFParams::initDisplayRadii(void)
{
  TPerTab tpt;
  int numAtoms = p_atmPtrs->size();
  p_displayRadii = new double[numAtoms];

  for (int iatom=0; iatom<numAtoms; iatom++) {
    p_displayRadii[iatom] = tpt.covalentRadius((*p_atmPtrs)[iatom]->atomicNumber());
  }

}

// ---------- Derived Class Accessors ----------


// ------------------------
// Private Member Functions
// ------------------------

// ---------- Friends ----------
ostream& operator<<(ostream& os, const UFFParams& uffparams)
{
  int numAtoms = uffparams.p_atmPtrs->size();
  os << "[UFFParams";
  os << "<UFFatomTypes=" << endl;

  int iat;
  for (iat=0; iat<(int)numAtoms-9; iat+=10 ) {
      
    os << uffparams.atomType(uffparams.uffIndex(iat))   << " "
       << uffparams.atomType(uffparams.uffIndex(iat+1)) << " "
       << uffparams.atomType(uffparams.uffIndex(iat+2)) << " "
       << uffparams.atomType(uffparams.uffIndex(iat+3)) << " "
       << uffparams.atomType(uffparams.uffIndex(iat+4)) << " "
       << uffparams.atomType(uffparams.uffIndex(iat+5)) << " "
       << uffparams.atomType(uffparams.uffIndex(iat+6)) << " "
       << uffparams.atomType(uffparams.uffIndex(iat+7)) << " "
       << uffparams.atomType(uffparams.uffIndex(iat+8)) << " "
       << uffparams.atomType(uffparams.uffIndex(iat+9)) << endl;
  }

  for ( ; iat<numAtoms; iat++ ) {
    os << uffparams.atomType(uffparams.uffIndex(iat)) << " ";
  }
  os << ">";
  os << "<UFFatomTypesIndices=" << endl;

  for ( iat=0; iat<(int)numAtoms-9; iat+=10 ) {
    os << uffparams.uffIndex(iat)   << " "
       << uffparams.uffIndex(iat+1) << " "
       << uffparams.uffIndex(iat+2) << " "
       << uffparams.uffIndex(iat+3) << " "
       << uffparams.uffIndex(iat+4) << " "
       << uffparams.uffIndex(iat+5) << " "
       << uffparams.uffIndex(iat+6) << " "
       << uffparams.uffIndex(iat+7) << " "
       << uffparams.uffIndex(iat+8) << " "
       << uffparams.uffIndex(iat+9) << endl;
  }

  for ( ; iat<numAtoms; iat++ ) {
    os << uffparams.uffIndex(iat) << " ";
  }
  os << ">";
  os << "]" << endl;
  return os;
}


// ---------- Encapsulated Behavior ----------

