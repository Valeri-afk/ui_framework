#include "ui_framework/primitives.hpp"
#include "cmath"
#include <algorithm>

#define DEFAULT_ELLIPSE_OVERSCAN 4

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

namespace
{
    // Вспомогательная функция для округления float в Sint16
    inline Sint16 toSint16(float value)
    {
        return static_cast<Sint16>(std::round(value));
    }

    // Вспомогательные функции для внутреннего использования (принимают Sint16)
    static bool hlineInt(SDL_Renderer *renderer, Sint16 x1, Sint16 x2, Sint16 y)
    {
        return SDL_RenderLine(renderer, x1, y, x2, y);
    }

    static bool pixelInt(SDL_Renderer *renderer, Sint16 x, Sint16 y)
    {
        return SDL_RenderPoint(renderer, x, y);
    }

    static bool vlineInt(SDL_Renderer *renderer, Sint16 x, Sint16 y1, Sint16 y2)
    {
        return SDL_RenderLine(renderer, x, y1, x, y2);
    }

    // Вспомогательные для эллипса/круга (работают с Sint16)
    static int _drawQuadrants(SDL_Renderer *renderer, Sint16 x, Sint16 y,
                              Sint16 dx, Sint16 dy, Sint32 f);

    static bool _ellipseRGBAInt(SDL_Renderer *renderer, Sint16 x, Sint16 y,
                                Sint16 rx, Sint16 ry,
                                Uint8 r, Uint8 g, Uint8 b, Uint8 a,
                                Sint32 f);

    static bool lineRGBAInt(SDL_Renderer *renderer, Sint16 x1, Sint16 y1,
                            Sint16 x2, Sint16 y2,
                            Uint8 r, Uint8 g, Uint8 b, Uint8 a)
    {
        bool result = true;
        result &= SDL_SetRenderDrawBlendMode(renderer,
                                             (a == 255) ? SDL_BLENDMODE_NONE : SDL_BLENDMODE_BLEND);
        result &= SDL_SetRenderDrawColor(renderer, r, g, b, a);
        result &= SDL_RenderLine(renderer, x1, y1, x2, y2);
        return result;
    }

    static bool vlineRGBAInt(SDL_Renderer *renderer, Sint16 x, Sint16 y1, Sint16 y2,
                             Uint8 r, Uint8 g, Uint8 b, Uint8 a)
    {
        bool result = true;
        result &= SDL_SetRenderDrawBlendMode(renderer,
                                             (a == 255) ? SDL_BLENDMODE_NONE : SDL_BLENDMODE_BLEND);
        result &= SDL_SetRenderDrawColor(renderer, r, g, b, a);
        result &= SDL_RenderLine(renderer, x, y1, x, y2);
        return result;
    }

    static bool hlineRGBAInt(SDL_Renderer *renderer, Sint16 x1, Sint16 x2, Sint16 y,
                             Uint8 r, Uint8 g, Uint8 b, Uint8 a)
    {
        bool result = true;
        result &= SDL_SetRenderDrawBlendMode(renderer,
                                             (a == 255) ? SDL_BLENDMODE_NONE : SDL_BLENDMODE_BLEND);
        result &= SDL_SetRenderDrawColor(renderer, r, g, b, a);
        result &= SDL_RenderLine(renderer, x1, y, x2, y);
        return result;
    }

    static bool pixelRGBAInt(SDL_Renderer *renderer, Sint16 x, Sint16 y,
                             Uint8 r, Uint8 g, Uint8 b, Uint8 a)
    {
        bool result = true;
        result &= SDL_SetRenderDrawBlendMode(renderer,
                                             (a == 255) ? SDL_BLENDMODE_NONE : SDL_BLENDMODE_BLEND);
        result &= SDL_SetRenderDrawColor(renderer, r, g, b, a);
        result &= SDL_RenderPoint(renderer, x, y);
        return result;
    }

