///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: PropVector.C
//
//
// DESIGN:
//   Each row in the vector has a corresponding label.
//   PropVector is write-once.
//
///////////////////////////////////////////////////////////////////////////////

#include <strstream>
using std::istrstream;

#include <stdlib.h>         // atoi
#include <math.h>           // sqrt

#include "util/ErrMsg.H"
#include "tdat/PropVector.H"


// ---------- Constructors ------------
///////////////////////////////////////////////////////////////////////////////
//
// Initialize empty vector
//
///////////////////////////////////////////////////////////////////////////////
PropVector::PropVector(const string& type,
                       const string& units) : TProperty(type, units)
{
  // rowLabel and valueLabel are empty strings from default string constructor

  p_values    = new vector<double>; 
  p_rowLabels = 0;                 
  p_classType = TProperty::PROPVECTOR;

}

///////////////////////////////////////////////////////////////////////////////
//
// Copy constructor
//
//////////////////////////////////////////////////////////////////////////////
PropVector::PropVector(const PropVector& propVector) : TProperty(propVector)
{
   p_rowLabel   = propVector.p_rowLabel;
   p_valueLabel = propVector.p_valueLabel;
   if (propVector.p_rowLabels != 0)
      p_rowLabels  = new vector<string>(*(propVector.p_rowLabels));
   else
      p_rowLabels   = 0;
   p_values     = new vector<double>(*(propVector.p_values));
}

 
// ---------- Virtual Destructor ------------
PropVector::~PropVector(void)
{
  delete p_values;
  if (p_rowLabels != 0)
     delete p_rowLabels;
}


// ---------- Operators ----------
   // defined in parent class


// ---------- Accessors ----------
int PropVector::rows(void) const
{
  return p_values->size();
}

/////////////////////////////////////////
bool PropVector::canScalarize(void) const
{
  // the only vectors that can be scalarized are
  // those with row label "Coordinate"
  
  if (p_rowLabel == "Coordinate")
    return true;
  else
    return false;
}

/////////////////////////////////////////
double PropVector::scalarize(void) const
{
  // returns the MAGNITUDE of the vector
  // assumes this will only be called if canScalarize is true

  double ret = 0.0;

  // calculate the sum of squares:
  for (int i = 0; i < p_values->size(); i++)
     ret += (*p_values)[i] * (*p_values)[i];

  // calculate the squareroot of the sum:
  if (ret != 0.0)
    ret = sqrt(ret);

  return ret;
}

/////////////////////////////////////////
double PropVector::value(int row) const
{
  // Return the value for the given row index
  // Assumes index is within bounds of the vector

  if (row < 0 || row >= p_values->size())
    EE_RT_ASSERT(false, EE_WARNING,
          "trying to access out-of-bounds index in PropVector");

  return (*p_values)[row];
}

/////////////////////////////////////////

vector<double>& PropVector::values(void) const
{
  return *p_values;  // return a reference to _values which cannot be changed
}

/////////////////////////////////////////

vector<string>* PropVector::rowLabels(void) const
{
  return p_rowLabels; // return all row labels (one for each value)
}

/////////////////////////////////////////

string PropVector::rowLabel(void) const
{
  return p_rowLabel; // label for all rows
}

/////////////////////////////////////////

string PropVector::valueLabel(void) const
{
  return p_valueLabel; // label for all values
}


// ------------ Modifiers ------------

///////////////////////////////////////////////////////////////////////////////
// Read list of values and corresponding labels to insert from the ifstream
//
//    Formatted as:
//       Size:xx
//       <rowlabel>:<value>
//       ...
//       END
//
// This function resets all values in the rowLabels and value vectors to those
// listed in the input stream.  Any previous values in the vectors are deleted.
//
// Assumes that the # of values = size
///////////////////////////////////////////////////////////////////////////////
void PropVector::setData(istream& istrm)
{
   char line[256];
   string tmpString;
   int colonPos;  

   istrm.getline(line,255);        // Read the size line
   int numRows = atoi(line+5);     // Remove the "Size:", get size as number

   p_values->clear();               // remove any pre-existing values
   if (p_rowLabels != 0)
     delete p_rowLabels;
   p_rowLabels = new vector<string>;

// Loop through the values, assigning to vector
   for (int i=0; i<numRows; i++)
   {
     if (!istrm.getline(line,255))
       EE_RT_ASSERT(false, EE_FATAL,
             "size specified in file is greater than the number of values");

     tmpString = line;
     colonPos  = tmpString.find(":");
     p_values->push_back(atof(tmpString.substr(colonPos+1).c_str())); // value
     p_rowLabels->push_back(tmpString.substr(0,colonPos));            // label
   }
}

///////////////////////////////////////////////////////////////////////////////
//
// Reset values given a vector of doubles as data
//  - assume the size of the array is the desired size of vector
//
///////////////////////////////////////////////////////////////////////////////
void PropVector::values(const vector<double>& values)
{

  delete p_values;        // remove any pre-existing values
  p_values = new vector<double>(values);
}

