#include "picopplib-grafix.h"
#include "segoeui.h"

void loadFonts(picopplib::Grafix& gfx)
{

    {
        picopplib::ByteArrayPtr mem(segoeui, segoeui_len);
        gfx.loadFont(mem, "Default");
    }
}