    // Внутренняя версия с Sint16 (не экспортируется)
    static bool rectangleRGBAInt(SDL_Renderer *renderer, Sint16 x1, Sint16 y1,
                                 Sint16 x2, Sint16 y2,
                                 Uint8 r, Uint8 g, Uint8 b, Uint8 a)
    {
        bool result;
        Sint16 tmp;
        SDL_FRect rect;

        if (x1 == x2)
        {
            if (y1 == y2)
                return pixelRGBAInt(renderer, x1, y1, r, g, b, a);
            else
                return vlineRGBAInt(renderer, x1, y1, y2, r, g, b, a);
        }
        else if (y1 == y2)
        {
            return hlineRGBAInt(renderer, x1, x2, y1, r, g, b, a);
        }

        if (x1 > x2)
        {
            tmp = x1;
            x1 = x2;
            x2 = tmp;
        }
        if (y1 > y2)
        {
            tmp = y1;
            y1 = y2;
            y2 = tmp;
        }

        rect.x = static_cast<float>(x1);
        rect.y = static_cast<float>(y1);
        rect.w = static_cast<float>(x2 - x1);
        rect.h = static_cast<float>(y2 - y1);

        result = true;
        result &= SDL_SetRenderDrawBlendMode(renderer,
                                             (a == 255) ? SDL_BLENDMODE_NONE : SDL_BLENDMODE_BLEND);
        result &= SDL_SetRenderDrawColor(renderer, r, g, b, a);
        result &= SDL_RenderRect(renderer, &rect);
        return result;
    }

    static bool boxRGBAInt(SDL_Renderer *renderer, Sint16 x1, Sint16 y1,
                           Sint16 x2, Sint16 y2,
                           Uint8 r, Uint8 g, Uint8 b, Uint8 a)
    {
        bool result;
        Sint16 tmp;
        SDL_FRect rect;

        if (x1 == x2)
        {
            if (y1 == y2)
                return pixelRGBAInt(renderer, x1, y1, r, g, b, a);
            else
                return vlineRGBAInt(renderer, x1, y1, y2, r, g, b, a);
        }
        else if (y1 == y2)
        {
            return hlineRGBAInt(renderer, x1, x2, y1, r, g, b, a);
        }

        if (x1 > x2)
        {
            tmp = x1;
            x1 = x2;
            x2 = tmp;
        }
        if (y1 > y2)
        {
            tmp = y1;
            y1 = y2;
            y2 = tmp;
        }

        rect.x = static_cast<float>(x1);
        rect.y = static_cast<float>(y1);
        rect.w = static_cast<float>(x2 - x1 + 1);
        rect.h = static_cast<float>(y2 - y1 + 1);

        result = true;
        result &= SDL_SetRenderDrawBlendMode(renderer,
                                             (a == 255) ? SDL_BLENDMODE_NONE : SDL_BLENDMODE_BLEND);
        result &= SDL_SetRenderDrawColor(renderer, r, g, b, a);
        result &= SDL_RenderFillRect(renderer, &rect);
        return result;
    }

    // Реализации _drawQuadrants и _ellipseRGBAInt
    int _drawQuadrants(SDL_Renderer *renderer, Sint16 x, Sint16 y,
                       Sint16 dx, Sint16 dy, Sint32 f)
    {
        bool result = true;
        Sint16 xpdx, xmdx;
        Sint16 ypdy, ymdy;

        if (dx == 0)
        {
            if (dy == 0)
            {
                result &= pixelInt(renderer, x, y);
            }
            else
            {
                ypdy = y + dy;
                ymdy = y - dy;
                if (f)
                {
                    result &= vlineInt(renderer, x, ymdy, ypdy);
                }
                else
                {
                    result &= pixelInt(renderer, x, ypdy);
                    result &= pixelInt(renderer, x, ymdy);
                }
            }
        }
        else
        {
            xpdx = x + dx;
            xmdx = x - dx;
            ypdy = y + dy;
            ymdy = y - dy;
            if (f)
            {
                result &= vlineInt(renderer, xpdx, ymdy, ypdy);
                result &= vlineInt(renderer, xmdx, ymdy, ypdy);
            }
            else
            {
                result &= pixelInt(renderer, xpdx, ypdy);
                result &= pixelInt(renderer, xmdx, ypdy);
                result &= pixelInt(renderer, xpdx, ymdy);
                result &= pixelInt(renderer, xmdx, ymdy);
            }
        }
        return result;
    }

