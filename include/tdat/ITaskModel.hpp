/**
 * @file
 *
 *
 */
#ifndef ITASKMODEL_HH
#define ITASKMODEL_HH

#include <string>

using std::string;

#include "util/Receiver.H"
#include "util/InvalidException.H"
#include "util/Serializable.H"

/**
 * Abstract base class for implementing data models for generic task models.
 * Tasks are single applications that have a bunch of inputs and can be 
 * executed.
 *
 * @todo replace InvalidException with TaskException?
 */
class ITaskModel : public Receiver, public Serializable 
{

  public:
    ITaskModel();
    virtual ~ITaskModel();

    ITaskModel(const ITaskModel& rhs);

    virtual void  setTitle(string title);
    virtual string getTitle() const;

    virtual void  setUrl(string url);
    virtual string getUrl() const;

    virtual void generateInputFile() throw(InvalidException) = 0;

    virtual void run() throw(InvalidException) = 0;


    // Receiver virtuals
    virtual void debugPrint() const;

  protected:

  private:

    string p_title;        /**< Task title/identifier */

    string p_url;          /**< Base location for this task's files/collections */

};
#endif 
