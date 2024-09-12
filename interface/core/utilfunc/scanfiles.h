// x3py framework: https://github.com/rhcad/x3py
#ifndef X3_UTILFUNC_SCANDIR_H
#define X3_UTILFUNC_SCANDIR_H

#ifndef _WIN32
#include <dirent.h>
#include <vector>
#include <string>
#endif

namespace x3 {

/**
* @brief 加载文件夹下的所有插件
* @param[in] filter 函数指针，该函数可根据插件的绝对路径进行加载
* @param[in] path 插件所在的文件夹
* @param[in] recursive 是否递归加载子文件夹中的插件
* @return 返回加载插件的总数量
*/
inline int scanfiles(bool (*filter)(const char* filename, const char* ext), 
                     const char* path, bool recursive)
{
    int count = 0;
    char filename[MAX_PATH];
    bool cancel = false;

#ifdef _WIN32
    //lstrcpynA 字符串拷贝
    // PathAppendA 字符串拼接,会自动在中间插入\\，得到插件文件夹\\*.*
    lstrcpynA(filename, path, MAX_PATH);
    PathAppendA(filename, "*.*");

    WIN32_FIND_DATAA fd;
    HANDLE hfind = ::FindFirstFileA(filename, &fd);
    BOOL loop = (hfind != INVALID_HANDLE_VALUE);

    for (; loop && !cancel; loop = ::FindNextFileA(hfind, &fd))
    {
        //若fd是隐藏文件或系统文件
        if (fd.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM))
        {           
        }
        else if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)//若fd是文件夹
        {
            if (fd.cFileName[0] != '.')//若不是隐藏文件夹
            {
                //如果fd是文件夹，就进行字符串拼接，获取文件夹绝对路径
                lstrcpynA(filename, path, MAX_PATH);
                PathAppendA(filename, fd.cFileName);
                PathAddBackslashA(filename);

                //这里传入文件夹路径，肯定加载不成功，但函数会恒定返回true，再取反就是false
                // Q 这里为什么要做这样的动作呢？调用一次有什么意义呢？
                cancel = !filter(filename, "");
                if (recursive && !cancel)
                {
                    //递归调用，加载插件
                    count += scanfiles(filter, filename, recursive);
                }
            }
        }
        else
        {
            //字符串拼接，得到插件DLL的绝对路径
            lstrcpynA(filename, path, MAX_PATH);
            PathAppendA(filename, fd.cFileName);

            //根据绝对路径加载插件，函数函数会恒定返回true，再取反就是false
            cancel = !filter(filename, PathFindExtensionA(filename));
            count++;
        }
    }

    ::FindClose(hfind);
#else
    std::vector<std::string> dirs;
    struct dirent *dp;
    DIR *dirp;

    dirp = opendir(path);
    if (dirp)
    {
        while (!cancel && (dp = readdir(dirp)) != NULL)
        {
            if (DT_DIR == dp->d_type && dp->d_name[0] != '.')
            {
                dirs.push_back(dp->d_name);
            }
            else if (DT_REG == dp->d_type)
            {
                strncpy(filename, path, MAX_PATH);
                PathAppendA(filename, dp->d_name);
                cancel = !filter(filename, "");
                count++;
            }
        }
        closedir(dirp);
    }

    std::vector<std::string>::const_iterator it = dirs.begin();
    for (; it != dirs.end() && !cancel; ++it)
    {
        strncpy(filename, path, MAX_PATH);
        PathAppendA(filename, it->c_str());
        PathAddBackslashA(filename);

        cancel = !filter(filename, PathFindExtensionA(filename));
        if (recursive && !cancel)
        {
            count += scanfiles(filter, filename, recursive);
        }
    }
#endif

    return count;
}

} // x3
#endif