///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: Queue.H
//
//
// CLASS SYNOPSIS:
//    Queue Objects Manage Queue Information for a Single Machine.
//
// EXPORTED TYPES:
//    Class Queue
//    Class QueueSpec
//
// LOCAL TYPES:
//
// SEE ALSO:
//    Class QueueManager
//      
// DESCRIPTION:
//    Queue objects maintain the constraints of individual queues for
//    any given machine.  These objects are managed by the QueueManager
//    class and most access should be made through there.
//    Queue constraints are attributes of Queue objects.  Unspecified
//    constraint values, such as unlimited file space, are filled with
//    a maximum value for the data type.
//
//    The QueueSpec structure is to be used in the QueueManager::selectQueue()
//    function.  It provides a "matching" template for locating queues with
//    sufficient resources to run a job.  Values are all initialized so
//    that only those constraints that have been set are checked.
//
// EXAMPLES:
//    testQueueMgr.C
//
///////////////////////////////////////////////////////////////////////////////
#ifndef QUEUE_HH
#define QUEUE_HH

#include <iostream>
  using std::ostream;

#include <string>
  using std::string;

class Queue;
class QueueSpec;
class Preferences;

class QueueSpec
{
   public:
      QueueSpec();

      unsigned int maxJobCPU;
      unsigned int maxProcessCPU;
      unsigned int runLimit;
      unsigned int processors;
      unsigned int fileLimit;
      unsigned int coreLimit;
      unsigned int memLimit;
      unsigned int scratchLimit;
      unsigned int dataLimit;
      unsigned int stackLimit;
};

class Queue
{

public:
  // Constructors - No User Created Objects
    // the QueueManager Class is Responsible for Creating These Objects
    friend class QueueManager;

  // Virtual Destructor
    virtual ~Queue(void);

  // Operators
    bool operator==(const Queue& queue) const;
    bool operator!=(const Queue& queue) const;
    bool operator<(const QueueSpec& queueSpec) const;

  // Modifiers - No User Accessible Modifiers

  // Accessors
    string    name(void) const;
    string    description(void) const;
    string    administrator(void) const;
    unsigned int maxSlots(void) const;
    string    priority(void) const;
    int          nice(void) const;
    // Processor Limits
    unsigned int maxJobCPU(void) const;
    unsigned int maxProcessCPU(void) const;
    unsigned int runLimit(void) const;
    unsigned int defRun(void) const;
    unsigned int minProcessors(void) const;
    unsigned int maxProcessors(void) const;
    unsigned int defProcessors(void) const;
    // File System Limits
    unsigned int fileLimit(void) const;
    unsigned int coreLimit(void) const;
    unsigned int memLimit(void) const;
    unsigned int defMemory(void) const;
    string       memUnits(void) const;
    unsigned int scratchLimit(void) const;
    unsigned int defScratch(void) const;
    unsigned int dataLimit(void) const;
    unsigned int stackLimit(void) const;

protected:

  // Modifiers
    void description(const string& description);
    void administrator(const string& administrator);
    void maxSlots(unsigned int maxSlots);
    void priority(const string& priority);
    void nice(int nice);
    void maxJobCPU(unsigned int maxJobCPU);
    void maxProcessCPU(unsigned int maxProcessCPU);
    void runLimit(unsigned int runLimit);
    void defRun(unsigned int defRun);
    void minProcessors(unsigned int minProcessors);
    void maxProcessors(unsigned int maxProcessors);
    void defProcessors(unsigned int defProcessors);
    void fileLimit(unsigned int fileLimit);
    void coreLimit(unsigned int coreLimit);
    void memLimit(unsigned int memLimit);
    void defMemory(unsigned int defMemory);
    void memUnits(const string& memUnits);
    void scratchLimit(unsigned int scratchLimit);
    void defScratch(unsigned int defScratch);
    void dataLimit(unsigned int dataLimit);
    void stackLimit(unsigned int stackLimit);

  // Derived Class Accessors
    Queue(const string& name);

private:

  // State
    string         p_name;
    string         p_description;
    string         p_administrator;
    unsigned int   p_maxSlots;
    string         p_priority;
    int            p_nice;
    unsigned int   p_maxJobCPU;            // in Seconds
    unsigned int   p_maxProcessCPU;        // in Seconds
    unsigned int   p_runLimit;             // in Minutes
    unsigned int   p_defRun;               // default run time
    unsigned int   p_minProcessors;
    unsigned int   p_maxProcessors;
    unsigned int   p_defProcessors;
    unsigned int   p_fileLimit;            // in Kilobytes
    unsigned int   p_coreLimit;            // in Kilobytes
    unsigned int   p_memLimit;             // units specified below
    unsigned int   p_defMemory;            // default memory
    string         p_memUnits;             // MB, MW, KB, etc.
    unsigned int   p_scratchLimit;         // in Megabytes
    unsigned int   p_defScratch;           // default scratch space
    unsigned int   p_dataLimit;            // in Megabytes
    unsigned int   p_stackLimit;           // in Megabytes

  // Encapsulated Behavior
    void    fillAttributesFrom(Preferences& prefs);
    unsigned int secondsFromString(const string& time) const;
    unsigned int minutesFromString(const string& time) const;

  // Friends
    friend ostream& operator<<(ostream& os, const Queue& queue);

}; // Queue

#endif /* QUEUE_HH */
