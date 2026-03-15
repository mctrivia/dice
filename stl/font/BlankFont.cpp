#include "../Font.h"
#include "Courier.h"

std::unique_ptr<Font> makeFontDef(FontStyle style)
{
    switch (style) {
        case FontStyle::Pixel:      return std::make_unique<PixelFont>();
        case FontStyle::HeavyPixel: return std::make_unique<HeavyPixelFont>();
        case FontStyle::Blank:      return std::make_unique<BlankFont>();
        case FontStyle::Seg7:       return std::make_unique<Seg7Font>();
        case FontStyle::Courier:
        default:                    return std::make_unique<Courier>();
    }
}
