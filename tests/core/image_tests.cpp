#include "ui_framework/components/image.hpp"

#include <cassert>

namespace
{
    class TestImage final : public ui::Image
    {
    public:
        ui::LayoutSize measureForTest(const ui::LayoutSize &available) const
        {
            return measureContent(available);
        }
    };

    void testDefaultState()
    {
        TestImage image;
        assert(image.getTexture() == nullptr);
        assert(image.getIntrinsicSize() == ui::LayoutSize{});
        assert(image.getFitMode() == ui::Image::FitMode::CONTAIN);
        assert(image.getTint() == ui::Colors::white);
    }

    void testIntrinsicSizeAndFitMode()
    {
        TestImage image;
        const ui::LayoutSize expected{128.0f, 64.0f};
        image.setIntrinsicSize(expected);
        assert(image.getIntrinsicSize() == expected);

        image.setFitMode(ui::Image::FitMode::STRETCH);
        assert(image.getFitMode() == ui::Image::FitMode::STRETCH);

        image.setFitMode(ui::Image::FitMode::COVER);
        assert(image.getFitMode() == ui::Image::FitMode::COVER);

        image.setFitMode(ui::Image::FitMode::CONTAIN);
        assert(image.getFitMode() == ui::Image::FitMode::CONTAIN);
    }

    void testTint()
    {
        TestImage image;
        const ui::Color tint{10, 20, 30, 40};
        image.setTint(tint);
        assert(image.getTint() == tint);
    }

    void testIntrinsicMeasureIsIndependentFromFitMode()
    {
        TestImage image;
        const ui::LayoutSize intrinsic{128.0f, 64.0f};
        image.setIntrinsicSize(intrinsic);

        const ui::LayoutSize available{1000.0f, 900.0f};
        for (const auto mode : {ui::Image::FitMode::STRETCH,
                                ui::Image::FitMode::CONTAIN,
                                ui::Image::FitMode::COVER})
        {
            image.setFitMode(mode);
            const ui::LayoutSize measured = image.measureForTest(available);
            assert(measured == intrinsic);
        }
    }
}

int main()
{
    testDefaultState();
    testIntrinsicSizeAndFitMode();
    testTint();
    testIntrinsicMeasureIsIndependentFromFitMode();
    return 0;
}
