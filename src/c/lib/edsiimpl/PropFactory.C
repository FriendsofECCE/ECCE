///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: PropFactory.C
//
//
// DESIGN:
//    PropFactory manages a collection of property definitions that
//    are loaded from a text format file.  The reference information
//    allows other codes to create valid properties and to
//    get important information, such as the units for any type of
//    property.  All the functions in PropFactory are static.
// 
//    PropFactory is used to instantiate valid children of the TProperty
//    class
//
///////////////////////////////////////////////////////////////////////////////

//#define DEBUG

// System includes:

// Library includes:

// Application includes:
   #include "util/ErrMsg.H"
   #include "util/LineReader.H"
   #include "util/StringTokenizer.H"
   #include "util/EcceException.H"
   #include "util/UnitFactory.H"
   #include "util/UnitFamily.H"

   #include "dsm/PropFactory.H"
   #include "tdat/PropValue.H"
   #include "tdat/PropString.H"
   #include "tdat/PropVector.H"
   #include "tdat/PropVecString.H"
   #include "tdat/PropTable.H"
   #include "tdat/PropVecTable.H"
   #include "tdat/PropVecGrid.H"
   #include "tdat/PropTSTable.H"
   #include "tdat/PropTSVecTable.H"
   #include "tdat/PropTSVecString.H"
   #include "tdat/PropTSVecVector.H"
   #include "tdat/PropTSVector.H"

   #include "dsm/EDSIServerCentral.H"  // produces URL(s) to web server(s)
   #include "dsm/EDSIFactory.H"
   #include "dsm/EDSI.H"

   #include "util/Ecce.H"

// Static initialization:
   vector<Property_Ref> PropFactory::propRefList;

//----------------------
//        Public
//----------------------

///////////////////////////////////////////////////////////////////////////////
// man
//
// Description
//    Destructor
//
///////////////////////////////////////////////////////////////////////////////
PropFactory::~PropFactory(void)
{ }  


///////////////////////////////////////////////////////////////////////////////
// man
//
// Description
//    Locate the given property in the Property_Refs vector and create the
//    appropriate TProperty subclass.  Also check the units entered to
//    ensure that they are valid.
//
// Implementation
//    Return NULL if the property type specified (i.e., shortName) is
//    invalid.  Use default units if the units are not specified or are
//    invalid.
///////////////////////////////////////////////////////////////////////////////

TProperty* PropFactory::getProperty(const string& shortName)
{
// Make sure propRef List has been loaded from file:
// Only load list the list if vector is empty
   if (propRefList.empty())
      initialize();

// Find shortName in propRefList
   size_t i;
   for (i = 0; i < propRefList.size(); i++)
      if (propRefList[i].short_name == shortName)
         break;

// Halt if shortName (i.e., class type) was not found in propRefList 
   if (i == propRefList.size())
   {
      ErrMsg().message("EE_PROP_INVALID", EE_FATAL, WHERE, shortName.c_str());
      return 0;
   } 

// Assign units (validation checking has been removed):
   string unitClass = propRefList[i].units;
   string validUnits = "";
   if (unitClass != "") { // use default units
      UnitFactory unitFactory = UnitFactory::getInstance();
      UnitFamily family = unitFactory.getUnitFamily("AU");
      try {
         validUnits = family.get(unitClass.c_str());
      } catch (InvalidException &ex) {
         validUnits = "";
      }
   }

// Instantiate appropriate TProperty subclass corresponding to the
// given property:
   TProperty* property;

   switch(propRefList[i].classType)
   {
     case TProperty::PROPVALUE :
        property =  new PropValue(shortName, validUnits); break;
     case TProperty::PROPSTRING :
        property =  new PropString(shortName, validUnits); break;
     case TProperty::PROPVECTOR :
        property =  new PropVector(shortName, validUnits); break;
     case TProperty::PROPVECSTRING :
        property =  new PropVecString(shortName, validUnits); break;
     case TProperty::PROPTABLE :
        property =  new PropTable(shortName, validUnits); break;
     case TProperty::PROPVECTABLE :
        property =  new PropVecTable(shortName, validUnits); break;
     case TProperty::PROPVECGRID :
        property =  new PropVecGrid(shortName, validUnits); break;
     case TProperty::PROPTSVECTABLE :
        property =  new PropTSVecTable(shortName, validUnits); break;
     case TProperty::PROPTSTABLE :
        property =  new PropTSTable(shortName, validUnits); break;
     case TProperty::PROPTSVECSTRING :
        property =  new PropTSVecString(shortName, validUnits); break;
     case TProperty::PROPTSVECTOR :
        property =  new PropTSVector(shortName, validUnits); break;
     case TProperty::PROPTSVECVECTOR :
        property =  new PropTSVecVector(shortName, validUnits); break;
     case TProperty::SUPER:
     default:
        property = NULL;
   };
   return property;
}

