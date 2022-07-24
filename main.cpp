
#include <map>
#include <list>
#include <vector>
#include <set>

#include <iostream>
#include <string>
#include <wchar.h>
#include <wctype.h>

#include <time.h>
#include <fcntl.h>
#include <io.h>
#include <ostream>
#include <assert.h>
#include <fstream>
#include <codecvt>


typedef std::wstring wstring_t;
typedef unsigned int UInt32;

std::map <wstring_t, size_t> maskedMap;

// https://secure.n-able.com/webhelp/NC_9-1-0_SO_en/Content/SA_docs/API_Level_Integration/API_Integration_URLEncoding.html
//////////////////////////////////////////////////////////////////////////

const wchar_t apostrophe = 0x0027;

const wchar_t eu_upper[48] = {
   L'\x00c0', L'\x00c1', L'\x00c2', L'\x00c3', L'\x00c4', L'\x00c5', L'\x0102', L'\x00c7', L'\x0106', L'\x010c', L'\x010e', L'\x00d0',
   L'\x00c9', L'\x00c8', L'\x00ca', L'\x00cb', L'\x011e', L'\x00cc', L'\x00cd', L'\x00ce', L'\x00cf', L'\x0141', L'\x0147', L'\x00d1',
   L'\x00d2', L'\x00d3', L'\x00d4', L'\x00d5', L'\x0158', L'\x015a', L'\x0218', L'\x0164', L'\x021a', L'\x00da', L'\x00d9', L'\x00db',
   L'\x016e', L'\x00dc', L'\x00dd', L'\x0178', L'\x0179', L'\x017b', L'\x017d', L'\x00de', L'\x00c6', L'\x00d6', L'\x00d8', L'\x1e9e'
};

const wchar_t eu_lower[48] = {
   L'\x00e0', L'\x00e1', L'\x00e2', L'\x00e3', L'\x00e4', L'\x00e5', L'\x0103', L'\x00e7', L'\x0107', L'\x010d', L'\x010f', L'\x00f0',
   L'\x00e9', L'\x00e8', L'\x00ea', L'\x00eb', L'\x011f', L'\x00ec', L'\x00ed', L'\x00ee', L'\x00ef', L'\x0142', L'\x0148', L'\x00f1',
   L'\x00f2', L'\x00f3', L'\x00f4', L'\x00f5', L'\x0159', L'\x015b', L'\x0219', L'\x0165', L'\x021b', L'\x00fa', L'\x00f9', L'\x00fb',
   L'\x016f', L'\x00fc', L'\x00fd', L'\x00ff', L'\x017a', L'\x017c', L'\x017e', L'\x00fe', L'\x00e6', L'\x00f6', L'\x00f8', L'\x00df'
};

const size_t SZ = 22;

const wchar_t eu_upper_ext[SZ] = {
   L'\x0100', L'\x0104', L'\x0110', L'\x0112', L'\x0116', L'\x0118', L'\x012a', L'\x012e', L'\x0136', L'\x0139', L'\x013b',
   L'\x013d', L'\x0143', L'\x0145', L'\x014c', L'\x0150', L'\x0154', L'\x0160', L'\x016a', L'\x0170', L'\x0172', L'\x01b5'
};

const wchar_t eu_lower_ext[SZ] = {
   L'\x0101', L'\x0105', L'\x0111', L'\x0113', L'\x0117', L'\x0119', L'\x012b', L'\x012f', L'\x0137', L'\x013a', L'\x013c',
   L'\x013e', L'\x0144', L'\x0146', L'\x014d', L'\x0151', L'\x0155', L'\x0161', L'\x016b', L'\x0171', L'\x0173', L'\x01b6'
};

wstring_t cstring_to_wstring(const char* c_str)
{
   std::string cstr(c_str);
   wstring_t w_str(cstr.begin(), cstr.end());
   return w_str;
}

bool isModificatorGroup(const wchar_t ch)
{
   // general group: 02B0—02FF
   return ((ch >= 0x02b0) && (ch <= 0x02ff));
}