    static bool arcRGBAInt(SDL_Renderer *renderer, Sint16 x, Sint16 y, Sint16 rad,
                           Sint16 start, Sint16 end,
                           Uint8 r, Uint8 g, Uint8 b, Uint8 a)
    {
        if (rad < 0)
            return false;
        if (rad == 0)
            return pixelRGBAInt(renderer, x, y, r, g, b, a);

        // нормализация углов
        start %= 360;
        end %= 360;
        while (start < 0)
            start += 360;
        while (end < 0)
            end += 360;
        start %= 360;
        end %= 360;

        // Определение октантов
        int startoct = start / 45;
        int endoct = end / 45;
        int oct = startoct - 1;
        Uint8 drawoct = 0;
        int stopval_start = 0, stopval_end = 0;

        do
        {
            oct = (oct + 1) % 8;
            if (oct == startoct)
            {
                double dstart = (double)start;
                double temp;
                switch (oct)
                {
                case 0:
                case 3:
                    temp = sin(dstart * M_PI / 180.0);
                    break;
                case 1:
                case 6:
                    temp = cos(dstart * M_PI / 180.0);
                    break;
                case 2:
                case 5:
                    temp = -cos(dstart * M_PI / 180.0);
                    break;
                case 4:
                case 7:
                    temp = -sin(dstart * M_PI / 180.0);
                    break;
                }
                stopval_start = (int)(temp * rad);
                if (oct % 2)
                    drawoct |= (1 << oct);
                else
                    drawoct &= 255 - (1 << oct);
            }
            if (oct == endoct)
            {
                double dend = (double)end;
                double temp;
                switch (oct)
                {
                case 0:
                case 3:
                    temp = sin(dend * M_PI / 180.0);
                    break;
                case 1:
                case 6:
                    temp = cos(dend * M_PI / 180.0);
                    break;
                case 2:
                case 5:
                    temp = -cos(dend * M_PI / 180.0);
                    break;
                case 4:
                case 7:
                    temp = -sin(dend * M_PI / 180.0);
                    break;
                }
                stopval_end = (int)(temp * rad);
                if (startoct == endoct)
                {
                    if (start > end)
                        drawoct = 255;
                    else
                        drawoct &= 255 - (1 << oct);
                }
                else if (oct % 2)
                    drawoct &= 255 - (1 << oct);
                else
                    drawoct |= (1 << oct);
            }
            else if (oct != startoct)
            {
                drawoct |= (1 << oct);
            }
        } while (oct != endoct);

        bool result = true;
        result &= SDL_SetRenderDrawBlendMode(renderer,
                                             (a == 255) ? SDL_BLENDMODE_NONE : SDL_BLENDMODE_BLEND);
        result &= SDL_SetRenderDrawColor(renderer, r, g, b, a);

        Sint16 cx = 0;
        Sint16 cy = rad;
        Sint16 df = 1 - rad;
        Sint16 d_e = 3;
        Sint16 d_se = -2 * rad + 5;

        do
        {
            Sint16 ypcy = y + cy;
            Sint16 ymcy = y - cy;
            if (cx > 0)
            {
                Sint16 xpcx = x + cx;
                Sint16 xmcx = x - cx;
                if (drawoct & 4)
                    result &= pixelInt(renderer, xmcx, ypcy);
                if (drawoct & 2)
                    result &= pixelInt(renderer, xpcx, ypcy);
                if (drawoct & 32)
                    result &= pixelInt(renderer, xmcx, ymcy);
                if (drawoct & 64)
                    result &= pixelInt(renderer, xpcx, ymcy);
            }
            else
            {
                if (drawoct & 96)
                    result &= pixelInt(renderer, x, ymcy);
                if (drawoct & 6)
                    result &= pixelInt(renderer, x, ypcy);
            }

            Sint16 xpcy = x + cy;
            Sint16 xmcy = x - cy;
            if (cx > 0 && cx != cy)
            {
                Sint16 ypcx = y + cx;
                Sint16 ymcx = y - cx;
                if (drawoct & 8)
                    result &= pixelInt(renderer, xmcy, ypcx);
                if (drawoct & 1)
                    result &= pixelInt(renderer, xpcy, ypcx);
                if (drawoct & 16)
                    result &= pixelInt(renderer, xmcy, ymcx);
                if (drawoct & 128)
                    result &= pixelInt(renderer, xpcy, ymcx);
            }
            else if (cx == 0)
            {
                if (drawoct & 24)
                    result &= pixelInt(renderer, xmcy, y);
                if (drawoct & 129)
                    result &= pixelInt(renderer, xpcy, y);
            }

            if (stopval_start == cx)
            {
                if (drawoct & (1 << startoct))
                    drawoct &= 255 - (1 << startoct);
                else
                    drawoct |= (1 << startoct);
            }
            if (stopval_end == cx)
            {
                if (drawoct & (1 << endoct))
                    drawoct &= 255 - (1 << endoct);
                else
                    drawoct |= (1 << endoct);
            }

            if (df < 0)
            {
                df += d_e;
                d_e += 2;
                d_se += 2;
            }
            else
            {
                df += d_se;
                d_e += 2;
                d_se += 4;
                cy--;
            }
            cx++;
        } while (cx <= cy);

        return result;
    }

