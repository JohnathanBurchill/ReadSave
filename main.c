/*

    ReadSave: main.c

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

#include "main.h"
#include "readsave.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

int main(int argc, char **argv)
{
    int status = 0;

    int nOptions = 0;
    bool summarize = false;
    char *variableName = NULL;

    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "--help") == 0)
        {
            usage(argv[0]);
            return EXIT_SUCCESS;
        }
        else if (strcmp(argv[i], "--about") == 0)
        {
            aboutThisProgram();
            return EXIT_SUCCESS;
        }
        else if (strcmp(argv[i], "--variable-summary") == 0)
        {
            nOptions++;
            summarize = true;
        }
        else if (strncmp(argv[i], "--variable=", 11) == 0)
        {
            if (strlen(argv[i]) == 11)
            {
                fprintf(stderr, "Missing variable name for %s\n", argv[i]);
                return EXIT_FAILURE;
            }
            nOptions++;
            variableName = argv[i] + 11;
        }
        else if (strncmp(argv[i], "--", 2) == 0)
        {
            fprintf(stderr, "Unknown option %s\n", argv[i]);
            return EXIT_FAILURE;
        }
    }

    if (argc - nOptions != 2)
    {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    char *savFile = argv[1];
    if (strcmp(savFile + strlen(savFile)-4, ".sav") != 0)
    {
        fprintf(stderr, "Expected a save file with extension .sav.\n");
        return EXIT_FAILURE;
    }

    VariableList variables = {0};
    SaveInfo fileInfo = {0};

    status = readSave(savFile, &fileInfo, &variables);

    fprintf(stdout, "SAV file created %s by %s.\n", fileInfo.date, fileInfo.operator);

    Variable *var = NULL;
    Variable *selectedVar = NULL;
    if (summarize)
    {
        if (variableName == NULL)
        {
            fprintf(stdout, "Variables:\n");
            for (int i = 0; i < variables.nVariables; i++)
                fprintf(stdout, " %s\n", variables.variableList[i].name);
        }
        else
            for (int i = 0; i < variables.nVariables; i++)
            {
                var = &variables.variableList[i];
                if (var->isArray && var->isStructure)
                {
                    for (int v = 0; v < var->arrayInfo.nElements; v++)
                    {
                        selectedVar = variableData(&((Variable*)var->data)[i], variableName);
                        if (selectedVar != NULL)
                        {
                            summarizeVariable(selectedVar);
                            break;
                        }
                    }
                }
                else
                {
                    selectedVar = variableData(var, variableName);
                    if (selectedVar != NULL)
                    {
                        summarizeVariable(selectedVar);
                        break;
                    }
                }

            }
    }
    // Other things to do

    bool extract = true;
    if (extract)
    {
        for (int i = 0; i < variables.nVariables; i++)
        {
            var = &variables.variableList[i];
            if (var->isArray && var->isStructure)
            {
                for (int v = 0; v < 1; v++)
                {
                    selectedVar = variableData(&((Variable*)var->data)[v], variableName);
                }
            }
            else
            {
                selectedVar = variableData(var, variableName);
                if (selectedVar != NULL)
                {
                    summarizeVariable(selectedVar);
                    break;
                }
            }
            if (selectedVar != NULL)
            {
                void *data = selectedVar->data;
                if (selectedVar->arrayInfo.nElements == 0)
                {
                    switch(selectedVar->dataType)
                    {
                        case DataTypeByte:
                            printf("%d\n", *(uint8_t*)data);
                            break;
                        case DataTypeInt16:
                            printf("%d\n", *(int16_t*)data);
                            break;
                        case DataTypeUInt16:
                            printf("%u\n", *(uint16_t*)data);
                            break;
                        case DataTypeInt32:
                            printf("%d\n", *(int32_t*)data);
                            break;
                        case DataTypeUInt32:
                            printf("%u\n", *(uint32_t*)data);
                            break;
                        case DataTypeInt64:
                            printf("%ld\n", *(int64_t*)data);
                            break;
                        case DataTypeUInt64:
                            printf("%ld\n", *(uint64_t*)data);
                            break;
                        case DataTypeFloat:
                            printf("%f\n", *(float*)data);
                            break;
                        case DataTypeDouble:
                            printf("%lf\n", *(double*)data);
                            break;
                        default:
                    }

                }
                else
                    for (int i = 0; i < selectedVar->arrayInfo.nElements; i++)
                    {
                        switch(selectedVar->dataType)
                        {
                            case DataTypeByte:
                                printf("%d\n", ((uint8_t*)data)[i]);
                                break;
                            case DataTypeInt16:
                                printf("%d\n", ((int16_t*)data)[i]);
                                break;
                            case DataTypeUInt16:
                                printf("%u\n", ((uint16_t*)data)[i]);
                                break;
                            case DataTypeInt32:
                                printf("%d\n", ((int32_t*)data)[i]);
                                break;
                            case DataTypeUInt32:
                                printf("%u\n", ((uint32_t*)data)[i]);
                                break;
                            case DataTypeInt64:
                                printf("%ld\n", ((int64_t*)data)[i]);
                                break;
                            case DataTypeUInt64:
                                printf("%ld\n", ((uint64_t*)data)[i]);
                                break;
                            case DataTypeFloat:
                                printf("%f\n", ((float*)data)[i]);
                                break;
                            case DataTypeDouble:
                                printf("%lf\n", ((double*)data)[i]);
                                break;
                            default:
                        }
                    }
            }
        }

    }

    return EXIT_SUCCESS;

}

void usage(char *name)
{
    fprintf(stdout, "Usage: %s <file.sav> [--variable-summary] [--variable=<variableName[.tag1][.tag2]...>] [--help] [--about]\n", name);
    fprintf(stdout, "Reads an IDL save file.\n");
    fprintf(stdout, "Options:\n");
    fprintf(stdout, "%20s : summary of save file variables.\n", "--variable-summary");
    fprintf(stdout, "%20s : this summary\n", "--help");
    fprintf(stdout, "%20s : author and license information\n", "--about");
    fprintf(stdout, "%s\n", "--variable=<variableName[.tag1][.tag2]...>");
    fprintf(stdout, "%s : operate on variableName, with optional structures tags tag1, tag2, etc., e.g., --variable=SKYMAP.PROJECT_UID\n", "");
    return;
}

void aboutThisProgram(void)
{
    fprintf(stdout, "IDL save file (.sav) variable reader.\n");
    fprintf(stdout, "Copyright (2022) Johnathan K. Burchill\n");
    fprintf(stdout, "This implementation is based on notes by Craig Markwardt (https://pages.physics.wisc.edu/~craigm/idl/savefmt/savefmt.html).\n");
    fprintf(stdout, "This program comes with ABSOLUTELY NO WARRANTY.\n");
    fprintf(stdout, "This is free software, and you are welcome to redistribute it\n");
    fprintf(stdout, "under the terms of the GNU General Public License.\n");
    fprintf(stdout, "See the file LICENSE in the source repository for details.\n");
    return;
}

