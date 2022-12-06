/*

    ReadSave: readsave.c

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

#include "readsave.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>

int readSave(char *savFile, SaveInfo *info, VariableList *variables)
{
    if (savFile == NULL || info == NULL || variables == NULL)
        return READSAVE_ARGUMENTS;

    int status = READSAVE_OK;
    
    struct stat fileInfo = {0};
    status = stat(savFile, &fileInfo);
    if (status != 0)
        return READSAVE_INPUT_FILE;

    FILE *f = fopen(savFile, "r");
    if (f == NULL)
        return READSAVE_INPUT_FILE;

    unsigned char *bytes = calloc(fileInfo.st_size, 1);
    if (bytes == NULL)
    {
        fclose(f);
        return READSAVE_MEM;
    }

    long nBytes = fileInfo.st_size;
    size_t read = fread(bytes, 1, fileInfo.st_size, f);
    fclose(f);
    if (read != (size_t)nBytes)
    {
        free(bytes);
        return READSAVE_INPUT_FILE;
    }

    if (strncmp(bytes, "SR", 2) != 0)
    {
        free(bytes);
        return READSAVE_INPUT_FILE;
    }

    if (bytes[2] != 0 || (bytes[3] != 4 && bytes[3] != 5))
    {
        free(bytes);
        return READSAVE_FILE_VERSION;
    }

    long offset = 4;

    long recordType = RecordTypeNotHandled;
    long nextRecordLowWord = 0;
    long nextRecordHighWord = 0;
    long nextOffset = 0;

    char *savInfo[6] = {0};
    char *date = "unknown";
    char *operator = "unknown";

    while (recordType != RecordTypeEndMarker && offset > 0 && offset < nBytes - 4)
    {
        recordType = readLong(bytes, nBytes, &offset);

        nextRecordLowWord = readULong(bytes, nBytes, &offset);
        nextRecordHighWord = readULong(bytes, nBytes, &offset);
        nextOffset = nextRecordLowWord + 65536*nextRecordHighWord;
        offset += 4;

        switch(recordType)
        {
            case RecordTypeTimestamp:
                offset += 4 * 256;
                for (int i = 0; i < 3; i++)
                {
                    status = readString(bytes, nBytes, &offset, &(savInfo[i]));
                    if (status != 0)
                        goto cleanup;
                }
                if (savInfo[0] != NULL)
                    info->date = strdup(savInfo[0]);
                else
                    info->date = strdup(date);

                if (savInfo[1] != NULL)
                    info->operator = strdup(savInfo[1]);
                else
                    info->operator = strdup(operator);
                offset = nextOffset;
                break;

            case RecordTypeVersion:
                for (int i = 3; i < 6; i++)
                {
                    status = readString(bytes, nBytes, &offset, &(savInfo[i]));
                    if (status != 0)
                        goto cleanup;
                }
                offset = nextOffset;
                break;

            case RecordTypeEndMarker:
                break;

            case RecordTypeVariable:
                status = readVariable(bytes, nBytes, &offset, variables);
                if (status != 0)
                    goto cleanup;
                offset = nextOffset;
                break;

            default:
                offset = nextOffset;
        }

    }


cleanup:

    if (bytes != NULL)
        free(bytes);
    for (int i = 0; i < 6; i++)
        if (savInfo[i] != NULL)
            free(savInfo[i]);

    return status;

}

void about(void)
{
    fprintf(stdout, "ReadSave: IDL save file (.sav) variable reader (C library).\n");
    fprintf(stdout, "Copyright (2022) Johnathan K. Burchill\n");
    fprintf(stdout, "This implementation is based on notes by Craig Markwardt (https://pages.physics.wisc.edu/~craigm/idl/savefmt/savefmt.html).\n");
    fprintf(stdout, "This program comes with ABSOLUTELY NO WARRANTY.\n");
    fprintf(stdout, "This is free software, and you are welcome to redistribute it\n");
    fprintf(stdout, "under the terms of the GNU General Public License.\n");
    fprintf(stdout, "See the file LICENSE in the source repository for details.\n");
    return;
}

int readString(unsigned char *bytes, long nBytes, long *offset, char **str)
{
    long strLength = readLong(bytes, nBytes, offset);
    *str = strndup((unsigned char *)(bytes + *offset), (unsigned int) strLength);
    if (*str == NULL)
        return READSAVE_MEM;
    long padded = 0;
    while (padded < strLength)
        padded += 4;
    *offset += padded;
    return READSAVE_OK;

}

float readFloat(unsigned char *bytes, long nBytes, long *offset)
{
    if (offset != NULL && bytes != NULL && *offset < nBytes - 3)
    {
        unsigned char f[4] = {0};
        for (int i = 0; i < 4; i++)
            f[i] = bytes[*offset + 3 - i];

        float value = *(float*)f;
        *offset += 4;
        return value; 
    }
    else
        return 0;
}

double readDouble(unsigned char *bytes, long nBytes, long *offset)
{
    if (offset != NULL && bytes != NULL && *offset < nBytes - 3)
    {
        unsigned char f[8] = {0};
        for (int i = 0; i < 8; i++)
            f[i] = bytes[*offset + 7 - i];

        double value = *(double*)f;
        *offset += 8;
        return value; 
    }
    else
        return 0;
}

long readLong(unsigned char *bytes, long nBytes, long *offset)
{
    if (offset != NULL && bytes != NULL && *offset < nBytes - 3)
    {
        long value = bytes[*offset + 3] + 256 * bytes[*offset + 2] + 256 * 256 * bytes[*offset + 1] + 256 * 256 * 256 * bytes[*offset];
        *offset+=4;
        return value; 
    }
    else
        return 0;
}

unsigned long readULong(unsigned char *bytes, long nBytes, long *offset)
{
    if (offset != NULL && bytes != NULL && *offset < nBytes)
    {
        unsigned long value = bytes[*offset + 3] + 256 * bytes[*offset + 2] + 256 * 256 * bytes[*offset + 1] + 256 * 256 * 256 * bytes[*offset];
        *offset+=4;
        return value; 
    }
    else
        return 0;
}

short readShort(unsigned char *bytes, long nBytes, long *offset)
{
    if (offset != NULL && bytes != NULL && *offset < nBytes)
    {
        short value = (short)(bytes[*offset + 3] + 256 * bytes[*offset + 2]);
        *offset += 4;
        return value; 
    }
    else
        return 0;
}

unsigned short readUShort(unsigned char *bytes, long nBytes, long *offset)
{
    if (offset != NULL && bytes != NULL && *offset < nBytes)
    {
        unsigned short value = bytes[*offset + 3] + 256 * bytes[*offset + 2];
        *offset += 4;
        return value; 
    }
    else
        return 0;
}

unsigned char readByte(unsigned char *bytes, long nBytes, long *offset)
{
    if (offset != NULL && bytes != NULL && *offset < nBytes)
    {
        long redundant = readLong(bytes, nBytes, offset);
        unsigned char value = bytes[*offset];
        *offset += 4;
        return value; 
    }
    else
        return 0;
}

int readVariable(unsigned char *bytes, long nBytes, long *offset, VariableList *variables)
{
    void *mem = realloc(variables->variableList, sizeof(Variable)*(variables->nVariables + 1));
    if (mem == NULL)
        return READSAVE_MEM;

    variables->variableList = mem;
    variables->nVariables++;
    Variable *var = &(variables->variableList[variables->nVariables-1]);
    bzero(var, sizeof(Variable));

    int status = 0;
    status = readString(bytes, nBytes, offset, &var->name);
    if (status != 0)
        return status;

    long dataType = readLong(bytes, nBytes, offset);
    var->dataType = dataType;
    var->flags = readLong(bytes, nBytes, offset);

    var->isArray = (var->flags & VariableFlagsArray) != 0;
    var->isStructure = (var->flags & VariableFlagsStructure) != 0;
    var->isScalar = !var->isArray && !var->isStructure;

    long variableStart = 0;

    // Read additional variable information as required
    Variable *tmp = NULL;
    if (var->isStructure)
    {
        Variable structDefinition = {0};
        structDefinition.name = strdup(var->name);
        structDefinition.isArray = true;
        structDefinition.isStructure = true;
        structDefinition.dataType = DataTypeStructure;
        structDefinition.flags = var->flags;
        status = initArray(bytes, nBytes, offset, &structDefinition);
        if (status != 0)
            return status;
 
        status = initStructure(bytes, nBytes, offset, &structDefinition);
        if (status != 0)
            return status;
 
        void *mem = calloc(structDefinition.arrayInfo.nElements, sizeof(Variable));
        if (mem == NULL)
            return READSAVE_MEM;
 
        var->data = mem;
        memcpy(&var->arrayInfo, &structDefinition.arrayInfo, sizeof(ArrayInfo));
        status = copyStructureInfo(&var->structInfo, &structDefinition.structInfo);
        if (status != 0)
            return status;
 
        for (int i = 0; i < structDefinition.arrayInfo.nElements; i++)
        {
            tmp = &(((Variable*)var->data)[i]);
            status = copyStructure(tmp, &structDefinition);
            tmp->isArray=false;
            if (status != 0)
                return status;
        }
    }
    else if (var->isArray)
    {
        status = initArray(bytes, nBytes, offset, var);
        if (status != 0)
            return status;
    }
    variableStart = readLong(bytes, nBytes, offset);
    if (variableStart != 7)
            return READSAVE_READ_VARIABLE;
    if (var->isStructure)
    {
        for (int i = 0; i < var->arrayInfo.nElements; i++)
        {
            status = readStructure(bytes, nBytes, offset, &(((Variable*)var->data)[i]));
            if (status != 0)
                return status;
        }
    }
    else if (var->isArray)
    {
        variableStart = readLong(bytes, nBytes, offset);
        if (variableStart != 7)
            return READSAVE_READ_VARIABLE;
        status = readArray(bytes, nBytes, offset, var);
        if (status != 0)
            return status;
    }
    else
    {
        variableStart = readLong(bytes, nBytes, offset);
        if (variableStart != 7)
            return READSAVE_READ_VARIABLE;
        status = readScalar(bytes, nBytes, offset, var);
        if (status != 0)
            return status;
    }

    return 0;

}

int readScalar(unsigned char *bytes, long nBytes, long *offset, Variable *var)
{
    if (offset == NULL || bytes == NULL || *offset >= nBytes || var == NULL)
        return READSAVE_ARGUMENTS;

    void *mem = NULL;

    int status = 0;

    long redundant = 0;
    switch (var->dataType)
    {
        case DataTypeString:
            char *str = NULL;
            long redundant = readLong(bytes, nBytes, offset);
            status = readString(bytes, nBytes, offset, &str);
            if (status != 0)
                return status;
            var->data = str;
            break;

        case DataTypeByte:
            mem = malloc(sizeof(unsigned char));
            redundant = readLong(bytes, nBytes, offset);
            if (mem == NULL)
                return READSAVE_MEM;            
            var->data = mem;
            *(unsigned char*)(var->data) = bytes[*offset];
            *offset+=4; // Next 32-bit boundary
            break;

        case DataTypeInt16:
        case DataTypeUInt16:
            mem = malloc(sizeof(int16_t));
            if (mem == NULL)
                return READSAVE_MEM;
            var->data = mem;
            *(int16_t*)(var->data) = readLong(bytes, nBytes, offset);
            break;

        case DataTypeInt32:
        case DataTypeUInt32:
        case DataTypeFloat:
            mem = malloc(sizeof(float));
            if (mem == NULL)
                return READSAVE_MEM;
            var->data = mem;
            *(float*)(var->data) = readFloat(bytes, nBytes, offset);
            break;

        case DataTypeInt64:
        case DataTypeUInt64:
        case DataTypeDouble:
            mem = malloc(sizeof(double));
            if (mem == NULL)
                return READSAVE_MEM;
            var->data = mem;
            *(double*)(var->data) = readDouble(bytes, nBytes, offset);
            break;

        case DataTypeComplexFloat:
            mem = malloc(2 * sizeof(float));
            if (mem == NULL)
                return READSAVE_MEM;
            var->data = mem;
            ((float*)var->data)[0] = readFloat(bytes, nBytes, offset);
            ((float*)var->data)[1] = readFloat(bytes, nBytes, offset);
            break;

        case DataTypeComplexDouble:
            mem = malloc(2 * sizeof(double));
            if (mem == NULL)
                return READSAVE_MEM;
            var->data = mem;
            ((double*)var->data)[0] = readDouble(bytes, nBytes, offset);
            ((double*)var->data)[1] = readDouble(bytes, nBytes, offset);
            break;

        default:
            break;
    }

    return 0;
}

int initArray(unsigned char *bytes, long nBytes, long *offset, Variable *var)
{
    if (offset == NULL || bytes == NULL || *offset >= nBytes || var == NULL)
        return READSAVE_ARGUMENTS;

    long arrayStart = readLong(bytes, nBytes, offset);
    if (arrayStart != 8)
        return READSAVE_READ_ARRAY;

    var->isArray = true;

    int status = 0;

    var->arrayInfo.nBytesPerElement = readLong(bytes, nBytes, offset);
    if (var->isStructure)
        var->arrayInfo.nBytesPerElement = sizeof(Variable);

    var->arrayInfo.nBytes = readLong(bytes, nBytes, offset);
    var->arrayInfo.nElements = readLong(bytes, nBytes, offset);;
    var->arrayInfo.nDims = readLong(bytes, nBytes, offset);
    var->arrayInfo.unknown1 = readLong(bytes, nBytes, offset);
    var->arrayInfo.unknown2 = readLong(bytes, nBytes, offset);
    var->arrayInfo.nMax = readLong(bytes, nBytes, offset);

    for (int i = 0; i < var->arrayInfo.nMax; i++)
        var->arrayInfo.dims[i] = readLong(bytes, nBytes, offset);

    void *mem = NULL;
    mem = calloc(var->arrayInfo.nElements, var->arrayInfo.nBytesPerElement);
    if (mem == NULL)
        return READSAVE_MEM;
    var->data = mem;

    return READSAVE_OK;
}

int readArray(unsigned char *bytes, long nBytes, long *offset, Variable *var)
{
    if (offset == NULL || bytes == NULL || *offset >= nBytes || var == NULL)
        return READSAVE_ARGUMENTS;

    unsigned char b[16] = {0};
    long redundant = 0;
    switch(var->dataType)
    {
        case DataTypeByte:
            redundant = readLong(bytes, nBytes, offset);
            memcpy((unsigned char*)var->data, bytes + *offset, var->arrayInfo.nElements * var->arrayInfo.nBytesPerElement);
            *offset += var->arrayInfo.nElements * var->arrayInfo.nBytesPerElement;
            while (*offset % 4 != 0)
                *offset += 1; // Next 32-bit boundary

            break;

        case DataTypeInt16:
        case DataTypeUInt16:
            for (int i = 0; i < var->arrayInfo.nElements; i++)
            {
                b[0] = bytes[*offset + 4*i + 3];
                b[1] = bytes[*offset + 4*i + 2];
                b[2] = bytes[*offset + 4*i + 1];
                b[3] = bytes[*offset + 4*i];
                ((uint16_t*)var->data)[i] = (uint16_t)(*(uint32_t*)b);
            }
            *offset += var->arrayInfo.nElements * 4;
            break;

        case DataTypeInt32:
        case DataTypeUInt32:
        case DataTypeFloat:
            for (int i = 0; i < var->arrayInfo.nElements; i++)
            {
                b[0] = bytes[*offset + 4*i + 3];
                b[1] = bytes[*offset + 4*i + 2];
                b[2] = bytes[*offset + 4*i + 1];
                b[3] = bytes[*offset + 4*i];
                ((uint32_t*)var->data)[i] = *(uint32_t*)b;
            }
            *offset += var->arrayInfo.nElements * 4;
            break;

        case DataTypeInt64:
        case DataTypeUInt64:
        case DataTypeDouble:
            for (int i = 0; i < var->arrayInfo.nElements; i++)
            {
                b[0] = bytes[*offset + 8*i + 7];
                b[1] = bytes[*offset + 8*i + 6];
                b[2] = bytes[*offset + 8*i + 5];
                b[3] = bytes[*offset + 8*i + 4];
                b[4] = bytes[*offset + 8*i + 3];
                b[5] = bytes[*offset + 8*i + 2];
                b[6] = bytes[*offset + 8*i + 1];
                b[7] = bytes[*offset + 8*i];
                ((uint64_t*)var->data)[i] = *(uint64_t*)b;
            }
            *offset += var->arrayInfo.nElements * 8;
            break;

        case DataTypeComplexFloat:
            for (int i = 0; i < var->arrayInfo.nElements; i++)
            {
                b[0] = bytes[*offset + 8*i + 7];
                b[1] = bytes[*offset + 8*i + 6];
                b[2] = bytes[*offset + 8*i + 5];
                b[3] = bytes[*offset + 8*i + 4];
                b[4] = bytes[*offset + 8*i + 3];
                b[5] = bytes[*offset + 8*i + 2];
                b[6] = bytes[*offset + 8*i + 1];
                b[7] = bytes[*offset + 8*i];
                ((float*)var->data)[2*i] = *(float*)(b+4);
                ((float*)var->data)[2*i+1] = *(float*)(b);
            }
            *offset += var->arrayInfo.nElements * 8;
            break;

        case DataTypeComplexDouble:
            for (int i = 0; i < var->arrayInfo.nElements; i++)
            {
                b[0] = bytes[*offset + 8*i + 15];
                b[1] = bytes[*offset + 8*i + 14];
                b[2] = bytes[*offset + 8*i + 13];
                b[3] = bytes[*offset + 8*i + 12];
                b[4] = bytes[*offset + 8*i + 11];
                b[5] = bytes[*offset + 8*i + 10];
                b[6] = bytes[*offset + 8*i + 9];
                b[7] = bytes[*offset + 8*i + 8];
                b[8] = bytes[*offset + 8*i + 7];
                b[9] = bytes[*offset + 8*i + 6];
                b[10] = bytes[*offset + 8*i + 5];
                b[11] = bytes[*offset + 8*i + 4];
                b[12] = bytes[*offset + 8*i + 3];
                b[13] = bytes[*offset + 8*i + 2];
                b[14] = bytes[*offset + 8*i + 1];
                b[15] = bytes[*offset + 8*i];
                ((double*)var->data)[2*i] = *(double*)(b+8);
                ((double*)var->data)[2*i+1] = *(double*)(b);
            }
            *offset += var->arrayInfo.nElements * 16;
            break;



        default:
    }

    return 0;
}

int initStructure(unsigned char *bytes, long nBytes, long *offset, Variable *variable)
{
    if (bytes == NULL || offset == NULL || *offset >= nBytes || variable == NULL)
        return READSAVE_ARGUMENTS;

    int status = 0;

    long structStart = readLong(bytes, nBytes, offset);
    if (structStart != 9)
        return READSAVE_READ_STRUCTURE;

    variable->isStructure = true;

    StructureInfo *info = &variable->structInfo;

    status = readString(bytes, nBytes, offset, &info->structureName);
    if (status != 0)
        return status;
    if (info->structureName == NULL || strlen(info->structureName) == 0)
    {
        char *name = "<anomymous structure>";
        info->structureName = strdup(name);
    }

    info->predef = readLong(bytes, nBytes, offset);
    info->nTags = readLong(bytes, nBytes, offset);
    long dummy = readLong(bytes, nBytes, offset);

    void *mem = calloc(info->nTags, sizeof(Variable));
    if (mem == NULL)
        return READSAVE_MEM;
    variable->data = (Variable*) mem;
    info->nTags;

    if ((info->predef & 0x01) == 1)
        return 0;

    Variable *var = NULL;
    for (int i = 0; i < info->nTags; i++)
    {
        var = &((Variable*)variable->data)[i];
        *offset += 4;
        var->dataType = readLong(bytes, nBytes, offset);
        var->flags = readLong(bytes, nBytes, offset);
    }

    for (int i = 0; i < info->nTags; i++)
    {
        var = &((Variable*)variable->data)[i];
        status = readString(bytes, nBytes, offset, &var->name);
        if (status != 0)
            return status;
    }

    // Read Array descriptions
    for (int i = 0; i < info->nTags; i++)
    {
        var = &((Variable*)variable->data)[i];
        if ((var->flags & 0x04) == 0 && (var->flags & 0x20) == 0)
            var->isScalar = true;
        if ((var->flags & 0x04) != 0)
        {
            status = initArray(bytes, nBytes, offset, var);
            if (status != 0)
                return status;
        }
    }

    // Read Structure descriptions
    for (int i = 0; i < info->nTags; i++)
    {
        var = &((Variable*)variable->data)[i];
        if ((var->flags & 0x20) != 0)
        {
            var->isStructure = true;
            status = initStructure(bytes, nBytes, offset, var);
            if (status != 0)
                return status;
            var->isArray = false;
        }
    }

    if ((info->predef & 0x02) != 0 || (info->predef & 0x04) != 0)
    {
        status = readString(bytes, nBytes, offset, &info->className);
        if (status != 0)
            return status;

        info->nSupClasses = readLong(bytes, nBytes, offset);

        if (info->nSupClasses > 0)
        {
            info->supClassNames = malloc(info->nSupClasses * sizeof(char*));
            if (info->supClassNames == NULL)
                return READSAVE_MEM;
            for (int s = 0; s < info->nSupClasses; s++)
            {
                status = readString(bytes, nBytes, offset, &(info->supClassNames[s]));
                    return status;
            }
            info->supClasses = malloc(info->nSupClasses * sizeof(Variable));
            for (int s = 0; s < info->nSupClasses; s++)
            {
                status = initStructure(bytes, nBytes, offset, &((Variable*)&info->supClasses)[s]);
                if (status != 0)
                    return status;
            }

        }

    }

    return READSAVE_OK;
}

int readStructure(unsigned char *bytes, long nBytes, long *offset, Variable *var)
{
    if (bytes == NULL || offset == NULL || *offset >= nBytes || var == NULL)
        return READSAVE_ARGUMENTS;

    int status = READSAVE_OK;

    Variable *tag = NULL;
    for (int i = 0; i < var->structInfo.nTags; i++)
    {
        tag = &((Variable *)var->data)[i];
        if (tag->isStructure)
        {
            status = readStructure(bytes, nBytes, offset, tag);
            if (status != 0)
                return status;
        }
        else if (tag->isArray)
        {
            status = readArray(bytes, nBytes, offset, tag);
            if (status != 0)
                return status;
        }
        else
        {
            status = readScalar(bytes, nBytes, offset, tag);
            if (status != 0)
                return status;
        }
    }

    return READSAVE_OK;

}

int summarizeStructure(Variable *variable, int indent)
{
    if (variable == NULL || !variable->isStructure)
        return READSAVE_ARGUMENTS;

    char typeName[255] = {0};

    Variable *tag = NULL;
    for (int i = 0; i < variable->structInfo.nTags; i++)
    {
        tag = &(((Variable *)variable->data)[i]);
        for (int d = 0; d < indent; d++)
            fprintf(stdout, " ");
        fprintf(stdout, ".%s", tag->name);
        if (tag->isStructure)
        {
            fprintf(stdout, "\n");
            summarizeStructure(tag, indent + 2);
        }
        else if (tag->isArray)
        {
            dataTypeName(tag->dataType, typeName);
            fprintf(stdout, " %s array(", typeName);
            for (int d = 0; d < tag->arrayInfo.nDims; d++)
            {
                fprintf(stdout, "%ld", tag->arrayInfo.dims[d]);
                if (d < tag->arrayInfo.nDims - 1)
                    fprintf(stdout, ",");
            }
            fprintf(stdout, ")\n");
        }
        else
        {
            dataTypeName(tag->dataType, typeName);
            fprintf(stdout, " %s", typeName);
            switch(tag->dataType)
            {
                case DataTypeString:
                    fprintf(stdout, " \"%s\"\n", (char*)(tag->data));
                    break;
                case DataTypeByte:
                    fprintf(stdout, " %d\n", *(unsigned char*)(tag->data));
                    break;
                case DataTypeInt16:
                    fprintf(stdout, " %d\n", *(int16_t*)(tag->data));
                    break;
                case DataTypeUInt16:
                    fprintf(stdout, " %u\n", *(uint16_t*)(tag->data));
                    break;
                case DataTypeInt32:
                    fprintf(stdout, " %d\n", *(int32_t*)(tag->data));
                    break;
                case DataTypeUInt32:
                    fprintf(stdout, " %u\n", *(uint32_t*)(tag->data));
                    break;
                case DataTypeInt64:
                    fprintf(stdout, " %ld\n", *(int64_t*)(tag->data));
                    break;
                case DataTypeUInt64:
                    fprintf(stdout, " %lu\n", *(uint64_t*)(tag->data));
                    break;
                case DataTypeFloat:
                    fprintf(stdout, " %f\n", *(float*)(tag->data));
                    break;
                case DataTypeDouble:
                    fprintf(stdout, " %lf\n", *(double*)(tag->data));
                    break;
                case DataTypeComplexFloat:
                    fprintf(stdout, "\n");
                    break;
                case DataTypeComplexDouble:
                    fprintf(stdout, "\n");
                    break;
                default:
                    fprintf(stdout, "\n");
            }
        }
    }

    return READSAVE_OK;
}

int copyStructure(Variable *dst, Variable *src)
{
    if (dst == NULL || src == NULL)
        return READSAVE_ARGUMENTS;

    int status = READSAVE_OK;

    if (src->name != NULL)
        dst->name = strdup(src->name);

    dst->dataType = src->dataType;
    dst->flags = src->flags;
    dst->isScalar = src->isScalar;
    dst->isArray = src->isArray;
    dst->isStructure = src->isStructure;
    memcpy(&dst->arrayInfo, &src->arrayInfo, sizeof(ArrayInfo));
    status = copyStructureInfo(&dst->structInfo, &src->structInfo);
    if (status != 0)
        return status;

    long nTags = src->structInfo.nTags;
    void *mem = calloc(nTags, sizeof(Variable));
    if (mem == NULL)
        return READSAVE_MEM;
    dst->data = mem;

    for (int i = 0; i < nTags; i++)
    {
        Variable *srctag = &(((Variable*)src->data)[i]);
        Variable *dsttag = &(((Variable*)dst->data)[i]);
        if (srctag->name != NULL)
            dsttag->name = strdup(srctag->name);
        dsttag->dataType = srctag->dataType;
        dsttag->flags = srctag->flags;
        dsttag->isScalar = srctag->isScalar;
        dsttag->isArray = srctag->isArray;
        dsttag->isStructure = srctag->isStructure;
        if (srctag->isStructure)
        {
            status = copyStructure(dsttag, srctag);
            if (status != 0)
                return status;
        }
        else if (srctag->isArray)
        {
            memcpy(&dsttag->arrayInfo, &srctag->arrayInfo, sizeof(ArrayInfo));
            mem = calloc(srctag->arrayInfo.nElements, srctag->arrayInfo.nBytesPerElement);
            if (mem == NULL)
                return READSAVE_MEM;
            dsttag->data = mem;

        }
    }

    return READSAVE_OK;
}

int copyStructureInfo(StructureInfo *dst, StructureInfo *src)
{
    if (dst == NULL || src == NULL)
        return READSAVE_ARGUMENTS;

    if (src->structureName != NULL)
    {
        dst->structureName = strdup(src->structureName);
        if (dst->structureName == NULL)
            return READSAVE_MEM;
    }

    dst->predef = src->predef;
    dst->nTags = src->nTags;
    dst->nSupClasses = src->nSupClasses;

    void *mem = NULL;

    if (src->className != NULL)
    {
        dst->className = strdup(src->className);
        if (dst->className == NULL)
            return READSAVE_MEM;
    }
    if (src->nSupClasses > 0)
    {
        mem = calloc(src->nSupClasses, sizeof(char *));
        if (mem == NULL)
            return READSAVE_MEM;
        dst->supClassNames = mem;

        for (int i = 0; i < src->nSupClasses; i++)
        {
            dst->supClassNames[i] = strdup(src->supClassNames[i]);
            if (dst->supClassNames[i] == NULL)
                return READSAVE_MEM;
        }

        mem = calloc(src->nSupClasses, sizeof(Variable));
        if (mem == NULL)
            return READSAVE_MEM;
        dst->supClasses = (Variable*)mem;
        memcpy(dst->supClasses, src->supClasses, src->nSupClasses * sizeof(Variable));

    }

    return READSAVE_OK;
}

int summarizeVariables(VariableList *variables)
{
    if (variables == NULL)
        return READSAVE_ARGUMENTS;

    Variable *var = NULL;
    bool isArray = false;
    bool isStruct = false;
    for (int i = 0; i < variables->nVariables; i++)
    {
        var = &(variables->variableList[i]);
        summarizeVariable(var);
    }

    return READSAVE_OK;

}

int summarizeVariable(Variable *var)
{
    if (var == NULL)
        return READSAVE_ARGUMENTS;

    int status = READSAVE_OK;

    char typeName[255] = {0};

    // Array holding structures of the same kind
    if (var->isStructure && var->isArray)
    {
        for (int i = 0; i < var->arrayInfo.nElements; i++)
        {
            status = summarizeVariable(&((Variable*)var->data)[i]);
            if (status != 0)
                return status;
        }
        return status;        
    }

    if (var->isScalar)
    {
        dataTypeName(var->dataType, typeName);
        fprintf(stdout, "%s (%s scalar)\n", var->name, typeName);
    }
    else if (var->isStructure)
    {
        fprintf(stdout, "%s (structure)\n", var->name);
        summarizeStructure(var, 2);
    }
    else if (var->isArray)
    {
        dataTypeName(var->dataType, typeName);
        fprintf(stdout, "%s (%s array(", var->name, typeName);
        for (int d = 0; d < var->arrayInfo.nDims; d++)
        {
            fprintf(stdout, "%ld", var->arrayInfo.dims[d]);
            if (d < var->arrayInfo.nDims - 1)
                fprintf(stdout, ",");
        }
        fprintf(stdout, "))\n");
    }
    else
        fprintf(stdout, " (no information)\n");

    return READSAVE_OK;
}

Variable * variableData(Variable *variable, char *dottedTagName)
{
    if (variable == NULL || dottedTagName == NULL)
        return NULL;

    if (!variable->isStructure || variable->structInfo.nTags == 0 || variable->data == NULL)
        return NULL;

    Variable *var = variable;

    char *requestedTag = strdup(dottedTagName);
    if (requestedTag == NULL)
        return NULL;

    for (int i = 0; i < strlen(requestedTag); i++)
        requestedTag[i] = toupper(requestedTag[i]);

    // Adapted from the strsep manual page example:
    char **ap = NULL;
    char *tagFields[42] = {0};
    int nRequestedTags = 0;
    for (ap = tagFields; (*ap = strsep(&requestedTag, ".")) != NULL;)
    {
        if (**ap != 0)
        {
            nRequestedTags++;
            if (++ap >= &tagFields[42])
                break;
        }
    }

    if (strcmp(tagFields[0], var->name) != 0)
        return NULL;

    if (nRequestedTags == 1)
        return var;

    int depth = 1;
    Variable *tag = NULL;
    int i = 0;
    long nTags = var->structInfo.nTags;
    for (i = 0; i < nTags; i++)
    {
        tag = &((Variable*)var->data)[i];
        if (strcmp(tagFields[depth], tag->name) == 0)
        {
            depth++;
            if (depth == nRequestedTags)
                return tag;
            if (!tag->isStructure)
                return NULL;
            var = tag;
            i = -1;
            nTags = var->structInfo.nTags;
        }
        
    }

    return NULL;

}

void dataTypeName(long dataType, char *name)
{
    if (name == NULL)
        return;

    switch (dataType)
    {
        case DataTypeByte:
            sprintf(name, "%s", "<s8>");
            break;
        case DataTypeInt16:
            sprintf(name, "%s", "<s16>");
            break;
        case DataTypeUInt16:
            sprintf(name, "%s", "<u16>");
            break;
        case DataTypeInt32:
            sprintf(name, "%s", "<s32>");
            break;
        case DataTypeUInt32:
            sprintf(name, "%s", "<u32>");
            break;
        case DataTypeInt64:
            sprintf(name, "%s", "<s64>");
            break;
        case DataTypeUInt64:
            sprintf(name, "%s", "<u64>");
            break;
        case DataTypeFloat:
            sprintf(name, "%s", "<float>");
            break;
        case DataTypeDouble:
            sprintf(name, "%s", "<double>");
            break;
        case DataTypeComplexFloat:
            sprintf(name, "%s", "<complex-float>");
            break;
        case DataTypeComplexDouble:
            sprintf(name, "%s", "<complex-double>");
            break;
        case DataTypeString:
            sprintf(name, "%s", "<string>");
            break;
        case DataTypeStructure:
            sprintf(name, "%s", "<structure>");
            break;
        case DataTypeHeapPointer:
            sprintf(name, "%s", "<heap-pointer>");
            break;
        case DataTypeObjectReference:
            sprintf(name, "%s", "<obj-ref>");
            break;
        break;
            sprintf(name, "%s", "<?>");
    }
    
    return;
}
