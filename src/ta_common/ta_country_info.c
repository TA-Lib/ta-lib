/* TA-LIB Copyright (c) 1999-2003, Mario Fortier
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 *
 * - Neither name of author nor the names of its contributors
 *   may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  JCR      J.C Roberts
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  112400 MF   First version.
 *  060402 MF   Remove reference to ISO3166 alpha-3 for legal
 *              reason. Base on valuable feedback from JCR
 *
 */

/* Description:
 *    This module provides cross-reference 
 *    of information identifying countries.
 *    
 *    This is base on ISO3166-1 alpha-2.
 *    Use with permission from OSI.
 *
 *    Cross reference are:
 *         - Country name
 *         - Standard 2 letters abbreviation. Example: "CA" for Canada.
 */

/**** Headers ****/
#include <stdlib.h>
#include <ctype.h>
#include "ta_country_info.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
const TA_CountryInfo TA_CountryInfoTable [TA_COUNTRY_INFO_TABLE_SIZE]= {

   /* Important:
    *   If you modify this array, make sure you keep
    *   the TA_CountryId enum in synch (see the .h)
    */

   /* Most common country in the context of TA-LIB. */
   { "UNITED STATES", {'U','S'} },
   { "CANADA", {'C','A'}},
   { "FRANCE", {'F','R'}},
   { "GERMANY", {'D','E'}},
   { "UNITED KINGDOM", {'U','K'}}, /* ISO3166 is GB */
   { "ITALY", {'I','T'}},
   { "MEXICO", {'M','X'}},
   { "JAPAN", {'J','P'}},

   /* Less common country in the context of TA-LIB. */
   { "AFGHANISTAN", {'A','F'}},
   { "ALBANIA", {'A','L'}},
   { "ALGERIA", {'D','Z'}},
   { "AMERICAN SAMOA", {'A','S'}},
   { "ANDORRA", {'A','D'}},
   { "ANGOLA", {'A','O'}},
   { "ANGUILLA", {'A','I'}},
   { "ANTARCTICA", {'A','Q'}},
   { "ANTIGUA AND BARBUDA", {'A','G'}},
   { "ARGENTINA", {'A','R'}},
   { "ARMENIA", {'A','M'}},
   { "ARUBA", {'A','W'}},
   { "AUSTRALIA", {'A','U'}},
   { "AUSTRIA", {'A','T'}},
   { "AZERBAIJAN", {'A','Z'}},
   { "BAHAMAS", {'B','S'}},
   { "BAHRAIN", {'B','H'}},
   { "BANGLADESH", {'B','D'}},
   { "BARBADOS", {'B','B'}},
   { "BELARUS", {'B','Y'}},
   { "BELGIUM", {'B','E'}},
   { "BELIZE", {'B','Z'}},
   { "BENIN", {'B','J'}},
   { "BERMUDA", {'B','M'}},
   { "BHUTAN", {'B','T'}},
   { "BOLIVIA", {'B','O'}},
   { "BOSNIA AND HERZEGOWINA", {'B','A'}},
   { "BOTSWANA", {'B','W'}},
   { "BOUVET ISLAND", {'B','V'}},
   { "BRAZIL", {'B','R'}},
   { "BRITISH INDIAN OCEAN TERRITORY", {'I','O'}},
   { "BRUNEI DARUSSALAM", {'B','N'}},
   { "BULGARIA", {'B','G'}},
   { "BURKINA FASO", {'B','F'}},
   { "BURUNDI", {'B','I'}},
   { "CAMBODIA", {'K','H'}},
   { "CAMEROON", {'C','M'}},
   { "CAPE VERDE", {'C','V'}},
   { "CAYMAN ISLANDS", {'K','Y'}},
   { "CENTRAL AFRICAN REPUBLIC", {'C','F'}},
   { "CHAD", {'T','D'}},
   { "CHILE", {'C','L'}},
   { "CHINA", {'C','N'}},
   { "CHRISTMAS ISLAND", {'C','X'}},
   { "COCOS (KEELING) ISLANDS", {'C','C'}},
   { "COLOMBIA", {'C','O'}},
   { "COMOROS", {'K','M'}},
   { "CONGO", {'C','G'}},
   { "COOK ISLANDS", {'C','K'}},
   { "COSTA RICA", {'C','R'}},
   { "COTE D'IVOIRE", {'C','I'}},
   { "CROATIA (local name: Hrvatska)", {'H','R'}},
   { "CUBA", {'C','U'}},
   { "CYPRUS", {'C','Y'}},
   { "CZECH REPUBLIC", {'C','Z'}},
   { "DENMARK", {'D','K'}},
   { "DJIBOUTI", {'D','J'}},
   { "DOMINICA", {'D','M'}},
   { "DOMINICAN REPUBLIC", {'D','O'}},
   { "EAST TIMOR", {'T','P'}},
   { "ECUADOR", {'E','C'}},
   { "EGYPT", {'E','G'}},
   { "EL SALVADOR", {'S','V'}},
   { "EQUATORIAL GUINEA", {'G','Q'}},
   { "ERITREA", {'E','R'}},
   { "ESTONIA", {'E','E'}},
   { "ETHIOPIA", {'E','T'}},
   { "FALKLAND ISLANDS (MALVINAS)", {'F','K'}},
   { "FAROE ISLANDS", {'F','O'}},
   { "FIJI", {'F','J'}},
   { "FINLAND", {'F','I'}},
   { "FRANCE, METROPOLITAN", {'F','X'}},
   { "FRENCH GUIANA", {'G','F'}},
   { "FRENCH POLYNESIA", {'P','F'}},
   { "FRENCH SOUTHERN TERRITORIES", {'T','F'}},
   { "GABON", {'G','A'}},
   { "GAMBIA", {'G','M'}},
   { "GEORGIA", {'G','E'}},
   { "GHANA", {'G','H'}},
   { "GIBRALTAR", {'G','I'}},
   { "GREECE", {'G','R'}},
   { "GREENLAND", {'G','L'}},
   { "GRENADA", {'G','D'}},
   { "GUADELOUPE", {'G','P'}},
   { "GUAM", {'G','U'}},
   { "GUATEMALA", {'G','T'}},
   { "GUINEA", {'G','N'}},
   { "GUINEA-BISSAU", {'G','W'}},
   { "GUYANA", {'G','Y'}},
   { "HAITI", {'H','T'}},
   { "HEARD AND MC DONALD ISLANDS", {'H','M'}},
   { "HONDURAS", {'H','N'}},
   { "HONG KONG", {'H','K'}},
   { "HUNGARY", {'H','U'}},
   { "ICELAND", {'I','S'}},
   { "INDIA", {'I','N'}},
   { "INDONESIA", {'I','D'}},
   { "IRAN (ISLAMIC REPUBLIC OF)", {'I','R'}},
   { "IRAQ", {'I','Q'}},
   { "IRELAND", {'I','E'}},
   { "ISRAEL", {'I','L'}},
   { "JAMAICA", {'J','M'}},
   { "JORDAN", {'J','O'}},
   { "KAZAKHSTAN", {'K','Z'}},
   { "KENYA", {'K','E'}},
   { "KIRIBATI", {'K','I'}},
   { "KOREA, DEMOCRATIC PEOPLE'S REPUBLIC OF", {'K','P'}},
   { "KOREA, REPUBLIC OF", {'K','R'}},
   { "KUWAIT", {'K','W'}},
   { "KYRGYZSTAN", {'K','G'}},
   { "LAO PEOPLE'S DEMOCRATIC REPUBLIC", {'L','A'}},
   { "LATVIA", {'L','V'}},
   { "LEBANON", {'L','B'}},
   { "LESOTHO", {'L','S'}},
   { "LIBERIA", {'L','R'}},
   { "LIBYAN ARAB JAMAHIRIYA", {'L','Y'}},
   { "LIECHTENSTEIN", {'L','I'}},
   { "LITHUANIA", {'L','T'}},
   { "LUXEMBOURG", {'L','U'}},
   { "MACAU", {'M','O'}},
   { "MACEDONIA, THE FORMER YUGOSLAV REPUBLIC OF", {'M','K'}},
   { "MADAGASCAR", {'M','G'}},
   { "MALAWI", {'M','W'}},
   { "MALAYSIA", {'M','Y'}},
   { "MALDIVES", {'M','V'}},
   { "MALI", {'M','L'}},
   { "MALTA", {'M','T'}},
   { "MARSHALL ISLANDS", {'M','H'}},
   { "MARTINIQUE", {'M','Q'}},
   { "MAURITANIA", {'M','R'}},
   { "MAURITIUS", {'M','U'}},
   { "MAYOTTE", {'Y','T'}},
   { "MICRONESIA, FEDERATED STATES OF", {'F','M'}},
   { "MOLDOVA, REPUBLIC OF", {'M','D'}},
   { "MONACO", {'M','C'}},
   { "MONGOLIA", {'M','N'}},
   { "MONTSERRAT", {'M','S'}},
   { "MOROCCO", {'M','A'}},
   { "MOZAMBIQUE", {'M','Z'}},
   { "MYANMAR", {'M','M'}},
   { "NAMIBIA", {'N','A'}},
   { "NAURU", {'N','R'}},
   { "NEPAL", {'N','P'}},
   { "NETHERLANDS", {'N','L'}},
   { "NETHERLANDS ANTILLES", {'A','N'}},
   { "NEW CALEDONIA", {'N','C'}},
   { "NEW ZEALAND", {'N','Z'}},
   { "NICARAGUA", {'N','I'}},
   { "NIGER", {'N','E'}},
   { "NIGERIA", {'N','G'}},
   { "NIUE", {'N','U'}},
   { "NORFOLK ISLAND", {'N','F'}},
   { "NORTHERN MARIANA ISLANDS", {'M','P'}},
   { "NORWAY", {'N','O'}},
   { "OMAN", {'O','M'}},
   { "PAKISTAN", {'P','K'}},
   { "PALAU", {'P','W'}},
   { "PANAMA", {'P','A'}},
   { "PAPUA NEW GUINEA", {'P','G'}},
   { "PARAGUAY", {'P','Y'}},
   { "PERU", {'P','E'}},
   { "PHILIPPINES", {'P','H'}},
   { "PITCAIRN", {'P','N'}},
   { "POLAND", {'P','L'}},
   { "PORTUGAL", {'P','T'}},
   { "PUERTO RICO", {'P','R'}},
   { "QATAR", {'Q','A'}},
   { "REUNION", {'R','E'}},
   { "ROMANIA", {'R','O'}},
   { "RUSSIAN FEDERATION", {'R','U'}},
   { "RWANDA", {'R','W'}},
   { "SAINT KITTS AND NEVIS", {'K','N'}},
   { "SAINT LUCIA", {'L','C'}},
   { "SAINT VINCENT AND THE GRENADINES", {'V','C'}},
   { "SAMOA", {'W','S'}},
   { "SAN MARINO", {'S','M'}},
   { "SAO TOME AND PRINCIPE", {'S','T'}},
   { "SAUDI ARABIA", {'S','A'}},
   { "SENEGAL", {'S','N'}},
   { "SEYCHELLES", {'S','C'}},
   { "SIERRA LEONE", {'S','L'}},
   { "SINGAPORE", {'S','G'}},
   { "SLOVAKIA (Slovak Republic)", {'S','K'}},
   { "SLOVENIA", {'S','I'}},
   { "SOLOMON ISLANDS", {'S','B'}},
   { "SOMALIA", {'S','O'}},
   { "SOUTH AFRICA", {'Z','A'}},
   { "SOUTH GEORGIA AND THE SOUTH SANDWICH ISLANDS", {'G','S'}},
   { "SPAIN", {'E','S'}},
   { "SRI LANKA", {'L','K'}},
   { "ST. HELENA", {'S','H'}},
   { "ST. PIERRE AND MIQUELON", {'P','M'}},
   { "SUDAN", {'S','D'}},
   { "SURINAME", {'S','R'}},
   { "SVALBARD AND JAN MAYEN ISLANDS", {'S','J'}},
   { "SWAZILAND", {'S','Z'}},
   { "SWEDEN", {'S','E'}},
   { "SWITZERLAND", {'C','H'}},
   { "SYRIAN ARAB REPUBLIC", {'S','Y'}},
   { "TAIWAN, PROVINCE OF CHINA", {'T','W'}},
   { "TAJIKISTAN", {'T','J'}},
   { "TANZANIA, UNITED REPUBLIC OF", {'T','Z'}},
   { "THAILAND", {'T','H'}},
   { "TOGO", {'T','G'}},
   { "TOKELAU", {'T','K'}},
   { "TONGA", {'T','O'}},
   { "TRINIDAD AND TOBAGO", {'T','T'}},
   { "TUNISIA", {'T','N'}},
   { "TURKEY", {'T','R'}},
   { "TURKMENISTAN", {'T','M'}},
   { "TURKS AND CAICOS ISLANDS", {'T','C'}},
   { "TUVALU", {'T','V'}},
   { "UGANDA", {'U','G'}},
   { "UKRAINE", {'U','A'}},
   { "UNITED ARAB EMIRATES", {'A','E'}},
   { "UNITED STATES MINOR OUTLYING ISLANDS", {'U','M'}},
   { "URUGUAY", {'U','Y'}},
   { "UZBEKISTAN", {'U','Z'}},
   { "VANUATU", {'V','U'}},
   { "VATICAN CITY STATE (HOLY SEE)", {'V','A'}},
   { "VENEZUELA", {'V','E'}},
   { "VIET NAM", {'V','N'}},
   { "VIRGIN ISLANDS (BRITISH)", {'V','G'}},
   { "VIRGIN ISLANDS (U.S.)", {'V','I'}},
   { "WALLIS AND FUTUNA ISLANDS", {'W','F'}},
   { "WESTERN SAHARA", {'E','H'}},
   { "YEMEN", {'Y','E'}},
   { "YUGOSLAVIA", {'Y','U'}},
   { "ZAIRE", {'Z','R'}},
   { "ZAMBIA", {'Z','M'}},
   { "ZIMBABWE", {'Z','W'}}
};

