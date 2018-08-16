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
	#define BR_NAMESPACE CEU_NAMESPACE_PREFIX ## _BR
#else
	#define BR_NAMESPACE BR
#endif

namespace BR_NAMESPACE {
  void VectorReader (lua_State * L, class ByteReader & reader, int arg, void *);
  void VectorReader (lua_State * L, class ByteReader & reader, const std::vector<unsigned char> & vec);

  struct ByteReaderFunc {
    bool (*mGetBytes)(lua_State * L, class ByteReader & reader, int arg, void * context); // Reader function
    void * mContext; // Context, if any
  };

  class ByteReader {
  public:
    const void * mBytes{nullptr}; // Resolved byte stream
    size_t mCount; // Number of bytes available in stream
    std::vector<size_t> mStrides;	// Zero or more strides associated with the stream
    int mPos; // Original (absolute) position, to allow later replacements

    ByteReader (lua_State * L, int arg, bool bReplace = true);

    static void Register (lua_State * L, ByteReaderFunc * func, bool bUseTop = false);
    static ByteReaderFunc * Register (lua_State * L);
  private:
    bool LookupBytes (lua_State * L);
    bool PointToBytes (lua_State * L, ByteReaderFunc * func);

    void PushError (lua_State * L, const char * format);
  };
}

using namespace BR_NAMESPACE;