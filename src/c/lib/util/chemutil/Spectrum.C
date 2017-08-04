#include <iostream>
using namespace std;
#include "util/Spectrum.H"

Spectrum::Spectrum(float min, float max, int slices)
{
   p_min = min;
   p_max = max;
   p_slices = slices;
}

Spectrum::Spectrum(const Spectrum& rhs)
{
   // Not implemented
}

 
Spectrum::~Spectrum()
{
}


/**
 * Calculate rgb values for the specified value
 * This method computes a color on the full rainbow spectrum.
 * Values near the minimum will be bluish, values near the maximum will
 * be redish.
 */
void Spectrum::getRainbowValue(float value, float& red, float& green, float& blue)
{
   // Handle bad range condition
   if (p_max <= p_min) {
      red = green = blue = 0.5;
      return;
   }

   float v;
   float range = p_max - p_min;
   v = (value-p_min) / range;
   if (v > 0.5) {
      if (v > 0.75) {
         v = (v - 0.75) * 4.0 ;
         red = 1.0 ;
         green = (1.0-v) * 0.365 ;
         blue = (1.0-v) * .039 ;
      } else {
         v = (v - 0.5) * 4.0 ;
         red = 1.0 ;
         green = 1.0 - v * 0.635 ;
         blue = v * .039 ;
      }
   } else {
      if (v > 0.25) {
         v = (v - 0.25) * 4.0 ;
         red = v ;
         green = 0.5 + v / 2.0 ;
         blue = 0.0 ;
      } else {
         v *= 4.0 ;
         red = 0.0 ;
         green = v / 2.0 ;
         blue = (1.0-v) ;
      }
   }

   if (red < 0.0)
      red = 0.0;
   else if (red > 1.0)
      red = 1.0;
   if (green < 0.0)
      green = 0.0;
   else if (green > 1.0)
      green = 1.0;
   if (blue < 0.0)
      blue = 0.0;
   else if (blue > 1.0)
      blue = 1.0;

}


/**
 * Generate rgb values for "value" over the current range
 * while fixing red which is our negative color.
 */
void Spectrum::getNegValue(float value, float& red, float& green, float& blue)
{
   // Handle bad range condition
   if (p_max <= p_min) {
      red = green = blue = 0.5;
      return;
   }

   float range = (p_max + -p_min);

   red = 1.0;  // fix red

   if (value > p_max) {
      // set to full white
      green = blue = 1.;
   } else if (value < p_min) {
      // set to full red
      green = blue = 0.;
   } else {
      green = blue = ((p_max -value)/range);
   }


}


/**
 * Generate rgb values for "value" over the current range
 * All values a shade of red between 00 and ff.
 * Works over range - not pos/neg.
 */
void Spectrum::getRedValue(float value, float& red, float& green, float& blue)
{
   // Handle bad range condition
   if (p_max <= p_min) {
      red = green = blue = 0.5;
      return;
   }

   float range = (p_max - p_min);

   red = 1.0;  // fix red

   if (value > p_max) {
      // set to full white
      green = blue = 1.;
   } else if (value < p_min) {
      // set to full red
      green = blue = 0.;
   } else {
      green = blue = ((p_max -value)/range);
   }


}


/**
 * Generate rgb values for "value" over the current range
 * All values a shade of green between 00 and ff.
 * Works over range - not pos/neg.
 */
void Spectrum::getGreenValue(float value, float& red, float& green, float& blue)
{
   // Handle bad range condition
   if (p_max <= p_min) {
      red = green = blue = 0.5;
      return;
   }

   float range = (p_max - p_min);

   green = 1.0;  // fix green

   if (value > p_max) {
      // set to full white
      red = blue = 1.;
   } else if (value < p_min) {
      // set to full green
      red = blue = 0.;
   } else {
      red = blue = ((p_max -value)/range);
   }


}


/**
 * Generate rgb values for "value" over the current range
 * while fixing blue which is our positive color.
 * Colors outside the range will be set to the min and max respectively.
 */