/**** Local declarations.              ****/
/* None */

/**** Local functions declarations.    ****/
/* None */

/**** Local variables definitions.     ****/
/* None */

/**** Global functions definitions.   ****/

const char *TA_CountryIdToAbbrev( TA_CountryId countryId )
{
   if( (countryId <= 0) || (countryId > TA_COUNTRY_INFO_TABLE_SIZE) )
      return (const char *)0; /* Not found. */

   return TA_CountryInfoTable[countryId-1].abbrev;
}

TA_CountryId TA_CountryAbbrevToId( const char *abbrev )
{
   unsigned int i;

   for( i=0; i < TA_COUNTRY_INFO_TABLE_SIZE; i++ )
   {
      if( TA_CountryInfoTable[i].abbrev[0] == (toupper(abbrev[0])) &&
          TA_CountryInfoTable[i].abbrev[1] == (toupper(abbrev[1])) )
         return i+1;
   }

   return TA_Country_ID_INVALID; /* Not found. */
}

const char *TA_CountryIdToName( TA_CountryId countryId )
{
   if( (countryId <= 0) || (countryId > TA_COUNTRY_INFO_TABLE_SIZE) )
      return (const char *)0; /* Not found. */

   return TA_CountryInfoTable[countryId-1].name;
}

unsigned int TA_CountryIdIsValid( TA_CountryId countryId )
{
   if( (countryId <= 0) || (countryId > TA_COUNTRY_INFO_TABLE_SIZE) )
      return 0; /* Invalid */
   else
      return 1; /* Valid */
}

/**** Local functions definitions.     ****/
/* None */
