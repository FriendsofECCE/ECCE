#include <string.h>

#include <iostream>
  using std::cout;
  using std::endl;

#include "util/ErrMsg.H"
#include "util/TDateTime.H"
#include "util/ETimer.H"

#include "tdat/TProperty.H"
#include "dsm/ActivityLog.H"
#include "dsm/PropFactory.H"
#include "dsm/PropertyInterpreter.H"
#include "dsm/PropertyDoc.H"

//#define timertest

PropertyInterpreter::PropertyInterpreter() {
  p_ecceJobLogAll = false;
  if (getenv("ECCE_LOG_ALL_EVENTS"))
    p_ecceJobLogAll = true;
}
PropertyDoc* PropertyInterpreter::parse(istream& propStream, const string& key, string& log,  const unsigned long step) {

  PropertyDoc *ret = 0;

  //Am I a property?  If not don't process.
 const Property_Ref* propRef = PropFactory::getPropRef(key);
  if (!propRef ) 
  {
    TDateTime date;
    string message = "No reference for property ";
    message += key;
    message += " found.  Unable to parse request."; 
    log += ActivityLog::entry(ActivityLog::ERROR, 
           "Reference Not Found", date, message.c_str()); 
  }

  if (propRef) 
    ret = processProperty(propRef , propStream, log, step);
  return ret;
}

//
//  Private function used for setting units of cache entry
//

string PropertyInterpreter::processUnits(istream &propStream, PropertyDoc* entry ) {

  string token;

  // Fill units string with units value parsed from the input stream 
  // If not at END
  propStream >> token;
  if ((propStream.good()) && (token.rfind("END") == string::npos) ) {
    entry->units(token.c_str());
    propStream >> token;
  }  

  // Return next token
  return token;
}
//
//  Private function used for setting dimensions of cache entry
//

string PropertyInterpreter::processSize(istream &propStream, PropertyDoc* entry ) {
  string token;

  vector<string> containerDim;


  //
  //  Process input until you reach the size token.
  //
  while (propStream.good() && (token.rfind("size:") == string::npos)) propStream >> token;

  //
  // Fill container array with dimension values parsed from the input stream.
  //
  propStream >> token;
  while ((token.rfind("values") == string::npos) && (token.rfind("label") == string::npos))
  {
    containerDim.push_back(token);
    propStream >> token;
  }  


   //
   //  Set size and size label attributes based on the tokens read from the stream.
   //
   switch(entry->reference()->classType)
   {
     case TProperty::PROPSTRING      :
     case TProperty::PROPVALUE       : break; 
     case TProperty::PROPVECTOR      :
     case TProperty::PROPVECSTRING   :
     case TProperty::PROPTSVECTOR      :
     case TProperty::PROPTSVECSTRING   :
     {
       entry->rows( containerDim[0].c_str());
       break;
     }
     case TProperty::PROPTSTABLE       : 
     {
       entry->rows( containerDim[1].c_str());
       entry->columns( containerDim[2].c_str());
       break;
     }
     case TProperty::PROPTSVECVECTOR   : 
     {
       entry->rows( containerDim[1].c_str());
       break;
     }
     case TProperty::PROPTABLE         : 
     {
       entry->rows( containerDim[0].c_str());
       entry->columns( containerDim[1].c_str());
       break;
     }
     case TProperty::PROPTSVECTABLE    :
     case TProperty::PROPVECTABLE      :
     {
       entry->vectors( containerDim[0].c_str());
       entry->rows( containerDim[1].c_str());
       entry->columns( containerDim[2].c_str());
       break;
     }
     case TProperty::PROPVECGRID       :
     case TProperty::SUPER             :
       break;
    }  
  return token;
}

//
//  Private function used for returning a formatted string of labels for a cache entry
//

