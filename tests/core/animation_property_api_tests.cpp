#include "ui_framework/ui_manager.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

namespace
{
struct TestFailure{std::string message;};
void expect(bool condition,const char* message){if(!condition)throw TestFailure{message};}
void expectNear(float actual,float expected,float epsilon,const char* message){if(std::fabs(actual-expected)>epsilon)throw TestFailure{message};}

class PropertyNode final : public ui::Node
{
public:
    ui::FloatAnimationProperty valueProperty() noexcept { return makeFloatAnimationProperty(value_, &value_); }
    float value() const noexcept { return value_; }
private:
    float value_ = 0.0f;
};

void test_ui_manager_animates_component_property()
{
    ui::UIManager ui;
    auto node = std::make_unique<PropertyNode>();
    PropertyNode* propertyNode = node.get();
    ui.addRoot(std::move(node));

    ui.animations().to(propertyNode->valueProperty(), 10.0f, 1.0f, ui::AnimationEasing::Linear);
    ui.advanceTime(0.25f);
    expectNear(propertyNode->value(), 2.5f, 0.0001f, "UIManager animation facade must advance exposed component properties");
    ui.advanceTime(0.75f);
    expectNear(propertyNode->value(), 10.0f, 0.0001f, "UIManager animation facade must finish the transition");
}

void test_component_property_replacement_uses_current_value()
{
    ui::UIManager ui;
    auto node = std::make_unique<PropertyNode>();
    PropertyNode* propertyNode = node.get();
    ui.addRoot(std::move(node));
    const auto property = propertyNode->valueProperty();

    ui.animations().to(property, 10.0f, 1.0f, ui::AnimationEasing::Linear);
    ui.advanceTime(0.4f);
    ui.animations().to(property, 8.0f, 0.6f, ui::AnimationEasing::Linear);
    ui.advanceTime(0.3f);
    expectNear(propertyNode->value(), 6.0f, 0.0001f, "replacement must start from the current presentation value");
}

void test_component_property_cancel_is_exposed()
{
    ui::UIManager ui;
    auto node = std::make_unique<PropertyNode>();
    PropertyNode* propertyNode = node.get();
    ui.addRoot(std::move(node));
    const auto property = propertyNode->valueProperty();

    ui.animations().to(property, 10.0f, 1.0f, ui::AnimationEasing::Linear);
    ui.advanceTime(0.4f);
    const float cancelledValue = propertyNode->value();
    ui.animations().cancel(property);
    ui.advanceTime(0.6f);
    expectNear(propertyNode->value(), cancelledValue, 0.0001f, "cancel must keep the current property value");
}
}

int main()
{
    try
    {
        test_ui_manager_animates_component_property();
        test_component_property_replacement_uses_current_value();
        test_component_property_cancel_is_exposed();
    }
    catch(const TestFailure& failure)
    {
        std::cerr << "Animation property API tests failed: " << failure.message << '\n';
        return EXIT_FAILURE;
    }
    std::cout << "Animation property API tests passed\n";
    return EXIT_SUCCESS;
}
