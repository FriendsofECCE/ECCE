#ifndef EWXINTCLIENTDATA_H_
#define EWXINTBOOLCLIENTDATA_H_

class ewxIntClientData : public wxClientData
{
  public:
    ewxIntClientData() : m_data(0) { }
    ewxIntClientData( const int& data ) : m_data(data) { }
    void SetData( const int& data ) { m_data = data; }
    const int& GetData() const { return m_data; }

  private:
    int m_data;
};

#endif // EWXINTCLIENTDATA_H_
