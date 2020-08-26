int convert_cw_number_to_ascii(long number_in)
{

  // number_in:  1 = dit, 2 = dah, 9 = a space

  switch (number_in)
  {
  case 12:
    return 65;
    break; // A
  case 2111:
    return 66;
    break;
  case 2121:
    return 67;
    break;
  case 211:
    return 68;
    break;
  case 1:
    return 69;
    break;
  case 1121:
    return 70;
    break;
  case 221:
    return 71;
    break;
  case 1111:
    return 72;
    break;
  case 11:
    return 73;
    break;
  case 1222:
    return 74;
    break;
  case 212:
    return 75;
    break;
  case 1211:
    return 76;
    break;
  case 22:
    return 77;
    break;
  case 21:
    return 78;
    break;
  case 222:
    return 79;
    break;
  case 1221:
    return 80;
    break;
  case 2212:
    return 81;
    break;
  case 121:
    return 82;
    break;
  case 111:
    return 83;
    break;
  case 2:
    return 84;
    break;
  case 112:
    return 85;
    break;
  case 1112:
    return 86;
    break;
  case 122:
    return 87;
    break;
  case 2112:
    return 88;
    break;
  case 2122:
    return 89;
    break;
  case 2211:
    return 90;
    break; // Z

  case 22222:
    return 48;
    break; // 0
  case 12222:
    return 49;
    break;
  case 11222:
    return 50;
    break;
  case 11122:
    return 51;
    break;
  case 11112:
    return 52;
    break;
  case 11111:
    return 53;
    break;
  case 21111:
    return 54;
    break;
  case 22111:
    return 55;
    break;
  case 22211:
    return 56;
    break;
  case 22221:
    return 57;
    break;
  case 112211:
    return '?';
    break; // ?
  case 21121:
    return 47;
    break; // /
#if !defined(OPTION_PROSIGN_SUPPORT)
  case 2111212:
    return '*';
    break; // BK
#endif
    //    case 221122: return 44; break;  // ,
    //    case 221122: return '!'; break;  // ! sp5iou 20180328
  case 221122:
    return ',';
    break;
  case 121212:
    return '.';
    break;
  case 122121:
    return '@';
    break;
  case 222222:
    return 92;
    break; // special hack; six dahs = \ (backslash)
  case 21112:
    return '=';
    break; // BT
  //case 2222222: return '+'; break;
  case 9:
    return 32;
    break; // special 9 = space

#ifndef OPTION_PS2_NON_ENGLISH_CHAR_LCD_DISPLAY_SUPPORT
  case 12121:
    return '+';
    break;
#else
  case 211112:
    return 45;
    break; // - // sp5iou
  case 212122:
    return 33;
    break; // ! //sp5iou
  case 1112112:
    return 36;
    break; // $ //sp5iou
#if !defined(OPTION_PROSIGN_SUPPORT)
  case 12111:
    return 38;
    break; // & // sp5iou
#endif
  case 122221:
    return 39;
    break; // ' // sp5iou
  case 121121:
    return 34;
    break; // " // sp5iou
  case 112212:
    return 95;
    break; // _ // sp5iou
  case 212121:
    return 59;
    break; // ; // sp5iou
  case 222111:
    return 58;
    break; // : // sp5iou
  case 212212:
    return 41;
    break; // KK (stored as ascii ) ) // sp5iou
#if !defined(OPTION_PROSIGN_SUPPORT)
  case 111212:
    return 62;
    break; // SK (stored as ascii > ) // sp5iou
#endif
  case 12121:
    return 60;
    break; // AR (store as ascii < ) // sp5iou
#endif //OPTION_PS2_NON_ENGLISH_CHAR_LCD_DISPLAY_SUPPORT

#if defined(OPTION_PROSIGN_SUPPORT)
#if !defined(OPTION_NON_ENGLISH_EXTENSIONS)
  case 1212:
    return PROSIGN_AA;
    break;
#endif
  case 12111:
    return PROSIGN_AS;
    break;
  case 2111212:
    return PROSIGN_BK;
    break;
  case 21211211:
    return PROSIGN_CL;
    break;
  case 21212:
    return PROSIGN_CT;
    break;
  case 21221:
    return PROSIGN_KN;
    break;
  case 211222:
    return PROSIGN_NJ;
    break;
  case 111212:
    return PROSIGN_SK;
    break;
  case 11121:
    return PROSIGN_SN;
    break;
  case 11111111:
    return PROSIGN_HH;
    break; // iz0rus
#else      //OPTION_PROSIGN_SUPPORT
  case 21221:
    return 40;
    break; // (KN store as ascii ( ) //sp5iou //aaaaaaa
#endif     //OPTION_PROSIGN_SUPPORT

#ifdef OPTION_NON_ENGLISH_EXTENSIONS
  // for English/Cyrillic/Western European font LCD controller (HD44780UA02):
  case 12212:
    return 197;
    break; // 'Å' - AA_capital (OZ, LA, SM)
  //case 12212: return 192; break;   // 'À' - A accent
  case 1212:
    return 198;
    break; // 'Æ' - AE_capital   (OZ, LA)
  //case 1212: return 196; break;    // 'Ä' - A_umlaut (D, SM, OH, ...)
  case 2222:
    return 138;
    break; // CH  - (Russian letter symbol)
  case 22122:
    return 209;
    break; // 'Ñ' - (EA)
  //case 2221: return 214; break;    // 'Ö' – O_umlaut  (D, SM, OH, ...)
  //case 2221: return 211; break;    // 'Ò' - O accent
  case 2221:
    return 216;
    break; // 'Ø' - OE_capital    (OZ, LA)
  case 1122:
    return 220;
    break; // 'Ü' - U_umlaut     (D, ...)
  case 111111:
    return 223;
    break; // beta - double S    (D?, ...)
  case 21211:
    return 199;
    break; // Ç
  case 11221:
    return 208;
    break; // Ð
  case 12112:
    return 200;
    break; // È
  case 11211:
    return 201;
    break; // É
  case 221121:
    return 142;
    break; // Ž
#endif     //OPTION_NON_ENGLISH_EXTENSIONS

  default:
#ifdef OPTION_UNKNOWN_CHARACTER_ERROR_TONE
    boop();
#endif //OPTION_UNKNOWN_CHARACTER_ERROR_TONE
    return unknown_cw_character;
    break;
  }
}

