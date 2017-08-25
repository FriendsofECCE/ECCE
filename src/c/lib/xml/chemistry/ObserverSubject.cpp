///////////////////////////////////////////////////////////////////////////////
// Subject Class
//
//
// DESIGN: Observer/Subject pattern allows observers to register for 
//         notification by a subject when a change has occurred in the subject.
//
//
///////////////////////////////////////////////////////////////////////////////


#include <algorithm>
  using std::find;

#include "dsm/ObserverSubject.H"

Observer::Observer()  
{
}


Observer::~Observer()
{
}

Subject::Subject()
{
}

Subject::~Subject( )
{
   p_observers.clear();
}


void Subject::attach( Observer* obs)  
{
   try  {
      // add this observer only if not already on the list
      list<Observer*>::iterator iter = find( p_observers.begin(),
                                             p_observers.end(),
                                             obs);
      if ( iter == p_observers.end() && obs != (Observer*)0)  {
         p_observers.push_back( obs);
      }
   }
   catch( std::exception& e)  {
   }
}



void Subject::detach( Observer* obs)  
{
   list<Observer*>::iterator iter = find( p_observers.begin(),
                                          p_observers.end(),
                                          obs);
   if ( iter != p_observers.end())
      p_observers.erase( iter);
}

void Subject::notify( const int flag)  
{
   list<Observer*>::iterator iter = p_observers.begin();
   while ( iter != p_observers.end())  {
      (*iter)->update( this, flag);
      iter++;
   }
}


void Subject::notifySelect( const int cid, const int tid)  
{
   list<Observer*>::iterator iter = p_observers.begin();
   while ( iter != p_observers.end())  {
      (*iter)->updateSelect( this, cid, tid);
      iter++;
   }
}
