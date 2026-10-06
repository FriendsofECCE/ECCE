/**
 * @file
 *
 * A vibrational (IR / Raman) spectrum as the Vibrational Frequencies
 * panel shows it (#214).
 *
 * No wx and no drawing: which sticks exist, what a scaling factor and a
 * line width do to them, which wavenumber range is on screen and where
 * the ticks go are decided here, so they can be tested from the command
 * line.  SpectrumCanvas.H only paints what this holds.
 */

#ifndef VIBSPECTRUM_HH
#define VIBSPECTRUM_HH

#include <string>
#include <vector>
using std::string;
using std::vector;

/** The two spectra a vibrational calculation can give. */
enum VibKind { VIB_IR = 0, VIB_RAMAN = 1 };

/** The wavenumber axis: a window on the data and its direction. */
class SpectrumAxis
{
  public:
    SpectrumAxis();

    /** Set the whole range the data spans; resets the view to it. */
    void setFull(double lo, double hi);
    double fullLo() const { return p_fullLo; }
    double fullHi() const { return p_fullHi; }

    double lo() const { return p_lo; }
    double hi() const { return p_hi; }
    double span() const { return p_hi - p_lo; }
    bool zoomed() const;

    /** High wavenumbers on the left, the convention for IR and Raman. */
    void setReversed(bool reversed) { p_reversed = reversed; }
    bool reversed() const { return p_reversed; }

    /** 0 at the left edge of the plot, 1 at the right. */
    double toFraction(double wavenumber) const;
    double fromFraction(double fraction) const;

    /** Show [a, b] (either order), clipped to the full range. */
    void zoomTo(double a, double b);
    /** Scale the window by factor (<1 zooms in) about a wavenumber. */
    void zoomAbout(double centre, double factor);
    /** Move the window by a wavenumber difference, staying in range. */
    void pan(double delta);
    void reset();

    /** Narrowest window allowed, cm-1. */
    static double minSpan() { return 20.0; }

  private:
    void clamp();

    double p_fullLo, p_fullHi, p_lo, p_hi;
    bool   p_reversed;
};

/** Tick positions for an axis: round numbers, 1, 2 or 5 times 10^n. */
struct SpectrumTicks
{
    vector<double> values;
    double         step;
    int            decimals;     // digits after the point a label needs
};

/** Ticks inside [lo, hi], at most about `target` of them. */
SpectrumTicks niceTicks(double lo, double hi, int target);

/**
 * A y axis from 0 up: `top` is the first tick at or above the maximum,
 * so the tallest feature is never clipped.
 */
struct SpectrumYAxis
{
    double top;
    SpectrumTicks ticks;
};
SpectrumYAxis niceYAxis(double maximum, int target);

/** One mode, with the quantities drawn for it. */
struct VibStick
{
    int    mode;            // zero-based row of the frequency table
    double wavenumber;      // scaled, cm-1; negative if imaginary
    double intensity;       // as drawn (see VibSpectrum::unitLabel)
    string irrep;
    bool   imaginary;
};

enum LineShape { LINE_GAUSSIAN, LINE_LORENTZIAN };

class VibSpectrum
{
  public:
    VibSpectrum();

    /**
     * The modes, from VIBFREQ and (optionally) VIBSYM.  Clears any
     * intensities.  `irreps` may be empty or shorter than the list.
     */
    void setModes(const vector<double>& wavenumbers,
                  const vector<string>& irreps);

    /**
     * Intensities for every mode, in the code's own `units` as written
     * in the property (e.g. "KM/Mole", "A^4/AMU").  A units string that
     * is not recognised makes the spectrum relative: drawn scaled so
     * its largest stick is 1.
     */
    void setIntensities(VibKind kind, const vector<double>& values,
                        const string& units);

    bool has(VibKind kind) const { return p_has[kind]; }
    int  modeCount() const { return (int)p_freq.size(); }

    /** Axis title for the pane, e.g. "IR intensity (km/mol)". */
    string axisLabel(VibKind kind) const;
    /** Just the unit, UTF-8: "km/mol", "Å⁴/amu", "relative". */
    string unitLabel(VibKind kind) const;
    bool   relative(VibKind kind) const { return p_relative[kind]; }

    /** Frequency scaling factor, applied to every wavenumber. */
    void   setScale(double scale) { p_scale = scale > 0 ? scale : 1.0; }
    double scale() const { return p_scale; }

    /**
     * Full width at half maximum of the broadening, cm-1.  0 means
     * sticks only.
     */
    void   setFwhm(double fwhm) { p_fwhm = fwhm > 0 ? fwhm : 0.0; }
    double fwhm() const { return p_fwhm; }
    void      setLineShape(LineShape shape) { p_shape = shape; }
    LineShape lineShape() const { return p_shape; }

    /**
     * Wavenumbers below this magnitude are the translations and
     * rotations some codes list among the modes (ORCA prints 0.00, NWChem
     * values of 1e-5).  They are not vibrations, so they get no stick.
     */
    static double zeroTolerance() { return 10.0; }

    /** Scaled wavenumber of mode i. */
    double wavenumber(int i) const { return p_freq[i] * p_scale; }
    bool   isVibration(int i) const;
    bool   isImaginary(int i) const;

    /**
     * The sticks of one spectrum: real vibrations and imaginary modes,
     * not the near-zero ones.  A mode whose intensity is zero stays in
     * the list, so it can still be hovered and so the table row it
     * belongs to is known.
     */
    vector<VibStick> sticks(VibKind kind) const;

    /**
     * The broadened spectrum at a wavenumber, in intensity per cm-1:
     * each real mode contributes an area-normalised line, so the
     * integral of a band is its stick intensity whatever the width and
     * overlapping bands add.  Imaginary modes contribute nothing.
     */
    double envelope(VibKind kind, double wavenumber) const;

    /** Largest stick height with wavenumber in [lo, hi]; 0 if none. */
    double maxStick(VibKind kind, double lo, double hi) const;
    /** Largest envelope value in [lo, hi] (0 when fwhm is 0). */
    double maxEnvelope(VibKind kind, double lo, double hi) const;

    /**
     * The wavenumber range worth showing by default: the modes plus a
     * margin, on round numbers.  Reaches below zero only when there is
     * an imaginary mode.
     */
    void defaultRange(double *lo, double *hi) const;

    /** Area-normalised line shape, 1/cm-1, centred on zero. */
    static double lineValue(LineShape shape, double offset, double fwhm);

    /** Whether the code's units string is one we know how to label. */
    static bool knownIntensityUnits(VibKind kind, const string& units);

  private:
    double drawn(VibKind kind, int i) const;

    vector<double> p_freq;
    vector<string> p_irrep;
    vector<double> p_inten[2];
    bool           p_has[2];
    bool           p_relative[2];
    string         p_units[2];
    double         p_scale, p_fwhm;
    LineShape      p_shape;
};

#endif