///////////////////////////////////////////////////////////////////////////////
// man
//
// Description
//   Return the Property_Ref structure for the given key.
//   Return NULL if given type is invalid.
///////////////////////////////////////////////////////////////////////////////
const Property_Ref* PropFactory::getPropRef(string shortName)
{

// Make sure propRefList has been loaded from file:
   if (propRefList.empty())
      initialize();

// Find shortName in propRefList
   size_t i;
   for (i = 0; i < propRefList.size(); i++) {
      if (propRefList[i].short_name == shortName) {
         break;
      }
   }

// warn if shortName (i.e., class type) was not found in propRefList 
// don't put on queue because applications aren't going to be looking for
// it.
   if (i == propRefList.size())
   {
#ifdef DEBUG
     EE_ASSERT(FALSE, EE_WARNING, shortName);
#endif
     return 0;
   } 

   return &propRefList[i];
}

///////////////////////////////////////////////////////////////////////////////
// man
//
// Description
//   Returns an ordered list of all the ProperyRefs that match the specified
//   classification (e.g. all Energy properties)
//
//   Resulting set is ordered by short_name.
//   Returns empty vector if no match found.
///////////////////////////////////////////////////////////////////////////////
set<Property_Ref, less<Property_Ref> > PropFactory::ofClassification(
                                     const string& classification)
{
// Create empty set:
   set<Property_Ref, less<Property_Ref> > result;
 
// Make sure propRefList has been loaded from file:
   if (propRefList.empty())
      initialize();

// Build set by matching classification in propRefList
   for (size_t i = 0; i < propRefList.size(); i++)
      if (propRefList[i].classification == classification)
	result.insert(propRefList[i]);

   return result;
}


///////////////////////////////////////////////////////////////////////////////
// man
//
// Description
//   Returns an ordered list of all the ProperyRefs that match the specified
//   dbclass storage type (e.g. all PropTSVectors)
//
//   Resulting set is ordered by short_name.
//   Returns empty vector if no match found.
///////////////////////////////////////////////////////////////////////////////
set<Property_Ref, less<Property_Ref> > PropFactory::ofStorageType(
                                     const string& storagetype)
{
// Create empty set:
   set<Property_Ref, less<Property_Ref> > result;
 
// Make sure propRefList has been loaded from file:
   if (propRefList.empty())
     initialize();

// Build set by matching storagetype in propRefList
   for (size_t i = 0; i < propRefList.size(); i++)
     if (propRefList[i].storagetype() == storagetype)
	     result.insert(propRefList[i]);

   return result;
}



///////////////////////////////////////////////////////////////////////////////
// man
//
// Description
//   Returns a unique, sorted list of all known units:
//
///////////////////////////////////////////////////////////////////////////////
set<string, less<string> > PropFactory::knownUnits(void)
{
// Create empty set:
   set<string, less<string> > result;
 
// Make sure propRefList has been loaded from file:
   if (propRefList.empty())
      initialize();

   for (size_t i = 0; i < propRefList.size(); i++)
     result.insert(propRefList[i].units);

   return result;    
}

/**
 * Returns a list of all the property refs that match the given
 * list of dimensions.
 *
 * Only valid for properties with dimension >= 1 (i.e. no PropValue.)
 */
vector<Property_Ref> PropFactory::propsIndexedBy(const vector<string>& dims)
{
  vector<Property_Ref> result;

  if (propRefList.empty()) {
    initialize();
  }

  for (size_t i = 0; i < propRefList.size(); ++i) {
    if (propRefList[i].index.size() >= dims.size()) {
      size_t j;
      for (j = 0; j < dims.size(); ++j) {
        if (dims[j] != propRefList[i].index[j]) {
          break;
        }
      }
      if (j == dims.size()) {
        result.push_back(propRefList[i]);
      }
    }
  }

  return result;
}

vector<Property_Ref> PropFactory::propsIndexedBy(const string& first,
        const string& second, const string& third)
{
  vector<string> dims;
  dims.push_back(first);
  if (!second.empty()) dims.push_back(second);
  if (!third.empty()) dims.push_back(third);
  return propsIndexedBy(dims);
}

