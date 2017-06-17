///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: Observer.H
//
//
// CLASS SYNOPSIS:
//
// EXPORTED TYPES:
//
// DESCRIPTION: Defines the Observer pattern - both the Subject and the
//              Observer.  Ref Design Patterns; Elements of Reusable 
//              Object-Oriented Software.  Erich Gamma etal.
//
// EXAMPLES:
//
///////////////////////////////////////////////////////////////////////////////
#ifndef OBSERVER_HH
#define OBSERVER_HH

// library includes
#include <string>
#include <list>
  using std::string;
  using std::list;

class Subject;

class Observer
{
   public:
      virtual ~Observer();

      virtual void update( Subject* , const int)  = 0;
      virtual void updateSelect( Subject* , const int, const int) = 0;

   protected:
      Observer();
};

class Subject
{
   public:
      virtual ~Subject();

      virtual void attach( Observer*);
      virtual void detach( Observer*);
      virtual void notify( const int);
      virtual void notifySelect( const int, const int);
 
      enum notify_type { LOADING, READY};

   protected:
      Subject();

   private:
      list<Observer*> p_observers;
};

#endif /* OBSERVER_HH */
