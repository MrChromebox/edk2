#include "FrontPage.h"
#include <Library/BaseLib.h>

/**
  Case-insensitive string comparison function.

  @param  Str1   First null-terminated string to compare.
  @param  Str2   Second null-terminated string to compare.

  @retval 0   The strings are equal (case-insensitive).
  @retval !=0 The strings are not equal.
**/
STATIC
INTN
StriCmp (
  IN CONST CHAR16  *Str1,
  IN CONST CHAR16  *Str2
  )
{
  CHAR16  Char1;
  CHAR16  Char2;

  while (*Str1 != L'\0') {
    Char1 = CharToUpper (*Str1);
    Char2 = CharToUpper (*Str2);

    if (Char1 != Char2) {
      return Char1 - Char2;
    }

    Str1++;
    Str2++;
  }

  return CharToUpper (*Str1) - CharToUpper (*Str2);
}

VOID
GetDeviceNameFromProduct (
  IN      CHAR16                  *Product,
  IN      UINTN                   BufferSize,
  OUT     CHAR16                  **DeviceName
  )
{
  // grouped by platform
  //SNB/IVB
  if (!StriCmp(Product, L"Butterfly")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Pavilion Chromebook 14");
  } else if (!StriCmp(Product, L"Link")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Google Chromebook Pixel 2013");
  } else if (!StriCmp(Product, L"Lumpy")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Samsung Chromebook Series 5 550");
  } else if (!StriCmp(Product, L"Parrot")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook C7/C710");
  } else if (!StriCmp(Product, L"Stout")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo ThinkPad X131e Chromebook");
  } else if (!StriCmp(Product, L"Stumpy")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Samsung Chromebox Series 3");
  }
  //HSW
    else if (!StriCmp(Product, L"Falco")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook 14");
  } else if (!StriCmp(Product, L"Leon")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Toshiba Chromebook");
  } else if (!StriCmp(Product, L"Mccloud")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebox CXI");
  } else if (!StriCmp(Product, L"Monroe")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"LG Chromebase");
  } else if (!StriCmp(Product, L"Panther")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebox CN60");
  } else if (!StriCmp(Product, L"Peppy")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook C720");
  } else if (!StriCmp(Product, L"Tricky")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Dell Chromebox 3010");
  } else if (!StriCmp(Product, L"Wolf")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Dell Chromebook 11 2013");
  } else if (!StriCmp(Product, L"Zako")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebox CB1");
  }
  //BDW
    else if (!StriCmp(Product, L"Auron")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 11 [C740/C910]");
  } else if (!StriCmp(Product, L"Auron_Paine")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 11 [C740]");
  } else if (!StriCmp(Product, L"Auron_Yuna")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 15 [C910]");
  } else if (!StriCmp(Product, L"Buddy")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebase 24");
  } else if (!StriCmp(Product, L"Gandof")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Toshiba Chromebook2 2015");
  } else if (!StriCmp(Product, L"Guado")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebox 2 / CN62");
  } else if (!StriCmp(Product, L"Lulu")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Dell Chromebook 13 7310");
  } else if (!StriCmp(Product, L"Rikku")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebox CXI2");
  } else if (!StriCmp(Product, L"Samus")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Google Chromebook Pixel 2015");
  } else if (!StriCmp(Product, L"Tidus")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo ThinkCentre Chromebox");
  }
  //BYT
    else if (!StriCmp(Product, L"Banjo")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 15 [CB3-571]");
  } else if (!StriCmp(Product, L"Candy")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Dell Chromebook 11 3120");
  } else if (!StriCmp(Product, L"Clapper")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo N20 Chromebook");
  } else if (!StriCmp(Product, L"Enguarde")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo N21 Chromebook");
  } else if (!StriCmp(Product, L"Glimmer")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo ThinkPad 11e");
  } else if (!StriCmp(Product, L"Gnawty")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 11 [CB3-111/131]");
  } else if (!StriCmp(Product, L"Heli")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Haier Chromebook G2");
  } else if (!StriCmp(Product, L"Kip")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook 11 G3/G4");
  } else if (!StriCmp(Product, L"Ninja")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"AOpen Chromebox Commercial");
  } else if (!StriCmp(Product, L"Orco")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo Ideapad 100s");
  } else if (!StriCmp(Product, L"Quawks")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook C300");
  } else if (!StriCmp(Product, L"Squawks")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook C200");
  } else if (!StriCmp(Product, L"Sumo")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"AOpen Chromebase Commercial");
  } else if (!StriCmp(Product, L"Swanky")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Toshiba Chromebook2 2014");
  } else if (!StriCmp(Product, L"Winky")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Samsung Chromebook 2");
  }
  //BSW
    else if (!StriCmp(Product, L"Banon")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 15 [CB5-532]");
  } else if (!StriCmp(Product, L"Celes")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Samsung Chromebook 3");
  } else if (!StriCmp(Product, L"Cyan")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook R11 [C738T]");
  } else if (!StriCmp(Product, L"Edgar")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 14");
  } else if (!StriCmp(Product, L"Kefka")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Dell Chromebook 11 3180/3189");
  } else if (!StriCmp(Product, L"Reks")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo N22 Chromebook");
  } else if (!StriCmp(Product, L"Relm")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 11 N7");
  } else if (!StriCmp(Product, L"Setzer")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook 11 G5");
  } else if (!StriCmp(Product, L"Terra")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook C202SA/C300SA");
  } else if (!StriCmp(Product, L"Ultima")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo ThinkPad 11e/Yoga Chromebook (G3)");
  } else if (!StriCmp(Product, L"Wizpig")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"White Box Braswell Chromebook");
  }
  //SKL
    else if (!StriCmp(Product, L"Asuka")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Dell Chromebook 13 3380");
  } else if (!StriCmp(Product, L"Caroline")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Samsung Chromebook Pro");
  } else if (!StriCmp(Product, L"Cave")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook Flip C302");
  } else if (!StriCmp(Product, L"Chell")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook 13 G1");
  } else if (!StriCmp(Product, L"Lars")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 14 for Work");
  } else if (!StriCmp(Product, L"Sentry")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo ThinkPad 13 Chromebook");
  }
  //APL
    else if (!StriCmp(Product, L"Astronaut")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 11 (C732)");
  } else if (!StriCmp(Product, L"Babymako")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook C403");
  } else if (!StriCmp(Product, L"Babymega")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook C223NA");
  } else if (!StriCmp(Product, L"Babytiger")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook C523NA");
  } else if (!StriCmp(Product, L"Blacktip")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"CTL Chromebook NL7/NL7T");
  } else if (!StriCmp(Product, L"Blue")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 15 [CB315]");
  } else if (!StriCmp(Product, L"Bruce")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Spin 15 [CP315]");
  } else if (!StriCmp(Product, L"Electro")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Spin 11 [R751T]");
  } else if (!StriCmp(Product, L"Epaulette")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 514");
  } else if (!StriCmp(Product, L"Lava")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Spin 11 [CP311]");
  } else if (!StriCmp(Product, L"Nasher")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Dell Chromebook 11 5190");
  } else if (!StriCmp(Product, L"Nasher360")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Dell Chromebook 11 5190 2-in-1");
  } else if (!StriCmp(Product, L"Pyro")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo Thinkpad 11e/Yoga 11e [G4]");
  } else if (!StriCmp(Product, L"Rabbid")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook C423");
  } else if (!StriCmp(Product, L"Reef")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook Flip C213SA");
  } else if (!StriCmp(Product, L"Robo")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo 100e Chromebook");
  } else if (!StriCmp(Product, L"Robo360")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo 500e Chromebook");
  } else if (!StriCmp(Product, L"Sand")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 15 [CB515-1HT]");
  } else if (!StriCmp(Product, L"Santa")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 11 [CB311-8H]");
  } else if (!StriCmp(Product, L"Snappy")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook x360 11 G1/11 G6/14 G5");
  } else if (!StriCmp(Product, L"Whitetip")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"CTL Chromebook J41/J41T");
  }
  //KBL
    else if (!StriCmp(Product, L"Akali")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 13");
  } else if (!StriCmp(Product, L"Akali 360")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Spin 13");
  } else if (!StriCmp(Product, L"Atlas")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Google Pixelbook Go 2019");
  } else if (!StriCmp(Product, L"Bard")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 715");
  } else if (!StriCmp(Product, L"Ekko")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 714");
  } else if (!StriCmp(Product, L"Endeavour")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Google Meet Series One");
  } else if (!StriCmp(Product, L"Eve")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Google Pixelbook 2017");
  } else if (!StriCmp(Product, L"Excelsior")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Google Meet Kit (KBL)");
  } else if (!StriCmp(Product, L"Jax")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"AOpen Chromebox Commercial 2");
  } else if (!StriCmp(Product, L"Karma")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebase 24I2");
  } else if (!StriCmp(Product, L"Kench")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebox G2");
  } else if (!StriCmp(Product, L"Leona")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook C425");
  }else if (!StriCmp(Product, L"Nautilus")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Samsung Chromebook Plus V2");
  } else if (!StriCmp(Product, L"Nocturne")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Google Pixel Slate");
  } else if (!StriCmp(Product, L"Pantheon")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo Yoga Chromebook C630");
  } else if (!StriCmp(Product, L"Rammus")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook C425/C433/C434");
  } else if (!StriCmp(Product, L"Shyvana")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook C433/C434");
  } else if (!StriCmp(Product, L"Sion")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebox CXI3");
  } else if (!StriCmp(Product, L"Sona")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook x360 14");
  } else if (!StriCmp(Product, L"Soraka")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook x2");
  } else if (!StriCmp(Product, L"Syndra")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook 15 G1");
  } else if (!StriCmp(Product, L"Teemo")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebox 3 / CN65");
  } else if (!StriCmp(Product, L"Vayne")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Dell Inspiron Chromebook 14");
  } else if (!StriCmp(Product, L"Wukong")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"CTL Chromebox CBx1");
  }
  //GLK
    else if (!StriCmp(Product, L"Ampton")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook Flip C214/C234");
  } else if (!StriCmp(Product, L"Apel")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook Flip C204");
  } else if (!StriCmp(Product, L"Bloog")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook x360 12b");
  } else if (!StriCmp(Product, L"Blooglet")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook 14a");
  } else if (!StriCmp(Product, L"Blooguard")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook x360 14a/14b");
  } else if (!StriCmp(Product, L"Blorb")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 315");
  } else if (!StriCmp(Product, L"Bluebird")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Samsung Chromebook 4");
  } else if (!StriCmp(Product, L"Bobba")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 311");
  } else if (!StriCmp(Product, L"Bobba360")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Spin 311/511");
  } else if (!StriCmp(Product, L"Casta")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Samsung Chromebook 4+");
  } else if (!StriCmp(Product, L"Dood")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"NEC Chromebook Y2");
  } else if (!StriCmp(Product, L"Dorp")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook 14 G6");
  } else if (!StriCmp(Product, L"Droid")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 314");
  } else if (!StriCmp(Product, L"Fleex")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Dell Chromebook 3100");
  } else if (!StriCmp(Product, L"Garg")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"CTL Chromebook NL71/CT/LTE");
  } else if (!StriCmp(Product, L"Garg360")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"CTL Chromebook NL71T/TW/TWB");
  } else if (!StriCmp(Product, L"Garfour")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"CTL Chromebook NL81/NL81T");
  } else if (!StriCmp(Product, L"Glk")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 311");
  } else if (!StriCmp(Product, L"Glk360")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Spin 311");
  } else if (!StriCmp(Product, L"Grabbiter")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Dell Chromebook 3100 2-in-1");
  } else if (!StriCmp(Product, L"Foob")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"CTL Chromebook VX11/VX11T");
  } else if (!StriCmp(Product, L"Foob360")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Poin2 Chromebook 11P");
  } else if (!StriCmp(Product, L"Laser")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo Chromebook C340");
  } else if (!StriCmp(Product, L"Laser14")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo Chromebook S340");
  } else if (!StriCmp(Product, L"Lick")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo Ideapad 3 Chromebook");
  } else if (!StriCmp(Product, L"Meep")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook x360 11 G2 EE");
  } else if (!StriCmp(Product, L"Mimrock")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook 11 G7 EE");
  } else if (!StriCmp(Product, L"Nospike")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook C424");
  } else if (!StriCmp(Product, L"Orbatrix")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Dell Chromebook 3400");
  } else if (!StriCmp(Product, L"Phaser")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo 100e Chromebook 2nd Gen");
  } else if (!StriCmp(Product, L"Phaser360")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo 300e Chromebook 2nd Gen");
  } else if (!StriCmp(Product, L"Phaser360s")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo 500e Chromebook 2nd Gen");
  } else if (!StriCmp(Product, L"Sparky")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 512");
  } else if (!StriCmp(Product, L"Sparky360")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Spin 512");
  } else if (!StriCmp(Product, L"Vorticon")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook 11 G8 EE");
  } else if (!StriCmp(Product, L"Vortininja")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook x360 11 G3 EE");
  }
  //WHL
    else if (!StriCmp(Product, L"Arcada")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Dell Latitude 5300 2-in-1 Chromebook Enterprise");
  } else if (!StriCmp(Product, L"Sarien")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Dell Latitude 5400 Chromebook Enterprise");
  }
  //CML
    else if (!StriCmp(Product, L"Akemi")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo Ideapad Flex 5 Chromebook");
  } else if (!StriCmp(Product, L"Ambassador")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"CTL Meet Compute System GQE15C");
  } else if (!StriCmp(Product, L"Dragonair")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook x360 14c");
  } else if (!StriCmp(Product, L"Drallion")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Dell Latitude 7410 Chromebook Enterprise");
  } else if (!StriCmp(Product, L"Dratini")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Pro c640 Chromebook");
  } else if (!StriCmp(Product, L"Duffy")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebox 4");
  } else if (!StriCmp(Product, L"Faffy")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Fanless Chromebox");
  } else if (!StriCmp(Product, L"Helios")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook Flip C436FA");
  } else if (!StriCmp(Product, L"Jinlon")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Elite c1030 Chromebook / Chromebook x360 13c");
  } else if (!StriCmp(Product, L"Kaisa")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebox CXI4");
  } else if (!StriCmp(Product, L"Kindred")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 712");
  } else if (!StriCmp(Product, L"Kled")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Spin 713 [CP713-2W]");
  } else if (!StriCmp(Product, L"Kohaku")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Samsung Galaxy Chromebook");
  } else if (!StriCmp(Product, L"Nightfury")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Samsung Galaxy Chromebook 2");
  } else if (!StriCmp(Product, L"Noibat")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebox G3");
  } else if (!StriCmp(Product, L"Wyvern")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"CTL Chromebox CBx2");
  }
  //Tigerlake
    else if (!StriCmp(Product, L"Chronicler")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"FMV Chromebook 14F	");
  } else if (!StriCmp(Product, L"Collis")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook Flip CX3");
  } else if (!StriCmp(Product, L"Copano")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook Flip CX5 [CX5400]");
  } else if (!StriCmp(Product, L"Delbin")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook Flip CX55/CX5/C536");
  } else if (!StriCmp(Product, L"Drobit")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook CX9 [CX9400]");
  } else if (!StriCmp(Product, L"Eldrid")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook x360 14c");
  } else if (!StriCmp(Product, L"Elemi")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Pro c640 G2 Chromebook");
  } else if (!StriCmp(Product, L"Lillipup")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"IdeaPad Flex 5i Chromebook");
  } else if (!StriCmp(Product, L"Lindar")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo 5i-14 Chromebook");
  } else if (!StriCmp(Product, L"Voema")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Spin 514 [CB514-2H]");
  } else if (!StriCmp(Product, L"Volet")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 515 [CB515-1W/1WT]");
  } else if (!StriCmp(Product, L"Volta")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 514 [CB514-1W/1WT]");
  } else if (!StriCmp(Product, L"Voxel")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Spin 713 [CP713-3W]");
  }
  //Jasperlake
    else if (!StriCmp(Product, L"Beadrix")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Beadrix JSL Chromebook");
  } else if (!StriCmp(Product, L"Beetley")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo Flex 3i Chromebook");
  } else if (!StriCmp(Product, L"Blipper")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo 3i-15 Chromebook");
  } else if (!StriCmp(Product, L"Bookem")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo 100e Chromebook Gen 3");
  } else if (!StriCmp(Product, L"Boten")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo 500e Chromebook Gen 3");
  } else if (!StriCmp(Product, L"Boxy")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo Chromebox Micro");
  } else if (!StriCmp(Product, L"Botenflex")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo Flex 3i-11 Chromebook");
  } else if (!StriCmp(Product, L"Bugzzy")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Samsung Galaxy Chromebook 2 360");
  } else if (!StriCmp(Product, L"Cret")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Dell Chromebook 3110");
  } else if (!StriCmp(Product, L"Cret360")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Dell Chromebook 3110 2-in-1");
  } else if (!StriCmp(Product, L"Dexi")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"AOPEN Chromebox Mini 2");
  } else if (!StriCmp(Product, L"Drawcia")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook x360 11 G4 EE");
  } else if (!StriCmp(Product, L"Drawlat")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook 11 G9 EE");
  } else if (!StriCmp(Product, L"Drawman")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook 14 G7");
  } else if (!StriCmp(Product, L"Drawper")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Fortis 14 G10 Chromebook");
  } else if (!StriCmp(Product, L"Galith")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook CX1500CKA");
  } else if (!StriCmp(Product, L"Galith360")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook CX1500FKA");
  } else if (!StriCmp(Product, L"Gallop")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook CX1700CKA");
  } else if (!StriCmp(Product, L"Galnat")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook CX1 CX1102");
  } else if (!StriCmp(Product, L"Galnat360")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook Flip CX1 CX1102");
  } else if (!StriCmp(Product, L"Galtic")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook CX1");
  } else if (!StriCmp(Product, L"Galtic360")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook CX1400FKA");
  } else if (!StriCmp(Product, L"Kracko")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"CTL Chromebook NL72");
  } else if (!StriCmp(Product, L"Kracko360")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"CTL Chromebook NL72T");
  } else if (!StriCmp(Product, L"Landia")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook x360 14a-ca1");
  } else if (!StriCmp(Product, L"Landrid")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook 15a-na0");
  } else if (!StriCmp(Product, L"Lantis")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook 14a");
  } else if (!StriCmp(Product, L"Madoo")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook x360 14b");
  } else if (!StriCmp(Product, L"Magister")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Spin 314");
  } else if (!StriCmp(Product, L"Maglet")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 512");
  } else if (!StriCmp(Product, L"Maglia")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Spin 512");
  } else if (!StriCmp(Product, L"Maglith")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 511");
  } else if (!StriCmp(Product, L"Magma")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 315");
  } else if (!StriCmp(Product, L"Magneto")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 314");
  } else if (!StriCmp(Product, L"Magolor")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Spin 511");
  } else if (!StriCmp(Product, L"Magpie")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 317");
  } else if (!StriCmp(Product, L"Metaknight")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"NEC Chromebook Y3");
  } else if (!StriCmp(Product, L"Palutena")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 314");
  } else if (!StriCmp(Product, L"Pasara")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Gateway Chromebook 15");
  } else if (!StriCmp(Product, L"Peezer")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 311");
  } else if (!StriCmp(Product, L"Pirette")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"CTL Chromebook PX11E");
  } else if (!StriCmp(Product, L"Pirika")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Gateway Chromebook 14");
  } else if (!StriCmp(Product, L"Sasuke")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Samsung Galaxy Chromebook Go");
  } else if (!StriCmp(Product, L"Sasukette")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Samsung Galaxy Chromebook Go 11");
  } else if (!StriCmp(Product, L"Storo")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook CR1100CKA");
  } else if (!StriCmp(Product, L"Storo360")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook Flip CR1100FKA");
  } else if (!StriCmp(Product, L"Taranza")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"ASUS Fanless Chromebox CF40");
  }
  //Alderlake/Raptorlake-U/P
    else if (!StriCmp(Product, L"Anahera")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Elite c640 14 inch G3 Chromebook");
  } else if (!StriCmp(Product, L"Aurash")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"AOpen Chromebox Commercial 3");
  } else if (!StriCmp(Product, L"Banshee")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Framework Laptop Chromebook Edition");
  } else if (!StriCmp(Product, L"Constitution")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Google Meet Series Two");
  } else if (!StriCmp(Product, L"Crota")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Dell Latitude 5430 Chromebook");
  } else if (!StriCmp(Product, L"Crota360")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Dell Latitude 5430 2-in-1 Chromebook");
  } else if (!StriCmp(Product, L"Dochi")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Plus Spin 514 [CP514-4HN]");
  } else if (!StriCmp(Product, L"Felwinter")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook Flip CX5 [CX5601]");
  } else if (!StriCmp(Product, L"Gimble")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook x360 14c-cd0");
  } else if (!StriCmp(Product, L"Gladios")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebox Enterprise G4");
  } else if (!StriCmp(Product, L"Intrepid")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"CTL Meet Compute System GQE20C");
  } else if (!StriCmp(Product, L"Kano")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Spin 714 [CP714-1WN]");
  } else if (!StriCmp(Product, L"Kinox")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo ThinkCentre M60q Chromebox");
  } else if (!StriCmp(Product, L"Kuldax")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"ASUS Chromebox 5 [CN67]");
  } else if (!StriCmp(Product, L"Lisbon")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"CTL Chromebox CBx3");
  } else if (!StriCmp(Product, L"Marasov")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook CX34");
  } else if (!StriCmp(Product, L"Mithrax")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook CX34 Flip");
  } else if (!StriCmp(Product, L"Moli")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebox CXI5");
  } else if (!StriCmp(Product, L"Omnigul")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Plus 515 [CB515-2H]");
  } else if (!StriCmp(Product, L"Omniknight")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Plus Enterprise 515");
  } else if (!StriCmp(Product, L"Osiris")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 516 GE [CBG516-1H]");
  } else if (!StriCmp(Product, L"Primus")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo ThinkPad C14 Gen 1 Chromebook");
  } else if (!StriCmp(Product, L"Redrix")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Elite Dragonfly Chromebook");
  } else if (!StriCmp(Product, L"Taeko")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo Flex 5i Chromebook");
  } else if (!StriCmp(Product, L"Taniks")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo IdeaPad Gaming Chromebook 16");
  } else if (!StriCmp(Product, L"Tarlo")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo 5i Chromebook 16\"");
  } else if (!StriCmp(Product, L"Vell")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Dragonfly Pro Chromebook");
  } else if (!StriCmp(Product, L"Volmar")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Vero 514");
  } else if (!StriCmp(Product, L"Zavala")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Vero 712");
  }
  //Alderlake-N
    else if (!StriCmp(Product, L"Anraggar")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"ASUS Chromebook CR12");
  } else if (!StriCmp(Product, L"Anraggar360")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"ASUS Chromebook CR12 Flip");
  } else if (!StriCmp(Product, L"Craask")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Spin 512");
  } else if (!StriCmp(Product, L"Craaskana")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 311");
  } else if (!StriCmp(Product, L"Craaskbowl")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Spin 511");
  } else if (!StriCmp(Product, L"Craaskino")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 315");
  } else if (!StriCmp(Product, L"Craaskov")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Spin 312");
  } else if (!StriCmp(Product, L"Craaskvin")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 511");
  } else if (!StriCmp(Product, L"Craasneto")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 314");
  } else if (!StriCmp(Product, L"Joxer")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"TBD ADL-N Chromebook");
  } else if (!StriCmp(Product, L"Joxero")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"TBD ADL-N Chromebook");
  } else if (!StriCmp(Product, L"Nereid")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"TBD ADL-N Chromebook");
  } else if (!StriCmp(Product, L"Nirwin")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"TBD ADL-N Chromebook");
  } else if (!StriCmp(Product, L"Nivviks")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"TBD ADL-N Chromebook");
  } else if (!StriCmp(Product, L"Pujjo")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo IdeaPad Flex 3i Chromebook");
  } else if (!StriCmp(Product, L"Pujjoflex")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo IdeaPad Flex 3i Chromebook");
  } else if (!StriCmp(Product, L"Pujjoteen")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo 14e Chromebook Gen 3");
  } else if (!StriCmp(Product, L"Pujjoteen15w")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo Ideapad Slim 3i Chromebook");
  } else if (!StriCmp(Product, L"Quandiso")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"CTL Chromebook NL73");
  } else if (!StriCmp(Product, L"Quandiso360")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"CTL Chromebook NL73T");
  } else if (!StriCmp(Product, L"Uldren")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Dell Chromebook 3120");
  } else if (!StriCmp(Product, L"Uldren360")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Dell Chromebook 3120 2-in-1");
  } else if (!StriCmp(Product, L"Xivu")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook CR11 [CR1102C]");
  } else if (!StriCmp(Product, L"Yahiko")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook 14a-nf0xxx");
  } else if (!StriCmp(Product, L"Yavijo")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Fortis 14\" G11 Chromebook");
  } else if (!StriCmp(Product, L"Yaviks")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook 15.6");
  } else if (!StriCmp(Product, L"Yavikso")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"TBD ADL-N Chromebook");
  } else if (!StriCmp(Product, L"Yavilla")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Fortis 11\" G10 Chromebook");
  } else if (!StriCmp(Product, L"Yavilly")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Fortis x360 11\" G5 Chromebook");
  }
  //Meteorlake
    else if (!StriCmp(Product, L"Screebo")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"ASUS ExpertBook CX54 Chromebook Plus [CX5403]");
  }
  //AMD StoneyRidge
    else if (!StriCmp(Product, L"Aleena")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 315");
  } else if (!StriCmp(Product, L"Barla")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook 11A G6 EE");
  } else if (!StriCmp(Product, L"Careena")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook 14");
  } else if (!StriCmp(Product, L"Grunt")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"AMD StonyRidge Chromebook");
  } else if (!StriCmp(Product, L"Kasumi")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 311");
  } else if (!StriCmp(Product, L"Liara")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo 14e Chromebook");
  } else if (!StriCmp(Product, L"Treeya")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo 100e Chromebook 2nd Gen");
  } else if (!StriCmp(Product, L"Treeya360")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo 300e Chromebook 2nd Gen");
  }
  // AMD Picasso
    else if (!StriCmp(Product, L"Berknip")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Pro c645 Chromebook Enterprise");
  } else if (!StriCmp(Product, L"Dirinboz")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook 14a");
  } else if (!StriCmp(Product, L"Ezkinil")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Spin 514");
  } else if (!StriCmp(Product, L"Gumboz")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook x360 14a");
  } else if (!StriCmp(Product, L"Morphius")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo ThinkPad C13 Yoga Chromebook");
  } else if (!StriCmp(Product, L"Vilboz")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo 100e Chromebook Gen 3");
  } else if (!StriCmp(Product, L"Vilboz14")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo 14e Chromebook Gen 2");
  } else if (!StriCmp(Product, L"Vilboz360")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo 300e Chromebook Gen 3");
  } else if (!StriCmp(Product, L"Woomax")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook Flip CM5");
  }
  // AMD Cezanne
    else if (!StriCmp(Product, L"Dewatt")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Spin 514");
  } else if (!StriCmp(Product, L"Nipperkin")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Elite c645 G2 Chroembook");
  }
  // AMD Mendocino
    else if (!StriCmp(Product, L"Crystaldrift")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"TBD Skyrim Chromebook");
  } else if (!StriCmp(Product, L"Frostflow")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"ASUS Chromebook CM34 Flip");
  } else if (!StriCmp(Product, L"Markarth")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Plus 514");
  } else if (!StriCmp(Product, L"Whiterun")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Dell Latitude 3445 Chromebook");
  }
}