set<Property_Ref, less<Property_Ref> > PropFactory::ofIndex(
        const vector<string>& dims)
{
  set<Property_Ref, less<Property_Ref> > result;

  vector<Property_Ref> vector_result = propsIndexedBy(dims);
  vector<Property_Ref>::iterator it;
  for (it = vector_result.begin(); it != vector_result.end(); ++it) {
    result.insert(*it);
  }

  return result;
}


set<Property_Ref, less<Property_Ref> > PropFactory::ofIndex(const string& first,
        const string& second, const string& third)
{
  set<Property_Ref, less<Property_Ref> > result;

  vector<Property_Ref> vector_result = propsIndexedBy(first, second, third);
  vector<Property_Ref>::iterator it;
  for (it = vector_result.begin(); it != vector_result.end(); ++it) {
    result.insert(*it);
  }

  return result;
}

///////////////////////////////////////////////////////////////////////////////
// man
//
// Description
//   Returns the name of the storage structure based on the
//   TProperty type.   Since all properties are stored as XML documents
//   the returned value will be used as the root element of the property 
//   document.
//
///////////////////////////////////////////////////////////////////////////////
string Property_Ref::storagetype() const {
  string ret = "";
  switch (classType)  {

  case    TProperty::PROPVALUE :       // single value
    ret = "value"; break;
  case    TProperty::PROPSTRING :      // single value
    ret = "string"; break;
  case    TProperty::PROPVECTOR :      // 1-d vector of values
    ret = "vector"; break;
  case    TProperty::PROPTABLE :       // 2-d table of values
    ret = "table"; break;
  case    TProperty::PROPVECTABLE :    // 3-d vector of tables
    ret = "vectable"; break;
  case    TProperty::PROPVECGRID :     // grid of values
    ret = "vecgrid"; break;
  case    TProperty::PROPVECSTRING :   // 1-d vector of strings
    ret = "vecstring"; break;
  case    TProperty::PROPTSVECSTRING : // 1-d vector of strings
    ret = "tsvecstring"; break;
  case    TProperty::PROPTSTABLE :     // 3-d vector of tables
    ret = "tstable"; break;
  case    TProperty::PROPTSVECTABLE :  // 3-d vector of tables
    ret = "tsvectable"; break;
  case    TProperty::PROPTSVECVECTOR : // 3-d vector of vectors
    ret = "tsvecvector"; break;
  case    TProperty::PROPTSVECTOR :    // 1-d vector of values
    ret = "tsvector"; break;
  case    TProperty::SUPER :           // 1-d vector of values
    ret = "super"; break;
  }
  return ret;
}

//----------------------
//        Private
//----------------------

