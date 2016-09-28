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

#ifndef BYTE_READER_H
#define BYTE_READER_H

extern "C" {
  #include "lua.h"
  #include "lauxlib.h"
}

#ifndef BYTE_READER_NO_VECTOR
#include <vector>

void VectorReader (lua_State * L, class ByteReader & reader, int arg, void *);
#endif

struct ByteReaderFunc {
  void (*mGetBytes)(lua_State * L, class ByteReader & reader, int arg, void * context); // Reader function
  void * mContext; // Context, if any
};

class ByteReader {
public:
  const void * mBytes; // Resolved byte stream
  size_t mCount; // Number of bytes available in stream

  ByteReader (lua_State * L, int arg, bool bReplace = true);

  static void Register (lua_State * L, ByteReaderFunc * func, bool bUseTop = false);
  static ByteReaderFunc * Register (lua_State * L);
private:
  void LookupBytes (lua_State * L, int arg);
  void PointToBytes (lua_State * L, int arg, ByteReaderFunc * func);
  void PushError (lua_State * L, const char * format, int arg);
};

// A `ByteReader` transforms Lua inputs adhering to a simple protocol into
// a bytes-count pair that other C and C++ APIs are able to safely consume.
// The protocol goes as follows:
//
// The object at index `arg` is examined.
//
// If it happens to be a string, we can use its bytes and length directly.
//
// Failing that, the object's metatable (if it has one) is queried for field
// **__bytes**. If no value is found, the object clearly does not honor the
// protocol, so we quit.
//
// Otherwise, if **__bytes** is neither a light userdata nor a function, we
// once again use the object's bytes and length directly.
//
// When **__bytes** is a light userdata, it is interpreted as a pointer to a
// `ByteReaderFunc` struct, containing a reader function **mGetBytes** and a
// user-supplied context **mContext**. (The userdata must be present as a key
// in the Lua registry.) The function is called as `func(L, reader, arg, mContext)`,
// where `L` and 'arg` are the Lua state and stack position of the object,
// respectively, and `reader` is our byte reader whose **mBytes** and **mCount**
// members the function should supply.
//
// In the foregoing cases, the object must be a full userdata.
//
// The remaining possiblity is that **__bytes** is a function, to be
// called as
//   object = func(object)
// The process (i.e. is the object a string? if not, does it have a **__bytes**
// metafield? et al.) then recurses on this new object and uses its result
// instead.
//
// When bytes are successfully found, the reader's **mBytes** member will point
// to them, with the byte count stored in **mCount**. If `bReplace` is **true**
// (the default), the bytes (either a string or full userdata) are moved into
// slot `arg`, overwriting the original object.
//
// Should an error happen along the way, **mBytes** will be NULL and an error
// message will be pushed on top of the stack.

#endif