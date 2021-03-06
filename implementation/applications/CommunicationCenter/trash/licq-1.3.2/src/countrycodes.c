// -*- c-basic-offset: 2 -*-
/* ----------------------------------------------------------------------------
 * Licq - A ICQ Client for Unix
 * Copyright (C) 1998 - 2003 Licq developers
 *
 * This program is licensed under the terms found in the LICENSE file.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>

// Localization
#include "gettext.h"

#include "licq_countrycodes.h"

const struct SCountry gCountries[NUM_COUNTRIES] =
{
  { tr_NOOP("Unspecified"), COUNTRY_UNSPECIFIED, 0, 0 },
  { tr_NOOP("Afghanistan"), 93, 93, 1 },
  { tr_NOOP("Albania"), 355, 355, 2 },
  { tr_NOOP("Algeria"), 213, 213, 3 },
  { tr_NOOP("American Samoa"), 684, 684, 4 },
  { tr_NOOP("Andorra"), 376, 376, 5 },
  { tr_NOOP("Angola"), 244, 244, 6 },
  { tr_NOOP("Anguilla"), 101, 1, 7 },
  { tr_NOOP("Antigua and Barbuda"), 1021, 1, 8 },
  { tr_NOOP("Antigua"), 102, 1, 9 },
  { tr_NOOP("Antilles"), 5902, 590, 10 },
  { tr_NOOP("Argentina"), 54, 54, 11 },
  { tr_NOOP("Armenia"), 374, 374, 12 },
  { tr_NOOP("Aruba"), 297, 297, 13 },
  { tr_NOOP("Ascension Island"), 247, 247, 14 },
  { tr_NOOP("Australia"), 61, 61, 15 },
  { tr_NOOP("Austria"), 43, 43, 16 },
  { tr_NOOP("Azerbaijan"), 994, 994, 17 },
  { tr_NOOP("Bahamas"), 103, 1, 18 },
  { tr_NOOP("Bahrain"), 973, 973, 19 },
  { tr_NOOP("Bangladesh"), 880, 880, 20 },
  { tr_NOOP("Barbados"), 104, 1, 21 },
  { tr_NOOP("Barbuda"), 120, 1, 22 },
  { tr_NOOP("Belarus"), 375, 375, 23 },
  { tr_NOOP("Belgium"), 32, 32, 24 },
  { tr_NOOP("Belize"), 501, 501, 25 },
  { tr_NOOP("Benin"), 229, 229, 26 },
  { tr_NOOP("Bermuda"), 105, 1, 27 },
  { tr_NOOP("Bhutan"), 975, 975, 28 },
  { tr_NOOP("Bolivia"), 591, 591, 29 },
  { tr_NOOP("Bosnia and Herzegovina"), 387, 387, 30 },
  { tr_NOOP("Botswana"), 267, 267, 31 },
  { tr_NOOP("Brazil"), 55, 55, 32 },
  { tr_NOOP("British Virgin Islands"), 106, 1, 33 },
  { tr_NOOP("Brunei"), 673, 673, 34 },
  { tr_NOOP("Bulgaria"), 359, 359, 35 },
  { tr_NOOP("Burkina Faso"), 226, 226, 36 },
  { tr_NOOP("Burundi"), 257, 257, 37 },
  { tr_NOOP("Cambodia"), 855, 855, 38 },
  { tr_NOOP("Cameroon"), 237, 237, 39 },
  { tr_NOOP("Canada"), 107, 1, 40 },
  { tr_NOOP("Canary Islands"), 178, 1, 41 },
  { tr_NOOP("Cape Verde Islands"), 238, 238, 42 },
  { tr_NOOP("Cayman Islands"), 108, 1, 43 },
  { tr_NOOP("Central African Republic"), 236, 236, 44 },
  { tr_NOOP("Chad"), 235, 235, 45 },
  { tr_NOOP("Chile, Republic of"), 56, 56, 46 },
  { tr_NOOP("China"), 86, 86, 47 },
  { tr_NOOP("Christmas Island"), 672, 672, 48 },
  { tr_NOOP("Cocos (Keeling) Islands"), 6102, 61, 49 },
  { tr_NOOP("Cocos-Keeling Islands"), 6101, 61, 50 },
  { tr_NOOP("Colombia"), 57, 57, 51 },
  { tr_NOOP("Comoros"), 2691, 269, 52 },
  { tr_NOOP("Congo, (Republic of the)"), 242, 242, 53 },
  { tr_NOOP("Congo, Democratic Republic of (Zaire)"), 243, 243, 54 },
  { tr_NOOP("Cook Islands"), 682, 682, 55 },
  { tr_NOOP("Costa Rica"), 506, 506, 56 },
  { tr_NOOP("Cote d'Ivoire"), 225, 225, 57 },
  { tr_NOOP("Croatia"), 385, 385, 58 },
  { tr_NOOP("Cuba"), 53, 53, 59 },
  { tr_NOOP("Cyprus"), 357, 357, 60 },
  { tr_NOOP("Czech Republic"), 42, 420, 61 },
  { tr_NOOP("Denmark"), 45, 45, 62 },
  { tr_NOOP("Diego Garcia"), 246, 246, 63 },
  { tr_NOOP("Djibouti"), 253, 253, 64 },
  { tr_NOOP("Dominica"), 109, 1, 65 },
  { tr_NOOP("Dominican Republic"), 110, 1, 66 },
  { tr_NOOP("Ecuador"), 593, 593, 67 },
  { tr_NOOP("Egypt"), 20, 20, 68 },
  { tr_NOOP("El Salvador"), 503, 503, 69 },
  { tr_NOOP("Equatorial Guinea"), 240, 240, 70 },
  { tr_NOOP("Eritrea"), 291, 291, 71 },
  { tr_NOOP("Estonia"), 372, 372, 72 },
  { tr_NOOP("Ethiopia"), 251, 251, 73 },
  { tr_NOOP("Faeroe Islands"), 298, 298, 74 },
  { tr_NOOP("Falkland Islands"), 500, 500, 75 },
  { tr_NOOP("Fiji"), 679, 679, 76 },
  { tr_NOOP("Finland"), 358, 358, 77 },
  { tr_NOOP("France"), 33, 33, 78 },
  { tr_NOOP("French Antilles"), 5901, 590, 79 },
  { tr_NOOP("French Guiana"), 594, 594, 80 },
  { tr_NOOP("French Polynesia"), 689, 689, 81 },
  { tr_NOOP("Gabon"), 241, 241, 82 },
  { tr_NOOP("Gambia"), 220, 220, 83 },
  { tr_NOOP("Georgia"), 995, 995, 84 },
  { tr_NOOP("Germany"), 49, 49, 85 },
  { tr_NOOP("Ghana"), 233, 233, 86 },
  { tr_NOOP("Gibraltar"), 350, 350, 87 },
  { tr_NOOP("Greece"), 30, 30, 88 },
  { tr_NOOP("Greenland"), 299, 299, 89 },
  { tr_NOOP("Grenada"), 111, 1, 90 },
  { tr_NOOP("Guadeloupe"), 590, 590, 91 },
  { tr_NOOP("Guam, US Territory of"), 671, 671, 92 },
  { tr_NOOP("Guatemala"), 502, 502, 93 },
  { tr_NOOP("Guinea"), 224, 224, 94 },
  { tr_NOOP("Guinea-Bissau"), 245, 245, 95 },
  { tr_NOOP("Guyana"), 592, 592, 96 },
  { tr_NOOP("Haiti"), 509, 509, 97 },
  { tr_NOOP("Honduras"), 504, 504, 98 },
  { tr_NOOP("Hong Kong"), 852, 852, 99 },
  { tr_NOOP("Hungary"), 36, 36, 100 },
  { tr_NOOP("Iceland"), 354, 354, 101 },
  { tr_NOOP("India"), 91, 91, 102 },
  { tr_NOOP("Indonesia"), 62, 62, 103 },
  { tr_NOOP("Iran (Islamic Republic of)"), 98, 98, 104 },
  { tr_NOOP("Iraq"), 964, 964, 105 },
  { tr_NOOP("Ireland"), 353, 353, 106 },
  { tr_NOOP("Israel"), 972, 972, 107 },
  { tr_NOOP("Italy"), 39, 39, 108 },
  { tr_NOOP("Jamaica"), 112, 1, 109 },
  { tr_NOOP("Japan"), 81, 81, 110 },
  { tr_NOOP("Jordan"), 962, 962, 111 },
  { tr_NOOP("Kazakhstan"), 705, 7, 112 },
  { tr_NOOP("Kenya"), 254, 254, 113 },
  { tr_NOOP("Kiribati"), 686, 686, 114 },
  { tr_NOOP("Korea (North Korea), Democratic People's Republic of"), 850, 850, 115 },
  { tr_NOOP("Korea (South Korea), Republic of"), 82, 82, 116 },
  { tr_NOOP("Kuwait"), 965, 965, 117 },
  { tr_NOOP("Kyrgyzstan"), 706, 7, 118 },
  { tr_NOOP("Lao People's Democratic Republic"), 856, 856, 119 },
  { tr_NOOP("Latvia"), 371, 371, 120 },
  { tr_NOOP("Lebanon"), 961, 961, 121 },
  { tr_NOOP("Lesotho"), 266, 266, 122 },
  { tr_NOOP("Liberia"), 231, 231, 123 },
  { tr_NOOP("Libyan Arab Jamahiriya"), 218, 218, 124 },
  { tr_NOOP("Liechtenstein"), 4101, 41, 125 },
  { tr_NOOP("Lithuania"), 370, 370, 126 },
  { tr_NOOP("Luxembourg"), 352, 352, 127 },
  { tr_NOOP("Macau"), 853, 853, 128 },
  { tr_NOOP("Macedonia (F.Y.R.O.M.)"), 389, 389, 129 },
  { tr_NOOP("Madagascar"), 261, 261, 130 },
  { tr_NOOP("Malawi"), 265, 265, 131 },
  { tr_NOOP("Malaysia"), 60, 60, 132 },
  { tr_NOOP("Maldives"), 960, 960, 133 },
  { tr_NOOP("Mali"), 223, 223, 134 },
  { tr_NOOP("Malta"), 356, 356, 135 },
  { tr_NOOP("Marshall Islands"), 692, 692, 136 },
  { tr_NOOP("Martinique"), 596, 596, 137 },
  { tr_NOOP("Mauritania"), 222, 222, 138 },
  { tr_NOOP("Mauritius"), 230, 230, 139 },
  { tr_NOOP("Mayotte Island"), 269, 269, 140 },
  { tr_NOOP("Mexico"), 52, 52, 141 },
  { tr_NOOP("Micronesia, Federated States of"), 691, 691, 142 },
  { tr_NOOP("Moldova, Republic of"), 373, 373, 143 },
  { tr_NOOP("Monaco"), 377, 377, 144 },
  { tr_NOOP("Mongolia"), 976, 976, 145 },
  { tr_NOOP("Montserrat"), 113, 1, 146 },
  { tr_NOOP("Morocco"), 212, 212, 147 },
  { tr_NOOP("Mozambique"), 258, 258, 148 },
  { tr_NOOP("Myanmar"), 95, 95, 149 },
  { tr_NOOP("Namibia"), 264, 264, 150 },
  { tr_NOOP("Nauru"), 674, 674, 151 },
  { tr_NOOP("Nepal"), 977, 977, 152 },
  { tr_NOOP("Netherlands Antilles"), 599, 599, 153 },
  { tr_NOOP("Netherlands"), 31, 31, 154 },
  { tr_NOOP("Nevis"), 114, 1, 155 },
  { tr_NOOP("New Caledonia"), 687, 687, 156 },
  { tr_NOOP("New Zealand"), 64, 64, 157 },
  { tr_NOOP("Nicaragua"), 505, 505, 158 },
  { tr_NOOP("Niger"), 227, 227, 159 },
  { tr_NOOP("Nigeria"), 234, 234, 160 },
  { tr_NOOP("Niue"), 683, 683, 161 },
  { tr_NOOP("Norfolk Island"), 6722, 672, 162 },
  { tr_NOOP("Norway"), 47, 47, 163 },
  { tr_NOOP("Oman"), 968, 968, 164 },
  { tr_NOOP("Pakistan"), 92, 92, 165 },
  { tr_NOOP("Palau"), 680, 680, 166 },
  { tr_NOOP("Panama"), 507, 507, 167 },
  { tr_NOOP("Papua New Guinea"), 675, 675, 168 },
  { tr_NOOP("Paraguay"), 595, 595, 169 },
  { tr_NOOP("Peru"), 51, 51, 170 },
  { tr_NOOP("Philippines"), 63, 63, 171 },
  { tr_NOOP("Poland"), 48, 48, 172 },
  { tr_NOOP("Portugal"), 351, 351, 173 },
  { tr_NOOP("Puerto Rico, Common Wealth of"), 121, 1, 174 },
  { tr_NOOP("Qatar"), 974, 974, 175 },
  { tr_NOOP("Reunion Island"), 262, 262, 176 },
  { tr_NOOP("Romania"), 40, 40, 177 },
  { tr_NOOP("Rota Island"), 6701, 670, 178 },
  { tr_NOOP("Russia"), 7, 7, 179 },
  { tr_NOOP("Rwanda"), 250, 250, 180 },
  { tr_NOOP("Saint Kitts and Nevis"), 1141, 1, 181 },
  { tr_NOOP("Saint Lucia"), 122, 1, 182 },
  { tr_NOOP("Saipan Island"), 670, 670, 183 },
  { tr_NOOP("San Marino"), 378, 378, 184 },
  { tr_NOOP("Sao Tome & Principe"), 239, 239, 185 },
  { tr_NOOP("Saudi Arabia"), 966, 966, 186 },
  { tr_NOOP("Scotland"), 442, 44, 187 },
  { tr_NOOP("Senegal"), 221, 221, 188 },
  { tr_NOOP("Seychelles"), 248, 248, 189 },
  { tr_NOOP("Sierra Leone"), 232, 232, 190 },
  { tr_NOOP("Singapore"), 65, 65, 191 },
  { tr_NOOP("Slovakia"), 4201, 421, 192 },
  { tr_NOOP("Slovenia"), 386, 386, 193 },
  { tr_NOOP("Solomon Islands"), 677, 677, 194 },
  { tr_NOOP("Somalia"), 252, 252, 195 },
  { tr_NOOP("South Africa"), 27, 27, 196 },
  { tr_NOOP("Spain"), 34, 34, 197 },
  { tr_NOOP("Sri Lanka"), 94, 94, 198 },
  { tr_NOOP("St. Helena"), 290, 290, 199 },
  { tr_NOOP("St. Kitts"), 115, 1, 200 },
  { tr_NOOP("St. Pierre and Miquelon"), 508, 508, 201 },
  { tr_NOOP("St. Vincent and the Grenadines"), 116, 1, 202 },
  { tr_NOOP("Sudan"), 249, 249, 203 },
  { tr_NOOP("Suriname"), 597, 597, 204 },
  { tr_NOOP("Swaziland"), 268, 268, 205 },
  { tr_NOOP("Sweden"), 46, 46, 206 },
  { tr_NOOP("Switzerland"), 41, 41, 207 },
  { tr_NOOP("Syrian Arab Republic"), 963, 963, 208 },
  { tr_NOOP("Taiwan"), 886, 886, 209 },
  { tr_NOOP("Tajikistan"), 708, 7, 210 },
  { tr_NOOP("Tanzania, United Republic of"), 255, 255, 211 },
  { tr_NOOP("Thailand"), 66, 66, 212 },
  { tr_NOOP("Tinian Island"), 6702, 670, 213 },
  { tr_NOOP("Togo"), 228, 228, 214 },
  { tr_NOOP("Tokelau"), 690, 690, 215 },
  { tr_NOOP("Tonga"), 676, 676, 216 },
  { tr_NOOP("Trinidad and Tobago"), 117, 1, 217 },
  { tr_NOOP("Tunisia"), 216, 216, 218 },
  { tr_NOOP("Turkey"), 90, 90, 219 },
  { tr_NOOP("Turkmenistan"), 709, 993, 220 },
  { tr_NOOP("Turks and Caicos Islands"), 118, 1, 221 },
  { tr_NOOP("Tuvalu"), 688, 688, 222 },
  { tr_NOOP("Uganda"), 256, 256, 223 },
  { tr_NOOP("Ukraine"), 380, 380, 224 },
  { tr_NOOP("United Arab Emirates"), 971, 971, 225 },
  { tr_NOOP("United Kingdom of Great Britain"), 44, 44, 226 },
  { tr_NOOP("Uruguay"), 598, 598, 227 },
  { tr_NOOP("USA"), 1, 1, 228 },
  { tr_NOOP("Uzbekistan"), 711, 998, 229 },
  { tr_NOOP("Vanuatu"), 678, 678, 230 },
  { tr_NOOP("Vatican City"), 379, 39, 231 },
  { tr_NOOP("Venezuela"), 58, 58, 232 },
  { tr_NOOP("Viet Nam"), 84, 84, 233 },
  { tr_NOOP("Virgin Islands of the United States"), 123, 1, 234 },
  { tr_NOOP("Wales"), 441, 44, 235 },
  { tr_NOOP("Wallis and Futuna Islands"), 681, 681, 236 },
  { tr_NOOP("Western Samoa"), 685, 685, 237 },
  { tr_NOOP("Yemen"), 967, 967, 238 },
  { tr_NOOP("Yugoslavia - Montenegro"), 382, 381, 239 },
  { tr_NOOP("Yugoslavia - Serbia"), 3811, 381, 240 },
  { tr_NOOP("Yugoslavia"), 381, 381, 241 },
  { tr_NOOP("Zambia"), 260, 260, 242 },
  { tr_NOOP("Zimbabwe"), 263, 263, 243 }
};


const struct SCountry *GetCountryByCode(unsigned short _nCountryCode)
{
   // do a simple linear search as there aren't too many countries
   unsigned short i = 0;
   while (i < NUM_COUNTRIES && gCountries[i].nCode != _nCountryCode) i++;
   if (i == NUM_COUNTRIES) return NULL;
   return &gCountries[i];
}

const struct SCountry *GetCountryByIndex(unsigned short _nIndex)
{
   if (_nIndex >= NUM_COUNTRIES) return NULL;
   return (&gCountries[_nIndex]);
}

const struct SCountry *GetCountryByName(const char *_szName)
{
   unsigned short i = 0;
   while (i < NUM_COUNTRIES && strcasecmp(gCountries[i].szName, _szName)) i++;
   if (i == NUM_COUNTRIES) return NULL;
   return &gCountries[i];
}
