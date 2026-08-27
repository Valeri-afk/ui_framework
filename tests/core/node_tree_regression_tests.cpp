#include "layout_system.hpp"
#include "node_tree.hpp"
#include "ui_framework/panel_node.hpp"
#include "ui_framework/stack_panel_node.hpp"
#include <cstdlib>
#include <iostream>
#include <memory>
namespace
{
struct TestFailure{const char* message;};
void expect(bool condition,const char* message){if(!condition)throw TestFailure{message};}
std::unique_ptr<ui::Node> makeNode(float w=100.0f,float h=100.0f){auto node=std::make_unique<ui::Node>();node->setSize(ui::LayoutSizeValue::fixed(w,h));return node;}
std::unique_ptr<ui::PanelNode> makePanel(float w=100.0f,float h=100.0f){auto panel=std::make_unique<ui::PanelNode>();panel->setSize(ui::LayoutSizeValue::fixed(w,h));return panel;}
struct LayoutFixture{ui::NodeTree tree;ui::LayoutSystem layout;LayoutFixture(){layout.setViewportSize({100.0f,100.0f});}void process(){layout.requestFullLayout(tree);layout.processLayoutQueue(tree);}};
void test_stale_layout_queue_entry_is_ignored_after_root_removal(){ui::NodeTree tree;ui::Node* root=tree.attachRoot(0,makePanel());tree.removeRoot(root);int callbacks=0;tree.forEachLayoutQueue([&](ui::Node&){++callbacks;});expect(callbacks==0,"removed layout root must be ignored when queue is consumed");}
void test_overlay_hit_test_precedes_root(){LayoutFixture f;ui::Node* root=f.tree.attachRoot(0,makeNode());ui::Node* overlay=f.tree.attachOverlay(0,makeNode());f.process();expect(f.tree.hitTest(50.0f,50.0f)==overlay,"overlay must win hit-testing over an overlapping root");expect(f.tree.hitTest(50.0f,50.0f)!=root,"overlapping root must not win over overlay");}
void test_modal_hit_test_never_escapes_modal_subtree(){LayoutFixture f;ui::Node* root=f.tree.attachRoot(0,makeNode());ui::Node* modal=f.tree.attachOverlay(0,makeNode(40.0f,40.0f));f.process();expect(f.tree.hitTest(20.0f,20.0f,modal)==modal,"modal hit-test must resolve the modal root inside its subtree");expect(f.tree.hitTest(80.0f,80.0f,modal)==nullptr,"modal hit-test must not fall through to another root");expect(f.tree.hitTest(80.0f,80.0f)==root,"normal hit-test must still reach the underlying root");}
void test_clip_to_bounds_blocks_outside_descendant(){LayoutFixture f;auto parent=std::make_unique<ui::StackPanelNode>();parent->setSize(ui::LayoutSizeValue::fixed(50.0f,50.0f));parent->setClipToBounds(true);ui::StackPanelNode* panel=parent.get();f.tree.attachRoot(0,std::move(parent));auto child=makeNode(40.0f,40.0f);child->setPosition({40.0f,40.0f});child->setPositionMode(ui::PositionMode::Absolute);ui::Node* childPtr=f.tree.attachChild(*panel,std::move(child),0);f.process();expect(f.tree.hitTest(45.0f,45.0f)==childPtr,"visible part of an overflowing child must remain hittable");expect(f.tree.hitTest(60.0f,60.0f)==nullptr,"clipToBounds must block hits outside the parent bounds");}
void test_invisible_and_disabled_subtrees_are_not_hittable(){LayoutFixture f;auto invisible=makePanel();invisible->setVisible(false);ui::Node* invisibleRoot=f.tree.attachRoot(0,std::move(invisible));auto disabled=makePanel();disabled->setEnabled(false);ui::Node* disabledRoot=f.tree.attachRoot(1,std::move(disabled));auto visible=makeNode();ui::Node* visibleRoot=f.tree.attachRoot(2,std::move(visible));f.process();expect(f.tree.hitTest(10.0f,10.0f)==visibleRoot,"visible enabled root must be the hit target");expect(f.tree.hitTest(10.0f,10.0f)!=invisibleRoot,"invisible root must never become the hit target");expect(f.tree.hitTest(10.0f,10.0f)!=disabledRoot,"disabled root must never become the hit target");}
}
int main(){try{test_stale_layout_queue_entry_is_ignored_after_root_removal();test_overlay_hit_test_precedes_root();test_modal_hit_test_never_escapes_modal_subtree();test_clip_to_bounds_blocks_outside_descendant();test_invisible_and_disabled_subtrees_are_not_hittable();}catch(const TestFailure& failure){std::cerr<<"NodeTree regression tests failed: "<<failure.message<<'\n';return EXIT_FAILURE;}std::cout<<"NodeTree regression tests passed\n";return EXIT_SUCCESS;}