bool isDiacriticGroup(const wchar_t ch)
{
   // general group: 0300—036F
   const wchar_t diacr[] = // [22]
   {
      0x0301,
      0x0300,
      0x0308,
      0x0302,
      0x0311,
      0x030c,
      0x030b,
      0x030f,
      0x030a,
      0x0307,
      0x0303,
      0x0342,
      0x0304,
      0x0306,

      0x0326,

      0x032f,
      0x0331,
      0x032c,

      0x0327,
      0x0328,
      0x0337,
      0x0338
   };
   return ((ch >= 0x0300) && (ch <= 0x036F));
}

bool isOutdatedGroup(const wchar_t ch)
{
   return ((ch >= 0x0370) && (ch <= 0x03FF));
}

bool isApostrophe(const wchar_t ch)
{
   if (
      (ch == 0x0027) ||  // 39
      (ch == 0x0060) ||  // 96
      (ch == 0x0091) ||  // 145
      (ch == 0x00b4) ||  // 180 
      (ch == 0x2019)     // 8217
      )
   {
      return true;
   }
   return false;
}

wchar_t translateChar(const wchar_t ch)
{
   const wchar_t space = 0x0020;

   if (ch < space)
   {
      return space;
   }

   if ((ch >= 0x0041) && (ch <= 0x005a)) // check latin symbols
   {
      return ch;
      //return towlower(ch);
   }

   if (isApostrophe(ch))
   {
      return apostrophe;
   }

   // replace hieroglyph symbols, also: (0x2028, 0x2029)
   if (ch >= 1280) // 0x0500
   {
      if ((ch >= 0x1e00) && (ch <= 0x1eff))
      {
		  return ch;
      }
      return space;
   }

   const wchar_t replaceTable[2] =
   {
      0x00a0,  // NBPS = 160
      0x0085 //,  // NEL

      //0x0022,  // """
      //0x0028,  // ("(")
      //0x0029,  // (")") 29

      // "{", "}", "|", "\", "^", "~", "[", "]", "`"
   };

   for (UInt32 i = 0; i < sizeof(replaceTable) / sizeof(replaceTable[0]); i++)
   {
      if (replaceTable[i] == ch)
      {
         return space;
      }
   }

   // check outside if return 0, to skip this symbol 
   if (ch == 0x00ad)  // soft NewLine-symbol
   {
      return 0;
   }

   if (isModificatorGroup(ch))
   {
      wprintf(L"isModificator=%x\n", ch);
      return ch;
   }
   else if (isDiacriticGroup(ch))
   {
      wprintf(L"isDiacritic=%x\n", ch);
      return ch;
   }
   else if (isOutdatedGroup(ch))
   {
      wprintf(L"isOutdated=%x\n", ch);
      return ch;
   }

   // special symbols
   if ((ch >= 0x0080) && (ch <= 0x00a0))
   {
      return space;
   }

   // separated symbols
   const wchar_t separated[21] =
   {
      0x00a1,
      0x00a4,  // general currency sign (164)
      0x00a6,
      0x00a8,
      0x00aa,  // indicator
      0x00ab,  // l-double quotation mark
      0x00ac,  // Not sign
      //
      0x00af,  // LF (U+000A): line feed
      0x00b0,  // CR (U+000D): carriage return
      0x00b1,  // NEL (U+0085): next line
      0x00b2,  // LS (U+2028): line separator
      0x00b3,  // PS (U+2029): paragraph separator
      //
      0x00b6,  // paragraph
      0x00b7,  // middle dot 
      0x00b8,  // cedilla
      0x00ba,  // indicator
      0x00bb,  // r-double quotation mark
      0x00bc,  // fraction
      0x00bd,  // fraction
      0x00be,  // fraction
      0x00bf   // inverted question
   };

   for (UInt32 i = 0; i < sizeof(separated) / sizeof(separated[0]); i++)
   {
      if (separated[i] == ch)
      {
         return space;
      }
   }

   // return input symbol without modifications
   return ch;
}

