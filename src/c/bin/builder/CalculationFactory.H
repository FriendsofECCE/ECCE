#ifndef CALCULATIONFACTORY_H_
#define CALCULATIONFACTORY_H_

#include <map>
  using std::map;
#include <string>
  using std::string;

#include "util/EcceURL.H"

class IPropCalculation;


typedef bool (*CalcOpenTestFn)(const EcceURL& url);
typedef IPropCalculation *(*CalcOpenFn)(const EcceURL& url);
typedef string (*CalcOpenMaskFn)(void);

class CalculationInfo;
typedef map<string,const CalculationInfo*> PropCalcInfoMap;
typedef PropCalcInfoMap::const_iterator PropCalcInfoMapIter;


class CalculationInfo
{
  public:
    CalculationInfo(const string& name,
            const string& label,
            CalcOpenTestFn openTestFn,
            CalcOpenFn openFn,
            CalcOpenMaskFn openMaskFn)
      : p_name(name)
      , p_label(label)
      , p_openTestFn(openTestFn)
      , p_openFn(openFn)
      , p_openMaskFn(openMaskFn)
    {
      Register();
    }

    ~CalculationInfo()
    {
      Unregister();
    }

    string GetName() const
        { return p_name; }
    string GetLabel() const
        { return p_label; }
    bool CanOpen(const EcceURL& url) const
        { return p_openTestFn ? (*p_openTestFn)(url) : false; }
    IPropCalculation *Open(const EcceURL& url) const
        { return p_openFn ? (*p_openFn)(url) : NULL; }
    string GetOpenMask() const
        { return p_openMaskFn ? (*p_openMaskFn)() : ""; }

    static const CalculationInfo *Find(const string& name);
    static PropCalcInfoMap GetInfo()
        { return s_info ? (*s_info) : PropCalcInfoMap(); }

  protected:
    void Register();
    void Unregister();

    string p_name;
    string p_label;
    CalcOpenTestFn p_openTestFn;
    CalcOpenFn p_openFn;
    CalcOpenMaskFn p_openMaskFn;

  private:
    static PropCalcInfoMap *s_info;
};

#define DECLARE_CALCULATION()                                                 \
    public:                                                                   \
        static CalculationInfo s_calcInfo;                                    \
        virtual CalculationInfo *GetCalcInfo() const;

#define IMPLEMENT_CALCULATION(name, label, openTestFn, openFn, openMaskFn)    \
    CalculationInfo name::s_calcInfo(                                         \
            #name,                                                            \
            #label,                                                           \
            (CalcOpenTestFn) openTestFn,                                      \
            (CalcOpenFn) openFn,                                              \
            (CalcOpenMaskFn) openMaskFn);                                     \
    CalculationInfo *name::GetCalcInfo() const                                \
        { return &name::s_calcInfo; }



class CalculationFactory
{
  public:
    static bool canOpen(const EcceURL& url);
    static IPropCalculation *open(const EcceURL& url);
    static string openMask();
};



class PropertyTaskFactoryHelper
{
  public:
    static bool canOpen(const EcceURL& url);
    static IPropCalculation *open(const EcceURL& url);
    static string inputMasks();

  private:
    DECLARE_CALCULATION()
};

#endif // CALCULATIONFACTORY_H_