///////////////////////////////////////////////////////////////////////////////
//
// Reset row labels given a vector of strings as data
//  - assume the size of the labels array is the same size as the values array
//    (it is up to developer to verify)
//
///////////////////////////////////////////////////////////////////////////////
void PropVector::rowLabels(const vector<string>* labels)
{
  if (p_rowLabels != 0) {
     delete p_rowLabels;
     p_rowLabels = 0;
  }
  if (labels != 0)
    p_rowLabels = new vector<string>(*labels);
}

///////////////////////////////////////////////////////////////////////////////
//
// Set the label of the rows of the PropVector
//
///////////////////////////////////////////////////////////////////////////////
void PropVector::rowLabel(const string& newRowLabel)
{
   p_rowLabel = newRowLabel;
}

///////////////////////////////////////////////////////////////////////////////
//
// Set the label of the values of the PropVector
//
///////////////////////////////////////////////////////////////////////////////
void PropVector::valueLabel(const string& newValueLabel)
{
   p_valueLabel = newValueLabel;
}

///////////////////////////////////////////////////////////////////////////////
//
// Reset all the values of the PropVector based on data from tt message
// One value per row.  Each row is delimited by CR
//
///////////////////////////////////////////////////////////////////////////////
bool PropVector::update(const string& msg)
{
   istrstream input(msg.c_str());
   char temp[100];
   if (p_values != 0)
     p_values->clear();

   while(input.getline(temp,100))
      p_values->push_back(atoi(temp));
   return true;
}

// ------------ Analysis Functions ------------

///////////////////////////////////////////////////////////////////////////////
//
//  Bin all values into a histogram and return values of x and y axis in two
//  vectors
//
///////////////////////////////////////////////////////////////////////////////

void PropVector::histogram(vector<double> *x, vector<double> *y)
{
  int nsize = p_values->size();
  if (nsize > 0) {
    int idx, ibin;
    double hmin, hmax;
    hmin = (*p_values)[0];
    hmax = (*p_values)[0];

    // find min and max values of vector
    for (idx=1; idx<nsize; idx++) {
      if ((*p_values)[idx] < hmin) hmin = (*p_values)[idx];
      if ((*p_values)[idx] > hmax) hmax = (*p_values)[idx];
    }

    double value;
    double delta = hmax - hmin;
    int nbin = 20;
    if (nsize < 20) nbin = nsize;
    delta = delta/((double)nbin);
    for (idx = 0; idx<nbin; idx++) {
      value = hmin + delta*((double)idx + 0.5);
      (*x).push_back(value);
      (*y).push_back(0.0);
    }

    // bin values into histogram
    for (idx=0; idx<nsize; idx++) {
      value = (*p_values)[idx];
      value -= hmin;
      ibin = (int)(value/delta);
      // protect against roundoff and account for the value that equals hmax
      if (ibin >= nbin) ibin = nbin-1;
      if (ibin < 0) ibin = 0;
      (*y)[ibin] += 1.0;
    }

    // normalize histogram
//    value = 1.0/(delta*((double)nsize));
//    for (idx = 0; idx<nbin; idx++) {
//      (*y)[idx] *= value;
//    }
  }
}

///////////////////////////////////////////////////////////////////////////////
//
// Evaluate the autocorrelation function of the values in the vector
//
///////////////////////////////////////////////////////////////////////////////
vector<double> PropVector::autocorrelate()
{
  vector<double> ret;
  int nsize = p_values->size();

  if (nsize > 0) {
    vector<double> fluc;
    vector<int> count;


    int idx, j;
    double v_avg = 0.0;

    // Get average value
    for (idx=0; idx<nsize; idx++) {
      v_avg += (*p_values)[idx];
    }
    v_avg /= ((double)nsize);

    // Get fluctuating value
    for (idx=0; idx<nsize; idx++) {
      fluc.push_back((*p_values)[idx]-v_avg);
    }

    // Find number of values to autocorrelate
    int nval = nsize/10;
    if (nval == 0) nval = 1;

    for (idx=0; idx<nval; idx++) {
      ret.push_back(0.0);
      count.push_back(0);
    }

    for (idx=0; idx<nsize; idx++) {
      int max = (nsize > idx+nval) ? idx+nval : nsize;
      for (j=idx; j<max; j++) {
        ret[j-idx] += fluc[idx]*fluc[j];
        count[j-idx]++;
      }
    }

    // normalize data
    v_avg = ret[0]/((double)count[0]);
    if (v_avg != 0.0) {
      v_avg = 1.0/v_avg;
    }

    for (idx=0; idx<nval; idx++) {
      if (count[idx] > 0) {
        ret[idx] = ret[idx]*v_avg/((double)count[idx]);
      }
    }
  }
  return ret;
}
