#include "common.h"

namespace WOF
{

    //////////////////////////////////////////////////////////////////////////

    std::string File::cwd()
    {
        const size_t size = 4096;
        char* buf = (char*)malloc(size);
        if (nullptr == buf)
            return std::string();

        if (nullptr == getcwd(buf, size))
        {
            buf[0] = 0;
        }
        std::string res(buf);
        free(buf);

        return res;
    }

    //---------------------------------------------------------//

    bool File::checkFileExistance(const std::string& path)
    {
        struct stat info;
        if (0 != stat(path.c_str(), &info))
            return 0;

        if (info.st_mode & S_IFREG)
            return true;

        return false;
    }

    //---------------------------------------------------------//

    std::vector<std::string> File::getDict(const std::string& path)
    {
        std::vector<std::string> res;

        std::ifstream file(path);

        std::string line;
        while (std::getline(file, line))
        {
            res.push_back(line);
        }

        return res;
    }

    //---------------------------------------------------------//

    //////////////////////////////////////////////////////////////////////////

} // namespace WOF