string PropertyInterpreter::processLabels(istream &propStream, PropertyDoc* entry, const string& labelType) {
  
  string token, labels;
  string llabelType = labelType;

  while (propStream.good() && (token.rfind("values:") == string::npos )) {
    labels = "";
    while (propStream.good() && (token.rfind("label") == string::npos) &&
          (token.rfind("values:") == string::npos)) {

      if (!labels.empty())
        labels += " ";
      labels  += token;       
      propStream >> token;


    }
    if (llabelType.rfind("row") != string::npos)
      entry->rowLabels(labels.c_str());
    else if (llabelType.rfind("column") != string::npos)
      entry->columnLabels(labels.c_str());
    else if (llabelType.rfind("vector") != string::npos)
      entry->vectorLabels(labels.c_str());

      if (token.rfind("label") != string::npos)
        llabelType = token;

    if (token.rfind("values:") != string::npos)
      break;
    propStream >> token;
  }
  return token;
}

//
//  Private function used for loading and formatted string of values for 
//  a cache entry.  This needs to be fast since files can be 10s of MB.
//  strtok is the only way to go for speed.  I did quite a few tests on
//  this to arrive at this conclusion.  Reading blocks of data might also
//  improve speed but then you have to worry about word bounderies.  This
//  should be good enough.
//

string 
PropertyInterpreter::processValues(istream& propStream, PropertyDoc* entry) {
#ifdef timertest
  ETimer timer;
  timer.start();
#endif

  const int MAXBUF = 2048;
  const int maxColumns = 4;
  string valueString;
  string token;
  int columnIndex = 1;
  char buf[MAXBUF];
  bool done = false;
  char *tag = 0;
  while (propStream.good() && !done) {

    propStream.getline(buf,MAXBUF);

    // This is here to help us notice when the parse scripts generate
    // lines too long to be read and processed correctly.  Alternatively,
    // we should modify this code to handle infinitly long lines.
    // TODO this should be flagged as an error condition and the job
    // should go to monitor error.
    if (strlen(buf) == MAXBUF-1) {
      cerr << "Line overflow " << entry->name() << buf << endl;
    }
    if (entry->reference()->classType != TProperty::PROPSTRING)
    {
      tag = strtok(buf," \t\n");
    }
    else
    {
      if ((strcmp(buf,"END") == 0) || (strstr(buf,"units:"))) {
        done = true;
        token = buf;
      }
      else {
        valueString.append(buf);
      }
    }
    while (tag && entry->reference()->classType != TProperty::PROPSTRING) {

      // Watch for the end or units tag
      if ((strcmp(tag,"END") == 0) || (strstr(tag,"units:"))) {
        done = true;
        token = buf;
        break;

      } else {

        if (!valueString.empty())
        {
          if (columnIndex >= maxColumns ) {
            valueString += "\n";
            columnIndex = 1;
          } else {
            if (!valueString.empty())
              valueString.append("   ");
            columnIndex++;
          }
        }
        valueString.append(tag);
        tag = strtok((char*)0, " \t\n");
      }
    } 
  }
#ifdef timertest
  timer.stop();
  cout << "time to read tokens as string " << timer.elapsedTime() << endl;
  timer.start();
#endif
  entry->values(valueString.c_str());

#ifdef timertest
  timer.stop();
  cout << "time to set value" << timer.elapsedTime() << endl;
#endif
  return token;
}

//
//  Private function used for returning a formatted string of values for a PropVecTable
//  entry.  This is a special case since the steps are generated in one dump from the
//  jobmonitor. 
//