void test_translateChar()
{
   _setmode(_fileno(stdout), _O_U16TEXT);

   for (size_t i = 0; i < 48; i++)
   {
      if (translateChar(eu_lower[i]) != eu_lower[i])
      {
         assert(false);
      }
      if (translateChar(eu_upper[i]) != eu_upper[i])
      {
         assert(false);
      }
   }

   for (size_t i = 0; i < 48; i++)
   {
      putwchar(eu_lower[i]);
      putwchar(L' ');
   }
   putwchar(L'\n');
   for (size_t i = 0; i < 48; i++)
   {
      putwchar(eu_upper[i]);
      putwchar(L' ');
   }
   putwchar(L'\n');
   for (size_t i = 0; i < SZ; i++)
   {
      putwchar(L'-');
      putwchar(L'-');
   }
   putwchar(L'\n');

   for (size_t i = 0; i < SZ; i++)
   {
      putwchar(eu_lower_ext[i]);
      putwchar(L' ');
   }
   putwchar(L'\n');
   for (size_t i = 0; i < SZ; i++)
   {
      putwchar(eu_upper_ext[i]);
      putwchar(L' ');
   }
   putwchar(L'\n');
   for (size_t i = 0; i < SZ; i++)
   {
      putwchar(L'-');
      putwchar(L'-');
   }
   putwchar(L'\n');
}

// Split by scanning inStr for the first occurrence of any of the wide characters that are part of Delim.
// Return filled list by tokens.
void wcstok(const wstring_t& inStr, const wchar_t* Delim, std::list <wstring_t>& outList)
{
   size_t offset = 0;
   while (offset < inStr.size())
   {
      const size_t pn = wcscspn(&inStr[offset], Delim);
      if (pn > 0)
      {
         outList.push_back(wstring_t(&inStr[offset], pn));
      }
      offset += pn + 1;
   }
}

//const wchar_t* wsltrim(const wstring_t& inStr, const wchar_t* Delim)
//{
//   const wchar_t * pwc = inStr.c_str();
//   const wchar_t * result = 0;
//
//   while ((pwc != NULL) && (*pwc != 0))
//   {
//      if (wcspbrk(pwc, Delim) == pwc)
//      {
//         pwc++;
//      }
//      else
//      {
//         result = pwc;
//         pwc = 0;
//      }
//   }
//   return result;
//}

inline
wstring_t& ltrim(wstring_t& inStr, const wstring_t& Delim)
{
   inStr.erase(0, inStr.find_first_not_of(Delim));
   return inStr;
}

inline
wstring_t& rtrim(wstring_t& inStr, const wstring_t& Delim)
{
   inStr.erase(inStr.find_last_not_of(Delim) + 1);
   return inStr;
}

inline
wstring_t& trim(wstring_t& inStr, const wstring_t& Delim)
{
   return ltrim(rtrim(inStr, Delim), Delim);
}

bool is_number(const wstring_t& inStr, size_t start_id = 0)
{
   bool result = (start_id < inStr.size());
   while (start_id < inStr.size())
   {
      if (!iswdigit(inStr[start_id]))
      {
         result = false;
         break;
      }
      start_id++;
   }
   return result;
}

bool is_anydigit(const wstring_t& inStr, size_t start_id = 0)
{
   bool result = false;
   while (start_id < inStr.size())
   {
      if (iswdigit(inStr[start_id]))
      {
         result = true;
         break;
      }
      start_id++;
   }
   return result;
}

bool is_digit(const wstring_t& inStr, size_t start_id = 0)
{
   bool result = true;
   while (start_id < inStr.size())
   {
      if (!iswdigit(inStr[start_id]))
      {
         result = false;
         break;
      }
      start_id++;
   }
   return result;
}

