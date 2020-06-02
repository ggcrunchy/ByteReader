/*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
* [ MIT license: http://www.opensource.org/licenses/mit-license.php ]
*/

#pragma once

extern "C" {
  #include "lua.h"
  #include "lauxlib.h"
}

#include <vector>

#ifdef BR_NAMESPACE_PREFIX
  #define BR_BEGIN_NAMESPACE() namespace BR_NAMESPACE_PREFIX {
  #define BR_CLOSE_NAMESPACE() }
  #define BR_END_NAMESPACE() BR_CLOSE_NAMESPACE()    \
                             using namespace BR_NAMESPACE_PREFIX;
#else
  #define BR_BEGIN_NAMESPACE()
  #define BR_CLOSE_NAMESPACE()
  #define BR_END_NAMESPACE()
#endif

BR_BEGIN_NAMESPACE()
  void VectorReader (lua_State * L, class ByteReader & reader, int arg, void *);
  void VectorReader (lua_State * L, class ByteReader & reader, const std::vector<unsigned char> & vec);

  struct ByteReaderFunc {
    bool (*mEnsureSize)(lua_State * L, class ByteReader & reader, int arg, void * context, const std::vector<size_t> & sizes) = nullptr;
    bool (*mGetBytes)(lua_State * L, class ByteReader & reader, int arg, void * context); // Reader function
    bool (*mGetStrides)(lua_State * L, class ByteReader & reader, int arg, void * context) = nullptr;
    void * mContext{nullptr}; // Context, if any
  };

  struct ByteReaderOpts {
    std::vector<size_t> mRequired;
    bool mGetStrides{false};
    bool mReplace{true};

    enum : size_t { kCurrent = ~0U };

    ByteReaderOpts & SetRequired (const std::vector<size_t> & req) { mRequired = req; return *this; }
    ByteReaderOpts & SetGetStrides (bool get) { mGetStrides = get; return *this; }
    ByteReaderOpts & SetReplace (bool replace) { mReplace = replace; return *this; }
  };

  class ByteReader {
  public:
    const void * mBytes{nullptr}; // Resolved byte stream
    size_t mCount; // Number of bytes available in stream
    size_t mNumComponents{0U};  // Components per stream element
    std::vector<size_t> mStrides;	// Zero or more strides associated with the stream
    int mPos; // Original (absolute) position, to allow later replacements

    ByteReader (lua_State * L, int arg, const ByteReaderOpts & opts = ByteReaderOpts{});

    static void Register (lua_State * L, ByteReaderFunc * func, bool bUseTop = false);
    static ByteReaderFunc * Register (lua_State * L);
  private:
    bool LookupBytes (lua_State * L, const ByteReaderOpts & opts);
    bool PointToBytes (lua_State * L, ByteReaderFunc * func, const ByteReaderOpts & opts);

    void PushError (lua_State * L, const char * format);
  };

  class ByteReaderWriter : public ByteReader {
  public:
    ByteReaderWriter (lua_State * L, int arg, const ByteReaderOpts & opts = ByteReaderOpts{});
  };

  class ByteReaderWriterSized : public ByteReader {
  public:
      ByteReaderWriterSized (lua_State * L, int arg, size_t size, const ByteReaderOpts & opts = ByteReaderOpts{});
  };

  class ByteReaderWriterMultipleSized : public ByteReader {
  public:
      ByteReaderWriterMultipleSized (lua_State * L, int arg, const std::vector<size_t> & sizes, const ByteReaderOpts & opts = ByteReaderOpts{});
  };
BR_END_NAMESPACE()