string PropertyInterpreter::processVecTableValues(istream& propStream, PropertyDoc* entry) {

  string valueString, token;
  unsigned long numRows = 0, numColumns = 0 , numVectors = 0 ;
  unsigned long rowIndex = 0, columnIndex = 0 , vectorIndex = 0;
  char *pn = NULL;

  pn = entry->rows();
  numRows = atol(pn);
  delete pn;

  pn = entry->columns();
  numColumns = atol(pn);
  delete pn;

  pn = entry->vectors();
  numVectors = atol(pn);
  delete pn;

  for (vectorIndex = 0; vectorIndex < numVectors; vectorIndex++) {
    for (rowIndex = 0; rowIndex < numRows; rowIndex++) {
      for (columnIndex = 0; columnIndex < numColumns; columnIndex++) {
        if (propStream.good() && (token.rfind("END") == string::npos) 
                              && (token.rfind("units:") == string::npos)) 
        {
          propStream >> token;
          if ((token.rfind("END") == string::npos) && 
              (token.rfind("units:") == string::npos)) {
            if (!valueString.empty()) valueString += "\n";
          } else
            break;
          valueString += token;
        }
        else 
          break;
      } 
    }
    entry->createMatrix(vectorIndex + 1);
    entry->values(valueString.c_str());
    valueString = ""; 
  }
  return token;
}

