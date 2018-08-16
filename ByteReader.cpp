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

// Byte reader function for std::vector<unsigned char>
void BR_NAMESPACE::VectorReader (lua_State * L, ByteReader & reader, int arg, void *)
{
  auto vec = static_cast<std::vector<unsigned char> *>(lua_touserdata(L, arg));

  reader.mBytes = vec->data();
  reader.mCount = vec->size();
}

void BR_NAMESPACE::VectorReader (lua_State * L, class ByteReader & reader, const std::vector<unsigned char> & vec)
{
  const void * ptr = &vec;

  lua_pushlightuserdata(L, const_cast<void *>(ptr)); // ..., vec

  VectorReader(L, reader, -1, nullptr);

  lua_pop(L, 1); // ...
}

// Constructor
ByteReader::ByteReader (lua_State * L, int arg, bool bReplace) : mPos{arg}
{
  if (arg < 0 && -arg <= lua_gettop(L)) mPos = lua_gettop(L) + arg + 1; // account for negative indices in stack

  // Take the object's length as a best guess of the byte count. If the object is a string,
  // these are exactly the bytes we want.
  mCount = lua_objlen(L, mPos);

  if (lua_isstring(L, mPos)) mBytes = lua_tostring(L, mPos);

  // Otherwise, the object must be a full userdata, whose __bytes metafield is queried.
  else
  {
    if (lua_type(L, mPos) == LUA_TUSERDATA && luaL_getmetafield(L, mPos, "__bytes")) // ...[, __bytes]
    {
      bool bGrew = LookupBytes(L);

      if (bGrew && bReplace && mBytes) lua_replace(L, mPos); // ..., bytes, ...
    }

    else PushError(L, "Unable to read bytes from %s at index %d"); // ..., err
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

  Register(L, func, true); // ...

  return func;
}

// Try to get bytes from an object's __bytes metafield
bool ByteReader::LookupBytes (lua_State * L)
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

    if (registered) return PointToBytes(L, func);

	else PushError(L, "Unregistered reader attached to %s at index %d");
  }

  else
  {
    lua_pushvalue(L, mPos); // ..., __bytes, object

    if (lua_pcall(L, 1, 1, 0) == 0) // ..., bytes / false[, err]
    {
	  ByteReader result{L, -1, true};

      mBytes = result.mBytes;
      mCount = result.mCount;

      return true;
    }
  }

  return false;
}

// Point to the userdata's bytes, possibly at an offset
bool ByteReader::PointToBytes (lua_State * L, ByteReaderFunc * func)
{
  if (lua_type(L, mPos) == LUA_TUSERDATA)
  {
    if (func)
    {
      int top = lua_gettop(L);

      if (func->mGetBytes(L, *this, mPos, func->mContext) && lua_gettop(L) > top)
	  {
        if (lua_gettop(L) - top > 1) lua_pushliteral(L, "Returned too many arguments");

        else return true;
	  }
	}

    else mBytes = static_cast<unsigned char *>(lua_touserdata(L, mPos));
  }

  else PushError(L, "Cannot point to %s at index %d");

  return false;
}

// Wrapper for common error-pushing pattern
void ByteReader::PushError (lua_State * L, const char * format)
{
  lua_pushfstring(L, format, luaL_typename(L, mPos), mPos); // ..., err
}