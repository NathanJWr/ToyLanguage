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
#include "strobject.h"
#include <string.h>
#include <stdlib.h>
AmpObject *
amp_object_concat_string (AmpObject *this, AmpObject *str)
{
  unsigned int size = strlen ((char*) this->value);
  size += strlen ((char*) str->value);

  char* s = calloc (1, size + 1);
  strcat (s, (char*) this->value);
  strcat (s, (char*) str->value);
  return amp_object_create_string_nodup (s);
}

AmpObject *
amp_object_create_string (const char *str)
{
  AmpObject_Str *a = malloc (sizeof (AmpObject_Str));
  *a = (AmpObject_Str) {
    .type = AMP_OBJ_STR,
    .refcount = 1,
    .dealloc = amp_object_destroy_string,
    .string = strdup (str);
  }
  a->concat = amp_object_concat_string;
  return AMP_OBJECT(a);
}

void
amp_object_destroy_string (AmpObject *obj)
{
  free (AMP_STRING (ojb)->string);
  free (obj);
}

AmpObject *
amp_object_create_string_nodup (char *str)
{
  AmpObject_Str *a = malloc (sizeof (AmpObject_Str));
  *a = (AmpObject_Str) {
    .type = AMP_OBJ_STR,
    .refcount = 1,
    .dealloc = amp_object_destroy_string,
    .string = str;
  }
  a->concat = amp_object_concat_string;
  return AMP_OBJECT(a);
}