void Spectrum::getPosValue(float value, float& red, float& green, float& blue)
{
   // Handle bad range condition
   if (p_max <= p_min) {
      red = green = blue = 0.5;
      return;
   }

   float range = (p_max + -p_min);

   blue = 1.0;  // fix blue

   if (value > p_max) {
      // Set to max (full blue)
      green = red = 0.;
      // Or set to a very dark blue
      //red = green = 0;
      //blue = .5;
   } else if (value < p_min) {
      // Set to min (full white)
      green = red = 1.;
      // Or set to a very dark red
      //blue = green = 0.;
      //red = .5;
   } else {
      green = red = ((p_max -value)/range);
   }
   //cout << "value/r,g,b " << value << " " << red << " " << green << " " << blue << endl;

}


/**
 * Generate rgb values for "value" over the current range
 * while fixing blue which is our positive color.
 * Colors outside the range will be set to the min and max respectively.
 */
void Spectrum::getBlueValue(float value, float& red, float& green, float& blue)
{
   // Handle bad range condition
   if (p_max <= p_min) {
      red = green = blue = 0.5;
      return;
   }

   float range = (p_max  - p_min);

   blue = 1.0;  // fix blue

   if (value > p_max) {
      // Set to max (full blue)
      green = red = 0.;
      // Or set to a very dark blue
      //red = green = 0;
      //blue = .5;
   } else if (value < p_min) {
      // Set to min (full white)
      green = red = 1.;
      // Or set to a very dark red
      //blue = green = 0.;
      //red = .5;
   } else {
      green = red = ((p_max -value)/range);
   }
   //cout << "value/r,g,b " << value << " " << red << " " << green << " " << blue << endl;

}


/**
 * Compute colors where red is neg, blue is positive.
 * From Erich,
 * Other programs use the two colors, blue and red, and then have a zero
 * charge be white.  As one moves from the most positive charge to zero,
 * the color changes from fully saturated blue to a less saturated blue
 * until white.  Then while going negative, white slowly becomes a more
 * saturated red color until the most negative is fully saturated red.
 * This two color scale is also easier for people who have various types
 * of color-blindness.
 */
void Spectrum::getPosNegValue(float value, float& red, float& green, float& blue)
{
   // Handle bad range condition
   if (p_max <= p_min) {
      red = green = blue = 0.5;
      return;
   }

   float range = (p_max + -p_min)/2;
   if (value > 0) {
      // Fix blue
      blue = 1.0;
      if (value > p_max) {
         // Set to max (full blue)
         green = red = 0.;
      } else {
         green = red = 1.0 - (value/range);
      }
   } else {
      // Fix red
      red = 1.0;
      if (value < p_min) {
         // set to full red
         green = blue = 0.;
      } else {
         green = blue = 1.0 - (-1*(value/range));
      }
   }
   //cout << "value/r,g,b " << value << " " << red << " " << green << " " << blue << endl;

}



/**
 * Same as getPosNegValue except pos>midpoint, neg=<midpoint
 * Numbers at the low end of the scale will be redish.
 * Numbers at the midpoint of the scale will be whiteish.
 * Numbers at the high end of the scale will be blueish.
 */
void Spectrum::getPosNegMidpointValue(float value, float& red, float& green, float& blue)
{
   // Handle bad range condition
   if (p_max <= p_min) {
      red = green = blue = 0.5;
      return;
   }

   float range = p_max + -p_min;
   float midpoint = p_min + (range/2.);
   if (value > midpoint) {
      // Fix blue
      blue = 1.0;
      if (value > p_max) {
         // Set to max (full blue)
         green = red = 0.;
      } else {
         green = red = 1.0 - ((value + - p_min)/ (range));
      }
   } else {
      // Fix red
      red = 1.0;
      if (value < p_min) {
         // set to full red
         green = blue = 0.;
      } else {
         green = blue = 0.0 + ((value + -p_min)/(range/2.));
      }
   }
   //cout << "value/r,g,b " << value << " " << p_min << " " << p_max << " " << midpoint << " " << range << " --> " << red << " " << green << " " << blue << endl;

}
