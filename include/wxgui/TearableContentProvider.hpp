#ifndef TEARABLECONTENTPROVIDER_H_
#define TEARABLECONTENTPROVIDER_H_

class TearableContentProvider
{
  public:
    TearableContentProvider() {;}
    virtual ~TearableContentProvider() {;}
    
    virtual wxWindow * GetTearableContent() = 0;
};

#endif // TEARABLECONTENTPROVIDER_H_

