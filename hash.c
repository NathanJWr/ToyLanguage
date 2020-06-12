/*
    This file is part of Ample.

    Ample is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Ample is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Ample.  If not, see <https://www.gnu.org/licenses/>.
*/
#include "hash.h"
#include "ssl.h"
uint64_t
hash_string (const char *s)
{
  const unsigned char *us = (const unsigned char *)s;
  uint64_t h = 0;
  while (*us != '\0')
    {
      h += h * HASH_MULTIPLIER + *us;
      us++;
    }
  return h;
}
bool
string_compare (const char *key, const char *input)
{
  return (0 == strcmp (key, input));
}

bool
int_compare (int key, int input)
{
  return (key == input);
}