    bool _ellipseRGBAInt(SDL_Renderer *renderer, Sint16 x, Sint16 y,
                         Sint16 rx, Sint16 ry,
                         Uint8 r, Uint8 g, Uint8 b, Uint8 a,
                         Sint32 f)
    {
        bool result;
        Sint32 rxi, ryi;
        Sint32 rx2, ry2, rx22, ry22;
        Sint32 error;
        Sint32 curX, curY, curXp1, curYm1;
        Sint32 scrX, scrY, oldX, oldY;
        Sint32 deltaX, deltaY;
        Sint32 ellipseOverscan;

        if ((rx < 0) || (ry < 0))
            return false;

        result = true;
        result &= SDL_SetRenderDrawBlendMode(renderer,
                                             (a == 255) ? SDL_BLENDMODE_NONE : SDL_BLENDMODE_BLEND);
        result &= SDL_SetRenderDrawColor(renderer, r, g, b, a);

        if (rx == 0)
        {
            if (ry == 0)
                return pixelInt(renderer, x, y);
            else
                return vlineInt(renderer, x, y - ry, y + ry);
        }
        else if (ry == 0)
        {
            return hlineInt(renderer, x - rx, x + rx, y);
        }

        // overscan
        rxi = rx;
        ryi = ry;
        if (rxi >= 512 || ryi >= 512)
            ellipseOverscan = DEFAULT_ELLIPSE_OVERSCAN / 4;
        else if (rxi >= 256 || ryi >= 256)
            ellipseOverscan = DEFAULT_ELLIPSE_OVERSCAN / 2;
        else
            ellipseOverscan = DEFAULT_ELLIPSE_OVERSCAN / 1;

        oldX = scrX = 0;
        oldY = scrY = ryi;
        result &= _drawQuadrants(renderer, x, y, 0, ry, f);

        rxi *= ellipseOverscan;
        ryi *= ellipseOverscan;
        rx2 = rxi * rxi;
        rx22 = rx2 + rx2;
        ry2 = ryi * ryi;
        ry22 = ry2 + ry2;
        curX = 0;
        curY = ryi;
        deltaX = 0;
        deltaY = rx22 * curY;

        error = ry2 - rx2 * ryi + rx2 / 4;
        while (deltaX <= deltaY)
        {
            curX++;
            deltaX += ry22;
            error += deltaX + ry2;
            if (error >= 0)
            {
                curY--;
                deltaY -= rx22;
                error -= deltaY;
            }
            scrX = curX / ellipseOverscan;
            scrY = curY / ellipseOverscan;
            if ((scrX != oldX && scrY == oldY) || (scrX != oldX && scrY != oldY))
            {
                result &= _drawQuadrants(renderer, x, y, scrX, scrY, f);
                oldX = scrX;
                oldY = scrY;
            }
        }

        if (curY > 0)
        {
            curXp1 = curX + 1;
            curYm1 = curY - 1;
            error = ry2 * curX * curXp1 + ((ry2 + 3) / 4) + rx2 * curYm1 * curYm1 - rx2 * ry2;
            while (curY > 0)
            {
                curY--;
                deltaY -= rx22;
                error += rx2;
                error -= deltaY;
                if (error <= 0)
                {
                    curX++;
                    deltaX += ry22;
                    error += deltaX;
                }
                scrX = curX / ellipseOverscan;
                scrY = curY / ellipseOverscan;
                if ((scrX != oldX && scrY == oldY) || (scrX != oldX && scrY != oldY))
                {
                    oldY--;
                    for (; oldY >= scrY; oldY--)
                    {
                        result &= _drawQuadrants(renderer, x, y, scrX, oldY, f);
                        if (f)
                            oldY = scrY - 1;
                    }
                    oldX = scrX;
                    oldY = scrY;
                }
            }
            if (!f)
            {
                oldY--;
                for (; oldY >= 0; oldY--)
                {
                    result &= _drawQuadrants(renderer, x, y, scrX, oldY, f);
                }
            }
        }
        return result;
    }

} // anonymous namespace