bool PropertyInterpreter::validatePropertyDoc(
                          PropertyDoc* propdoc,
                          string& os)
{
  bool ret = true;
   if (!propdoc->name())
   {
     ret = false;
     TDateTime date;
     string message = "Property XML document is invalid.  ";
     message += "No 'name' attribute was processed ";
     message += " found.  Unable to parse request.";
     os += ActivityLog::entry(ActivityLog::ERROR,
            "Parsing", date, message.c_str());
   } else if (!propdoc->units())
   {
     ret = false;
     TDateTime date;
     string message = "Property XML document is invalid.  ";
     message += "No 'units' attribute was processed for ";
     message += propdoc->name();
     message += ".   Unable to parse request.";
     os += ActivityLog::entry(ActivityLog::ERROR,
            "Parsing", date, message.c_str());
   } else if (!propdoc->values())
   {
     ret = false;
     TDateTime date;
     string message = "Property XML document is invalid.  ";
     message += "No values were processed for ";
     message += propdoc->name();
     message += ".   Unable to parse request.";
     os += ActivityLog::entry(ActivityLog::ERROR,
            "Parsing", date, message.c_str());

   }
   if (ret == false)
     return ret;

   switch(propdoc->reference()->classType)
   {
     case TProperty::PROPSTRING        :
     case TProperty::PROPVALUE         :   break;
     case TProperty::PROPVECTOR        :
     {
       if (!propdoc->rows())
       {
         ret = false;
         TDateTime date;
         string message = "Property XML document is invalid.  ";
         message += "No 'rows' attribute was processed for ";
         message += propdoc->name();
         message += ".   Unable to parse request.";
         os += ActivityLog::entry(ActivityLog::ERROR,
         "Parsing", date, message.c_str());

       } else 
       {
         unsigned long rows = atol(propdoc->rows()); 
         if (rows <= 0)
         {
           ret = false;
           TDateTime date;
           string message = "Property XML document is invalid.  ";
           message += "'rows' attribute = ";
           message += propdoc->rows();
           message += " for ";
           message += propdoc->name();
           message += ".   Unable to parse request.";
           os += ActivityLog::entry(ActivityLog::ERROR,
           "Parsing", date, message.c_str());
         }
       }
       if (!propdoc->rowLabel())
       {
         ret = false;
           TDateTime date;
           string message = "Property XML document is invalid.  ";
           message += "No 'rowLabel' attribute was processed for ";
           message += propdoc->name();
           message += ".   Unable to parse request.";
           os += ActivityLog::entry(ActivityLog::ERROR,
           "Parsing", date, message.c_str());

       }
#if (!defined(INSTALL) && defined(DEBUG))

       if (!propdoc->rowLabels())
       {
         TDateTime date;
         string message = "Property XML document is invalid.  ";
         message += "No 'rowLabels' attribute was processed for ";
         message += propdoc->name();
         message += ".   Unable to parse request.";
         os += ActivityLog::entry(ActivityLog::WARNING,
         "Parsing", date, message.c_str());
       }
#endif
       break;
     }
     case TProperty::PROPVECSTRING     :
     {
       if (!propdoc->rows())
       {
         ret = false;
         TDateTime date;
         string message = "Property XML document is invalid.  ";
         message += "No 'rows' attribute was processed for ";
         message += propdoc->name();
         message += ".   Unable to parse request.";
         os += ActivityLog::entry(ActivityLog::ERROR,
         "Parsing", date, message.c_str());

       } else 
       {
         unsigned long rows = atol(propdoc->rows()); 
         if (rows <= 0)
         {
           ret = false;
           TDateTime date;
           string message = "Property XML document is invalid.  ";
           message += "'rows' attribute = ";
           message += propdoc->rows();
           message += " for ";
           message += propdoc->name();
           message += ".   Unable to parse request.";
           os += ActivityLog::entry(ActivityLog::ERROR,
           "Parsing", date, message.c_str());
         }
       }
       break;
     }
     case TProperty::PROPTABLE         :
     {
       if (!propdoc->rows())
       {
         ret = false;
         TDateTime date;
         string message = "Property XML document is invalid.  ";
         message += "No 'rows' attribute was processed for ";
         message += propdoc->name();
         message += ".   Unable to parse request.";
         os += ActivityLog::entry(ActivityLog::ERROR,
         "Parsing", date, message.c_str());

       } else
       {
         unsigned long rows = atol(propdoc->rows());
         if (rows <= 0)
         {
           ret = false;
           TDateTime date;
           string message = "Property XML document is invalid.  ";
           message += "'rows' attribute = ";
           message += propdoc->rows();
           message += " for ";
           message += propdoc->name();
           message += ".   Unable to parse request.";
           os += ActivityLog::entry(ActivityLog::ERROR,
           "Parsing", date, message.c_str());

         }
       }

       if (!propdoc->columns())
       {
         ret = false;
         TDateTime date;
         string message = "Property XML document is invalid.  ";
         message += "No 'columns' attribute was processed for ";
         message += propdoc->name();
         message += ".   Unable to parse request.";
         os += ActivityLog::entry(ActivityLog::ERROR,
         "Parsing", date, message.c_str());

       } else
       {
         unsigned long columns = atol(propdoc->columns());
         if (columns <= 0)
         {
           ret = false;
           TDateTime date;
           string message = "Property XML document is invalid.  ";
           message += "'columns' attribute = ";
           message += propdoc->columns();
           message += " for ";
           message += propdoc->name();
           message += ".   Unable to parse request.";
           os += ActivityLog::entry(ActivityLog::ERROR,
           "Parsing", date, message.c_str());

         }
       }
       if (!propdoc->columnLabel())
       {
         ret = false;
         TDateTime date;
         string message = "Property XML document is invalid.  ";
         message += "No 'columnLabel' attribute was processed for ";
         message += propdoc->name();
         message += ".   Unable to parse request.";
         os += ActivityLog::entry(ActivityLog::WARNING,
         "Parsing", date, message.c_str());
       }
       if (!propdoc->columnLabels())
       {
         ret = false;
         TDateTime date;
         string message = "Property XML document is invalid.  ";
         message += "No 'columnLabels' attribute was processed for ";
         message += propdoc->name();
         message += ".   Unable to parse request.";
         os += ActivityLog::entry(ActivityLog::WARNING,
         "Parsing", date, message.c_str());

       }

       if (!propdoc->rowLabel())
       {
         ret = false;
         TDateTime date;
         string message = "Property XML document is invalid.  ";
         message += "No 'rowLabel' attribute was processed for ";
         message += propdoc->name();
         message += ".   Unable to parse request.";
         os += ActivityLog::entry(ActivityLog::ERROR,
         "Parsing", date, message.c_str());
       }
       if (!propdoc->rowLabels())
       {
         ret = false;
         TDateTime date;
         string message = "Property XML document is invalid.  ";
         message += "No 'rowLabels' attribute was processed for ";
         message += propdoc->name();
         message += ".   Unable to parse request.";
         os += ActivityLog::entry(ActivityLog::ERROR,
         "Parsing", date, message.c_str());

       }
       break;
     }
     case TProperty::PROPVECTABLE      :
     {

       if (!propdoc->rows())
       {
         ret = false;
         TDateTime date;
         string message = "Property XML document is invalid.  ";
         message += "No 'rows' attribute was processed for ";
         message += propdoc->name();
         message += ".   Unable to parse request.";
         os += ActivityLog::entry(ActivityLog::ERROR,
         "Parsing", date, message.c_str());

       } else
       {
         unsigned long rows = atol(propdoc->rows());
         if (rows <= 0)
         {
           ret = false;
           TDateTime date;
           string message = "Property XML document is invalid.  ";
           message += "'rows' attribute = ";
           message += propdoc->rows();
           message += " for ";
           message += propdoc->name();
           message += ".   Unable to parse request.";
           os += ActivityLog::entry(ActivityLog::ERROR,
           "Parsing", date, message.c_str());
         }
       }
       if (!propdoc->rowLabel())
       {
         ret = false;
           TDateTime date;
           string message = "Property XML document is invalid.  ";
           message += "No 'rowLabel' attribute was processed for ";
           message += propdoc->name();
           message += ".   Unable to parse request.";
           os += ActivityLog::entry(ActivityLog::ERROR,
           "Parsing", date, message.c_str());

       }
       if (!propdoc->rowLabels())
       {
         TDateTime date;
         string message = "Property XML document is invalid.  ";
         message += "No 'rowLabels' attribute was processed for ";
         message += propdoc->name();
         message += ".   Unable to parse request.";
         os += ActivityLog::entry(ActivityLog::WARNING,
         "Parsing", date, message.c_str());
       }

       if (!propdoc->columns())
       {
         ret = false;
         TDateTime date;
         string message = "Property XML document is invalid.  ";
         message += "No 'columns' attribute was processed for ";
         message += propdoc->name();
         message += ".   Unable to parse request.";
         os += ActivityLog::entry(ActivityLog::ERROR,
         "Parsing", date, message.c_str());

       } else
       {
         unsigned long columns = atol(propdoc->columns());
         if (columns <= 0)
         {
           ret = false;
           TDateTime date;
           string message = "Property XML document is invalid.  ";
           message += "'columns' attribute = ";
           message += propdoc->columns();
           message += " for ";
           message += propdoc->name();
           message += ".   Unable to parse request.";
           os += ActivityLog::entry(ActivityLog::ERROR,
           "Parsing", date, message.c_str());

         }
       }
       if (!propdoc->columnLabel())
       {
         ret = false;
         TDateTime date;
         string message = "Property XML document is invalid.  ";
         message += "No 'columnLabel' attribute was processed for ";
         message += propdoc->name();
         message += ".   Unable to parse request.";
         os += ActivityLog::entry(ActivityLog::WARNING,
         "Parsing", date, message.c_str());
       }
       if (!propdoc->columnLabels())
       {
         ret = false;
         TDateTime date;
         string message = "Property XML document is invalid.  ";
         message += "No 'columnLabels' attribute was processed for ";
         message += propdoc->name();
         message += ".   Unable to parse request.";
         os += ActivityLog::entry(ActivityLog::WARNING,
         "Parsing", date, message.c_str());

       }

       if (!propdoc->vectorLabel())
       {
         ret = false;
         TDateTime date;
         string message = "Property XML document is invalid.  ";
         message += "No 'vectorLabel' attribute was processed for ";
         message += propdoc->name();
         message += ".   Unable to parse request.";
         os += ActivityLog::entry(ActivityLog::WARNING,
         "Parsing", date, message.c_str());
       }
       if (!propdoc->vectorLabels())
       {
         ret = false;
         TDateTime date;
         string message = "Property XML document is invalid.  ";
         message += "No 'vectorLabels' attribute was processed for ";
         message += propdoc->name();
         message += ".   Unable to parse request.";
         os += ActivityLog::entry(ActivityLog::WARNING,
         "Parsing", date, message.c_str());

       }
       break;
     }
     case TProperty::PROPTSVECTOR      :
     {
       if (!propdoc->rowLabel())
       {
         ret = false;
         TDateTime date;
         string message = "Property XML document is invalid.  ";
         message += "No 'rowLabel' attribute was processed for ";
         message += propdoc->name();
         message += ".   Unable to parse request.";
         os += ActivityLog::entry(ActivityLog::ERROR,
         "Parsing", date, message.c_str());
       }

       if (propdoc->lastStep() == 0)
       {
         ret = false;
         TDateTime date;
         string message = "Property XML document is invalid.  ";
         message += "No time step element(s) found for ";
         message += propdoc->name();
         message += ".   Unable to parse request.";
         os += ActivityLog::entry(ActivityLog::ERROR,
         "Parsing", date, message.c_str());
       }
     }
       break;
     case TProperty::PROPTSVECSTRING   :
     {
       if (propdoc->lastStep() == 0)
       {
         ret = false;
         TDateTime date;
         string message = "Property XML document is invalid.  ";
         message += "No time step element(s) found for ";
         message += propdoc->name();
         message += ".   Unable to parse request.";
         os += ActivityLog::entry(ActivityLog::ERROR,
         "Parsing", date, message.c_str());
       }
       break;
     }
     case TProperty::PROPTSTABLE       :
     case TProperty::PROPTSVECVECTOR   : break;
     case TProperty::PROPTSVECTABLE    :
     {

       if (!propdoc->rows())
       {
         ret = false;
         TDateTime date;
         string message = "Property XML document is invalid.  ";
         message += "No 'rows' attribute was processed for ";
         message += propdoc->name();
         message += ".   Unable to parse request.";
         os += ActivityLog::entry(ActivityLog::ERROR,
         "Parsing", date, message.c_str());

       } else
       {
         unsigned long rows = atol(propdoc->rows());
         if (rows <= 0)
         {
           ret = false;
           TDateTime date;
           string message = "Property XML document is invalid.  ";
           message += "'rows' attribute = ";
           message += propdoc->rows();
           message += " for ";
           message += propdoc->name();
           message += ".   Unable to parse request.";
           os += ActivityLog::entry(ActivityLog::ERROR,
           "Parsing", date, message.c_str());
         }
       }

       if (!propdoc->rowLabel())
       {
         ret = false;
           TDateTime date;
           string message = "Property XML document is invalid.  ";
           message += "No 'rowLabel' attribute was processed for ";
           message += propdoc->name();
           message += ".   Unable to parse request.";
           os += ActivityLog::entry(ActivityLog::ERROR,
           "Parsing", date, message.c_str());

       }
       if (!propdoc->rowLabels())
       {
         TDateTime date;
         string message = "Property XML document is invalid.  ";
         message += "No 'rowLabels' attribute was processed for ";
         message += propdoc->name();
         message += ".   Unable to parse request.";
         os += ActivityLog::entry(ActivityLog::WARNING,
         "Parsing", date, message.c_str());
       }

       if (!propdoc->columns())
       {
         ret = false;
         TDateTime date;
         string message = "Property XML document is invalid.  ";
         message += "No 'columns' attribute was processed for ";
         message += propdoc->name();
         message += ".   Unable to parse request.";
         os += ActivityLog::entry(ActivityLog::ERROR,
         "Parsing", date, message.c_str());

       } else
       {
         unsigned long columns = atol(propdoc->columns());
         if (columns <= 0)
         {
           ret = false;
           TDateTime date;
           string message = "Property XML document is invalid.  ";
           message += "'columns' attribute = ";
           message += propdoc->columns();
           message += " for ";
           message += propdoc->name();
           message += ".   Unable to parse request.";
           os += ActivityLog::entry(ActivityLog::ERROR,
           "Parsing", date, message.c_str());

         }
       }
       if (!propdoc->columnLabel())
       {
         ret = false;
         TDateTime date;
         string message = "Property XML document is invalid.  ";
         message += "No 'columnLabel' attribute was processed for ";
         message += propdoc->name();
         message += ".   Unable to parse request.";
         os += ActivityLog::entry(ActivityLog::WARNING,
         "Parsing", date, message.c_str());
       }
       if (!propdoc->columnLabels())
       {
         ret = false;
         TDateTime date;
         string message = "Property XML document is invalid.  ";
         message += "No 'columnLabels' attribute was processed for ";
         message += propdoc->name();
         message += ".   Unable to parse request.";
         os += ActivityLog::entry(ActivityLog::WARNING,
         "Parsing", date, message.c_str());

       }


       if (!propdoc->vectorLabel())
       {
         ret = false;
         TDateTime date;
         string message = "Property XML document is invalid.  ";
         message += "No 'vectorLabel' attribute was processed for ";
         message += propdoc->name();
         message += ".   Unable to parse request.";
         os += ActivityLog::entry(ActivityLog::WARNING,
         "Parsing", date, message.c_str());
       }

       if (propdoc->lastStep() == 0)
       {
         ret = false;
         TDateTime date;
         string message = "Property XML document is invalid.  ";
         message += "No time step element(s) found for ";
         message += propdoc->name();
         message += ".   Unable to parse request.";
         os += ActivityLog::entry(ActivityLog::ERROR,
         "Parsing", date, message.c_str());
       }
       break;
     }
     case TProperty::PROPVECGRID       :
     case TProperty::SUPER             :
       break;
   }
 
  return ret;
}

