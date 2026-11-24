// Optional headless ROM runner; link against mGBA built with the matching flags.h.
// Usage: emulator_runner ROM SAVE < commands
// Commands: wait FRAMES, key GBA_BITMASK, shot PATH.ppm, reset, save PATH.sav.
#include <mgba-util/vfs.h>
#include <mgba/core/core.h>
#include <mgba/flags.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static color_t pixels[240 * 160];
int main(int argc, char** argv)
{
    if (argc != 3)
        return 2;
    struct mCore* core = mCoreFind(argv[1]);
    if (!core || !core->init(core))
        return 3;
    mCoreInitConfig(core, "addrboy-test");
    core->setVideoBuffer(core, pixels, 240);
    if (!mCoreLoadFile(core, argv[1]))
        return 4;
    if (strcmp(argv[2], "-") && !mCoreLoadSaveFile(core, argv[2], true))
        return 5;
    core->reset(core);
    char command[32], argument[1024];
    while (scanf("%31s", command) == 1)
    {
        if (!strcmp(command, "reset"))
        {
            core->reset(core);
            continue;
        }
        if (scanf("%1023s", argument) != 1)
            return 6;
        if (!strcmp(command, "wait"))
            for (int i = 0; i < atoi(argument); ++i)
                core->runFrame(core);
        else if (!strcmp(command, "key"))
        {
            core->setKeys(core, (uint32_t)strtoul(argument, NULL, 0));
            for (int i = 0; i < 8; ++i)
                core->runFrame(core);
            core->setKeys(core, 0);
            for (int i = 0; i < 45; ++i)
                core->runFrame(core);
        }
        else if (!strcmp(command, "shot"))
        {
            FILE* file = fopen(argument, "wb");
            if (!file)
                return 7;
            fprintf(file, "P6\n240 160\n255\n");
            for (int i = 0; i < 240 * 160; ++i)
            {
                uint8_t rgb[] = { pixels[i] & 255, (pixels[i] >> 8) & 255, (pixels[i] >> 16) & 255 };
                fwrite(rgb, 1, 3, file);
            }
            fclose(file);
        }
        else if (!strcmp(command, "save"))
        {
            void* data = NULL;
            size_t size = core->savedataClone(core, &data);
            FILE* file = fopen(argument, "wb");
            if (!file || !data)
                return 8;
            fwrite(data, 1, size, file);
            fclose(file);
            free(data);
        }
        else
            return 9;
    }
    core->deinit(core);
    return 0;
}
