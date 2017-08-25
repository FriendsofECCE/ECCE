/**
 * @file 
 *
 *
 */
#ifndef SPECTRUM_HH
#define SPECTRUM_HH

#include <iostream>
  using std::ostream;
  using std::istream;


/**
 * Class to generate color spectrum displays.
 * This class supports the following types of spectra
 * <li>full rainbow colors all red to all blue
 * <li>positive spectrum white to blue
 * <li>negative spectrum white to red
 * <li>pos/negative spectrum blue to white to red
 *
 * Usage:
 *   Spectrum spectrum(minvalue, maxvalue, numberofcolors);
 *   foreach thing
 *      spectrum.getRainbowValue(valuebetween_ min_max, r,g,b);
 */
class Spectrum 
{
   public:

      Spectrum(float min, float max, int slices);

      virtual ~Spectrum();

      void getRainbowValue(float value, float& r, float&g, float& b);
      void getNegValue(float value, float& r, float&g, float& b);
      void getRedValue(float value, float& r, float&g, float& b);
      void getPosValue(float value, float& r, float&g, float& b);
      void getBlueValue(float value, float& r, float&g, float& b);
      void getGreenValue(float value, float& r, float&g, float& b);
      void getPosNegValue(float value, float& r, float&g, float& b);
      void getPosNegMidpointValue(float value, float& r, float&g, float& b);


   protected:
      Spectrum(const Spectrum& rhs);

      float p_min;
      float p_max;
      int p_slices;


}; 

#endif 