void trimming(const std::map <wstring_t, size_t>& filterMap, std::list <wstring_t>& outList)
{
   wstring_t key(L"x_)(-!?0123456789@;,.:#&%$*^'~/\\");

   for (std::list <wstring_t>::iterator it = outList.begin(); it != outList.end(); )
   {
      wstring_t& wstr = *it;

      rtrim(wstr, L"\x0022\x0027\x0028\x0029\x002d\x002a\x003a\x005b\x005d\x003f\x002e\x002f");
      ltrim(wstr, L"\x0022\x0027\x0028\x0029\x002d\x002a\x003a\x005b\x005d\x002b");

      //rtrim(wstr, L"\x0023\x0026\x0027\x0028\x0029\x002a\x002d\x002e\x002f\x003a\x003b\x003c\x003d\x003e\x003f\x005c\x007e\x00a9\x00ae\x005f");
      //ltrim(wstr, L"\x0023\x0026\x0027\x0028\x0029\x002a\x002d\x002f\x005c\x007e\x00a9\x00ae\x005f");

      {
         bool skip = true;

         wstring_t tstr = wstr;
         for (int i = 0; i < tstr.length(); i++)
         {
            if ((tstr[i] >= 0x0041) && (tstr[i] <= 0x005a))
            {
               tstr[i] = towlower(tstr[i]);
            }

            skip = skip && (wcschr(key.c_str(), tstr[i]) != NULL);
         }


         if (skip || (tstr.length() <= 2))
         {
            it = outList.erase(it);
         }
         else
         {
            if (wstr[0] == L'#')
            {
               ltrim(wstr, L"#");
               ltrim(tstr, L"#");
               it = outList.erase(it);
            }
            else
            {
               //todo: isdigit, www, https
               auto fit = filterMap.find(tstr);
               if (fit != filterMap.end() ||
                  tstr == L"&#124" ||
                  tstr == L"&quot" ||
                  tstr == L"&amp" ||
                  tstr == L"nbsp" ||
                  tstr == L"&lt" ||
                  tstr == L"&gt" ||
                  tstr.length() == 1 ||
                  tstr.find(L"http") != std::string::npos ||
                  tstr.find(L"java.") != std::string::npos ||
                  tstr.find(L"com.") != std::string::npos ||
                  tstr.find(L"org.") != std::string::npos ||
                  tstr.find(L".org") != std::string::npos ||
                  tstr.find(L".com") != std::string::npos ||
                  tstr.find(L".txt") != std::string::npos ||
                  tstr.find(L".log") != std::string::npos ||
                  tstr.find(L".exe") != std::string::npos ||
                  tstr.find(L".sh") != std::string::npos ||
                  tstr.find(L".py") != std::string::npos ||
                  tstr.find(L".js") != std::string::npos ||
                  tstr.find(L".php") != std::string::npos ||
                  tstr.find(L".htm") != std::string::npos ||
                  tstr.find(L".png") != std::string::npos ||
                  tstr.find(L".jpg") != std::string::npos ||
                  tstr.find(L"?php") != std::string::npos ||
                  tstr.find(L".jar") != std::string::npos ||
                  tstr.find(L".obj") != std::string::npos ||
                  tstr.find(L".dll") != std::string::npos ||

                  tstr.find(L"tf.") != std::string::npos ||
                  tstr.find(L"::") != std::string::npos ||
                  tstr.find(L"$$") != std::string::npos ||
                  tstr.find(L"~/") != std::string::npos ||
                  tstr.find(L"//") != std::string::npos ||
                  tstr.find(L"0x") != std::string::npos ||
                  tstr.find(L"$\\") != std::string::npos ||
                  tstr.find(L"/.") != std::string::npos ||
                  tstr.find(L"/&") != std::string::npos ||
                  tstr.find(L"/%") != std::string::npos ||
                  tstr.find(L"./") != std::string::npos ||
                  tstr.find(L"'/") != std::string::npos ||
                  tstr.find(L"('") != std::string::npos ||
                  tstr.find(L"$(") != std::string::npos ||
                  tstr.find(L").") != std::string::npos ||
                  tstr.find(L"(&") != std::string::npos ||
                  tstr.find(L"-&") != std::string::npos ||
                  tstr.find(L"%&") != std::string::npos ||
                  tstr.find(L"__") != std::string::npos ||
                  tstr.find(L"--") != std::string::npos ||
                  tstr.find(L"\\\\") != std::string::npos ||
                  tstr.find(L".*") != std::string::npos ||
                  tstr.find(L".$") != std::string::npos ||
                  tstr.find(L"($") != std::string::npos ||
                  tstr.find(L"$-") != std::string::npos ||
                  tstr.find(L"\\?") != std::string::npos ||
                  tstr.find(L"/*") != std::string::npos ||
                  tstr.find(L"*/") != std::string::npos ||
                  (wcspbrk(tstr.c_str(), L":][=^{}") != 0)
                  )
               {
                  it = outList.erase(it);
               }
               else
               {
                  auto check = [&filterMap, &tstr](const wchar_t* Delim)->bool
                  {
                     std::list<std::wstring> tmpList;
                     wcstok(tstr, Delim, tmpList);
                     if (tmpList.size() == 1) return false;

                     for (auto itt = tmpList.begin(); itt != tmpList.end(); )
                     {
                        if (filterMap.find(*itt) != filterMap.end())
                        {
                           itt = tmpList.erase(itt);
                        }
                        else
                        {
                           break;
                        }
                     }
                     return tmpList.empty();
                  };

                  // check with splitting by mask: xx/xx/xx
                  if (check(L"./()\\"))
                  {
                     it = outList.erase(it);
                  }
                  else
                  {
                     // check with splitting by mask: xx-xx-xx
                     if (check(L"-"))
                     {
                        auto itf = maskedMap.find(tstr);
                        if (itf != maskedMap.end()) { itf->second++; }
                        else { maskedMap[tstr] = 1; }
                        //////////////////////////////////////////////
                        // tstr = *it
                        it = outList.erase(it);
                     }
                     else
                     {
                        ++it;
                     }
                  }
               }
            }
         }
      }
   }
}