// ------------------------------------------------------------
// Публичные функции в пространстве имён primitives
// ------------------------------------------------------------
namespace primitives
{

    bool rectangleRGBA(SDL_Renderer *renderer, float x1, float y1, float x2, float y2,
                       Uint8 r, Uint8 g, Uint8 b, Uint8 a)
    {
        return rectangleRGBAInt(renderer, toSint16(x1), toSint16(y1),
                                toSint16(x2), toSint16(y2), r, g, b, a);
    }

    bool roundedRectangleRGBA(SDL_Renderer *renderer, float x1, float y1, float x2, float y2,
                              float rad, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
    {
        Sint16 ix1 = toSint16(x1);
        Sint16 iy1 = toSint16(y1);
        Sint16 ix2 = toSint16(x2);
        Sint16 iy2 = toSint16(y2);
        Sint16 irad = toSint16(rad);

        if (renderer == nullptr)
            return false;
        if (irad < 0)
            return false;
        if (irad <= 1)
            return rectangleRGBAInt(renderer, ix1, iy1, ix2, iy2, r, g, b, a);

        if (ix1 == ix2)
        {
            if (iy1 == iy2)
                return pixelRGBAInt(renderer, ix1, iy1, r, g, b, a);
            else
                return vlineRGBAInt(renderer, ix1, iy1, iy2, r, g, b, a);
        }
        else if (iy1 == iy2)
        {
            return hlineRGBAInt(renderer, ix1, ix2, iy1, r, g, b, a);
        }

        Sint16 tmp;
        if (ix1 > ix2)
        {
            tmp = ix1;
            ix1 = ix2;
            ix2 = tmp;
        }
        if (iy1 > iy2)
        {
            tmp = iy1;
            iy1 = iy2;
            iy2 = tmp;
        }

        Sint16 w = ix2 - ix1;
        Sint16 h = iy2 - iy1;
        if ((irad * 2) > w)
            irad = w / 2;
        if ((irad * 2) > h)
            irad = h / 2;

        Sint16 xx1 = ix1 + irad;
        Sint16 xx2 = ix2 - irad;
        Sint16 yy1 = iy1 + irad;
        Sint16 yy2 = iy2 - irad;

        bool result = true;
        result &= arcRGBAInt(renderer, xx1, yy1, irad, 180, 270, r, g, b, a);
        result &= arcRGBAInt(renderer, xx2, yy1, irad, 270, 360, r, g, b, a);
        result &= arcRGBAInt(renderer, xx1, yy2, irad, 90, 180, r, g, b, a);
        result &= arcRGBAInt(renderer, xx2, yy2, irad, 0, 90, r, g, b, a);

        if (xx1 <= xx2)
        {
            result &= hlineRGBAInt(renderer, xx1, xx2, iy1, r, g, b, a);
            result &= hlineRGBAInt(renderer, xx1, xx2, iy2, r, g, b, a);
        }
        if (yy1 <= yy2)
        {
            result &= vlineRGBAInt(renderer, ix1, yy1, yy2, r, g, b, a);
            result &= vlineRGBAInt(renderer, ix2, yy1, yy2, r, g, b, a);
        }
        return result;
    }

    bool boxRGBA(SDL_Renderer *renderer, float x1, float y1, float x2, float y2,
                 Uint8 r, Uint8 g, Uint8 b, Uint8 a)
    {
        return boxRGBAInt(renderer, toSint16(x1), toSint16(y1),
                          toSint16(x2), toSint16(y2), r, g, b, a);
    }

    bool roundedBoxRGBA(SDL_Renderer *renderer, float x1, float y1, float x2, float y2,
                        float rad, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
    {
        Sint16 ix1 = toSint16(x1);
        Sint16 iy1 = toSint16(y1);
        Sint16 ix2 = toSint16(x2);
        Sint16 iy2 = toSint16(y2);
        Sint16 irad = toSint16(rad);

        if (renderer == nullptr)
            return false;
        if (irad < 0)
            return false;
        if (irad <= 1)
            return boxRGBAInt(renderer, ix1, iy1, ix2, iy2, r, g, b, a);

        if (ix1 == ix2)
        {
            if (iy1 == iy2)
                return pixelRGBAInt(renderer, ix1, iy1, r, g, b, a);
            else
                return vlineRGBAInt(renderer, ix1, iy1, iy2, r, g, b, a);
        }
        else if (iy1 == iy2)
        {
            return hlineRGBAInt(renderer, ix1, ix2, iy1, r, g, b, a);
        }

        Sint16 tmp;
        if (ix1 > ix2)
        {
            tmp = ix1;
            ix1 = ix2;
            ix2 = tmp;
        }
        if (iy1 > iy2)
        {
            tmp = iy1;
            iy1 = iy2;
            iy2 = tmp;
        }

        Sint16 w = ix2 - ix1 + 1;
        Sint16 h = iy2 - iy1 + 1;
        Sint16 r2 = irad + irad;
        if (r2 > w)
        {
            irad = w / 2;
            r2 = irad + irad;
        }
        if (r2 > h)
        {
            irad = h / 2;
        }

        Sint16 cx = 0;
        Sint16 cy = irad;
        Sint16 ocx = (Sint16)0xffff;
        Sint16 ocy = (Sint16)0xffff;
        Sint16 df = 1 - irad;
        Sint16 d_e = 3;
        Sint16 d_se = -2 * irad + 5;
        Sint16 xpcx, xmcx, xpcy, xmcy;
        Sint16 ypcy, ymcy, ypcx, ymcx;
        Sint16 x = ix1 + irad;
        Sint16 y = iy1 + irad;
        Sint16 dx = ix2 - ix1 - irad - irad;
        Sint16 dy = iy2 - iy1 - irad - irad;

        bool result = true;
        result &= SDL_SetRenderDrawBlendMode(renderer,
                                             (a == 255) ? SDL_BLENDMODE_NONE : SDL_BLENDMODE_BLEND);
        result &= SDL_SetRenderDrawColor(renderer, r, g, b, a);

        do
        {
            xpcx = x + cx;
            xmcx = x - cx;
            xpcy = x + cy;
            xmcy = x - cy;
            if (ocy != cy)
            {
                if (cy > 0)
                {
                    ypcy = y + cy;
                    ymcy = y - cy;
                    result &= hlineInt(renderer, xmcx, xpcx + dx, ypcy + dy);
                    result &= hlineInt(renderer, xmcx, xpcx + dx, ymcy);
                }
                else
                {
                    result &= hlineInt(renderer, xmcx, xpcx + dx, y);
                }
                ocy = cy;
            }
            if (ocx != cx)
            {
                if (cx != cy)
                {
                    if (cx > 0)
                    {
                        ypcx = y + cx;
                        ymcx = y - cx;
                        result &= hlineInt(renderer, xmcy, xpcy + dx, ymcx);
                        result &= hlineInt(renderer, xmcy, xpcy + dx, ypcx + dy);
                    }
                    else
                    {
                        result &= hlineInt(renderer, xmcy, xpcy + dx, y);
                    }
                }
                ocx = cx;
            }

            if (df < 0)
            {
                df += d_e;
                d_e += 2;
                d_se += 2;
            }
            else
            {
                df += d_se;
                d_e += 2;
                d_se += 4;
                cy--;
            }
            cx++;
        } while (cx <= cy);

        if (dx > 0 && dy > 0)
        {
            result &= boxRGBAInt(renderer, ix1, iy1 + irad + 1, ix2, iy2 - irad,
                                 r, g, b, a);
        }
        return result;
    }

    bool lineRGBA(SDL_Renderer *renderer, float x1, float y1, float x2, float y2,
                  Uint8 r, Uint8 g, Uint8 b, Uint8 a)
    {
        return lineRGBAInt(renderer, toSint16(x1), toSint16(y1),
                           toSint16(x2), toSint16(y2), r, g, b, a);
    }

    bool vlineRGBA(SDL_Renderer *renderer, float x, float y1, float y2,
                   Uint8 r, Uint8 g, Uint8 b, Uint8 a)
    {
        return vlineRGBAInt(renderer, toSint16(x), toSint16(y1), toSint16(y2), r, g, b, a);
    }

    bool hlineRGBA(SDL_Renderer *renderer, float x1, float x2, float y,
                   Uint8 r, Uint8 g, Uint8 b, Uint8 a)
    {
        return hlineRGBAInt(renderer, toSint16(x1), toSint16(x2), toSint16(y), r, g, b, a);
    }

    bool pixelRGBA(SDL_Renderer *renderer, float x, float y,
                   Uint8 r, Uint8 g, Uint8 b, Uint8 a)
    {
        return pixelRGBAInt(renderer, toSint16(x), toSint16(y), r, g, b, a);
    }

    bool arcRGBA(SDL_Renderer *renderer, float x, float y, float rad,
                 Sint16 start, Sint16 end,
                 Uint8 r, Uint8 g, Uint8 b, Uint8 a)
    {
        return arcRGBAInt(renderer, toSint16(x), toSint16(y), toSint16(rad), start, end, r, g, b, a);
    }

    bool filledCircleRGBA(SDL_Renderer *renderer, float x, float y, float rad,
                          Uint8 r, Uint8 g, Uint8 b, Uint8 a)
    {
        return _ellipseRGBAInt(renderer, toSint16(x), toSint16(y),
                               toSint16(rad), toSint16(rad),
                               r, g, b, a, 1);
    }

} // namespace primitives
