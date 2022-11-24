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

int main(int argc, char **argv)
{
    int status = 0;

    int nOptions = 0;
    bool showVariables = false;

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
            showVariables = true;
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


    if (showVariables)
        summarizeVariables(&variables);

    // Other things to do

    return EXIT_SUCCESS;

}

void usage(char *name)
{
    fprintf(stdout, "Usage: %s <file.sav> [--variable-summary] [--help] [--about]\n", name);
    fprintf(stdout, "Reads an IDL save file.\n");
    fprintf(stdout, "Options:\n");
    fprintf(stdout, "%20s : summary of save file variables.\n", "--variable-summary");
    fprintf(stdout, "%20s : this summary\n", "--help");
    fprintf(stdout, "%20s : author and license information\n", "--about");
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

