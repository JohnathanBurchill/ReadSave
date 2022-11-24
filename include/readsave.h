/*

    ReadSave: include/readsave.h

    Copyright (C) 2022  Johnathan K Burchill

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef _READSAVE_H
#define _READSAVE_H

#include <stdlib.h>
#include <stdbool.h>

enum RecordTypes
{
    RecordTypeNotHandled = -1,
    RecordTypeStartMarker = 0,
    RecordTypeCommonVariable = 1,
    RecordTypeVariable = 2,
    RecordTypeSystemVariable = 3,
    RecordTypeEndMarker = 6,
    RecordTypeTimestamp = 10,
    RecordTypeCompiled = 12,
    RecordTypeIdentification = 13,
    RecordTypeVersion = 14,
    RecordTypeHeapHeader = 15,
    RecordTypeHeapData = 16,
    RecordTypePromote64 = 17,
    RecordTypeNotice = 19
};

enum VariableFlags
{
    VariableFlagsSystemVariable = 0x02,
    VariableFlagsArray = 0x04,
    VariableFlagsUnknown = 0x10,
    VariableFlagsStructure = 0x20
};

enum DataTypes
{
    DataTypeUndefined = 0,
    DataTypeByte = 1,
    DataTypeInt16 = 2,
    DataTypeInt32 = 3,
    DataTypeFloat = 4,
    DataTypeDouble = 5,
    DataTypeComplexFloat = 6,
    DataTypeString = 7,
    DataTypeStructure = 8,
    DataTypeComplexDouble = 9,
    DataTypeHeapPointer = 10,
    DataTypeObjectReference = 11,
    DataTypeUInt16 = 12,
    DataTypeUInt32 = 13,
    DataTypeInt64 = 14,
    DataTypeUInt64 = 15
};

typedef struct ArrayInfo
{
    long nBytesPerElement;
    long nBytes;
    long nElements;
    long nDims;
    long unknown1;
    long unknown2;
    long nMax;
    long dims[8];

} ArrayInfo;

typedef struct StructureInfo
{
    char *structureName;
    long predef;
    long nTags;
    char *className;
    long nSupClasses;
    char **supClassNames;
    void *supClasses; // Array of Variable for supp class information

} StructureInfo;

typedef struct Variable
{
    char *name;
    long dataType;
    long flags;
    void *data;
    bool isScalar;
    bool isStructure;
    bool isArray;
    ArrayInfo arrayInfo;
    StructureInfo structInfo;
} Variable;

typedef struct VariableList
{
    Variable *variableList;
    size_t nVariables;
} VariableList;

typedef struct SaveInfo
{
    char *date;
    char *operator;

} SaveInfo;

enum ReadSave
{
    READSAVE_OK = 0,
    READSAVE_INPUT_FILE = 1,
    READSAVE_MEM = 2,
    READSAVE_READ_SCALAR = 3,
    READSAVE_READ_ARRAY = 4,
    READSAVE_READ_STRUCTURE = 5,
    READSAVE_READ_VARIABLE = 6,
    READSAVE_FILE_VERSION = 7,
    READSAVE_ARGUMENTS = 8

};

int readSave(char *filename, SaveInfo *info, VariableList *variables);

int readString(char *bytes, long nBytes, long *offset, char **str);
float readFloat(char *bytes, long nBytes, long *offset);
double readDouble(char *bytes, long nBytes, long *offset);
long readLong(char *bytes, long nBytes, long *offset);
unsigned long readULong(char *bytes, long nBytes, long *offset);
short readShort(char *bytes, long nBytes, long *offset);
unsigned short readUShort(char *bytes, long nBytes, long *offset);
unsigned char readByte(char *bytes, long nBytes, long *offset);

int readVariable(char *bytes, long nBytes, long *offset, VariableList *variables);
int readScalar(char *bytes, long nBytes, long *offset, Variable *var);
int initArray(char *bytes, long nBytes, long *offset, Variable *var);
int readArray(char *bytes, long nBytes, long *offset, Variable *var);
int initStructure(char *bytes, long nBytes, long *offset, Variable *variable);
int copyStructure(Variable *dst, Variable *src);
int copyStructureInfo(StructureInfo *dst, StructureInfo *src);
int readStructure(char *bytes, long nBytes, long *offset, Variable *variable);

int summarizeVariables(VariableList *variables);
int summarizeStructure(Variable *variable, int indent);

#endif // _READSAVE_H
