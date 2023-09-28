// Obtained these Color Name mapping from http://chir.ag/projects/ntc/
uint16_t RGB_COLOR_ARRAY_SIZE = 6;
const PROGMEM unsigned long RGB_COLOR_HEX[]
{
        0xFF0000, // Red
    
        0xFFFF00, // Yellow
        0x008000, // Green
        0x0000FF, // Blue

        0x000000, // Black
        0xFFFFFF, // White

};
const char *const RGB_COLOR_HEX_NAME[]{"Red",
                          
                                       "Yellow",
                                       "Green",
                                       "Blue",
                    
                                       "Black",
                                       "White",
                                  };

const char *ColorNameString(uint8_t red, uint8_t green, uint8_t blue)
{
    byte R = (byte)red;
    byte G = (byte)green;
    byte B = (byte)blue;
    unsigned long HEXSTR = ((long)R << 16L) | ((long)G << 8L) | (long)B;
    const char *return_name;
    int16_t minDistance = -1, cl = -1;
    for (uint16_t i = 0; i < RGB_COLOR_ARRAY_SIZE; i++)
    {
        unsigned long current_color = RGB_COLOR_HEX[i];
        if (current_color == HEXSTR)
        {
            cl = i;
            break;
        }
        byte current_red = current_color >> 16, current_green = (current_color & 0x00ff00) >> 8, current_blue = (current_color & 0x0000ff);
        int16_t distance = sqrt(pow(red - (uint8_t)current_red, 2) + pow(green - (uint8_t)current_green, 2) + pow(blue - (uint8_t)current_blue, 2));
        if (distance < minDistance or minDistance < 0)
        {
            minDistance = distance;
            cl = i;
        }
    }
    (cl < 0) ? return_name = "------" : return_name = RGB_COLOR_HEX_NAME[cl];
    return return_name;
}

const char *ColorNameString(unsigned long HEXSTR)
{
    const char *return_name;
    int16_t minDistance = -1, cl = -1;
    byte red = HEXSTR >> 16, green = (HEXSTR & 0x00ff00) >> 8, blue = (HEXSTR & 0x00ff00);
    for (uint16_t i = 0; i < RGB_COLOR_ARRAY_SIZE; i++)
    {
        unsigned long current_color = RGB_COLOR_HEX[i];
        if (current_color == HEXSTR)
        {
            cl = i;
            break;
        }
        byte current_red = current_color >> 16, current_green = (current_color & 0x00ff00) >> 8, current_blue = (current_color & 0x0000ff);
        int16_t distance = sqrt(pow(red - (uint8_t)current_red, 2) + pow(green - (uint8_t)current_green, 2) + pow(blue - (uint8_t)current_blue, 2));
        if (distance < minDistance or minDistance < 0)
        {
            minDistance = distance;
            cl = i;
        }
    }
    (cl < 0) ? return_name = "------" : return_name = RGB_COLOR_HEX_NAME[cl];
    return return_name;
}