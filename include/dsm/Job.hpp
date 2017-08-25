#ifndef JOB_H_
#define JOB_H_

#include <string>
  using std::string;

#include "dsm/ResourceDescriptor.H"

/**
 * Interface for state management.
 * Modeled from our TaskJob class, meant to encapsulate the Job-related
 * aspects.  Should also include here launch data and job data in addition
 * to simple state management.
 */
class Job
{
  public:

    virtual bool setStartDate(const string& date) = 0;
    virtual string getStartDate() const = 0;

    virtual bool setCompletionDate(const string& date) = 0;
    virtual string getCompletionDate() const = 0;

    virtual bool canChangeState(string& message) const = 0;
    virtual bool setState(const ResourceDescriptor::RUNSTATE& state) = 0;
    virtual ResourceDescriptor::RUNSTATE getState() const = 0;
    virtual bool resetForRerun(const ResourceDescriptor::RUNSTATE& toState=ResourceDescriptor::STATE_ILLEGAL) = 0;
    virtual bool resetForRestart() = 0;
};

#endif // JOB_H_
