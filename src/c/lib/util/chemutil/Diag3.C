//#define DEBUG
///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: Diag3.C
//
//
// DESIGN:
//
///////////////////////////////////////////////////////////////////////////////

#include <math.h>

#include "util/Diag3.H"


// ---------- Constructors ------------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//
//  Implementation
//
///////////////////////////////////////////////////////////////////////////////
Diag3::Diag3(double xx, double xy, double xz, 
             double yy, double yz, double zz)
{
  p_xx = xx;
  p_xy = xy;
  p_xz = xz;
  p_yy = yy;
  p_yz = yz;
  p_zz = zz; 
}


 
// ---------- Virtual Destructor ------------
///////////////////////////////////////////////////////////////////////////////
///  man
//
//  Description
//    Nothing to do.
//
///////////////////////////////////////////////////////////////////////////////
Diag3::~Diag3(void)
{
}


// ------------ Modifiers ------------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//
//  Implementation
//
///////////////////////////////////////////////////////////////////////////////
void Diag3::calculate(double ew[3], double e0[3], double e1[3],
                      double e2[3])
{
  double a[7],w[4],eivr[4][4];
  a[1] = p_xx;
  a[2] = p_xy;
  a[3] = p_yy;
  a[4] = p_xz;
  a[5] = p_yz;
  a[6] = p_zz;
//
//     use jacobi method to emulate eispack rs
//     this routine uses a variable threshold jacobi method
//     it gives very good eigenvalues and eigenvectors
//     the routine is much faster than the old hdiag routine written
//     at m.i.t. that uses the jacobi method but not the variable
//     threshold technique that is applied here
//
//     This code, originally written in fortran, has been crudely
//     converted to C++ to diagonalize 3x3 matrices. No effort has
//     been made to convert indices from the usual fortran [1,n] to
//     the C++ convention of [0,n-1]. All arrays have been increased
//     in size to accomodate the extra index. The original for this
//     function is the stpr_sjacobi subroutine in NWChem.
//
//-----parameters---------------
      double sq2inv = 1.0/sqrt(2.0);
      double t1     = 1.0e-12;
      double t2     = 1.0e-12;
//------------------------------
  int i,j;
  int n = 3;
  double avgf = (double)(n*(n-1))*0.55;
  for (j=1; j<=n; j++) {
    for (i=1; i<=n; i++) {
      eivr[i][j]=0.0;
    }
    eivr[j][j]=1.0;
  }
//
//     Find the absolutely largest element of a.
//
  double atop=0.0;
  int ij = 0;
  for (i=1; i<=n; i++) {
    for (j=1; j<=i; j++) {
      ij++;
      if (atop < fabs(a[ij])) atop = fabs(a[ij]);
    }
  }
  if (atop >= 0.0) {
//
//     Calculate the stopping criterion -- dstop.
//
    double d = 0.0;
    ij = 0;
    for (i=1; i<=n; i++) {
      for (j=1; j<=i-1; j++) {
          ij++;
          d = d + a[ij]*a[ij];
      }
      ij++;
    }
    double dstop=t1*d;
//
//     Calculate the threshold, thrsh.
//
    double thrsh= sqrt(d/avgf);
//
//     Start a sweep.
//
    int iflag=1;
    while (iflag != 0) {
      iflag = 0;
      int jcol;
      for (jcol=2; jcol<=n; jcol++) {
        int jcoltr = jcol*(jcol-1)/2;
        int jcol1=jcol-1;
        int irow;
        for (irow=1; irow<=jcol1; irow++) {
          int indxa = jcoltr + irow;
          double aij=a[indxa];
//
//         Compare the off-diagonal element with thrsh.
//
          double daij = fabs(aij);
          if (daij <= thrsh) continue;
          int irowtr = irow*(irow-1)/2;
          indxa = irowtr + irow;
          double aii=a[indxa];
          indxa = jcoltr + jcol;
          double ajj=a[indxa];
          double s=ajj-aii;
          double ds = fabs(s);
//
//         The chosen rotation is less than the rounding.
//         Do not rotate.
//
          if (daij < t2*ds) continue;
          iflag=1;
          double t,u,c;
          if (t2*daij >= ds) {
//
//           Rotation is very close to 45 degrees,
//           sin and cos = 1/(root 2).
//
            s = sq2inv;
            c = s;
          } else {
//
//           Rotation is not very close to 45 degrees.
//
            t = aij/s;
            u = 0.25/sqrt(0.25+t*t);
            c = sqrt(0.5+u);
            s = 2.0*t*u/c;
          }
//
//         Calculate new elements of matrix a.
//
          for (i=1; i<=irow; i++) {
            t         = a[irowtr + i];
            u         = a[jcoltr + i];
            a[irowtr + i] = c*t-s*u;
            a[jcoltr + i] = s*t+c*u;
          }
          int i2 = irow+2;
          if (i2 <= jcol) {
            for (i=i2; i<=jcol; i++) {
              int itr = (i-1)*(i-2)/2;
              t           = a[jcoltr + i-1];
              u           = a[itr + irow];
              a[jcoltr + i-1] = s*u+c*t;
              a[itr + irow] = c*u-s*t;
            }
          }
          a[jcoltr + jcol] = s*aij+c*ajj;
          a[irowtr + irow] = c*a[irowtr + irow]-s*(c*aij-s*ajj);
          for (j=jcol; j<=n; j++) {
            int jtr = j*(j-1)/2;
            t         = a[jtr + irow];
            u         = a[jtr + jcol];
            a[jtr + irow] = c*t-s*u;
            a[jtr + jcol] = s*t+c*u;
          }
//
//         Rotation completed. see if eigenvectors are wanted by
//         user.
//
          for (i=1; i<=n; i++) {
            t=eivr[i][irow];
            eivr[i][irow]=c*t-eivr[i][jcol]*s;
            eivr[i][jcol]=s*t+eivr[i][jcol]*c;
          }
//
//         Calculate the new norm d and compare with dstop.
//
          s=aij;
          d=d-s*s;
          if (d < dstop) {
//
//           Recalculate dstop and thrsh to discard rounding errors.
//
            d=0.0;
            ij = 0;
            for (i=1; i<=n; i++) {
              for (j=1; j<=i-1; j++) {
                ij++;
                d = d + a[ij]*a[ij];
              }
              ij++;
            }
            dstop=t1*d;
          }
          thrsh=sqrt(d/avgf);
        }
      }
    }
  }
//
//     Fill eigenvalue vector.
//
  int idiag = 0;
  for (i=1; i<=n; i++) {
    idiag+=i;
    w[i] = a[idiag];
  }
//
//     Arrange eigenvalues & vectors in ascending order.
//
  int isort=1;
  while (isort == 1) {
    isort=0;
    for (i=1; i<=n-1; i++) {
      if (w[i] > w[i+1]) {
        double temp=w[i];
        w[i]=w[i+1];
        w[i+1]=temp;
        for (int k = 1; k<=n; k++) {
          temp=eivr[k][i];
          eivr[k][i]=eivr[k][i+1];
          eivr[k][i+1]=temp;
        }
        isort=1;
      }
    }
  }
  for (i=1; i<=3; i++) {
    ew[i-1] = w[i];
    e0[i-1] = eivr[i][1];
    e1[i-1] = eivr[i][2];
    e2[i-1] = eivr[i][3];
  }
  return;
}
