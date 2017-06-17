/**
 * @file
 *
 *
 */
#ifndef FEEDBACKSAVEHANDLER_H
#define FEEDBACKSAVEHANDLER_H

#include <string>
  using std::string;

/**
 * Interface for windows using the WxFeedback class that want to process save events.
 */
class FeedbackSaveHandler
{    
   public:
      virtual void processSave() = 0;

};

#endif