///////////////////////////////////////////////////////////////////////////////
//
// Description
//    Load the static RropertyRef vector from the "props" file
//    stored on the DAV server in the /Ecce/v2.0 directory.
//
// Implementation
//    We verify that the text file can be opened and then 
//    iterate through the file reading records, creating Property_Ref
//    objects, and adding them to the propRefList vector.
//
//    Classes KeyValueReader
//    StringTokenizer are used to parse the file.
//
//    ** This function should only be called once, when the propRefList
//    vector is empty.
///////////////////////////////////////////////////////////////////////////////
void PropFactory::initialize() throw (EcceException)
{
// Open file containing Property_Ref data:
// Set up connection to DAV server via EDSI interface:

   EDSIServerCentral servers;
   EcceURL url = servers.getConfigLocation();
   url = url.getChild("properties");

// Make virtual connection to URL via EDSI:
   EDSI* connection = EDSIFactory::getEDSI(url);
   
// Read props file from DAV server:
// (cast istream* as ifstream* so it can be used in KeyValueReader)
   ifstream* inFile = (ifstream*) connection->getDataSet();

// Throw exception if file unable to be opened:
   if (inFile == 0) {
     string msg = "Unable to open properties database.";
     msg.append(connection->m_msgStack.getMessage());
     throw EcceException(msg, WHERE);
   }

// Parse file using LineReader and StringTokenizer, and build the
// propRefList vector:

   const char *sep = "\t\n";
   LineReader reader(*inFile);
   string line,classification,name,key,units,dbclass;
   Property_Ref propRef;

   while (reader.getLine(line)) {
     StringTokenizer column(line);

     classification = column.next(sep);
     name = column.next(sep);
     key = column.next(sep);
     units = column.next(sep);
     dbclass = column.next(sep);

     if (classification.empty() || name.empty() || key.empty() ||
         units.empty() || dbclass.empty())  {
       string msg = "Error parsing properties database: ";
       msg.append(line);
       throw EcceException(msg, WHERE);
     }

     propRef.classification = classification.c_str();
     propRef.long_name = name.c_str();
     propRef.short_name = key.c_str();
     propRef.units = units.c_str();
     propRef.dbclass = dbclass.c_str();

     // convert dbclass string to enumerated value
     if (propRef.dbclass.substr(0,9) == "PropValue")
       propRef.classType = TProperty::PROPVALUE;
     if (propRef.dbclass.substr(0,10) == "PropString")
       propRef.classType = TProperty::PROPSTRING;
     else if (propRef.dbclass.substr(0,10) == "PropVector") 
       propRef.classType = TProperty::PROPVECTOR;
     else if (propRef.dbclass.substr(0,12) == "PropTSVector")
       propRef.classType = TProperty::PROPTSVECTOR;
     else if (propRef.dbclass.substr(0,9) == "PropTable") 
       propRef.classType = TProperty::PROPTABLE;
     else if (propRef.dbclass.substr(0,11) == "PropTSTable")
       propRef.classType = TProperty::PROPTSTABLE;
     else if (propRef.dbclass.substr(0,12) == "PropVecTable")
       propRef.classType = TProperty::PROPVECTABLE;
     else if (propRef.dbclass.substr(0,11) == "PropVecGrid")
       propRef.classType = TProperty::PROPVECGRID;
     else if (propRef.dbclass.substr(0,14) == "PropTSVecTable")
       propRef.classType = TProperty::PROPTSVECTABLE;
     else if (propRef.dbclass.substr(0,15) == "PropTSVecVector")
       propRef.classType = TProperty::PROPTSVECVECTOR;
     else if (propRef.dbclass.substr(0,15) == "PropTSVecString")
       propRef.classType = TProperty::PROPTSVECSTRING;
     else if (propRef.dbclass.substr(0,13) == "PropVecString")
       propRef.classType = TProperty::PROPVECSTRING;

     // parse dbclass for indices
     propRef.index.clear();
     size_t index = dbclass.find("<");
     if (index != string::npos && index > 0) {
       string result = dbclass;
       // extract stuff between <> characters
       result = result.substr(index+1);
       result = result.substr(0,result.size()-1);
       StringTokenizer tokenizer(result, ",");
       propRef.index = tokenizer.tokenize(",");
     }

     propRefList.push_back(propRef);

   }

   delete (istream*) inFile; // getDataSet returns an istream*, so we must
                             // only delete this kind of object
   delete connection;
}

/**********************************************************
  Property_Ref Accessors
***********************************************************/

bool Property_Ref::canGraph(void) const
{
  return (dbclass.find("PropTSVector") != string::npos);
}

bool Property_Ref::canLabel(void) const
{ return (dbclass.find("PropVector<Atom>") != string::npos); }

int Property_Ref::dimensions(void) const
{
  // alternative implementation is to have
  // return index.size();
  switch(classType)
  {
     case TProperty::PROPSTRING      : 
     case TProperty::PROPVALUE       : return 0;
     case TProperty::PROPVECTOR      :
     case TProperty::PROPTSVECTOR    :
     case TProperty::PROPVECSTRING   :
     case TProperty::PROPTSVECSTRING : return 1;
     case TProperty::PROPTSVECVECTOR :
     case TProperty::PROPTSTABLE     :
     case TProperty::PROPTABLE       : return 2;
     case TProperty::PROPVECGRID     :
     case TProperty::PROPVECTABLE    : 
     case TProperty::PROPTSVECTABLE  : return 3;
     default : EE_ASSERT(0, EE_FATAL, "unknown property class" + dbclass);
               return -1;
  };
}

/**
 * Returns the name of the parameter comprising that dimension of the property.
 *
 * Not valid for properties of dimension 0.  For example, dimension(1) of
 * DBVecTable<Geometry Step,Atom,Coordinate> would return "Geometry Step",
 * 2 -> "Atom", 3 -> "Coordinate".
 *
 * Index out of bounds returns "Invalid".
 */
string Property_Ref::dimension(size_t dim) const
{
  string result = "Invalid";
  size_t zero = 0;

  if (!index.empty() && dim != zero && dim <= index.size()) {
    result = index[dim-1];
  }

  return result;
}

bool operator<(const Property_Ref& ref1, const Property_Ref& ref2)
{
  return (ref1.short_name < ref2.short_name);
}

ostream& operator<<(ostream& os, const Property_Ref& propref)
{
  os << "classification: " << propref.classification << endl
     << "long_name: " << propref.long_name << endl
     << "short_name: " << propref.short_name << endl
     << "units: " << propref.units << endl
     << "dbclass: " << propref.dbclass << endl
     << "classtype: " << TProperty::classTypeToStr(propref.classType) << endl;
  return os;
}





