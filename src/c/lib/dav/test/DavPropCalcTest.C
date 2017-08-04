#include "util/Bookmark.H"
//#include "dsm/DavPropCalculation.H"
#include "dsm/EDSIServerCentral.H"  // produces URL(s) to web server(s)

#include "tdat/PropTSVecTable.H"
#include "dsm/EDSIFactory.H"
#include "tdat/TGBSConfig.H"
#include "dsm/EDSIGaussianBasisSetLibrary.H"
#include "tdat/TTCPair.H"

#include <string>
#include <vector>
#include <iostream.h>
#include <iomanip.h>

int main()
{
// Set up connection to DAV server via EDSI interface:
// Get URL to sessions file on DAV server:
   EDSIServerCentral servers;
   Bookmark mount = servers.getDefaultProvider();
   string url = mount.getUrl();
                "/users/clansing/SC99demos/Combustion/isobutane";

   // Testing EDSIGaussianBasisSetLibrary:
#if 000
   EDSIGaussianBasisSetLibrary egbsl(servers.getDefaultBasisSetLibrary());
   vector<TGaussianBasisSet*> gbsList
     = egbsl.lookup("d-aug-cc-pVDZ Diffuse", TGaussianBasisSet::diffuse);

   vector<const char*>* ats = egbsl.atoms("d-aug-cc-pVDZ Diffuse", 
			    TGaussianBasisSet::diffuse);
   //cout << "size of atoms list = " << ats->size() << endl;

   cout << "cont type = " 
	<< contractionType_formatter[gbsList[0]->p_contType] << endl;
   cout << "spherical = " << coordSys_formatter[gbsList[0]->p_coordSys]
	<< endl;
#endif

   // Testing GBS
   
   ICalculation* calc = EDSIFactory::getICalculation(url.c_str());
   TGBSConfig* config2 = calc->gbsConfig();
   TGBSConfig* config = new TGBSConfig(*config2);
cout << "size of config = " << config->size() << endl;
   // config->coordsys(TGaussianBasisSet::Cartesian);
   config->type(TGBSConfig::simple);
   cout << "group tag = " << (*config->begin()).first << endl;
   calc->gbsConfig(config);
   delete config;
   delete config2;

#if 000
   TTagCountPair pair;
   vector<TTagCountPair> pairs;

   pair.tag = "U";
   pair.count = 1;
   pairs.push_back(pair);

   pair.tag = "O";
   pair.count = 2;
   pairs.push_back(pair);

   pair.tag = "Cl";
   pair.count = 4;
   pairs.push_back(pair);

   cout << "num functions = " << config->num_functions(pairs) << endl;
   cout << "num primitives = " << config->num_primitives(pairs) << endl;

   // more gbs stuff   
   vector<TGaussianBasisSet*> gbs = config->getGBSList("O");
   cout << "size of gbs list = " << gbs.size() << endl;
   for (int i = 0; i < gbs.size(); i++) {
     cout << "gbs name = " << gbs[i]->p_name << endl;
     cout << "gbs category = " 
	  << gbsCategory_formatter[gbs[i]->p_category] << endl;
     cout << "gbs type = " 
	  << gbs_type_formatter[gbs[i]->p_type] << endl;
     cout << "num O sets" << gbs[i]->num_contracted_sets("O") << endl;
   }
#endif

#if 000
     cout << "num C sets" << gbs[i]->num_contracted_sets("C") << endl;
     vector<double> coefficients = gbs[i]->coefficients("C", 1);
     vector<double> exponents = gbs[i]->exponents("C", 1); 
     vector<TGaussianBasisSet::AngularMomentum> 
         func_types = gbs[i]->func_types("C", 1);
     Contraction* cont = gbs[i]->getContraction("C", 1);
  
     cout << "exponents" << endl;
     for (int j = 0; j < exponents.size(); j++)
       cout << exponents[j] << endl;
     cout << "coefficients" << endl;
     for (j = 0; j < coefficients.size(); j++)
       cout << coefficients[j] << endl;
     //cout << "coefficient 3 = " << cont->coefficient(2,0) << endl;   
     cout << "func_types size = " << func_types.size() << endl;
     for (j = 0; j < func_types.size(); j++)
       cout << shell_formatter[func_types[j]] << endl; 
   }
#endif

#if 000
   // Testing jobdata
   url = mount.getURL().toString() + 
                "/users/clansing/SC99demos/Combustion/butane";

   ICalculation* propCalc = EDSIFactory::getICalculation(url.c_str());
   Jobdata* jobdata;
   jobdata = propCalc->jobdata();
   cout << "jobid = " << jobdata->jobid << endl
	<< "clientid = " << jobdata->clientid << endl
	<< "jobpath = " << jobdata->jobpath << endl;
#endif

#if 000
   // Testing row labels
   IPropCalculation* propCalc = EDSIFactory::getIPropCalculation(url.c_str());
   TProperty* property = propCalc->getProperty("QUADPOLE");
   cerr << "got property" << endl;
   vector<string>* labels = ((PropVector*) property)->rowLabels();
   cerr << "got labels" << endl;
   if (labels != 0) {
      for (int i = 0; i < labels->size(); i++)
        cout << (*labels)[i] << endl;
   }
#endif

#if 000
   const vector<string> propList = propCalc.propertyNames();


   cout << "size of list = " << propList.size() << endl;

   for (int i = 0; i < propList.size(); i++)
     cout << propList[i] << endl;

   // TESTING A PROPTSVECTABLE
   TProperty* tprop = propCalc.getProperty("GEOMTRACE");
   PropTSVecTable* property = (PropTSVecTable*) tprop;   

   cout << property->units() << endl
        << property->type() << endl
        << property->rows() << endl
        << property->columns() << endl
        << property->tables() << endl
        << property->rowLabel() << endl
        << property->columnLabel() << endl
        << property->tableLabel() << endl;


   for (i = 0; i < property->rows(); i++)
   {
     for (int j = 0; j < property->columns(); j++)
        cout << setprecision(8) << property->value(0,i,j) << " ";
     cout << endl;
   } 

   // TESTING A PROPVALUE
   TProperty* tprop = propCalc.getProperty("TWOELEC");
   PropValue* property = (PropValue*) tprop;

   cout << property->units() << endl
        << property->type() << endl
        << property->label() << endl
        << setprecision(10) << property->value() << endl;

   // TESTING A PROPTSVECTOR
   TProperty* tprop = propCalc.getProperty("EGRAD");
   PropTSVector* property = (PropTSVector*) tprop;
   
   cout << property->units() << endl
        << property->type() << endl
        << property->rowLabel() << endl;

   for (int i = 0; i < property->rows(); i++)
      cout << setprecision(10) << property->value(i) << endl;
 
#endif
   return 0;
}
