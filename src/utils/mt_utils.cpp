
#include "utils/mt_utils.h"

namespace mtsai 
{
namespace utils
{
    double cpuSecond() 
    {
        struct timeval tp;
        gettimeofday(&tp, NULL);
        return ( (double)tp.tv_sec + (double)tp.tv_usec*1.e-6);
    }

    bool has_only_digits(const std::string s) 
    {
        return s.find_first_not_of( "0123456789" ) == std::string::npos;
    }
    
    /*
    * Split string by specific token
    * text       : source string
    * separator  : character for split string
    * return     : container contains separated strings
    */
    std::vector<std::string> split(const std::string &text, char separator) 
    {
        std::vector<std::string> tokens;
        size_t start=0, end=0;
        while((end = text.find(separator, start)) != std::string::npos) {
            if (end != start)
            {
                tokens.push_back(text.substr(start, end - start));
            }
            start = end + 1;
        }
        if (end != start)
        {
            tokens.push_back(text.substr(start));
        }
        return tokens;
    }

    /*
    * Split string by multiple token
    */
    template<typename Elem>
    inline std::vector<tstring<Elem>> splitMultipleToken(tstring<Elem> text, tstring<Elem> const & delimiters)
    {
        auto tokens = std::vector<tstring<Elem>>{};
        size_t pos, prev_pos = 0;

        // std::string::npos means "no matches"
        while ((pos = text.find_first_of(delimiters, prev_pos)) != std::string::npos)
        {
            if (pos > prev_pos)
            {
                tokens.push_back(text.substr(prev_pos, pos - prev_pos));
            }
            prev_pos = pos + 1;
        }

        if (prev_pos < text.length())
        {
            // std::string::npos means "the end of string"
            tokens.push_back(text.substr(prev_pos, std::string::npos));
        }
        return tokens;
    }
    
    

    /*
     * Compute the similarity of two string
     */
    int editDistance(std::string a, std::string b)
    {
        std::string tmpA = a, tmpB = b;
        if(a.length() > b.length()) {
            tmpA = b;
            tmpB = a;
        }
        /// <summary>
        /// Computes the Levenshtein distance between two strings.

        // Allocate distance matrix
        std::vector<std::vector<int> > d(tmpA.length()+1, std::vector<int>(tmpB.length()+1) );
        // Compute distance
        for (int i = 0; i <= tmpA.length(); i++)
            d[i][0] = i;
        for (int j = 0; j <= tmpB.length(); j++)
            d[0][j] = j;

        for (int i = 1; i <= tmpA.length(); i++)
        {
            for (int j = 1; j <= tmpB.length(); j++)
            {
                if(tmpA[i - 1] == tmpB[j - 1])
                {
                    // No change required
                    d[i][j] = d[i - 1][j - 1];
                }
                else
                {
                    d[i][j] =
                        std::min(d[i - 1][j] + 1,    // Deletion
                        std::min(d[i][j - 1] + 1,    // Insertion
                        d[i - 1][j - 1] + 1));       // Substitution
                }
            }
        }

        // Return final value
        return d[ tmpA.length() ][ tmpB.length() ];
    }

    /***
     * Directory Relative (Linux only)
     */ 
    bool checkFolderExist (std::string target_folder) {

        struct stat sb;
        if (stat(target_folder.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode))
        {
            // printf("%s exists\n", target_folder.c_str());
            return true;
        }
        else
        {
            printf("%s does not exist\n", target_folder.c_str());
            return false;
        }
    }
    bool checkFolderRecursive(std::string target_folder) {

        bool slash_first = false;
        if (target_folder.front() == '/')
        {
            slash_first = true;
        }

        std::vector<std::string> path_list = split(target_folder, '/');
        std::string path = "";
        if (slash_first)
        {
            path = "/";
        }

        for (int i = 0; i < path_list.size(); ++i)
        {
            path = path + path_list.at(i);
            if (path.back() != '/')
            {
                path += "/";
            }
            if (!checkFolderExist(path))
            {
                // printf("%s does not exist\n", path.c_str());
                return false;
            } else {
                // printf("%s exists\n", path.c_str());
            }
        }

        return true;
    }

    bool createFolder(std::string target_folder) {

        // printf("create target_folder: %s\n", target_folder.c_str());

        const int dir_err = mkdir(target_folder.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        // const int dir_err = mkdir(target_folder.c_str(), 755);
        if (-1 == dir_err)
        {
            printf("Error creating directory!\n");
            return false;
        } else {
            return true;
        }
    }

    bool createFolderRecursive(std::string target_folder) {

        bool slash_first = false;
        if (target_folder.front() == '/')
        {
            slash_first = true;
        }

        std::vector<std::string> path_list = split(target_folder, '/');

        std::string path = "";
        if (slash_first)
        {
            path = "/";
        }

        for (int i = 0; i < path_list.size(); ++i)
        {
            path = path + path_list.at(i) + '/';
            if (!checkFolderExist(path))
            {
                if (!createFolder(path)) {
                    printf("create %s faile\n", path.c_str());
                    return false;
                } else {
                    // printf("%s is created\n", path.c_str());
                }
            } else {
                // printf("%s is exist\n", path.c_str());
            }
        }

        return true;
    }




    template std::vector<tstring<char>> splitMultipleToken(tstring<char> text, tstring<char> const & delimiters);
} // namespace utils
} // namespace mtsai

