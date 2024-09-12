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
* @brief �����ļ����µ����в��
* @param[in] filter ����ָ�룬�ú����ɸ��ݲ���ľ���·�����м���
* @param[in] path ������ڵ��ļ���
* @param[in] recursive �Ƿ�ݹ�������ļ����еĲ��
* @return ���ؼ��ز����������
*/
inline int scanfiles(bool (*filter)(const char* filename, const char* ext), 
                     const char* path, bool recursive)
{
    int count = 0;
    char filename[MAX_PATH];
    bool cancel = false;

#ifdef _WIN32
    //lstrcpynA �ַ�������
    // PathAppendA �ַ���ƴ��,���Զ����м����\\���õ�����ļ���\\*.*
    lstrcpynA(filename, path, MAX_PATH);
    PathAppendA(filename, "*.*");

    WIN32_FIND_DATAA fd;
    HANDLE hfind = ::FindFirstFileA(filename, &fd);
    BOOL loop = (hfind != INVALID_HANDLE_VALUE);

    for (; loop && !cancel; loop = ::FindNextFileA(hfind, &fd))
    {
        //��fd�������ļ���ϵͳ�ļ�
        if (fd.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM))
        {           
        }
        else if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)//��fd���ļ���
        {
            if (fd.cFileName[0] != '.')//�����������ļ���
            {
                //���fd���ļ��У��ͽ����ַ���ƴ�ӣ���ȡ�ļ��о���·��
                lstrcpynA(filename, path, MAX_PATH);
                PathAppendA(filename, fd.cFileName);
                PathAddBackslashA(filename);

                //���ﴫ���ļ���·�����϶����ز��ɹ�����������㶨����true����ȡ������false
                // Q ����ΪʲôҪ�������Ķ����أ�����һ����ʲô�����أ�
                cancel = !filter(filename, "");
                if (recursive && !cancel)
                {
                    //�ݹ���ã����ز��
                    count += scanfiles(filter, filename, recursive);
                }
            }
        }
        else
        {
            //�ַ���ƴ�ӣ��õ����DLL�ľ���·��
            lstrcpynA(filename, path, MAX_PATH);
            PathAppendA(filename, fd.cFileName);

            //���ݾ���·�����ز��������������㶨����true����ȡ������false
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