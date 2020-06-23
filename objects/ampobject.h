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
#ifndef AMP_OBJECT_H_
#define AMP_OBJECT_H_
typedef enum AmpObjectType { AMP_OBJ_INT, AMP_OBJ_STR, AMP_OBJ_BOOL } AmpObjectType;

struct AmpObject;
typedef struct AmpObject AmpObject;

typedef struct AmpOperations {
  AmpObject *(*add)(AmpObject *, AmpObject *);
  AmpObject *(*sub)(AmpObject *, AmpObject *);
  AmpObject *(*div)(AmpObject *, AmpObject *);
  AmpObject *(*mult)(AmpObject *, AmpObject *);
} AmpOperations;
typedef struct AmpObjectInfo {
  AmpObjectType type;
  AmpOperations ops;
} AmpObjectInfo;
#define AMP_OBJECT_HEADER                                                      \
  unsigned int refcount;                                                       \
  AmpObjectInfo *info;                                                         \
  void (*dealloc)(AmpObject *)

struct AmpObject {
  AMP_OBJECT_HEADER;
};
#define AMP_OBJECT(obj) ((AmpObject *)(obj))
void AmpObjectIncrementRefcount(AmpObject *obj);
void AmpObjectDecrementRefcount(AmpObject *obj);
void AmpObjectDestroyBasic(AmpObject *obj);

AmpObject *AmpObjectUnsupportedOperation(AmpObject *, AmpObject *);
void AmpObjectInitializeOperationsToUnsupported (AmpOperations *ops);
#endif