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
#endif

struct ByteReaderUchar {
  unsigned char * mBytes; // Stream of bytes
  size_t mCount; // Byte count
};

class ByteReader {
public:
  const void * mBytes; // Resolved byte stream
  size_t mCount; // Number of bytes available in stream
  bool mForbidden; // Byte-reading explicitly disallowed?

  ByteReader (lua_State * L, int arg, bool bReplace = true);

  enum { eForbidden, eVector, eUchar, eNone };
private:
  void LookupBytes (lua_State * L, int arg);
  void PointToBytes (lua_State * L, int arg, int option);
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
// **__bytes**. Should no value at all be found, the object clearly does not
// honor the protocol, so we quit.
//
// When **__bytes** has a value of **"forbidden"**, the object's type is
// understood to say "byte-reading is specifically disallowed". In this
// situation, there is nothing more we can do, so we quit; the reader's
// **mForbidden** member will be **true** to indicate the special
// circumstances.
//
// If access is allowed, we next check whether the object is a full userdata.
// If so, there are a few possibilities:
//
// - **__bytes** has a value of **"uchar"**: the userdata's first bytes contain
// a `ByteReaderUchar` struct, with
// **mBytes** and **mCount** members.
//
// - **__bytes** has a value of **"vector"**: the userdata's first bytes hold a
// `std::vector<unsigned char>`. Its contents supply the bytes, with `size()`
// as the count. **BYTE\_READER\_NO\_VECTOR** may be defined to disable the
// vector code path.
//
// - Last but not least, we use the userdata's bytes and length directly, as
// done with strings. When **__bytes** contains an integer, it is assumed to be
// an offset (between 0 and the length minus 1) where valid bytes begin, with
// the final length shortened accordingly.
//
// If the object was not a full userdata, **__bytes** must be a function, to be
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