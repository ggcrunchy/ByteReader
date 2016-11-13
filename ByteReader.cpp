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

#ifndef BYTE_READER_NO_VECTOR

// Byte reader function for std::vector<unsigned char>
void VectorReader (lua_State * L, ByteReader & reader, int arg, void *)
{
  auto vec = static_cast<std::vector<unsigned char> *>(lua_touserdata(L, arg));

  reader.mBytes = vec->data();
  reader.mCount = vec->size();
}

#endif

// Constructor
ByteReader::ByteReader (lua_State * L, int arg, bool bReplace) : mBytes(nullptr)
{
  // Take the object's length as a best guess of the byte count. If the object is a string,
  // these are exactly the bytes we want.
  mCount = lua_objlen(L, arg);

  if (lua_isstring(L, arg)) mBytes = lua_tostring(L, arg);

  // Otherwise, the object must be a full userdata, whose __bytes metafield is queried.
  else
  {
    if (arg < 0 && -arg <= lua_gettop(L)) arg = lua_gettop(L) + arg + 1; // account for negative indices in stack

    if (lua_type(L, arg) == LUA_TUSERDATA && luaL_getmetafield(L, arg, "__bytes")) // ...[, __bytes]
    {
      bool bGrew = LookupBytes(L, arg);

      if (bGrew && bReplace && mBytes) lua_replace(L, arg); // ..., bytes, ...
    }

    else PushError(L, "Unable to read bytes from %s at index %i", arg); // ..., err
  }
}

// Helper to register a reader function
void ByteReader::Register (lua_State * L, ByteReaderFunc * func, bool bUseTop)
{
  lua_pushlightuserdata(L, func); // ...[, top], func
  lua_rawget(L, LUA_REGISTRYINDEX);	// ...[, top], func?

  if (!lua_isnil(L, -1)) lua_pop(L, bUseTop ? 2 : 1); // ...

  else
  {
    lua_pop(L, 1); // ...[, top]
    lua_pushlightuserdata(L, func); // ...[, top], func

	if (bUseTop) lua_insert(L, -2); // ..., func, top

    else lua_pushboolean(L, 1); // ..., func, true

	lua_rawset(L, LUA_REGISTRYINDEX); // ...; registry[func] = top / true
  }
}

// Helper to register a reader function that handles some of the memory details
ByteReaderFunc * ByteReader::Register (lua_State * L)
{
  ByteReaderFunc * func = static_cast<ByteReaderFunc *>(lua_newuserdata(L, sizeof(ByteReaderFunc))); // ..., func

  func->mGetBytes = nullptr;
  func->mContext = nullptr;

  Register(L, func, true);

  return func;
}

// Try to get bytes from an object's __bytes metafield
bool ByteReader::LookupBytes (lua_State * L, int arg)
{
  if (!lua_isfunction(L, -1))
  {
    ByteReaderFunc * func = nullptr;
    int registered = 1;

    if (lua_type(L, -1) == LUA_TLIGHTUSERDATA)
    {
      func = static_cast<ByteReaderFunc *>(lua_touserdata(L, -1));

      lua_rawget(L, LUA_REGISTRYINDEX);	// ..., func?

      registered = !lua_isnil(L, -1);
    }

	lua_pop(L, 1); // ...

    if (registered) PointToBytes(L, arg, func);

	else PushError(L, "Unregistered reader attached to %s at index %i", arg);
  }

  else
  {
    lua_pushvalue(L, arg); // ..., __bytes, object

    if (lua_pcall(L, 1, 1, 0) == 0) // ..., bytes / false[, err]
    {
      ByteReader result(L, -1, true);

      mBytes = result.mBytes;
      mCount = result.mCount;

      return true;
    }
  }

  return false;
}

// Point to the userdata's bytes, possibly at an offset
void ByteReader::PointToBytes (lua_State * L, int arg, ByteReaderFunc * func)
{
  if (lua_type(L, arg) == LUA_TUSERDATA)
  {
    if (func) func->mGetBytes(L, *this, arg, func->mContext);

    else mBytes = static_cast<unsigned char *>(lua_touserdata(L, arg));
  }

  else PushError(L, "Cannot point to %s at index %i", arg);
}

// Wrapper for common error-pushing pattern
void ByteReader::PushError (lua_State * L, const char * format, int arg)
{
  lua_pushfstring(L, format, luaL_typename(L, arg), arg); // ..., err
}