void appendToMap(const std::list <wstring_t>& inList, std::map <wstring_t, size_t>& ioMap)
{
   for (std::list <wstring_t>::const_iterator it = inList.begin(); it != inList.end(); ++it)
   {
      wstring_t str = *it;

      for (int i = 0; i < str.length(); i++)
      {
         if ((str[i] >= 0x0041) && (str[i] <= 0x005a))
         {
            str[i] = towlower(str[i]);
         }
      }

      bool valid = !str.empty();
      if (valid)
      {
         if (
            wcschr(str.c_str(), L'*') ||
            wcschr(str.c_str(), L':') ||
            wcschr(str.c_str(), L'$') ||
            wcschr(str.c_str(), L'&') ||
            wcschr(str.c_str(), L'%') ||
            wcschr(str.c_str(), L'_') ||
            wcschr(str.c_str(), L'.') ||
            wcschr(str.c_str(), L'>') ||
            wcschr(str.c_str(), L'<') ||
            wcschr(str.c_str(), L'=') ||
            wcschr(str.c_str(), L'~') ||
            wcschr(str.c_str(), L'@') ||
            wcschr(str.c_str(), L'#') ||
            wcschr(str.c_str(), L'(') ||
            wcschr(str.c_str(), L')') ||

            wcschr(str.c_str(), L'{') ||
            wcschr(str.c_str(), L'|') ||
            wcschr(str.c_str(), L'}') ||
            wcschr(str.c_str(), L'^') ||
            wcschr(str.c_str(), L']') ||
            wcschr(str.c_str(), L'[') ||

            wcschr(str.c_str(), L'\x002f') ||
            wcschr(str.c_str(), L'\x005c') ||
            //(str.front() == L'+') ||
            //(str.front() == L'-') ||

            wcschr(str.c_str(), L'?') )
            valid = false;
      }

      if (valid)
      {
         for (auto ch : str)
         {
            if (
               (ch >= 0x00c0 && ch <= 0x00d6) ||
               (ch >= 0x00d8 && ch <= 0x00f6) ||
               (ch >= 0x00f8 && ch <= 0x00ff) ||
               (ch >= 0x0100 && ch <= 0x024f) ||   // extended latin A,B
               isDiacriticGroup(ch) ||
               isOutdatedGroup(ch)  ||
               (wcsstr(str.c_str(), L"www")  != 0) ||
               (wcsstr(str.c_str(), L"http") != 0) ||
               (wcsstr(str.c_str(), L"::")   != 0) ||
               (wcsstr(str.c_str(), L"==")   != 0) ||
               (wcsstr(str.c_str(), L"--")   != 0) ||
               (wcsstr(str.c_str(), L"//")   != 0) ||
               (wcsstr(str.c_str(), L"&&")   != 0) ||
               (wcsstr(str.c_str(), L"><")   != 0) ||
               (wcsstr(str.c_str(), L"</")   != 0) ||
               (wcsstr(str.c_str(), L"<<")   != 0) ||
               (wcsstr(str.c_str(), L">>")   != 0) ||
               (wcsstr(str.c_str(), L">=")   != 0) ||
               (wcsstr(str.c_str(), L"<=")   != 0) ||
               (wcsstr(str.c_str(), L"=>")   != 0) ||
               (wcsstr(str.c_str(), L">-")   != 0) ||
               (wcsstr(str.c_str(), L"<-")   != 0) ||
               (wcsstr(str.c_str(), L"->")   != 0) ||
               (wcsstr(str.c_str(), L"*.")   != 0) ||
               (wcsstr(str.c_str(), L"&#")   != 0) ||
               (wcsstr(str.c_str(), L"...")  != 0) )
            {
               valid = false;
               break;
            }
         }
      }

      if (valid)
      {
         if (!str.empty() && !is_anydigit(str))
         {
            std::map <wstring_t, size_t>::iterator it = ioMap.find(str);

            bool result = true;

            for (int i = 0; i < str.length() && result; i++)
            {
               result = ((str[i] < 0x0400) || (str[i] > 0x04ff));
            }

            if (result)
            {
               if (it != ioMap.end())
               {
                  it->second = it->second + 1;
               }
               else
               {
                  ioMap[str] = 1;
               }
            }
         }
      }
   }
}

