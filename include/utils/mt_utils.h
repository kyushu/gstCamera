#ifndef WMAI_UTILS_HPP
#define WMAI_UTILS_HPP

#include <string>
#include <sstream>
#include <vector>
#include <chrono>
#include <sys/time.h> // gettimeofday
#include <ctime> /* time_t, struct tm, difftime, time, mktime */

// Directory relative
#include <dirent.h>
#include <sys/stat.h>   // create directory

namespace mtsai
{
namespace utils
{

    double cpuSecond();
    
    bool has_only_digits(const std::string s);
    
    /*
    * Split string by specific token
    * text       : source string
    * separator  : character for split string
    * return     : container contains separated strings
    */
    std::vector<std::string> split(const std::string &text, char separator);

    template <class Elem>
    using tstring = std::basic_string<Elem, std::char_traits<Elem>, std::allocator<Elem> >;
    template <class Elem>
    using tstringstream = std::basic_stringstream<Elem, std::char_traits<Elem>, std::allocator<Elem>>;

    template<typename Elem>
    inline std::vector<tstring<Elem>> splitMultipleToken(tstring<Elem> text, tstring<Elem> const & delimiters);

    /*
     * Compute the similarity of two string
     */
    int editDistance(std::string a, std::string b);

    /***
     * Directory Relative (Linux only)
     */ 
    bool checkFolderExist (std::string target_folder);
    bool checkFolderRecursive(std::string target_folder);
    bool createFolder(std::string target_folder);
    bool createFolderRecursive(std::string target_folder);

    

} // utuils
} // mtsai


#endif