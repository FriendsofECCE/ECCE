/**
 * @file
 *
 *
 */
#ifndef RESOURCEUTILS_HH
#define RESOURCEUTILS_HH

#include <string>
  using std::string;
#include "dsm/ResourceDescriptor.H"



/**
 * Methods for ResourceDescriptor's RESOURCETYPE, CONTENTTYPE, APPLICATIONTYPE,
 * and RUNSTATE enumerated types.
 */
class ResourceUtils
{
public:

  static ResourceDescriptor::RESOURCETYPE stringToResourceType(const string& str);
  static string resourceTypeToString(ResourceDescriptor::RESOURCETYPE resourceType);

  static ResourceDescriptor::CONTENTTYPE stringToContentType(const string& str);
  static string contentTypeToString(ResourceDescriptor::CONTENTTYPE contentType);

  static ResourceDescriptor::APPLICATIONTYPE stringToApplicationType(const string& str);
  static string applicationTypeToString(ResourceDescriptor::APPLICATIONTYPE contentType);

  static ResourceDescriptor::RUNSTATE stringToState(const string& str);
  static string stateToString(ResourceDescriptor::RUNSTATE state);

  static ResourceDescriptor::IconType stringToIconType(const string& str);
  static string iconTypeToString(ResourceDescriptor::IconType iconType);

  static ResourceDescriptor::ResourceCategory stringToResourceCategory(const string& str);
  static string resourceCategoryToString(ResourceDescriptor::ResourceCategory resourceCategory);

};

#endif 
