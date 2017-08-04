/**
 * @file
 *
 *
 */
#include <iostream>
  using std::cout;
  using std::endl;

#include "tdat/ITaskModel.H"



/**
 * Constructor
 */
ITaskModel::ITaskModel()
{
}


/**
 * Destructor
 */
ITaskModel::~ITaskModel()
{
}


ITaskModel::ITaskModel(const ITaskModel& rhs)
{
   setTitle(rhs.getTitle());
   setUrl(rhs.getUrl());
}


/**
 * Set task title
 */
void ITaskModel::setTitle(string title)
{
  p_title = title;
}


/**
 * Get task title
 */
string ITaskModel::getTitle() const
{
  return p_title;
}


/**
 * Set task url
 */
void ITaskModel::setUrl(string url)
{
  p_url = url;
}


/**
 * Get task url
 */
string ITaskModel::getUrl() const
{
  return p_url;
}


void ITaskModel::debugPrint() const
{
   cout << "Title: " << getTitle() << endl;
   cout << "Url: " << getUrl() << endl;
}
