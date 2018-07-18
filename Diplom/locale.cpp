#include "includes.h"
#include "defines.h"

void choose_locale()
{
    switch (LANG[0])
    {
    case 'r':
        locale::global(locale("russian_Russia.1251"));
        break;
    case 'e':
        locale::global(locale(""));
        break;
    case 'k':
        locale::global(locale("kazakh_Kazakhstan.1251"));
        break;
    case 't':
        locale::global(locale("turkish_Turkey.1254"));
    }
}