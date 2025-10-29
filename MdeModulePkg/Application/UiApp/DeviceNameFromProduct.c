#include "FrontPage.h"

VOID
GetDeviceNameFromProduct (
  IN      CHAR16                  *Product,
  IN      UINTN                   BufferSize,
  OUT     CHAR16                  **DeviceName
  )
{
  // grouped by platform
  //SNB/IVB
  if (!StrCmp(Product, L"Butterfly")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Pavilion Chromebook 14");
  } else if (!StrCmp(Product, L"Link")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Google Chromebook Pixel 2013");
  } else if (!StrCmp(Product, L"Lumpy")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Samsung Chromebook Series 5 550");
  } else if (!StrCmp(Product, L"Parrot")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook C7/C710");
  } else if (!StrCmp(Product, L"Stout")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo ThinkPad X131e Chromebook");
  } else if (!StrCmp(Product, L"Stumpy")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Samsung Chromebox Series 3");
  }
  //HSW
    else if (!StrCmp(Product, L"Falco")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook 14");
  } else if (!StrCmp(Product, L"Leon")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Toshiba Chromebook");
  } else if (!StrCmp(Product, L"Mccloud")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebox CXI");
  } else if (!StrCmp(Product, L"Monroe")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"LG Chromebase");
  } else if (!StrCmp(Product, L"Panther")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebox CN60");
  } else if (!StrCmp(Product, L"Peppy")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook C720");
  } else if (!StrCmp(Product, L"Tricky")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Dell Chromebox 3010");
  } else if (!StrCmp(Product, L"Wolf")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Dell Chromebook 11 2013");
  } else if (!StrCmp(Product, L"Zako")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebox CB1");
  }
  //BDW
    else if (!StrCmp(Product, L"Auron")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 11 [C740/C910]");
  } else if (!StrCmp(Product, L"Auron_Paine")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 11 [C740]");
  } else if (!StrCmp(Product, L"Auron_Yuna")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 15 [C910]");
  } else if (!StrCmp(Product, L"Buddy")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebase 24");
  } else if (!StrCmp(Product, L"Gandof")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Toshiba Chromebook2 2015");
  } else if (!StrCmp(Product, L"Guado")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebox 2 / CN62");
  } else if (!StrCmp(Product, L"Lulu")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Dell Chromebook 13 7310");
  } else if (!StrCmp(Product, L"Rikku")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebox CXI2");
  } else if (!StrCmp(Product, L"Samus")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Google Chromebook Pixel 2015");
  } else if (!StrCmp(Product, L"Tidus")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo ThinkCentre Chromebox");
  }
  //BYT
    else if (!StrCmp(Product, L"Banjo")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 15 [CB3-571]");
  } else if (!StrCmp(Product, L"Candy")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Dell Chromebook 11 3120");
  } else if (!StrCmp(Product, L"Clapper")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo N20 Chromebook");
  } else if (!StrCmp(Product, L"Enguarde")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo N21 Chromebook");
  } else if (!StrCmp(Product, L"Glimmer")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo ThinkPad 11e");
  } else if (!StrCmp(Product, L"Gnawty")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 11 [CB3-111/131]");
  } else if (!StrCmp(Product, L"Heli")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Haier Chromebook G2");
  } else if (!StrCmp(Product, L"Kip")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook 11 G3/G4");
  } else if (!StrCmp(Product, L"Ninja")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"AOpen Chromebox Commercial");
  } else if (!StrCmp(Product, L"Orco")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo Ideapad 100s");
  } else if (!StrCmp(Product, L"Quawks")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook C300");
  } else if (!StrCmp(Product, L"Squawks")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook C200");
  } else if (!StrCmp(Product, L"Sumo")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"AOpen Chromebase Commercial");
  } else if (!StrCmp(Product, L"Swanky")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Toshiba Chromebook2 2014");
  } else if (!StrCmp(Product, L"Winky")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Samsung Chromebook 2");
  }
  //BSW
    else if (!StrCmp(Product, L"Banon")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 15 [CB5-532]");
  } else if (!StrCmp(Product, L"Celes")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Samsung Chromebook 3");
  } else if (!StrCmp(Product, L"Cyan")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook R11 [C738T]");
  } else if (!StrCmp(Product, L"Edgar")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 14");
  } else if (!StrCmp(Product, L"Kefka")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Dell Chromebook 11 3180/3189");
  } else if (!StrCmp(Product, L"Reks")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo N22 Chromebook");
  } else if (!StrCmp(Product, L"Relm")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 11 N7");
  } else if (!StrCmp(Product, L"Setzer")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook 11 G5");
  } else if (!StrCmp(Product, L"Terra")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook C202SA/C300SA");
  } else if (!StrCmp(Product, L"Ultima")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo ThinkPad 11e/Yoga Chromebook (G3)");
  } else if (!StrCmp(Product, L"Wizpig")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"White Box Braswell Chromebook");
  }
  //SKL
    else if (!StrCmp(Product, L"Asuka")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Dell Chromebook 13 3380");
  } else if (!StrCmp(Product, L"Caroline")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Samsung Chromebook Pro");
  } else if (!StrCmp(Product, L"Cave")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook Flip C302");
  } else if (!StrCmp(Product, L"Chell")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook 13 G1");
  } else if (!StrCmp(Product, L"Lars")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 14 for Work");
  } else if (!StrCmp(Product, L"Sentry")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo ThinkPad 13 Chromebook");
  }
  //APL
    else if (!StrCmp(Product, L"Astronaut")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 11 (C732)");
  } else if (!StrCmp(Product, L"Babymako")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook C403");
  } else if (!StrCmp(Product, L"Babymega")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook C223NA");
  } else if (!StrCmp(Product, L"Babytiger")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook C523NA");
  } else if (!StrCmp(Product, L"Blacktip")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"CTL Chromebook NL7/NL7T");
  } else if (!StrCmp(Product, L"Blue")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 15 [CB315]");
  } else if (!StrCmp(Product, L"Bruce")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Spin 15 [CP315]");
  } else if (!StrCmp(Product, L"Electro")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Spin 11 [R751T]");
  } else if (!StrCmp(Product, L"Epaulette")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 514");
  } else if (!StrCmp(Product, L"Lava")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Spin 11 [CP311]");
  } else if (!StrCmp(Product, L"Nasher")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Dell Chromebook 11 5190");
  } else if (!StrCmp(Product, L"Nasher360")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Dell Chromebook 11 5190 2-in-1");
  } else if (!StrCmp(Product, L"Pyro")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo Thinkpad 11e/Yoga 11e [G4]");
  } else if (!StrCmp(Product, L"Rabbid")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook C423");
  } else if (!StrCmp(Product, L"Reef")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook Flip C213SA");
  } else if (!StrCmp(Product, L"Robo")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo 100e Chromebook");
  } else if (!StrCmp(Product, L"Robo360")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo 500e Chromebook");
  } else if (!StrCmp(Product, L"Sand")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 15 [CB515-1HT]");
  } else if (!StrCmp(Product, L"Santa")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 11 [CB311-8H]");
  } else if (!StrCmp(Product, L"Snappy")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook x360 11 G1/11 G6/14 G5");
  } else if (!StrCmp(Product, L"Whitetip")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"CTL Chromebook J41/J41T");
  }
  //KBL
    else if (!StrCmp(Product, L"Akali")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 13");
  } else if (!StrCmp(Product, L"Akali 360")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Spin 13");
  } else if (!StrCmp(Product, L"Atlas")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Google Pixelbook Go 2019");
  } else if (!StrCmp(Product, L"Bard")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 715");
  } else if (!StrCmp(Product, L"Ekko")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 714");
  } else if (!StrCmp(Product, L"Endeavour")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Google Meet Series One");
  } else if (!StrCmp(Product, L"Eve")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Google Pixelbook 2017");
  } else if (!StrCmp(Product, L"Excelsior")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Google Meet Kit (KBL)");
  } else if (!StrCmp(Product, L"Jax")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"AOpen Chromebox Commercial 2");
  } else if (!StrCmp(Product, L"Karma")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebase 24I2");
  } else if (!StrCmp(Product, L"Kench")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebox G2");
  } else if (!StrCmp(Product, L"Leona")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook C425");
  }else if (!StrCmp(Product, L"Nautilus")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Samsung Chromebook Plus V2");
  } else if (!StrCmp(Product, L"Nocturne")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Google Pixel Slate");
  } else if (!StrCmp(Product, L"Pantheon")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo Yoga Chromebook C630");
  } else if (!StrCmp(Product, L"Rammus")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook C425/C433/C434");
  } else if (!StrCmp(Product, L"Shyvana")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook C433/C434");
  } else if (!StrCmp(Product, L"Sion")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebox CXI3");
  } else if (!StrCmp(Product, L"Sona")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook x360 14");
  } else if (!StrCmp(Product, L"Soraka")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook x2");
  } else if (!StrCmp(Product, L"Syndra")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook 15 G1");
  } else if (!StrCmp(Product, L"Teemo")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebox 3 / CN65");
  } else if (!StrCmp(Product, L"Vayne")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Dell Inspiron Chromebook 14");
  } else if (!StrCmp(Product, L"Wukong")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"CTL Chromebox CBx1");
  }
  //GLK
    else if (!StrCmp(Product, L"Ampton")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook Flip C214/C234");
  } else if (!StrCmp(Product, L"Apel")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook Flip C204");
  } else if (!StrCmp(Product, L"Bloog")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook x360 12b");
  } else if (!StrCmp(Product, L"Blooglet")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook 14a");
  } else if (!StrCmp(Product, L"Blooguard")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook x360 14a/14b");
  } else if (!StrCmp(Product, L"Blorb")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 315");
  } else if (!StrCmp(Product, L"Bluebird")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Samsung Chromebook 4");
  } else if (!StrCmp(Product, L"Bobba")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 311");
  } else if (!StrCmp(Product, L"Bobba360")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Spin 311/511");
  } else if (!StrCmp(Product, L"Casta")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Samsung Chromebook 4+");
  } else if (!StrCmp(Product, L"Dood")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"NEC Chromebook Y2");
  } else if (!StrCmp(Product, L"Dorp")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook 14 G6");
  } else if (!StrCmp(Product, L"Droid")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 314");
  } else if (!StrCmp(Product, L"Fleex")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Dell Chromebook 3100");
  } else if (!StrCmp(Product, L"Garg")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"CTL Chromebook NL71/CT/LTE");
  } else if (!StrCmp(Product, L"Garg360")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"CTL Chromebook NL71T/TW/TWB");
  } else if (!StrCmp(Product, L"Garfour")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"CTL Chromebook NL81/NL81T");
  } else if (!StrCmp(Product, L"Glk")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 311");
  } else if (!StrCmp(Product, L"Glk360")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Spin 311");
  } else if (!StrCmp(Product, L"Grabbiter")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Dell Chromebook 3100 2-in-1");
  } else if (!StrCmp(Product, L"Foob")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"CTL Chromebook VX11/VX11T");
  } else if (!StrCmp(Product, L"Foob360")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Poin2 Chromebook 11P");
  } else if (!StrCmp(Product, L"Laser")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo Chromebook C340");
  } else if (!StrCmp(Product, L"Laser14")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo Chromebook S340");
  } else if (!StrCmp(Product, L"Lick")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo Ideapad 3 Chromebook");
  } else if (!StrCmp(Product, L"Meep")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook x360 11 G2 EE");
  } else if (!StrCmp(Product, L"Mimrock")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook 11 G7 EE");
  } else if (!StrCmp(Product, L"Nospike")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook C424");
  } else if (!StrCmp(Product, L"Orbatrix")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Dell Chromebook 3400");
  } else if (!StrCmp(Product, L"Phaser")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo 100e Chromebook 2nd Gen");
  } else if (!StrCmp(Product, L"Phaser360")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo 300e Chromebook 2nd Gen");
  } else if (!StrCmp(Product, L"Phaser360s")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo 500e Chromebook 2nd Gen");
  } else if (!StrCmp(Product, L"Sparky")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 512");
  } else if (!StrCmp(Product, L"Sparky360")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Spin 512");
  } else if (!StrCmp(Product, L"Vorticon")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook 11 G8 EE");
  } else if (!StrCmp(Product, L"Vortininja")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook x360 11 G3 EE");
  }
  //WHL
    else if (!StrCmp(Product, L"Arcada")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Dell Latitude 5300 2-in-1 Chromebook Enterprise");
  } else if (!StrCmp(Product, L"Sarien")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Dell Latitude 5400 Chromebook Enterprise");
  }
  //CML
    else if (!StrCmp(Product, L"Akemi")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo Ideapad Flex 5 Chromebook");
  } else if (!StrCmp(Product, L"Ambassador")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"CTL Meet Compute System GQE15C");
  } else if (!StrCmp(Product, L"Dragonair")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook x360 14c");
  } else if (!StrCmp(Product, L"Drallion")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Dell Latitude 7410 Chromebook Enterprise");
  } else if (!StrCmp(Product, L"Dratini")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Pro c640 Chromebook");
  } else if (!StrCmp(Product, L"Duffy")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebox 4");
  } else if (!StrCmp(Product, L"Faffy")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Fanless Chromebox");
  } else if (!StrCmp(Product, L"Helios")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook Flip C436FA");
  } else if (!StrCmp(Product, L"Jinlon")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Elite c1030 Chromebook / Chromebook x360 13c");
  } else if (!StrCmp(Product, L"Kaisa")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebox CXI4");
  } else if (!StrCmp(Product, L"Kindred")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 712");
  } else if (!StrCmp(Product, L"Kled")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Spin 713 [CP713-2W]");
  } else if (!StrCmp(Product, L"Kohaku")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Samsung Galaxy Chromebook");
  } else if (!StrCmp(Product, L"Nightfury")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Samsung Galaxy Chromebook 2");
  } else if (!StrCmp(Product, L"Noibat")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebox G3");
  } else if (!StrCmp(Product, L"Wyvern")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"CTL Chromebox CBx2");
  }
  //Tigerlake
    else if (!StrCmp(Product, L"Chronicler")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"FMV Chromebook 14F	");
  } else if (!StrCmp(Product, L"Collis")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook Flip CX3");
  } else if (!StrCmp(Product, L"Copano")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook Flip CX5 [CX5400]");
  } else if (!StrCmp(Product, L"Delbin")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook Flip CX55/CX5/C536");
  } else if (!StrCmp(Product, L"Drobit")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook CX9 [CX9400]");
  } else if (!StrCmp(Product, L"Eldrid")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook x360 14c");
  } else if (!StrCmp(Product, L"Elemi")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Pro c640 G2 Chromebook");
  } else if (!StrCmp(Product, L"Lillipup")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"IdeaPad Flex 5i Chromebook");
  } else if (!StrCmp(Product, L"Lindar")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo 5i-14 Chromebook");
  } else if (!StrCmp(Product, L"Voema")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Spin 514 [CB514-2H]");
  } else if (!StrCmp(Product, L"Volet")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 515 [CB515-1W/1WT]");
  } else if (!StrCmp(Product, L"Volta")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 514 [CB514-1W/1WT]");
  } else if (!StrCmp(Product, L"Voxel")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Spin 713 [CP713-3W]");
  }
  //Jasperlake
    else if (!StrCmp(Product, L"Beadrix")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Beadrix JSL Chromebook");
  } else if (!StrCmp(Product, L"Beetley")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo Flex 3i Chromebook");
  } else if (!StrCmp(Product, L"Blipper")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo 3i-15 Chromebook");
  } else if (!StrCmp(Product, L"Bookem")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo 100e Chromebook Gen 3");
  } else if (!StrCmp(Product, L"Boten")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo 500e Chromebook Gen 3");
  } else if (!StrCmp(Product, L"Boxy")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo Chromebox Micro");
  } else if (!StrCmp(Product, L"Botenflex")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo Flex 3i-11 Chromebook");
  } else if (!StrCmp(Product, L"Bugzzy")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Samsung Galaxy Chromebook 2 360");
  } else if (!StrCmp(Product, L"Cret")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Dell Chromebook 3110");
  } else if (!StrCmp(Product, L"Cret360")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Dell Chromebook 3110 2-in-1");
  } else if (!StrCmp(Product, L"Dexi")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"AOPEN Chromebox Mini 2");
  } else if (!StrCmp(Product, L"Drawcia")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook x360 11 G4 EE");
  } else if (!StrCmp(Product, L"Drawlat")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook 11 G9 EE");
  } else if (!StrCmp(Product, L"Drawman")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook 14 G7");
  } else if (!StrCmp(Product, L"Drawper")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Fortis 14 G10 Chromebook");
  } else if (!StrCmp(Product, L"Galith")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook CX1500CKA");
  } else if (!StrCmp(Product, L"Galith360")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook CX1500FKA");
  } else if (!StrCmp(Product, L"Gallop")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook CX1700CKA");
  } else if (!StrCmp(Product, L"Galnat")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook CX1 CX1102");
  } else if (!StrCmp(Product, L"Galnat360")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook Flip CX1 CX1102");
  } else if (!StrCmp(Product, L"Galtic")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook CX1");
  } else if (!StrCmp(Product, L"Galtic360")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook CX1400FKA");
  } else if (!StrCmp(Product, L"Kracko")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"CTL Chromebook NL72");
  } else if (!StrCmp(Product, L"Kracko360")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"CTL Chromebook NL72T");
  } else if (!StrCmp(Product, L"Landia")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook x360 14a-ca1");
  } else if (!StrCmp(Product, L"Landrid")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook 15a-na0");
  } else if (!StrCmp(Product, L"Lantis")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook 14a");
  } else if (!StrCmp(Product, L"Madoo")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook x360 14b");
  } else if (!StrCmp(Product, L"Magister")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Spin 314");
  } else if (!StrCmp(Product, L"Maglet")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 512");
  } else if (!StrCmp(Product, L"Maglia")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Spin 512");
  } else if (!StrCmp(Product, L"Maglith")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 511");
  } else if (!StrCmp(Product, L"Magma")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 315");
  } else if (!StrCmp(Product, L"Magneto")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 314");
  } else if (!StrCmp(Product, L"Magolor")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Spin 511");
  } else if (!StrCmp(Product, L"Magpie")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 317");
  } else if (!StrCmp(Product, L"Metaknight")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"NEC Chromebook Y3");
  } else if (!StrCmp(Product, L"Palutena")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 314");
  } else if (!StrCmp(Product, L"Pasara")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Gateway Chromebook 15");
  } else if (!StrCmp(Product, L"Peezer")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 311");
  } else if (!StrCmp(Product, L"Pirette")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"CTL Chromebook PX11E");
  } else if (!StrCmp(Product, L"Pirika")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Gateway Chromebook 14");
  } else if (!StrCmp(Product, L"Sasuke")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Samsung Galaxy Chromebook Go");
  } else if (!StrCmp(Product, L"Sasukette")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Samsung Galaxy Chromebook Go 11");
  } else if (!StrCmp(Product, L"Storo")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook CR1100CKA");
  } else if (!StrCmp(Product, L"Storo360")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook Flip CR1100FKA");
  } else if (!StrCmp(Product, L"Taranza")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"ASUS Fanless Chromebox CF40");
  }
  //Alderlake/Raptorlake-U/P
    else if (!StrCmp(Product, L"Anahera")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Elite c640 14 inch G3 Chromebook");
  } else if (!StrCmp(Product, L"Aurash")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"AOpen Chromebox Commercial 3");
  } else if (!StrCmp(Product, L"Banshee")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Framework Laptop Chromebook Edition");
  } else if (!StrCmp(Product, L"Constitution")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Google Meet Series Two");
  } else if (!StrCmp(Product, L"Crota")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Dell Latitude 5430 Chromebook");
  } else if (!StrCmp(Product, L"Crota360")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Dell Latitude 5430 2-in-1 Chromebook");
  } else if (!StrCmp(Product, L"Dochi")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Plus Spin 514 [CP514-4HN]");
  } else if (!StrCmp(Product, L"Felwinter")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook Flip CX5 [CX5601]");
  } else if (!StrCmp(Product, L"Gimble")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook x360 14c-cd0");
  } else if (!StrCmp(Product, L"Gladios")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebox Enterprise G4");
  } else if (!StrCmp(Product, L"Intrepid")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"CTL Meet Compute System GQE20C");
  } else if (!StrCmp(Product, L"Kano")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Spin 714 [CP714-1WN]");
  } else if (!StrCmp(Product, L"Kinox")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo ThinkCentre M60q Chromebox");
  } else if (!StrCmp(Product, L"Kuldax")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"ASUS Chromebox 5 [CN67]");
  } else if (!StrCmp(Product, L"Lisbon")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"CTL Chromebox CBx3");
  } else if (!StrCmp(Product, L"Marasov")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook CX34");
  } else if (!StrCmp(Product, L"Mithrax")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook CX34 Flip");
  } else if (!StrCmp(Product, L"Moli")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebox CXI5");
  } else if (!StrCmp(Product, L"Omnigul")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Plus 515 [CB515-2H]");
  } else if (!StrCmp(Product, L"Omniknight")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Plus Enterprise 515");
  } else if (!StrCmp(Product, L"Osiris")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 516 GE [CBG516-1H]");
  } else if (!StrCmp(Product, L"Primus")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo ThinkPad C14 Gen 1 Chromebook");
  } else if (!StrCmp(Product, L"Redrix")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Elite Dragonfly Chromebook");
  } else if (!StrCmp(Product, L"Taeko")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo Flex 5i Chromebook");
  } else if (!StrCmp(Product, L"Taniks")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo IdeaPad Gaming Chromebook 16");
  } else if (!StrCmp(Product, L"Tarlo")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo 5i Chromebook 16\"");
  } else if (!StrCmp(Product, L"Vell")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Dragonfly Pro Chromebook");
  } else if (!StrCmp(Product, L"Volmar")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Vero 514");
  } else if (!StrCmp(Product, L"Zavala")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Vero 712");
  }
  //Alderlake-N
    else if (!StrCmp(Product, L"Anraggar")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"ASUS Chromebook CR12");
  } else if (!StrCmp(Product, L"Anraggar360")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"ASUS Chromebook CR12 Flip");
  } else if (!StrCmp(Product, L"Craask")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Spin 512");
  } else if (!StrCmp(Product, L"Craaskana")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 311");
  } else if (!StrCmp(Product, L"Craaskbowl")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Spin 511");
  } else if (!StrCmp(Product, L"Craaskino")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 315");
  } else if (!StrCmp(Product, L"Craaskov")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Spin 312");
  } else if (!StrCmp(Product, L"Craaskvin")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 511");
  } else if (!StrCmp(Product, L"Craasneto")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 314");
  } else if (!StrCmp(Product, L"Joxer")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"TBD ADL-N Chromebook");
  } else if (!StrCmp(Product, L"Joxero")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"TBD ADL-N Chromebook");
  } else if (!StrCmp(Product, L"Nereid")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"TBD ADL-N Chromebook");
  } else if (!StrCmp(Product, L"Nirwin")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"TBD ADL-N Chromebook");
  } else if (!StrCmp(Product, L"Nivviks")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"TBD ADL-N Chromebook");
  } else if (!StrCmp(Product, L"Pujjo")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo IdeaPad Flex 3i Chromebook");
  } else if (!StrCmp(Product, L"Pujjoflex")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo IdeaPad Flex 3i Chromebook");
  } else if (!StrCmp(Product, L"Pujjoteen")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo 14e Chromebook Gen 3");
  } else if (!StrCmp(Product, L"Pujjoteen15w")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo Ideapad Slim 3i Chromebook");
  } else if (!StrCmp(Product, L"Quandiso")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"CTL Chromebook NL73");
  } else if (!StrCmp(Product, L"Quandiso360")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"CTL Chromebook NL73T");
  } else if (!StrCmp(Product, L"Uldren")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Dell Chromebook 3120");
  } else if (!StrCmp(Product, L"Uldren360")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Dell Chromebook 3120 2-in-1");
  } else if (!StrCmp(Product, L"Xivu")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook CR11 [CR1102C]");
  } else if (!StrCmp(Product, L"Yahiko")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook 14a-nf0xxx");
  } else if (!StrCmp(Product, L"Yavijo")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Fortis 14\" G11 Chromebook");
  } else if (!StrCmp(Product, L"Yaviks")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook 15.6");
  } else if (!StrCmp(Product, L"Yavikso")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"TBD ADL-N Chromebook");
  } else if (!StrCmp(Product, L"Yavilla")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Fortis 11\" G10 Chromebook");
  } else if (!StrCmp(Product, L"Yavilly")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Fortis x360 11\" G5 Chromebook");
  }
  //Meteorlake
    else if (!StrCmp(Product, L"Screebo")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"ASUS ExpertBook CX54 Chromebook Plus [CX5403]");
  }
  //AMD StoneyRidge
    else if (!StrCmp(Product, L"Aleena")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 315");
  } else if (!StrCmp(Product, L"Barla")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook 11A G6 EE");
  } else if (!StrCmp(Product, L"Careena")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook 14");
  } else if (!StrCmp(Product, L"Grunt")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"AMD StonyRidge Chromebook");
  } else if (!StrCmp(Product, L"Kasumi")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook 311");
  } else if (!StrCmp(Product, L"Liara")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo 14e Chromebook");
  } else if (!StrCmp(Product, L"Treeya")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo 100e Chromebook 2nd Gen");
  } else if (!StrCmp(Product, L"Treeya360")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo 300e Chromebook 2nd Gen");
  }
  // AMD Picasso
    else if (!StrCmp(Product, L"Berknip")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Pro c645 Chromebook Enterprise");
  } else if (!StrCmp(Product, L"Dirinboz")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook 14a");
  } else if (!StrCmp(Product, L"Ezkinil")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Spin 514");
  } else if (!StrCmp(Product, L"Gumboz")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Chromebook x360 14a");
  } else if (!StrCmp(Product, L"Morphius")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo ThinkPad C13 Yoga Chromebook");
  } else if (!StrCmp(Product, L"Vilboz")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo 100e Chromebook Gen 3");
  } else if (!StrCmp(Product, L"Vilboz14")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo 14e Chromebook Gen 2");
  } else if (!StrCmp(Product, L"Vilboz360")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Lenovo 300e Chromebook Gen 3");
  } else if (!StrCmp(Product, L"Woomax")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Asus Chromebook Flip CM5");
  }
  // AMD Cezanne
    else if (!StrCmp(Product, L"Dewatt")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Spin 514");
  } else if (!StrCmp(Product, L"Nipperkin")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"HP Elite c645 G2 Chroembook");
  }
  // AMD Mendocino
    else if (!StrCmp(Product, L"Crystaldrift")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"TBD Skyrim Chromebook");
  } else if (!StrCmp(Product, L"Frostflow")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"ASUS Chromebook CM34 Flip");
  } else if (!StrCmp(Product, L"Markarth")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Acer Chromebook Plus 514");
  } else if (!StrCmp(Product, L"Whiterun")) {
    StrCatS (*DeviceName, BufferSize / sizeof (CHAR16), L"Dell Latitude 3445 Chromebook");
  }
}
