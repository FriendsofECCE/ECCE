#ifndef EWXBOOLCLIENTDATA_H_
#define EWXBOOLCLIENTDATA_H_

class ewxBoolClientData : public wxClientData
{
  public:
    ewxBoolClientData() : m_data() { }
    ewxBoolClientData( const bool& data ) : m_data(data) { }
    void SetData( const bool& data ) { m_data = data; }
    const bool& GetData() const { return m_data; }

  private:
    bool m_data;
};

#endif // EWXBOOLCLIENTDATA_H_
