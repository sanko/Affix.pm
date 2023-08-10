#include "std.h"

class DLLEXPORT Box
{
  public:
    DLLEXPORT Box() : m_width(0), m_length(0), m_height(0){}; // Default constructor
    DLLEXPORT Box(int i);         // Initialize a Box with equal dimensions (i.e. a cube)
    DLLEXPORT Box(int, int, int); // Initialize a Box with custom dimensions
    DLLEXPORT int Volume();

  private:
    // Will have value of 0 when default constructor is called.
    // If we didn't zero-init here, default constructor would
    // leave them uninitialized with garbage values.
    int m_width{0};
    int m_length{0};
    int m_height{0};
};

DLLEXPORT Box::Box(int i) : m_width(i), m_length(i), m_height(i){};

DLLEXPORT Box::Box(int width, int length, int height)
    : m_width(width), m_length(length), m_height(height){};

DLLEXPORT int Box::Volume() {
    return m_width * m_length * m_height;
};