PropertyDoc* PropertyInterpreter::processProperty(
                    const Property_Ref* propref,
                    istream& propStream, string& os, const unsigned long step) 
{
   
#ifdef timertest
  ETimer timer;
  timer.start();
#endif
    string lastToken;

  // Create cache entry based on key determined from parsed input
  // and propref structure used to describe class type.
  //
  // Create XML document and fill with default values.  After this you basically have an
  // Empty property structure for a particular storage type defined.  
  //
    
  PropertyDoc *ret = new PropertyDoc(propref);
  //
  // If property is time series create a step element. This is a no-op if
  // the property is not a time series property.
  //
  EE_RT_ASSERT(ret, EE_FATAL, "PropertyDoc not created");
  ret->createStep(step);

  //
  //Now begin filling container with additional data provided by the stream:
  //  sizes, labels, values
  // 


  //
  //Process sizes
  //
    lastToken = processSize(propStream, ret);

  //
  //Process labels  
  //
  if (lastToken.rfind("label") != string::npos)
    lastToken = processLabels(propStream, ret, lastToken);

  //
  //Process values  
  //
  if (lastToken.rfind("values") != string::npos) 
  {
    if (propref->classType == TProperty::PROPVECTABLE ) 
      lastToken = processVecTableValues(propStream, ret);
    else
      lastToken = processValues(propStream, ret);
  }

  //
  //Process Units
  //
  if (lastToken.rfind("units:") != string::npos) {
    processUnits(propStream, ret);
  }

  //
  //I need to analyze the XML document to determine whether
  //it should be returned or dumped because it is incomplete   
  //or incorrect in some way.
  if (!validatePropertyDoc(ret, os))
  {
    delete ret;
    ret = NULL;
  }

#ifdef timertest
  timer.stop();
  cout << "time to process property" << timer.elapsedTime() << endl;
  timer.start();
#endif

  return ret;
}