void report(
   const std::map <wstring_t, size_t>& baseMap,
   const std::map <wstring_t, size_t>& newMap,
   const std::map <wstring_t, size_t>& diffMap,
   const std::map <wstring_t, size_t>& resultMap)
{
   wprintf(L"TextCleaner, version 0.80 (UTF-16LE)\n");
   if (!baseMap.empty())
   {
      wprintf(L"words: base.dictionary (%u) loaded.\n", baseMap.size());
   }
   else
   {
      wprintf(L"demo-mode:\n");
      test_translateChar();
   }

   if (!newMap.empty() || !diffMap.empty() || !resultMap.empty())
   {
      wprintf(L"words: update (%u) loaded.\n", newMap.size());
      wprintf(L"words: diff.dictionary (%u/%u) done.\n", diffMap.size(), newMap.size());
      wprintf(L"words: final.dictionary (%u) done.\n", resultMap.size());
   }
   if (!baseMap.empty())
   {
      wprintf(L"========== Build: succeeded ==========\n");
   }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void processString(const wstring_t& wstr, const std::map <wstring_t, size_t>& filterMap, std::map <wstring_t, size_t>& ioMap, FILE* filteredOut)
{
   std::list <wstring_t> tokenList;

   wcstok(wstr, L"\x0020\x0021\x002c\x003b\x007c", tokenList);   // " !,;|"

   trimming(filterMap, tokenList);

   if (filteredOut)
   {
      auto i = tokenList.begin();
      while (i != tokenList.end())
      {
         fputws(i->c_str(), filteredOut);
         i++;
         if (i != tokenList.end())
         {
            fputwc(L' ', filteredOut);
         }
         else
         {
            fputwc(L'\n', filteredOut);
         }
      }
   }

   appendToMap(tokenList, ioMap);
}


void loadFile_utf8(const char* filepath, const std::wstring& filename_out, const std::map <wstring_t, size_t>& filterMap, std::map <wstring_t, size_t>& ioMap, const std::wstring& filtered_out)
{
   setlocale(LC_ALL, "Russian");
   //////////////////////////////////////////////////////////////////////////

   const std::wstring filename_in = cstring_to_wstring(filepath);

   FILE *pFile = _wfopen(filename_in.c_str(), L"rt, ccs=UTF-8");

   // MSDN: Allowed values of encoding are UNICODE, UTF-8, and UTF-16LE.

   if (pFile == NULL)
   {
      wprintf(L"can't load file: %s\n", filename_in.c_str());
      return;
   }

   FILE *pOutput = (filename_out.length() > 0) ? _wfopen(filename_out.c_str(), L"w, ccs=UTF-16LE") : 0;
   FILE *pOutputF = (filtered_out.length() > 0) ? _wfopen(filtered_out.c_str(), L"w, ccs=UTF-16LE") : 0;
   UInt32 lineNumber = 0;
   /////////////////////////////////////////////////////////////////////////

   wchar_t buff[16384];
   buff[0] = 0;
   wchar_t* pBuff = buff;

   wchar_t wch = 0;
   size_t str_sz = 0;

   // Read file in UTF-8
   std::ifstream f(filepath);
   std::wbuffer_convert<std::codecvt_utf8<wchar_t>> conv(f.rdbuf());
   std::wistream wf(&conv);

   // Print string in UTF-8
#ifdef _WIN32
   _setmode(_fileno(stdout), _O_WTEXT);
#else
   std::setlocale(LC_ALL, "");
#endif

   //////////////////////////////////////////////////////////////////////////
   while ((wch = wf.get()) != WEOF)
   {
      if (wch == L'\n')
      {
         if (pOutput) fputwc(wch, pOutput);

         if (pBuff != buff)
         {
            *pBuff = 0;
            pBuff = buff;

            {
               const wstring_t wstr(buff);
               assert(str_sz == wstr.size());

               processString(wstr, filterMap, ioMap, pOutputF);
            }

            str_sz = 0;
         }
         else
         {
            //printf("!!! NL empty, skipped\n");
         }
         lineNumber++;
      }
      else
      {
         const wchar_t tch = translateChar(wch);

         // check if need to skip symbol.
         if (tch > 0)
         {
            if (pOutput) fputwc(wch, pOutput);  //write original symbol

            *pBuff = tch;
            pBuff++;
            str_sz++;
         }
      }
   }

   fclose(pFile);
   if (pOutput) fclose(pOutput);
   if (pOutputF) fclose(pOutputF);
}


void loadFile(const std::wstring& filename_in, const std::wstring& filename_out, std::map <wstring_t, size_t>& ioMap)
{
   setlocale(LC_ALL, "Russian");
   //////////////////////////////////////////////////////////////////////////

   FILE *pFile = _wfopen(filename_in.c_str(), L"rt, ccs=UTF-8");

   // MSDN: Allowed values of encoding are UNICODE, UTF-8, and UTF-16LE.

   if (pFile == NULL)
   {
      wprintf(L"can't load file: %s\n", filename_in.c_str());
      return;
   }

   FILE *pOutput = (filename_out.length() > 0) ? _wfopen(filename_out.c_str(), L"w, ccs=UTF-16LE") : 0;
   UInt32 lineNumber = 0;
   /////////////////////////////////////////////////////////////////////////

   wchar_t buff[16384];
   buff[0] = 0;
   wchar_t* pBuff = buff;

   wchar_t wch = 0;
   size_t str_sz = 0;

   //////////////////////////////////////////////////////////////////////////
   while ((wch = fgetwc(pFile)) != WEOF)
   {
      if (wch == L'\n')
      {
         if (pOutput) fputwc(wch, pOutput);

         if (pBuff != buff)
         {
            *pBuff = 0;
            pBuff = buff;

            //processString(buff, str_sz, ioMap);
            {
               wstring_t wstr(buff);
               assert(str_sz == wstr.size());

               ltrim(wstr, L"#");
               if (!wstr.empty())
               {
                  if (!is_digit(wstr))
                  {
                     ioMap[wstr] = 1;
                  }
               }
            }

            str_sz = 0;
         }
         else
         {
            //printf("!!! NL empty, skipped\n");
         }
         lineNumber++;
      }
      else
      {
         const wchar_t tch = translateChar(wch);

         // check if need to skip symbol.
         if (tch > 0)
         {
            if (pOutput) fputwc(tch, pOutput);

            *pBuff = tch;
            pBuff++;
            str_sz++;
         }
      }
   }

   fclose(pFile);
   if (pOutput) fclose(pOutput);
}


void mapToFile(const wstring_t filepath, const std::map <wstring_t, size_t>& iMap, const wstring_t title)
{
   FILE* pOutFile = _wfopen(wstring_t(filepath + L"--dictionary.dictionary").c_str(), L"w, ccs=UTF-16LE");
   FILE* pMaskedFile = _wfopen(wstring_t(filepath + L"--masked.dictionary").c_str(), L"w, ccs=UTF-16LE");

   if (!title.empty())
   {
      const wstring_t prefix(L"###");
      fputws(prefix.c_str(), pOutFile);
      fputwc(L' ', pOutFile);
      fputws(title.c_str(), pOutFile);
      fputwc(L' ', pOutFile);
      fputws(prefix.c_str(), pOutFile);
      fputwc(L'\n', pOutFile);
   }

   wstring_t str;

   for (std::map <wstring_t, size_t>::const_iterator it = iMap.begin(); it != iMap.end(); ++it)
   {
      //if (it->second > 2)
      {
         fputws(&(it->first[0]), pOutFile);
         fputwc(L' ', pOutFile);

         str = std::to_wstring(it->second);
         fputws(&(str[0]), pOutFile);

         fputwc(L'\n', pOutFile);
      }
   }

   if (!maskedMap.empty())
   {
      for (auto itt = maskedMap.begin(); itt != maskedMap.end(); itt++)
      {
         fputws(&(itt->first[0]), pMaskedFile);
         fputwc(L' ', pMaskedFile);

         str = std::to_wstring(itt->second);
         fputws(&(str[0]), pMaskedFile);

         fputwc(L'\n', pMaskedFile);
      }
   }

   fclose(pOutFile);
   fclose(pMaskedFile);
}


int main(int argc, char* argv[])
{
   std::map <wstring_t, size_t> mainMap;
   std::map <wstring_t, size_t> filterMap;

   if (argc < 2)
   {
      report(mainMap, filterMap, filterMap, filterMap);
      wprintf(L"No extra command-line arguments passed:\n");
      wprintf(L"%s <input-file.u16>\n", cstring_to_wstring(argv[0]).c_str());
      wprintf(L"%s <filter-dictionary.u16> <input-file.u16>\n", cstring_to_wstring(argv[0]).c_str());
   }
   else
   {
      wprintf(L"Text-cleaner [Version 33] (c) Diixo\n");
      if (argc == 3)
      {
         const wstring_t filterFile = cstring_to_wstring(argv[1]);
         const wstring_t mainFile = cstring_to_wstring(argv[2]);

         loadFile(filterFile, wstring_t(), filterMap);
         loadFile_utf8(argv[2], mainFile + L".u16", filterMap, mainMap, mainFile + L"--filtered.u16");
         mapToFile(mainFile, mainMap, wstring_t());
      }
      if (argc == 2)
      {
         const wstring_t mainFile = cstring_to_wstring(argv[1]);

         loadFile_utf8(argv[1], mainFile + L".u16", filterMap, mainMap, wstring_t());
         mapToFile(mainFile, mainMap, wstring_t());

         //report(mainMap, cmpMap, diffMap, resultMap);
      }
   }

   return 0;
}
