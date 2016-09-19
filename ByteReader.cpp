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

#include "ByteReader.h"

// Constructor
ByteReader::ByteReader (lua_State * L, int arg, bool bReplace) : mBytes(NULL), mForbidden(false)
{
  // Take the object's length as a best guess of the byte count. If the object is a string,
  // these are exactly the bytes we want. Otherwise, look up the __bytes metafield.
  mCount = lua_objlen(L, arg);

  if (lua_isstring(L, arg)) mBytes = lua_tostring(L, arg);

  else
  {
    if (arg < 0 && -arg <= lua_gettop(L)) arg = lua_gettop(L) + arg + 1; // account for negative indices in stack

    if (luaL_getmetafield(L, arg, "__bytes")) // ...[, __bytes]
    {
      LookupBytes(L, arg);

      if (bReplace && mBytes) lua_replace(L, arg); // ..., bytes, ...
    }

    else PushError(L, "Unable to read bytes from %s at index %i", arg); // ..., err
  }
}

// Try to get bytes from an object's __bytes metafield
void ByteReader::LookupBytes (lua_State * L, int arg)
{
  const char * names[] = { "forbidden", "vector", "uchar", "none", NULL };
  int options[] = { eForbidden, eVector, eUchar, eNone };
  int res = lua_isstring(L, arg) ? options[luaL_checkoption(L, arg, "none", names)] : eNone;

  if (res == eForbidden)
  {
    mForbidden = true;

    PushError(L, "Byte-reading explicitly forbidden from %s at index %i", arg); // ..., __bytes, err
  }

  else if (!lua_isfunction(L, -1)) PointToBytes(L, arg, res);

  else
  {
    lua_pushvalue(L, arg); // ..., __bytes, object

    if (lua_pcall(L, 1, 1, 0) == 0) // ..., bytes / false[, err]
    {
      ByteReader result(L, -1, false);

      mBytes = result.mBytes;
      mCount = result.mCount;
    }
  }
}

// Point to the userdata's bytes, possibly at an offset
void ByteReader::PointToBytes (lua_State * L, int arg, int option)
{
  if (lua_type(L, arg) == LUA_TUSERDATA)
  {
    if (option == eVector)
    {
#ifndef BYTE_READER_NO_VECTOR
      auto vec = (std::vector<unsigned char> *)lua_touserdata(L, arg);

      mBytes = &vec[0];
      mCount = vec->size();
#else
      PushError(L, "Vector support is disabled");
#endif
    }

    else if (option == eUchar)
    {
      auto uchar = (ByteReaderUchar *)lua_touserdata(L, arg);

      mBytes = uchar->mBytes;
      mCount = uchar->mCount;
    }

    else
    {
      lua_Integer offset = lua_isnumber(L, -1) ? luaL_optinteger(L, -1, 0) : 0;

      if (offset < 0 || size_t(offset) > mCount) PushError(L, "Offset of %s at index %i fails bounds check", arg);

      else
      {
        mBytes = ((unsigned char *)lua_touserdata(L, arg)) + offset;
        mCount -= offset;
      }
    }
  }

  else PushError(L, "Cannot point to %s at index %i", arg);
}

// Wrapper for common error-pushing pattern
void ByteReader::PushError (lua_State * L, const char * format, int arg)
{
  lua_pushfstring(L, format, luaL_typename(L, arg), arg); // ..., err
}